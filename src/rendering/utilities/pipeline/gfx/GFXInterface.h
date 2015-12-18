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

#ifndef Blob_Game_GFXInterface_h
#define Blob_Game_GFXInterface_h

#include <CommonC/Allocator.h>
#include "GFXBuffer.h"
#include "GFXTexture.h"
#include "GFXFramebuffer.h"
#include "GFXShaderLibrary.h"
#include "GFXShader.h"

#pragma mark - Required Buffer Callbacks
typedef GFXBuffer (*GFXBufferConstructorCallback)(CCAllocatorType Allocator, GFXBufferHint Hint, size_t Size, const void *Data);
typedef void (*GFXBufferDestructorCallback)(GFXBuffer Internal);
typedef GFXBufferHint (*GFXBufferGetHintCallback)(GFXBuffer Internal);
typedef size_t (*GFXBufferGetSizeCallback)(GFXBuffer Internal);
typedef size_t (*GFXBufferReadBufferCallback)(GFXBuffer Internal, ptrdiff_t Offset, size_t Size, void *Data);
typedef size_t (*GFXBufferWriteBufferCallback)(GFXBuffer Internal, ptrdiff_t Offset, size_t Size, const void *Data);

#pragma mark Required Texture Callbacks
typedef GFXTexture (*GFXTextureConstructorCallback)(CCAllocatorType Allocator, GFXTextureHint Hint, CCColourFormat Format, size_t Width, size_t Height, size_t Depth, CCPixelData Data);
typedef void (*GFXTextureDestructorCallback)(GFXTexture Texture);
typedef GFXTextureHint (*GFXTextureGetHintCallback)(GFXTexture Texture);
typedef void (*GFXTextureGetSizeCallback)(GFXTexture Texture, size_t *Width, size_t *Height, size_t *Depth);
typedef void (*GFXTextureSetFilterModeCallback)(GFXTexture Texture, GFXTextureHint FilterType, GFXTextureHint FilterMode);
typedef void (*GFXTextureSetAddressModeCallback)(GFXTexture Texture, GFXTextureHint Coordinate, GFXTextureHint AddressMode);

#pragma mark Required Framebuffer Callbacks
typedef GFXFramebuffer (*GFXFramebufferConstructorCallback)(CCAllocatorType Allocator, GFXFramebufferAttachment *Attachments, size_t Count);
typedef void (*GFXFramebufferDestructorCallback)(GFXFramebuffer Framebuffer);
typedef GFXFramebufferAttachment *(*GFXFramebufferGetAttachmentCallback)(GFXFramebuffer Framebuffer, size_t Index);

#pragma mark Required Shader Library Callbacks

typedef GFXShaderLibrary (*GFXShaderLibraryConstructorCallback)(CCAllocatorType Allocator);
typedef void (*GFXShaderLibraryDestructorCallback)(GFXShaderLibrary Library);
typedef const GFXShaderSource (*GFXShaderLibraryCompileCallback)(GFXShaderLibrary Library, GFXShaderSourceType Type, const char *Name, const char *Source);
typedef const GFXShaderSource (*GFXShaderLibraryGetSourceCallback)(GFXShaderLibrary Library, const char *Name);

#pragma mark Required Shader Callbacks

typedef GFXShader (*GFXShaderConstructorCallback)(CCAllocatorType Allocator, GFXShaderSource Vertex, GFXShaderSource Fragment);
typedef void (*GFXShaderDestructorCallback)(GFXShader Shader);
typedef GFXShaderInput (*GFXShaderGetInputCallback)(GFXShader Shader, const char *Name);



#pragma mark - Optional Buffer Callbacks
typedef void (*GFXBufferInvalidateCallback)(GFXBuffer Internal);
typedef size_t (*GFXBufferCopyBufferCallback)(GFXBuffer SrcInternal, ptrdiff_t SrcOffset, size_t Size, GFXBuffer DstInternal, ptrdiff_t DstOffset);
typedef size_t (*GFXBufferFillBufferCallback)(GFXBuffer Internal, ptrdiff_t Offset, size_t Size, uint8_t Fill);

#pragma mark Optional Texture Callbacks
typedef void (*GFXTextureInvalidateCallback)(GFXTexture Texture);


#pragma mark -

typedef struct {
    GFXBufferConstructorCallback create;
    GFXBufferDestructorCallback destroy;
    GFXBufferGetHintCallback hints;
    GFXBufferGetSizeCallback size;
    GFXBufferReadBufferCallback read;
    GFXBufferWriteBufferCallback write;
    struct {
        GFXBufferInvalidateCallback invalidate;
        GFXBufferCopyBufferCallback copy;
        GFXBufferFillBufferCallback fill;
    } optional;
} GFXBufferInterface;

typedef struct {
    GFXTextureConstructorCallback create;
    GFXTextureDestructorCallback destroy;
    GFXTextureGetHintCallback hints;
    GFXTextureGetSizeCallback size;
    GFXTextureSetFilterModeCallback setFilterMode;
    GFXTextureSetAddressModeCallback setAddressMode;
    struct {
        GFXTextureInvalidateCallback invalidate;
    } optional;
} GFXTextureInterface;

typedef struct {
    GFXFramebufferConstructorCallback create;
    GFXFramebufferDestructorCallback destroy;
    GFXFramebufferGetAttachmentCallback attachment;
} GFXFramebufferInterface;

typedef struct {
    GFXShaderLibraryConstructorCallback create;
    GFXShaderLibraryDestructorCallback destroy;
    GFXShaderLibraryCompileCallback compile;
    GFXShaderLibraryGetSourceCallback source;
} GFXShaderLibraryInterface;

typedef struct {
    GFXShaderConstructorCallback create;
    GFXShaderDestructorCallback destroy;
    GFXShaderGetInputCallback input;
} GFXShaderInterface;

#endif
