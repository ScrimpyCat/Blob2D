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

#include "EngineSetup.h"
#include <CommonGameKit/CommonGameKit.h>
#include "ExpressionSetup.h"
#include "Configuration.h"
#include "ProjectExpressions.h"


void AnimationInterpolator(int *Previous, int *Next, double Time, int *Result)
{
    *Result = (int)(((1.0 - Time) * (double)*Previous) + (Time * (double)*Next)); //basic lerp
}

void B2EnginePreSetup(void)
{
    B2ExpressionSetup();
}

void B2EngineSetup(void)
{
    switch (B2EngineConfiguration.renderer.pipeline)
    {
        case B2EngineRenderPipelineNone:
            CC_LOG_ERROR("No renderer pipeline was selected");
            break;
            
        case B2EngineRenderPipelineOpenGL:
            GLGFXSetup();
            break;
            
        case B2EngineRenderPipelineMetal:
            MTLGFXSetup();
            break;
    }
    
    CCAssetManagerCreate();
    
    CCEntityManagerCreate();
    
    
    //Load Global Assets
    CCCollection Matches = CCCollectionCreate(CC_STD_ALLOCATOR, CCCollectionHintHeavyEnumerating, sizeof(FSPath), FSPathDestructorForCollection);
    
    CCCollectionInsertElement(Matches, &(FSPath){ FSPathCreate("*/") });
    
    CCOrderedCollection CorePaths = FSManagerGetContentsAtPath(CCAssetPath, Matches, FSMatchSkipHidden | FSMatchSkipFile);
    
    if (CorePaths)
    {
        CC_COLLECTION_FOREACH(FSPath, Path, CorePaths)
        {
            FSPathComponent Dir = FSPathGetComponentAtIndex(Path, FSPathGetComponentCount(Path) - 1);
            if (!strcmp(FSPathComponentGetString(Dir), "font")) CCOrderedCollectionPrependElement(B2EngineConfiguration.directory.fonts, &(FSPath){ FSPathCopy(Path) });
            else if (!strcmp(FSPathComponentGetString(Dir), "shaders")) CCOrderedCollectionPrependElement(B2EngineConfiguration.directory.shaders, &(FSPath){ FSPathCopy(Path) });
            else if (!strcmp(FSPathComponentGetString(Dir), "ui")) CCOrderedCollectionPrependElement(B2EngineConfiguration.directory.layouts, &(FSPath){ FSPathCopy(Path) });
        }
        
        CCCollectionDestroy(CorePaths);
    }
    
    
    //Load GFX libraries
    CCCollectionRemoveAllElements(Matches);
    CCCollectionInsertElement(Matches, &(FSPath){ FSPathCreate(".gfxlib") });
    
    CC_COLLECTION_FOREACH(FSPath, Path, B2EngineConfiguration.directory.shaders)
    {
        CCOrderedCollection Paths = FSManagerGetContentsAtPath(Path, Matches, FSMatchSkipHidden | FSMatchSkipDirectory);
        
        if (Paths)
        {
            CC_COLLECTION_FOREACH(FSPath, LibPath, Paths)
            {
                CCExpression LibExpr = CCExpressionCreateFromSourceFile(LibPath);
                CCExpressionEvaluate(LibExpr);
                CCExpressionDestroy(LibExpr);
            }
            
            CCCollectionDestroy(Paths);
        }
    }
    
    //Load assets
    CCCollectionRemoveAllElements(Matches);
    CCCollectionInsertElement(Matches, &(FSPath){ FSPathCreate(".asset") });
    
    CCCollection GlobalAssetPaths[] = {
        B2EngineConfiguration.directory.shaders,
        B2EngineConfiguration.directory.fonts,
    };
    
    for (size_t Loop = 0; Loop < sizeof(GlobalAssetPaths) / sizeof(typeof(*GlobalAssetPaths)); Loop++)
    {
        CC_COLLECTION_FOREACH(FSPath, Path, GlobalAssetPaths[Loop])
        {
            CCOrderedCollection Paths = FSManagerGetContentsAtPath(Path, Matches, FSMatchSkipHidden | FSMatchSkipDirectory);
            
            if (Paths)
            {
                CC_COLLECTION_FOREACH(FSPath, LibPath, Paths)
                {
                    CCExpression AssetExpr = CCExpressionCreateFromSourceFile(LibPath);
                    CCExpressionEvaluate(AssetExpr);
                    CCExpressionDestroy(AssetExpr);
                }
                
                CCCollectionDestroy(Paths);
            }
        }
    }
    
    
    //Register Systems
    CCRenderSystemRegister();
    CCInputSystemRegister();
    CCAnimationSystemRegister();
    CCScriptableInterfaceSystemRegister();
    
    //Register Components
    CCRenderComponentRegister();
    CCInputMapKeyboardComponentRegister();
    CCInputMapGroupComponentRegister();
    CCInputMapMousePositionComponentRegister();
    CCInputMapMouseButtonComponentRegister();
    CCInputMapMouseScrollComponentRegister();
    CCInputMapMouseDropComponentRegister();
    CCInputMapControllerAxesComponentRegister();
    CCInputMapControllerButtonComponentRegister();
    CCAnimationKeyframeComponentRegister();
    CCAnimationInterpolateComponentRegister();
    CCScriptableInterfaceDynamicFieldComponentRegister();
    
    
    //Create Managers Dependent on ECS :(
    GUIManagerCreate();
    
    
    //Load Dependent Assets
    CCCollection AssetPaths[] = {
        B2EngineConfiguration.directory.layouts
    };
    
    for (size_t Loop = 0; Loop < sizeof(AssetPaths) / sizeof(typeof(*AssetPaths)); Loop++)
    {
        CC_COLLECTION_FOREACH(FSPath, Path, AssetPaths[Loop])
        {
            CCOrderedCollection Paths = FSManagerGetContentsAtPath(Path, Matches, FSMatchSkipHidden | FSMatchSkipDirectory);
            if (Paths)
            {
                CC_COLLECTION_FOREACH(FSPath, LibPath, Paths)
                {
                    CCExpression AssetExpr = CCExpressionCreateFromSourceFile(LibPath);
                    CCExpressionEvaluate(AssetExpr);
                    CCExpressionDestroy(AssetExpr);
                }
                
                CCCollectionDestroy(Paths);
            }
        }
    }
    
    CCCollectionDestroy(Matches);
}
