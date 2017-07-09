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

#include "ProjectExpressions.h"
#include "Configuration.h"

#warning Need to add a copy function
static void B2ProjectExpressionValueGameConfigDestructor(B2EngineConfig *Data)
{
    CC_SAFE_Free(Data->title);
    if (Data->directory.fonts) CCCollectionDestroy(Data->directory.fonts);
    if (Data->directory.levels) CCCollectionDestroy(Data->directory.levels);
    if (Data->directory.rules) CCCollectionDestroy(Data->directory.rules);
    if (Data->directory.textures) CCCollectionDestroy(Data->directory.textures);
    if (Data->directory.shaders) CCCollectionDestroy(Data->directory.shaders);
    if (Data->directory.sounds) CCCollectionDestroy(Data->directory.sounds);
    if (Data->directory.layouts) CCCollectionDestroy(Data->directory.layouts);
    if (Data->directory.logs) FSPathDestroy(Data->directory.logs);
    if (Data->directory.tmp) FSPathDestroy(Data->directory.tmp);
    CC_SAFE_Free(Data);
}

static FSPath B2ProjectExpressionPathFromExpression(CCExpression Expression, CCExpression State)
{
    FSPath Path = NULL;
    if (CCExpressionGetType(Expression) == CCExpressionValueTypeString)
    {
        CC_STRING_TEMP_BUFFER(Buffer, CCExpressionGetString(Expression)) Path = FSPathCreate(Buffer);
    }
    
    else if (CCExpressionGetType(Expression) == CCExpressionValueTypeList)
    {
        Path = FSPathCreate("");
        CC_COLLECTION_FOREACH(CCExpression, Part, CCExpressionGetList(Expression))
        {
            CCExpression Result = CCExpressionEvaluate(Part);
            if (CCExpressionGetType(Result) == CCExpressionValueTypeString)
            {
                CC_STRING_TEMP_BUFFER(Buffer, CCExpressionGetString(Result))
                {
                    CCOrderedCollection Components = FSPathConvertPathToComponents(Buffer, FALSE);
                    CC_COLLECTION_FOREACH(FSPathComponent, Component, Components)
                    {
                        FSPathAppendComponent(Path, FSPathComponentCopy(Component));
                    }
                    CCCollectionDestroy(Components);
                }
            }
            
            else
            {
                FSPathDestroy(Path);
                return NULL;
            }
        }
    }
    
    if (FSPathIsRelativePath(Path))
    {
        CCExpression CurrentDir = CCExpressionGetState(State, CC_STRING("@cd"));
        if (CurrentDir)
        {
            CC_STRING_TEMP_BUFFER(Buffer, CCExpressionGetString(CurrentDir))
            {
                FSPath Base = FSPathCreate(Buffer);
                for (size_t Loop = 1, Count = FSPathGetComponentCount(Path); Loop < Count; Loop++)
                {
                    FSPathAppendComponent(Base, FSPathComponentCopy(FSPathGetComponentAtIndex(Path, Loop)));
                }
                
                FSPathDestroy(Path);
                Path = Base;
            }
        }
    }
    
    return Path;
}

