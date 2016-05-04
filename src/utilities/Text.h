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

#ifndef Blob_Game_Text_h
#define Blob_Game_Text_h

#include "Font.h"

typedef enum {
    CCTextAlignmentLeft,
    CCTextAlignmentCenter,
    CCTextAlignmentRight
} CCTextAlignment;

typedef enum {
    CCTextVisibilityMultiline = (0 << 0),
    CCTextVisibilitySingleLine = (1 << 0),
    
    CCTextVisibilityWord = (0 << 1),
    CCTextVisibilityCharacter = (1 << 1)
} CCTextVisibility;

typedef struct {
    /// The string the attribute applies to
    CCString string;
    /// The font to be used for the attribute
    CCFont font;
    /// The colour of the text
    CCColourRGBA colour;
    /// The size of the text
    CCVector2D scale;
    /// The positioning of the text
    CCVector2D offset;
    /// The positioning applied around the anchor points. Positive values shift the top/right, negative values shit the bottom/left.
    CCVector2D anchoredOffset;
    /// The character distancing in the text
    float space;
    /// The character softness of the text (0.0 = hard, 1.0 = very soft)
    float softness;
    /// The thickness of the text (0.0 = very thin, 1.0 = very thick)
    float thickness;
} CCTextAttribute;

/*!
 * @brief The renderable text.
 * @description Allows @b CCRetain.
 */
typedef struct CCTextInfo *CCText;


/*!
 * @brief Create the renderable text.
 * @description A convenient object to format renderable text. Re-setting fields
 *              is allowed with minimal impact on performance if it does not need
 *              to change much state internally.
 *
 * @param Allocator The allocator to be used for allocations.
 * @return The text. Must be destroyed.
 */
CC_NEW CCText CCTextCreate(CCAllocatorType Allocator);

/*!
 * @brief Destroy the text.
 * @param Text The text.
 */
void CCTextDestroy(CCText CC_DESTROY(Text));

/*!
 * @brief Set the string to be rendered.
 * @param Text The text.
 * @param AttributedStrings The collection of @b CCTextAttribute
 */
void CCTextSetString(CCText Text, CCOrderedCollection CC_RETAIN(AttributedStrings));

/*!
 * @brief Set alignment of the text.
 * @param Text The text.
 * @param Alignment The alignment to be used.
 */
void CCTextSetAlignment(CCText Text, CCTextAlignment Alignment);

/*!
 * @brief Set the visibility control of the text.
 * @param Text The text.
 * @param Visibility The visibility control to be used.
 */
void CCTextSetVisibility(CCText Text, CCTextVisibility Visibility);

/*!
 * @brief Set frame containing the text.
 * @param Text The text.
 * @param Frame The frame containing the text.
 */
void CCTextSetFrame(CCText Text, CCRect Frame);

/*!
 * @brief Get the full length of the text.
 * @param Text The text.
 * @return The length.
 */
size_t CCTextGetLength(CCText Text);

/*!
 * @brief Get the visible length of the text.
 * @description The number of characters visible in the frame.
 * @param Text The text.
 * @return The visible length.
 */
size_t CCTextGetVisibleLength(CCText Text);

/*!
 * @brief Set the visible length of the text.
 * @description The number of characters that should be visible in the frame, or
 *              as many that can fit.
 *
 * @param Text The text.
 * @param Length The maximum visible length. To allow as many as possible set to
 *        SIZE_MAX.
 */
void CCTextSetVisibleLength(CCText Text, size_t Length);

/*!
 * @brief Get the current offset of the text.
 * @description The offset to begin rendering from.
 * @param Text The text.
 * @return The offset.
 */
size_t CCTextGetOffset(CCText Text);

/*!
 * @brief Set the current offset of the text.
 * @description The offset to begin rendering from. Note: if word visibility is
 *              used, and the offset is set inside a word, then it'll cause 
 *              partial rendering of that word.
 *
 * @param Text The text.
 * @param Offset The offset.
 */
void CCTextSetOffset(CCText Text, size_t Offset);

#endif