/*
 *  Copyright (c) 2015, Stefan Johnson
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without modification,
 *  are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice, this list
 *     of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, this
 *     list of conditions and the following disclaimer in the documentation and/or other
 *     materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define CC_QUICK_COMPILE
#define CC_LOG_OPTION CCLogOptionOutputAll

#include "Base.h"
#include "EngineMain.h"
#include <stdio.h>
#include <stdlib.h>
#include <GLFW/glfw3.h>

#include "PlatformSetup.h"
#include "EngineSetup.h"
#include "Configuration.h"

void (*B2EngineSetupBegin)(void) = NULL;
void (*B2EngineSetupComplete)(void) = NULL;

GLFWwindow *B2Window = NULL;

uint64_t B2UpdateThreadTimeSlice = (uint64_t)((1.0 / 60.0) * UINT64_C(100000000));

FSPath B2EnginePath = NULL;

#pragma mark - Timing

#if CC_PLATFORM_APPLE
#include <mach/mach.h>

static mach_timebase_info_data_t Timebase;
#elif CC_PLATFORM_WINDOWS
#include <windows.h>
#endif

#define TIMER_BASE 1000000000

static uint64_t Timer(void)
{
#if CC_PLATFORM_APPLE
    unsigned _BitInt(128) Time = mach_absolute_time();
    
    return (Time * Timebase.numer) / Timebase.denom;
#elif CC_PLATFORM_UNIX
    struct timespec Time;
    
#if CLOCK_UPTIME_RAW
    clock_gettime(CLOCK_UPTIME_RAW, &Time);
#elif CLOCK_MONOTONIC_COARSE
    clock_gettime(CLOCK_MONOTONIC_COARSE, &Time);
#elif CLOCK_MONOTONIC_RAW
    clock_gettime(CLOCK_MONOTONIC_RAW, &Time);
#else
    clock_gettime(CLOCK_MONOTONIC, &Time);
#endif
    
    return (Time.tv_sec * TIMER_BASE) + Time.tv_nsec;
#else
    return glfwGetTime() * TIMER_BASE;
#endif
}

static uint64_t ScaleTimerFactor;

static uint64_t ScaleUpTimer(void)
{
#if CC_PLATFORM_WINDOWS
    LARGE_INTEGER Time;
    
    QueryPerformanceCounter(&Time);
    
    return Time * ScaleTimerFactor;
#else
    return glfwGetTimerValue() * ScaleTimerFactor;
#endif
}

static uint64_t ScaleDownTimer(void)
{
#if CC_PLATFORM_WINDOWS
    LARGE_INTEGER Time;
    
    QueryPerformanceCounter(&Time);
    
    return Time / ScaleTimerFactor;
#else
    return glfwGetTimerValue() / ScaleTimerFactor;
#endif
}

#if CC_PLATFORM_WINDOWS
static uint64_t WinGetFrequency(void)
{
    LARGE_INTEGER Freq;
    
    QueryPerformanceFrequency(&Freq);
    
    return Freq.QuadPart;
}

static uint64_t WinGetTimer(void)
{
    LARGE_INTEGER Time;
    
    QueryPerformanceCounter(&Time);
    
    return Time.QuadPart;
}
#endif

static inline void InitFromSecondsTimer(uint64_t (*GetFrequency)(void), uint64_t (*GetTimer)(void))
{
    const uint64_t Freq = GetFrequency();
    const uint64_t Multiplier = TIMER_BASE / Freq;
    
    if (Multiplier)
    {
        if (TIMER_BASE % Freq)
        {
            CCTimestamp = Timer;
        }
        
        else if (Multiplier == 1)
        {
            CCTimestamp = GetTimer;
        }
        
        else
        {
            ScaleTimerFactor = Multiplier;
            CCTimestamp = ScaleUpTimer;
        }
    }
    
    else
    {
        if (Freq % TIMER_BASE)
        {
            CCTimestamp = Timer;
        }
        
        else
        {
            ScaleTimerFactor = Freq / TIMER_BASE;
            CCTimestamp = ScaleDownTimer;
        }
    }
}

static void InitTimer(void)
{
#if CC_PLATFORM_APPLE
    if (mach_timebase_info(&Timebase) != KERN_SUCCESS)
    {
        Timebase.numer = 0;
        Timebase.denom = 1;
    }
    
    if ((Timebase.numer == 1) && (Timebase.denom == 1)) CCTimestamp = mach_absolute_time;
    else CCTimestamp = Timer;
#elif CC_PLATFORM_WINDOWS
    InitFromSecondsTimer(WinGetFrequency, WinGetTimer);
#elif CC_PLATFORM_UNIX
    CCTimestamp = Timer;
#else
    InitFromSecondsTimer(glfwGetTimerFrequency, glfwGetTimerValue);
#endif
}

#pragma mark - Window callbacks

static void ErrorCallback(int Error, const char *Description)
{
    CC_LOG_ERROR("GLFW Error [%d]: %s", Error, Description);
}

static mtx_t RenderLock;
static void FramebufferSizeCallback(GLFWwindow *Window, int Width, int Height)
{
    mtx_lock(&RenderLock);
    CCWindowSetFrameSize((CCVector2Di){ .x = Width, .y = Height });
    mtx_unlock(&RenderLock);
}

#pragma mark - Rendering

static void Render(GFXFramebuffer Final, GFXFramebufferAttachment *FinalTarget)
{
    FinalTarget->load = GFXFramebufferAttachmentActionFlagClearOnce | GFXFramebufferAttachmentActionLoad;
    
//    CCComponentSystemRun(CCComponentSystemExecutionTypeRender); //TODO: pass in render target
    
    GUIManagerLock();
    GUIManagerUpdate();
    GUIManagerRender(Final, 0);
    GUIManagerUnlock();
}

static CCConcurrentIndexBuffer FinalTargetIndexBuffer;
GFXBlit FinalTargetBlit[3];
GFXCommandBuffer FinalCommandBuffer[3];
static int SynchronizedBlit(GLFWwindow *Window)
{
    size_t BlitIndex = SIZE_MAX;
    while (!glfwWindowShouldClose(Window))
    {
        size_t Index;
        if (CCConcurrentIndexBufferReadAcquire(FinalTargetIndexBuffer, &Index))
        {
            if (BlitIndex != SIZE_MAX) CCConcurrentIndexBufferDiscard(FinalTargetIndexBuffer, BlitIndex);
            
            BlitIndex = Index;
        }
        
        if (B2EngineConfiguration.renderer.pipeline == B2EngineRenderPipelineOpenGL)
        {
            mtx_lock(&RenderLock);
            glfwMakeContextCurrent(Window);
        }
        
        if ((BlitIndex != SIZE_MAX) && (FinalCommandBuffer[BlitIndex]))
        {
            GFXCommandBufferRecord(FinalCommandBuffer[BlitIndex]);
            B2PlatformFramebufferSetup(Window);
            GFXBlitSubmit(FinalTargetBlit[BlitIndex]);
            
            GFXCommandBufferCommit(FinalCommandBuffer[BlitIndex], TRUE);
            FinalCommandBuffer[BlitIndex] = NULL;
        }
        
        if (B2EngineConfiguration.renderer.pipeline == B2EngineRenderPipelineOpenGL)
        {
            glfwSwapBuffers(Window);
            mtx_unlock(&RenderLock);
        }
    }
    
    return EXIT_SUCCESS;
}

static void SetRenderTarget(GFXBlit Blit, GFXFramebuffer *Final, GFXTexture *FinalTexture, GFXFramebufferAttachment **FinalTarget)
{
    const CCVector2Di FrameSize = CCWindowGetFrameSize();
    
    if (*FinalTexture)
    {
        size_t Width, Height;
        GFXTextureGetSize(*FinalTexture, &Width, &Height, NULL);
        
        if ((Width != FrameSize.x) || (Height != FrameSize.y))
        {
            if (*Final) GFXFramebufferDestroy(*Final);
            GFXTextureDestroy(*FinalTexture);
            
            *Final = NULL;
            *FinalTexture = NULL;
        }
    }
    
    if (!*Final)
    {
        *FinalTexture = GFXTextureCreate(CC_STD_ALLOCATOR, GFXTextureHintDimension2D | (GFXTextureHintFilterModeNearest << GFXTextureHintFilterMin) | (GFXTextureHintFilterModeNearest << GFXTextureHintFilterMag), CCColourFormatRGB8Unorm, FrameSize.x, FrameSize.y, 1, NULL);
        GFXFramebufferAttachment Attachment = GFXFramebufferAttachmentCreateColour(CCRetain(*FinalTexture), GFXFramebufferAttachmentActionFlagClearOnce | GFXFramebufferAttachmentActionLoad, GFXFramebufferAttachmentActionStore, CCVector4DFill(0.0f));
        *Final = GFXFramebufferCreate(CC_STD_ALLOCATOR, &Attachment, 1);
        
        *FinalTarget = GFXFramebufferGetAttachment(*Final, 0);
        
        const CCRect Region = { .position = CCVector2DFill(0.0f), .size = CCVector2DMake(FrameSize.x, FrameSize.y) };
        GFXBlitSetSource(Blit, *Final, 0, Region);
        GFXBlitSetDestination(Blit, GFXFramebufferDefault(), 0, Region);
    }
}

static int RenderAsyncLoop(GLFWwindow *Window)
{
    int err;
    if ((err = mtx_init(&RenderLock, mtx_plain)) != thrd_success)
    {
        CC_LOG_ERROR("Failed to create render thread lock (%d)", err);
    }
    
    const size_t IndexCount = sizeof(FinalTargetBlit) / sizeof(typeof(*FinalTargetBlit));
    FinalTargetIndexBuffer = CCConcurrentIndexBufferCreate(CC_STD_ALLOCATOR, IndexCount);
    
    thrd_t BlitThread;
    if ((err = thrd_create(&BlitThread, (thrd_start_t)SynchronizedBlit, B2Window)) != thrd_success)
    {
        //TODO: fallback to RenderSyncLoop
        CC_LOG_ERROR("Failed to create render thread (%d)", err);
        
        glfwDestroyWindow(B2Window);
        glfwTerminate();
        
        return EXIT_FAILURE;
    }
    
    if (B2EngineConfiguration.renderer.pipeline == B2EngineRenderPipelineOpenGL)
    {
        mtx_lock(&RenderLock);
        glfwMakeContextCurrent(Window);
    }
    
    for (size_t Loop = 0; Loop < IndexCount; Loop++)
    {
        FinalTargetBlit[Loop] = GFXBlitCreate(CC_STD_ALLOCATOR);
        GFXBlitSetFilterMode(FinalTargetBlit[Loop], GFXTextureHintFilterModeNearest);
    }
    
    if (B2EngineConfiguration.renderer.pipeline == B2EngineRenderPipelineOpenGL) mtx_unlock(&RenderLock);
    
    GFXTexture FinalTexture[IndexCount] = {NULL};
    GFXFramebufferAttachment *FinalTarget[IndexCount] = {NULL};
    GFXFramebuffer Final[IndexCount] = {NULL};
    while (!glfwWindowShouldClose(Window))
    {
        if (B2EngineConfiguration.renderer.pipeline == B2EngineRenderPipelineOpenGL)
        {
            mtx_lock(&RenderLock);
            glfwMakeContextCurrent(Window);
        }
        
        CCWindowFrameStep();
        
        const size_t BlitIndex = CCConcurrentIndexBufferWriteAcquire(FinalTargetIndexBuffer);
        
        if (FinalCommandBuffer[BlitIndex]) GFXCommandBufferDestroy(FinalCommandBuffer[BlitIndex]);
        FinalCommandBuffer[BlitIndex] = GFXCommandBufferCreate(CC_STD_ALLOCATOR);
        GFXCommandBufferRecord(FinalCommandBuffer[BlitIndex]);
        
        SetRenderTarget(FinalTargetBlit[BlitIndex], &Final[BlitIndex], &FinalTexture[BlitIndex], &FinalTarget[BlitIndex]);
        Render(Final[BlitIndex], FinalTarget[BlitIndex]);
        
        CCConcurrentIndexBufferStage(FinalTargetIndexBuffer, BlitIndex);
        if (B2EngineConfiguration.renderer.pipeline == B2EngineRenderPipelineOpenGL) mtx_unlock(&RenderLock);
    }
    
    thrd_join(BlitThread, NULL);
    
    for (size_t Loop = 0; Loop < IndexCount; Loop++)
    {
        GFXBlitDestroy(FinalTargetBlit[Loop]);
        
        if (Final[Loop]) GFXFramebufferDestroy(Final[Loop]);
        if (FinalTexture[Loop]) GFXTextureDestroy(FinalTexture[Loop]);
        
    }
    
    return EXIT_SUCCESS;
}

static int RenderSyncLoop(GLFWwindow *Window)
{
    int err;
    if ((err = mtx_init(&RenderLock, mtx_plain)) != thrd_success)
    {
        CC_LOG_ERROR("Failed to create render thread lock (%d)", err);
    }
    
    if (B2EngineConfiguration.renderer.pipeline == B2EngineRenderPipelineOpenGL) glfwMakeContextCurrent(Window);
    
    GFXBlit Blit = GFXBlitCreate(CC_STD_ALLOCATOR);
    GFXBlitSetFilterMode(Blit, GFXTextureHintFilterModeNearest);
    
    GFXTexture FinalTexture = NULL;
    GFXFramebufferAttachment *FinalTarget = NULL;
    GFXFramebuffer Final = NULL;
    while (!glfwWindowShouldClose(Window))
    {
        if (B2EngineConfiguration.renderer.pipeline == B2EngineRenderPipelineOpenGL) mtx_lock(&RenderLock);
        CCWindowFrameStep();
        
        GFXCommandBuffer CommandBuffer = GFXCommandBufferCreate(CC_STD_ALLOCATOR);
        GFXCommandBufferRecord(CommandBuffer);
        
        SetRenderTarget(Blit, &Final, &FinalTexture, &FinalTarget);
        Render(Final, FinalTarget);
        
        B2PlatformFramebufferSetup(Window);
        GFXBlitSubmit(Blit);
        
        GFXCommandBufferCommit(CommandBuffer, TRUE);
        
        if (B2EngineConfiguration.renderer.pipeline == B2EngineRenderPipelineOpenGL)
        {
            glfwSwapBuffers(Window);
            mtx_unlock(&RenderLock);
        }
    }
    
    GFXBlitDestroy(Blit);
    
    if (Final) GFXFramebufferDestroy(Final);
    if (FinalTexture) GFXTextureDestroy(FinalTexture);
    
    return EXIT_SUCCESS;
}

static int RenderLoop(GLFWwindow *Window)
{
    return B2EngineConfiguration.renderer.vsync ? RenderSyncLoop(Window) : RenderAsyncLoop(Window);
}

#pragma mark - Input callbacks

#pragma mark Controller
static CCString ControllerGetName(size_t Index)
{
    return CCStringCreate(CC_STD_ALLOCATOR, CCStringEncodingASCII | CCStringHintCopy, glfwGetJoystickName((int)Index));
}

static void ControllerUpdateAxes(size_t Index, CCArray(float) Axes)
{
    int Count;
    const float *Values = glfwGetJoystickAxes((int)Index, &Count);
    
    if (Count == CCArrayGetCount(Axes)) CCArrayReplaceElementsAtIndex(Axes, 0, Values, Count);
    else
    {
        CCArrayRemoveAllElements(Axes);
        CCArrayAppendElements(Axes, Values, Count);
    }
}

static void ControllerUpdateButtons(size_t Index, CCArray(_Bool) Buttons)
{
    int Count;
    const unsigned char *Values = glfwGetJoystickButtons((int)Index, &Count);
    
#if GLFW_PRESS == 1 && GLFW_RELEASE == 0 && BOOL_WIDTH == UCHAR_WIDTH
    if (Count == CCArrayGetCount(Buttons)) CCArrayReplaceElementsAtIndex(Buttons, 0, Values, Count);
    else
    {
        CCArrayRemoveAllElements(Buttons);
        CCArrayAppendElements(Buttons, Values, Count);
    }
#else
    if (Count == CCArrayGetCount(Buttons)) for (int Loop2 = 0; Loop2 < Count; Loop2++) CCArrayReplaceElementAtIndex(Buttons, Loop2, &(_Bool){ Values[Loop2] });
    else
    {
        CCArrayRemoveAllElements(Buttons);
        for (int Loop2 = 0; Loop2 < Count; Loop2++) CCArrayAppendElement(Buttons, &(_Bool){ Values[Loop2] });
    }
#endif
}

static void ControllerUpdateHats(size_t Index, CCArray(CCControllerHat) Hats)
{
    _Static_assert((CCControllerHatCenter == GLFW_HAT_CENTERED) &&
                   (CCControllerHatUp == GLFW_HAT_UP) &&
                   (CCControllerHatRight == GLFW_HAT_RIGHT) &&
                   (CCControllerHatDown == GLFW_HAT_DOWN) &&
                   (CCControllerHatLeft == GLFW_HAT_LEFT) &&
                   (CCControllerHatRightUp == GLFW_HAT_RIGHT_UP) &&
                   (CCControllerHatRightDown == GLFW_HAT_RIGHT_DOWN) &&
                   (CCControllerHatLeftUp == GLFW_HAT_LEFT_UP) &&
                   (CCControllerHatLeftDown == GLFW_HAT_LEFT_DOWN), "Controller hats enum does not match");
    
    int Count;
    const unsigned char *Values = glfwGetJoystickHats((int)Index, &Count);
    
    if (Count == CCArrayGetCount(Hats)) for (int Loop2 = 0; Loop2 < Count; Loop2++) CCArrayReplaceElementAtIndex(Hats, Loop2, &(CCControllerHat){ Values[Loop2] });
    else
    {
        CCArrayRemoveAllElements(Hats);
        for (int Loop2 = 0; Loop2 < Count; Loop2++) CCArrayAppendElement(Hats, &(CCControllerHat){ Values[Loop2] });
    }
}

static void ControllerConnected(int Index, int Connected)
{
    CCControllerConnect(Index, Connected == GLFW_CONNECTED);
}

#pragma mark Keyboard

static void KeyboardInput(GLFWwindow *Window, int Keycode, int Scancode, int Action, int Mods)
{
    CCKeyboardInput(Keycode < CCKeyboardKeycodeCount ? Keycode : CCKeyboardKeycodeUnknown, Action, Mods);
}

static void KeyboardCharInput(GLFWwindow *Window, unsigned int Codepoint, int Mods)
{
    CCKeyboardCharInput(Codepoint, Mods);
}

#pragma mark Mouse

static void MouseDropInput(GLFWwindow *Window, int Count, const char **Files)
{
    CCOrderedCollection(CCString) FileList = CCCollectionCreate(CC_STD_ALLOCATOR, CCCollectionHintHeavyEnumerating | CCCollectionHintConstantLength | CCCollectionHintConstantElements | CCCollectionHintSizeSmall, sizeof(CCString), CCStringDestructorForCollection);
    
    for (int Loop = 0; Loop < Count; Loop++) CCOrderedCollectionAppendElement(FileList, &(CCString){ CCStringCreate(CC_STD_ALLOCATOR, (CCStringHint)CCStringEncodingASCII, Files[Loop]) });
    
    CCMouseDropInput(FileList);
    
    CCCollectionDestroy(FileList);
}

static void MouseScrollInput(GLFWwindow *Window, double x, double y)
{
    CCMouseScrollInput(CCVector2DMake(x, y));
}

static void MouseButtonInput(GLFWwindow *Window, int Button, int Action, int Mods)
{
    CCMouseButtonInput(Button < CCMouseButtonCount ? Button : CCMouseButtonUnknown, Action, Mods);
}

static void MousePositionInput(GLFWwindow *Window, double x, double y)
{
    int Height;
    glfwGetWindowSize(Window, NULL, &Height);
    
    CCMousePositionInput(CCVector2DMake(x, Height - y));
}

static void MouseEnterInput(GLFWwindow *Window, int Entered)
{
    CCMouseEnterInput(Entered);
}

#pragma mark Window

static void WindowFocus(GLFWwindow *Window, int Focus)
{
    CCInputWindowFocus(Focus);
}

#pragma mark - Updating

static void B2Wait(B2Thread Thread, uint64_t Available, uint64_t ReturnBy)
{
}

B2WaitingCallback B2Waiting = B2Wait;

B2ECSConfig B2EngineECS;

static void WaitUntil(B2Thread Thread, uint64_t Wait, uint64_t Target)
{
    B2Waiting(B2ThreadUpdate, Wait, Target);
    
    Wait = Target - CCTimestamp();
    
    if (Wait < Target)
    {
        if (Wait < ECS_TIME_FROM_MILLISECONDS(2))
        {
            while (CCTimestamp() < Target) CC_SPIN_WAIT();
        }
        
        else thrd_sleep(&(struct timespec){ .tv_nsec = Wait }, NULL);
    }
}

static int UpdateLoop(GLFWwindow *Window)
{
    B2TimeScale TickScale[B2EngineECS.count];
    uint64_t TickWait[B2EngineECS.count];
    uint64_t TickTime[B2EngineECS.count];
    size_t FixedTimeStep[B2EngineECS.count], VariableTimeStep[B2EngineECS.count];
    
    memset(TickTime, 0, sizeof(TickTime));
    
    for (size_t Loop = 0; Loop < B2EngineECS.count; Loop++)
    {
        ECSMutationApply(B2EngineECS.instances[Loop].context);
    }
    
    uint64_t PrevTime = CCTimestamp();
    size_t FixedCount = 0, VariableCount = 0;
    
    while (!glfwWindowShouldClose(Window))
    {
        if (atomic_load_explicit(&B2EngineECS.tick.changed, memory_order_relaxed))
        {
            atomic_store_explicit(&B2EngineECS.tick.changed, FALSE, memory_order_relaxed);
            
            atomic_thread_fence(memory_order_acq_rel);
            
            FixedCount = 0;
            VariableCount = 0;
            
            for (size_t Loop = 0; Loop < B2EngineECS.count; Loop++)
            {
                TickScale[Loop] = atomic_load_explicit(&B2EngineECS.tick.scale[Loop], memory_order_relaxed);
                TickWait[Loop] = atomic_load_explicit(&B2EngineECS.tick.wait[Loop], memory_order_relaxed);
                
                if ((TickScale[Loop].numerator) && (TickScale[Loop].denominator))
                {
                    if (TickWait[Loop])
                    {
                        FixedTimeStep[FixedCount++] = Loop;
                    }
                    
                    else
                    {
                        VariableTimeStep[VariableCount++] = Loop;
                        TickTime[Loop] = 0;
                    }
                }
            }
        }
        
        const uint64_t Time = CCTimestamp();
        const uint64_t DeltaTime = Time - PrevTime;
        PrevTime = Time;
        
        uint64_t SleepTime = B2UpdateThreadTimeSlice;
        
        for (size_t Loop = 0; Loop < VariableCount; Loop++)
        {
            const size_t Index = VariableTimeStep[Loop];
            
            ECSTick(B2EngineECS.instances[Index].context, B2EngineECS.instances[Index].groups, B2EngineECS.instances[Index].groupCount, B2EngineECS.instances[Index].state, (DeltaTime * TickScale[Index].numerator) / TickScale[Index].denominator);
            
            ECSMutationApply(B2EngineECS.instances[Index].context);
        }
        
        for (size_t Loop = 0; Loop < FixedCount; Loop++)
        {
            const size_t Index = FixedTimeStep[Loop];
            
            TickTime[Index] += (DeltaTime * TickScale[Index].numerator) / TickScale[Index].denominator;
            
            if (TickTime[Index] / TickWait[Index])
            {
                TickTime[Index] -= TickWait[Index];
                
                ECSTick(B2EngineECS.instances[Index].context, B2EngineECS.instances[Index].groups, B2EngineECS.instances[Index].groupCount, B2EngineECS.instances[Index].state, TickWait[Index]);
                
                ECSMutationApply(B2EngineECS.instances[Index].context);
                
                if (TickTime[Index] / TickWait[Index]) SleepTime = 0;
                else if ((TickWait[Index] - TickTime[Index]) < SleepTime) SleepTime = TickWait[Index] - TickTime[Index];
            }
        }
        
        const uint64_t Elapsed = CCTimestamp() - PrevTime;
        
        if (SleepTime > Elapsed)
        {
            const uint64_t Wait = SleepTime - Elapsed;
            const uint64_t Target = SleepTime + PrevTime;
            
            WaitUntil(B2ThreadUpdate, Wait, Target);
        }
    }
    
    return EXIT_SUCCESS;
}

#pragma mark -

double B2InputFrequency = 0.001;

static int EngineMain(int argc, const char *argv[])
{
    if (B2EngineSetupBegin) B2EngineSetupBegin();
    B2EnginePreSetup();
    B2ConfigureOptions(argc, argv);
    
    B2PlatformSetup();
    glfwSetErrorCallback(ErrorCallback);
    
    if (!glfwInit())
    {
        CC_LOG_ERROR("Failed to initalized GLFW");
        return EXIT_FAILURE;
    }
    
    InitTimer();
    
    switch (B2EngineConfiguration.renderer.pipeline)
    {
        case B2EngineRenderPipelineOpenGL:
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
            break;
            
        default:
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            break;
    }
    
    glfwWindowHint(GLFW_SRGB_CAPABLE, GL_TRUE);
    
    glfwWindowHint(GLFW_RED_BITS, 32);
    glfwWindowHint(GLFW_GREEN_BITS, 32);
    glfwWindowHint(GLFW_BLUE_BITS, 32);
    glfwWindowHint(GLFW_ALPHA_BITS, 0);
    glfwWindowHint(GLFW_DEPTH_BITS, 0);
    glfwWindowHint(GLFW_STENCIL_BITS, 0);
    
    B2Window = glfwCreateWindow(B2EngineConfiguration.window.width, B2EngineConfiguration.window.height, B2EngineConfiguration.title, B2EngineConfiguration.window.fullscreen ? glfwGetPrimaryMonitor() : NULL, NULL);
    if (!B2Window)
    {
        CC_LOG_ERROR("Failed to create window");
        glfwTerminate();
        return EXIT_FAILURE;
    }
    
    CCControllerSetup(GLFW_JOYSTICK_LAST + 1, ControllerGetName, ControllerUpdateAxes, ControllerUpdateButtons, ControllerUpdateHats);
    glfwSetJoystickCallback(ControllerConnected);
    for (size_t Loop = 0; Loop <= GLFW_JOYSTICK_LAST; Loop++) CCControllerConnect(Loop, glfwJoystickPresent((int)Loop));
    
    glfwSetWindowFocusCallback(B2Window, WindowFocus);
    
    glfwSetKeyCallback(B2Window, KeyboardInput);
    glfwSetCharModsCallback(B2Window, KeyboardCharInput);
    
    glfwSetDropCallback(B2Window, MouseDropInput);
    glfwSetScrollCallback(B2Window, MouseScrollInput);
    glfwSetMouseButtonCallback(B2Window, MouseButtonInput);
    glfwSetCursorPosCallback(B2Window, MousePositionInput);
    glfwSetCursorEnterCallback(B2Window, MouseEnterInput);
    
    CCVector2Di FrameSize;
    glfwSetFramebufferSizeCallback(B2Window, FramebufferSizeCallback);
    glfwGetFramebufferSize(B2Window, &FrameSize.x, &FrameSize.y);
    CCWindowSetFrameSize(FrameSize);
    
    if (B2EngineConfiguration.renderer.pipeline == B2EngineRenderPipelineOpenGL) glfwMakeContextCurrent(B2Window);
    B2PlatformWindowSetup(B2Window);
    
    B2EngineSetup();
    if (B2EngineSetupComplete) B2EngineSetupComplete();
    
    glfwMakeContextCurrent(NULL);
    
    int err;
    thrd_t RenderThread;
    if ((err = thrd_create(&RenderThread, (thrd_start_t)RenderLoop, B2Window)) != thrd_success)
    {
        //Possibly fallback to single threaded implementation?
        CC_LOG_ERROR("Failed to create render thread (%d)", err);
        
        glfwDestroyWindow(B2Window);
        glfwTerminate();
        
        return EXIT_FAILURE;
    }
    
    thrd_t UpdateThread;
    if ((err = thrd_create(&UpdateThread, (thrd_start_t)UpdateLoop, B2Window)) != thrd_success)
    {
        //Possibly fallback to single threaded implementation?
        CC_LOG_ERROR("Failed to create update thread (%d)", err);
        
        glfwSetWindowShouldClose(B2Window, TRUE);
        
        thrd_join(RenderThread, NULL);
        
        glfwDestroyWindow(B2Window);
        glfwTerminate();
        
        return EXIT_FAILURE;
    }
    
    size_t ClipboardCounter = 0;
    CCString ClipboardCache = CC_STRING("");
    
    while (!glfwWindowShouldClose(B2Window))
    {
        const size_t CurrentClipboardCounter = CCClipboardCounter();
        
        if (ClipboardCounter != CurrentClipboardCounter)
        {
            ClipboardCounter = CurrentClipboardCounter;
            
            CCClipboardClipping Clipping = CCClipboardGet();
            
            if (Clipping.type == &CC_REFLECT(CCString))
            {
                ClipboardCache = CCStringCopy((CCString)Clipping.data);
                
                CC_STRING_TEMP_BUFFER(Buffer, (CCString)Clipping.data) glfwSetClipboardString(B2Window, Buffer);
                
                CCStringDestroy((CCString)Clipping.data);
            }
            
            else CCFree(Clipping.data);
        }
        
        else
        {
            // TODO: On mac use NSPasteboard changeCount
            const char *String = glfwGetClipboardString(B2Window);
            
            CC_STRING_TEMP_BUFFER(Buffer, ClipboardCache)
            {
                if (strcmp(Buffer, String))
                {
                    CCStringDestroy(ClipboardCache);
                    
                    ClipboardCache = CCStringCreate(CC_STD_ALLOCATOR, CCStringHintCopy | CCStringEncodingUTF8, String);
                    
                    CCClipboardSet((CCClipboardClipping){ .type = &CC_REFLECT(CCString), .data = (void*)CCStringCopy(ClipboardCache) });
                    
                    ClipboardCounter++;
                }
            }
        }
        
        CCControllerDiffUpdateState();
        
        glfwWaitEventsTimeout(B2InputFrequency);
    }
    
    CCStringDestroy(ClipboardCache);
    
    
    thrd_join(UpdateThread, NULL);
    thrd_join(RenderThread, NULL);
    
    glfwDestroyWindow(B2Window);
    glfwTerminate();
    
    return EXIT_SUCCESS;
}

int B2EngineRun(int argc, const char *argv[])
{
    char Path[] = __FILE__;
    if ((sizeof(__FILE__) > sizeof("Blob2D/src/EngineMain.c")) && (!strcmp(Path + (sizeof(__FILE__) - sizeof("Blob2D/src/EngineMain.c")), "Blob2D/src/EngineMain.c")))
    {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warray-bounds"
        Path[sizeof(__FILE__) - sizeof("Blob2D/src/EngineMain.c")] = 0;
#pragma clang diagnostic pop
        
        CCFileFilterInputAddPath(Path);
    }

    if (!B2EnginePath)
    {
#if CC_PLATFORM_OS_X || CC_PLATFORM_IOS
        CFBundleRef Bundle = CFBundleGetBundleWithIdentifier(CFSTR("io.scrimpycat.Blob2D"));
        if (Bundle)
        {
            CFURLRef ResourceURL = CFBundleCopyResourcesDirectoryURL(Bundle);
            char ResourcePath[PATH_MAX];
            
            if ((!ResourceURL) || (!CFURLGetFileSystemRepresentation(ResourceURL, TRUE, (UInt8*)ResourcePath, sizeof(ResourcePath))))
            {
                CC_LOG_ERROR("Could not find engine asset path");
                return EXIT_FAILURE;
            }
            
            CFRelease(ResourceURL);
            
            B2EnginePath = FSPathCreateFromSystemPath(ResourcePath);
            FSPathAppendComponent(B2EnginePath, FSPathComponentCreate(FSPathComponentTypeDirectory, "assets"));
        }
        
        else
#endif
        {
            CCOrderedCollection(FSPathComponent) Path = FSPathConvertSystemPathToComponents(argv[0], TRUE);
            CCOrderedCollectionRemoveLastElement(Path);
            FSPath ProjectFolder = FSPathCreateFromComponents(Path);
            
            CCCollection(FSPath) Paths = CCCollectionCreate(CC_STD_ALLOCATOR, CCCollectionHintSizeSmall, sizeof(FSPath), FSPathDestructorForCollection);
            CCCollectionInsertElement(Paths, &(FSPath){ FSPathCreate("Blob2D/assets/") });
            CCOrderedCollection(FSPath) Matches = FSManagerGetContentsAtPath(ProjectFolder, Paths, FSMatchSearchRecursively);
            CCCollectionDestroy(Paths);
            FSPathDestroy(ProjectFolder);
            
            if (CCCollectionGetCount(Matches) != 1)
            {
                CCCollectionDestroy(Matches);
                CC_LOG_ERROR("Could not find engine asset path");
                return EXIT_FAILURE;
            }
            
            B2EnginePath = CCRetain(*(FSPath*)CCOrderedCollectionGetElementAtIndex(Matches, 0));
            CCCollectionDestroy(Matches);
        }
    }
    
    return CCMain(EngineMain, argc, argv);
}
