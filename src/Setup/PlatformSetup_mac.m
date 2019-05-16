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

#define GLFW_EXPOSE_NATIVE_COCOA
#define GLFW_EXPOSE_NATIVE_NSGL

#import "PlatformSetup.h"
#import <GLFW/glfw3native.h>
#import "Configuration.h"
@import QuartzCore.CAMetalLayer;
@import Metal;

void B2PlatformSetup(void)
{
    @autoreleasepool {
        [[NSUserDefaults standardUserDefaults] setBool: NO forKey: @"ApplePressAndHoldEnabled"];
    }
}

static CAMetalLayer *RenderLayer;
void B2PlatformWindowSetup(GLFWwindow *Window)
{
    if (B2EngineConfiguration.renderer.pipeline == B2EngineRenderPipelineMetal)
    {
        @autoreleasepool {
            NSView *View = ((NSWindow*)glfwGetCocoaWindow(Window)).contentView;
            
            RenderLayer = [CAMetalLayer layer];
            RenderLayer.device = MTLCreateSystemDefaultDevice(); // TODO: Use notifications to manage devices
            RenderLayer.pixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
            RenderLayer.framebufferOnly = YES;
            RenderLayer.frame = View.layer.frame;
            RenderLayer.colorspace = nil;
            
            View.layer = RenderLayer;
        }
    }
}

void B2PlatformFramebufferSetup(GLFWwindow *Window)
{
    if (B2EngineConfiguration.renderer.pipeline == B2EngineRenderPipelineMetal)
    {
        @autoreleasepool {
            id <CAMetalDrawable>Drawable = [RenderLayer nextDrawable];
            MTLGFXSetDrawable(Drawable, Drawable.texture);
        }
    }
}
