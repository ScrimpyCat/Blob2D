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

#ifndef Blob_Game_InputMapControllerButtonComponent_h
#define Blob_Game_InputMapControllerButtonComponent_h

#include "InputMapControllerComponent.h"

#define CC_INPUT_MAP_CONTROLLER_BUTTON_COMPONENT_ID (CCInputMapTypeControllerButton | CC_INPUT_COMPONENT_FLAG)

extern const char * const CCInputMapControllerButtonComponentName;

typedef struct {
    CC_COMPONENT_INHERIT(CCInputMapControllerComponentClass);
    int32_t button;
    float ramp;
} CCInputMapControllerButtonComponentClass, *CCInputMapControllerButtonComponentPrivate;


void CCInputMapControllerButtonComponentRegister(void);
void CCInputMapControllerButtonComponentDeregister(void);


static inline void CCInputMapControllerButtonComponentInitialize(CCComponent Component, CCComponentID id)
{
    CCInputMapControllerComponentInitialize(Component, id);
    ((CCInputMapControllerButtonComponentPrivate)Component)->button = 0;
    ((CCInputMapControllerButtonComponentPrivate)Component)->ramp = 0.0f;
}

static inline void CCInputMapControllerButtonComponentDeallocate(CCComponent Component)
{
    CCInputMapControllerComponentDeallocate(Component);
}

static inline int32_t CCInputMapControllerButtonComponentGetButton(CCComponent Component)
{
    return ((CCInputMapControllerButtonComponentPrivate)Component)->button;
}

static inline void CCInputMapControllerButtonComponentSetButton(CCComponent Component, int32_t Button)
{
    ((CCInputMapControllerButtonComponentPrivate)Component)->button = Button;
}

static inline float CCInputMapControllerButtonComponentGetRamp(CCComponent Component)
{
    return ((CCInputMapControllerButtonComponentPrivate)Component)->ramp;
}

static inline void CCInputMapControllerButtonComponentSetRamp(CCComponent Component, float Ramp)
{
    ((CCInputMapControllerButtonComponentPrivate)Component)->ramp = Ramp;
}

#endif
