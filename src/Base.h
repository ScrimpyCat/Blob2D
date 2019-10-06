/*
 *  Copyright (c) 2016, Stefan Johnson
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


#ifndef Blob2D_Base_h
#define Blob2D_Base_h

#define B2_TYPES(func, ...) \
CC_CUSTOM_COMMON_GAME_KIT_TYPES(func, __VA_ARGS_); \
func(__VA_ARGS__, B2LaunchOption); \
func(__VA_ARGS__, B2EngineRenderPipeline); \
func(__VA_ARGS__, B2EngineConfig);

#define B2_TYPES_(func, ...) \
CC_CUSTOM_COMMON_GAME_KIT_TYPES_(func, __VA_ARGS_); \
func(__VA_ARGS__, B2LaunchOption); \
func(__VA_ARGS__, B2EngineRenderPipeline); \
func(__VA_ARGS__, B2EngineConfig);

#ifndef CC_CUSTOM_TYPES
#define CC_CUSTOM_TYPES B2_TYPES
#endif

#ifndef CC_CUSTOM_TYPES_
#define CC_CUSTOM_TYPES_ B2_TYPES_
#endif

#include <CommonGameKit/CommonGameKit.h>

#endif
