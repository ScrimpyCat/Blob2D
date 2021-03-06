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
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIAB2E
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef Blob2D_Configuration_h
#define Blob2D_Configuration_h

#include <Blob2D/Base.h>

typedef enum {
    B2LaunchOptionGame,
    B2LaunchOptionEditor,
    B2LaunchOptionTest
} B2LaunchOption;

typedef enum {
    B2EngineRenderPipelineNone,
    B2EngineRenderPipelineOpenGL,
    B2EngineRenderPipelineMetal
} B2EngineRenderPipeline;

typedef struct {
    B2LaunchOption launch;
    FSPath project;
    char *title;
    struct {
        int width;
        int height;
        _Bool fullscreen; //TODO: Change to specifying monitors
    } window;
    struct {
        B2EngineRenderPipeline pipeline;
        _Bool vsync;
    } renderer;
    struct {
        CCOrderedCollection(FSPath) fonts;
        CCOrderedCollection(FSPath) levels;
        CCOrderedCollection(FSPath) rules;
        CCOrderedCollection(FSPath) entities;
        CCOrderedCollection(FSPath) textures;
        CCOrderedCollection(FSPath) shaders;
        CCOrderedCollection(FSPath) sounds;
        CCOrderedCollection(FSPath) layouts;
        FSPath save;
        FSPath logs;
        FSPath tmp;
    } directory;
} B2EngineConfig;


extern B2EngineConfig B2EngineConfiguration;
void B2ConfigureOptions(int argc, const char *argv[]);

#endif
