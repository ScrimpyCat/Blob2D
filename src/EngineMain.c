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

#include "EngineMain.h"
#include <stdio.h>
#include <stdlib.h>
#include <GLFW/glfw3.h>
#include <CommonGameKit/CommonGameKit.h>

#include "PlatformSetup.h"
#include "EngineSetup.h"
#include "Configuration.h"

void (*B2EngineSetupBegin)(void) = NULL;
void (*B2EngineSetupComplete)(void) = NULL;

GLFWwindow *B2Window = NULL;

double B2UpdateThreadTimeSlice = 1.0 / 60.0;

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
    
    CCComponentSystemRun(CCComponentSystemExecutionTypeRender); //TODO: pass in render target
    
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

static void ControllerUpdateAxes(size_t Index, CCOrderedCollection Axes)
{
    int Count;
    const float *Values = glfwGetJoystickAxes((int)Index, &Count);
    
    if (Count == CCCollectionGetCount(Axes)) for (int Loop2 = 0; Loop2 < Count; Loop2++) CCOrderedCollectionReplaceElementAtIndex(Axes, &Values[Loop2], Loop2);
    else
    {
        CCCollectionRemoveAllElements(Axes);
        for (int Loop2 = 0; Loop2 < Count; Loop2++) CCOrderedCollectionAppendElement(Axes, &Values[Loop2]);
    }
}

static void ControllerUpdateButtons(size_t Index, CCOrderedCollection Buttons)
{
    int Count;
    const unsigned char *Values = glfwGetJoystickButtons((int)Index, &Count);
    
    if (Count == CCCollectionGetCount(Buttons)) for (int Loop2 = 0; Loop2 < Count; Loop2++) CCOrderedCollectionReplaceElementAtIndex(Buttons, &Values[Loop2], Loop2);
    else
    {
        CCCollectionRemoveAllElements(Buttons);
        for (int Loop2 = 0; Loop2 < Count; Loop2++) CCOrderedCollectionAppendElement(Buttons, &Values[Loop2]);
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
    CCOrderedCollection FileList = CCCollectionCreate(CC_STD_ALLOCATOR, CCCollectionHintHeavyEnumerating | CCCollectionHintConstantLength | CCCollectionHintConstantElements | CCCollectionHintSizeSmall, sizeof(CCString), CCStringDestructorForCollection);
    
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
    CCInputSystemWindowFocus(Focus);
}

#pragma mark - Updating

static int UpdateLoop(GLFWwindow *Window)
{
    while (!glfwWindowShouldClose(Window))
    {
        double Wait = CCTimestamp();
        
        CCEntityManagerUpdate();
        CCComponentSystemRun(CCComponentSystemExecutionTypeUpdate);
        
        Wait = (B2UpdateThreadTimeSlice - (CCTimestamp() - Wait)) * 1000000000.0;
        thrd_sleep(&(struct timespec){ .tv_nsec = (Wait < 0 ? 0 : (uint64_t)Wait) }, NULL);
    }
    
    return EXIT_SUCCESS;
}

#pragma mark -

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
    
    CCControllerSetup(GLFW_JOYSTICK_LAST + 1, ControllerGetName, ControllerUpdateAxes, ControllerUpdateButtons);
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
    
    while (!glfwWindowShouldClose(B2Window))
    {
        CCComponentSystemRun(CCComponentSystemExecutionTypeInput);
        glfwWaitEvents();
    }
    
    
    thrd_join(UpdateThread, NULL);
    thrd_join(RenderThread, NULL);
    
    glfwDestroyWindow(B2Window);
    glfwTerminate();
    
    return EXIT_SUCCESS;
}

int B2EngineRun(int argc, const char *argv[])
{
    CCTimestamp = glfwGetTime;
    
    return CCMain(EngineMain, argc, argv);
}