CCExpression B2ProjectExpressionGame(CCExpression Expression)
{
    B2EngineConfig Config = {
        .launch = B2LaunchOptionGame,
        .project = NULL,
        .title = NULL,
        .window = {
            .width = 0,
            .height = 0,
            .fullscreen = FALSE
        },
        .directory = {
            .fonts = NULL,
            .levels = NULL,
            .rules = NULL,
            .textures = NULL,
            .shaders = NULL,
            .sounds = NULL,
            .layouts = NULL,
            .logs = NULL,
            .tmp = NULL
        }
    };
    
    CCEnumerator Enumerator;
    CCCollectionGetEnumerator(CCExpressionGetList(Expression), &Enumerator);
    
    CCExpression *Expr = CCCollectionEnumeratorNext(&Enumerator);
    if (Expr)
    {
        CCExpression Result = CCExpressionEvaluate(*Expr);
        if (CCExpressionGetType(Result) == CCExpressionValueTypeString)
        {
            char *Title;
            CC_SAFE_Malloc(Title, CCStringGetSize(CCExpressionGetString(Result)) + 1,
                           CC_LOG_ERROR("Failed to get title for CCEngineConfig from expression due to allocation failure. Allocation size: %zu", CCStringGetSize(CCExpressionGetString(Result)) + 1);
                           );
            
            *CCStringCopyCharacters(CCExpressionGetString(Result), 0, CCStringGetLength(CCExpressionGetString(Result)), Title) = 0;
            
            Config.title = Title;
        }
        
        for (Expr = NULL; (Expr = CCCollectionEnumeratorNext(&Enumerator)); )
        {
            Result = CCExpressionEvaluate(*Expr);
            if (CCExpressionGetType(Result) == CCExpressionValueTypeExpression)
            {
                size_t ArgCount = CCCollectionGetCount(CCExpressionGetList(Result)) - 1;
                
                CCEnumerator Enumerator;
                CCCollectionGetEnumerator(CCExpressionGetList(Result), &Enumerator);
                
                Expr = CCCollectionEnumeratorGetCurrent(&Enumerator);
                
                if (Expr)
                {
                    Result = CCExpressionEvaluate(*Expr);
                    if (CCExpressionGetType(Result) == CCExpressionValueTypeAtom)
                    {
                        if (CCStringEqual(CCExpressionGetAtom(Result), CC_STRING("default-resolution:")))
                        {
                            if (ArgCount == 2)
                            {
                                CCExpression Width = CCExpressionEvaluate(*(CCExpression*)CCCollectionEnumeratorNext(&Enumerator)), Height = CCExpressionEvaluate(*(CCExpression*)CCCollectionEnumeratorNext(&Enumerator));
                                
                                if ((CCExpressionGetType(Width) == CCExpressionValueTypeInteger) && (CCExpressionGetType(Height) == CCExpressionValueTypeInteger))
                                {
                                    Config.window.width = CCExpressionGetInteger(Width);
                                    Config.window.height = CCExpressionGetInteger(Height);
                                }
                                
                                else CC_EXPRESSION_EVALUATOR_LOG_OPTION_ERROR("default-resolution", "width:integer height:integer");
                            }
                            
                            else CC_EXPRESSION_EVALUATOR_LOG_OPTION_ERROR("default-resolution", "width:integer height:integer");
                        }
                        
                        else if (CCStringEqual(CCExpressionGetAtom(Result), CC_STRING("default-fullscreen:")))
                        {
                            if (ArgCount == 1)
                            {
                                CCExpression Fullscreen = CCExpressionEvaluate(*(CCExpression*)CCCollectionEnumeratorNext(&Enumerator));
                                
                                if (CCExpressionGetType(Fullscreen) == CCExpressionValueTypeInteger)
                                {
                                    Config.window.fullscreen = CCExpressionGetInteger(Fullscreen);
                                }
                                
                                else CC_EXPRESSION_EVALUATOR_LOG_OPTION_ERROR("default-fullscreen", "fullscreen:boolean");
                            }
                            
                            else CC_EXPRESSION_EVALUATOR_LOG_OPTION_ERROR("default-fullscreen", "fullscreen:boolean");
                        }
                        
                        else if (CCStringHasPrefix(CCExpressionGetAtom(Result), CC_STRING("dir-")))
                        {
                            CCString Dir = CCExpressionGetAtom(Result);
                            
                            struct {
                                CCString atom;
                                void *attribute;
                                _Bool path;
                            } Commands[] = {
                                { CC_STRING("fonts:"), &Config.directory.fonts, FALSE },
                                { CC_STRING("levels:"), &Config.directory.levels, FALSE },
                                { CC_STRING("rules:"), &Config.directory.rules, FALSE },
                                { CC_STRING("textures:"), &Config.directory.textures, FALSE },
                                { CC_STRING("shaders:"), &Config.directory.shaders, FALSE },
                                { CC_STRING("sounds:"), &Config.directory.sounds, FALSE },
                                { CC_STRING("layouts:"), &Config.directory.layouts, FALSE },
                                { CC_STRING("entities:"), &Config.directory.entities, FALSE },
                                { CC_STRING("logs:"), &Config.directory.logs, TRUE },
                                { CC_STRING("tmp:"), &Config.directory.tmp, TRUE }
                            };
                            
                            for (size_t Loop = 0; Loop < sizeof(Commands) / sizeof(typeof(*Commands)); Loop++)
                            {
                                if (CCStringHasSuffix(Dir, Commands[Loop].atom))
                                {
                                    if (Commands[Loop].path)
                                    {
                                        if (ArgCount == 1)
                                        {
                                            *(FSPath*)(Commands[Loop].attribute) = B2ProjectExpressionPathFromExpression(CCExpressionEvaluate(*(CCExpression*)CCCollectionEnumeratorNext(&Enumerator)), Result);
                                        }
                                        
                                        else
                                        {
                                            CC_STRING_TEMP_BUFFER(Buffer, CCExpressionGetString(Result)) CC_EXPRESSION_EVALUATOR_LOG_OPTION_ERROR(Buffer, "path:string|list");
                                        }
                                    }
                                    
                                    else
                                    {
                                        CCCollection Directories = CCCollectionCreate(CC_STD_ALLOCATOR, CCCollectionHintSizeSmall | CCCollectionHintHeavyEnumerating, sizeof(FSPath), FSPathDestructorForCollection);
                                        
                                        //TODO: Make a directory expression
                                        for (Expr = NULL; (Expr = CCCollectionEnumeratorNext(&Enumerator)); )
                                        {
                                            FSPath Dir = B2ProjectExpressionPathFromExpression(CCExpressionEvaluate(*Expr), Expression);
                                            CCCollectionInsertElement(Directories, &Dir);
                                        }
                                        
                                        *(CCCollection*)(Commands[Loop].attribute) = Directories;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    B2EngineConfig *Result;
    CC_SAFE_Malloc(Result, sizeof(B2EngineConfig),
                   CC_LOG_ERROR("Failed to allocate CCEngineConfig for expression. Allocation size: %zu", sizeof(B2EngineConfig));
                   );
    
    if (Result)
    {
        *(B2EngineConfig*)Result = Config;
        
        return CCExpressionCreateCustomType(CC_STD_ALLOCATOR, (CCExpressionValueType)B2ProjectExpressionValueTypeGameConfig, Result, NULL, (CCExpressionValueDestructor)B2ProjectExpressionValueGameConfigDestructor);
    }
    
    else B2ProjectExpressionValueGameConfigDestructor(&Config);
    
    return Expression;
}
