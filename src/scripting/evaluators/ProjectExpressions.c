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
#include "ProjectExpressions.h"
#include "Configuration.h"

#warning Need to add a copy function
static void B2ProjectExpressionValueGameConfigDestructor(B2EngineConfig *Data)
{
    CC_SAFE_Free(Data->title);
    if (Data->directory.fonts) CCCollectionDestroy(Data->directory.fonts);
    if (Data->directory.levels) CCCollectionDestroy(Data->directory.levels);
    if (Data->directory.rules) CCCollectionDestroy(Data->directory.rules);
    if (Data->directory.entities) CCCollectionDestroy(Data->directory.entities);
    if (Data->directory.textures) CCCollectionDestroy(Data->directory.textures);
    if (Data->directory.shaders) CCCollectionDestroy(Data->directory.shaders);
    if (Data->directory.sounds) CCCollectionDestroy(Data->directory.sounds);
    if (Data->directory.layouts) CCCollectionDestroy(Data->directory.layouts);
    if (Data->directory.save) FSPathDestroy(Data->directory.save);
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
            if (CCExpressionGetType(Part) == CCExpressionValueTypeString)
            {
                CC_STRING_TEMP_BUFFER(Buffer, CCExpressionGetString(Part))
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

_Thread_local const char *TempTitle = NULL;
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
        .renderer = {
            .pipeline = B2EngineRenderPipelineNone,
            .vsync = TRUE
        },
        .directory = {
            .fonts = CCCollectionCreate(CC_STD_ALLOCATOR, CCCollectionHintOrdered | CCCollectionHintSizeSmall | CCCollectionHintHeavyEnumerating, sizeof(FSPath), FSPathDestructorForCollection),
            .levels = CCCollectionCreate(CC_STD_ALLOCATOR, CCCollectionHintOrdered | CCCollectionHintSizeSmall | CCCollectionHintHeavyEnumerating, sizeof(FSPath), FSPathDestructorForCollection),
            .rules = CCCollectionCreate(CC_STD_ALLOCATOR, CCCollectionHintOrdered | CCCollectionHintSizeSmall | CCCollectionHintHeavyEnumerating, sizeof(FSPath), FSPathDestructorForCollection),
            .entities = CCCollectionCreate(CC_STD_ALLOCATOR, CCCollectionHintOrdered | CCCollectionHintSizeSmall | CCCollectionHintHeavyEnumerating, sizeof(FSPath), FSPathDestructorForCollection),
            .textures = CCCollectionCreate(CC_STD_ALLOCATOR, CCCollectionHintOrdered | CCCollectionHintSizeSmall | CCCollectionHintHeavyEnumerating, sizeof(FSPath), FSPathDestructorForCollection),
            .shaders = CCCollectionCreate(CC_STD_ALLOCATOR, CCCollectionHintOrdered | CCCollectionHintSizeSmall | CCCollectionHintHeavyEnumerating, sizeof(FSPath), FSPathDestructorForCollection),
            .sounds = CCCollectionCreate(CC_STD_ALLOCATOR, CCCollectionHintOrdered | CCCollectionHintSizeSmall | CCCollectionHintHeavyEnumerating, sizeof(FSPath), FSPathDestructorForCollection),
            .layouts = CCCollectionCreate(CC_STD_ALLOCATOR, CCCollectionHintOrdered | CCCollectionHintSizeSmall | CCCollectionHintHeavyEnumerating, sizeof(FSPath), FSPathDestructorForCollection),
            .save = NULL,
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
            TempTitle = Title;
        }
        
        for (Expr = NULL; (Expr = CCCollectionEnumeratorNext(&Enumerator)); )
        {
            Result = CCExpressionEvaluate(*Expr);
            if (CCExpressionGetType(Result) == CCExpressionValueTypeList)
            {
                size_t ArgCount = CCCollectionGetCount(CCExpressionGetList(Result)) - 1;
                
                CCEnumerator Enumerator;
                CCCollectionGetEnumerator(CCExpressionGetList(Result), &Enumerator);
                
                Expr = CCCollectionEnumeratorGetCurrent(&Enumerator);
                
                if (Expr)
                {
                    CCExpression Option = *Expr;
                    if (CCExpressionGetType(Option) == CCExpressionValueTypeAtom)
                    {
                        if (CCStringEqual(CCExpressionGetAtom(Option), CC_STRING("default-resolution:")))
                        {
                            if (ArgCount == 2)
                            {
                                CCExpression Width = *(CCExpression*)CCCollectionEnumeratorNext(&Enumerator), Height = *(CCExpression*)CCCollectionEnumeratorNext(&Enumerator);
                                
                                if ((CCExpressionGetType(Width) == CCExpressionValueTypeInteger) && (CCExpressionGetType(Height) == CCExpressionValueTypeInteger))
                                {
                                    Config.window.width = CCExpressionGetInteger(Width);
                                    Config.window.height = CCExpressionGetInteger(Height);
                                }
                                
                                else CC_EXPRESSION_EVALUATOR_LOG_OPTION_ERROR("default-resolution", "width:integer height:integer");
                            }
                            
                            else CC_EXPRESSION_EVALUATOR_LOG_OPTION_ERROR("default-resolution", "width:integer height:integer");
                        }
                        
                        else if (CCStringEqual(CCExpressionGetAtom(Option), CC_STRING("default-fullscreen:")))
                        {
                            if (ArgCount == 1)
                            {
                                CCExpression Fullscreen = *(CCExpression*)CCCollectionEnumeratorNext(&Enumerator);
                                
                                if (CCExpressionGetType(Fullscreen) == CCExpressionValueTypeInteger)
                                {
                                    Config.window.fullscreen = CCExpressionGetInteger(Fullscreen);
                                }
                                
                                else CC_EXPRESSION_EVALUATOR_LOG_OPTION_ERROR("default-fullscreen", "fullscreen:boolean");
                            }
                            
                            else CC_EXPRESSION_EVALUATOR_LOG_OPTION_ERROR("default-fullscreen", "fullscreen:boolean");
                        }
                        
                        else if (CCStringEqual(CCExpressionGetAtom(Option), CC_STRING("renderer:")))
                        {
                            if (ArgCount == 1)
                            {
                                CCExpression Renderer = *(CCExpression*)CCCollectionEnumeratorNext(&Enumerator);
                                
                                if (CCExpressionGetType(Renderer) == CCExpressionValueTypeAtom)
                                {
                                    CCString Pipeline = CCExpressionGetAtom(Renderer);
                                    if (CCStringEqual(Pipeline, CC_STRING(":opengl"))) Config.renderer.pipeline = B2EngineRenderPipelineOpenGL;
                                    else if (CCStringEqual(Pipeline, CC_STRING(":metal"))) Config.renderer.pipeline = B2EngineRenderPipelineMetal;
                                    else CC_EXPRESSION_EVALUATOR_LOG_ERROR("Incorrect usage of renderer: unknown argument (%S)", Pipeline);
                                }
                                
                                else CC_EXPRESSION_EVALUATOR_LOG_OPTION_ERROR("renderer", "renderer:atom");
                            }
                            
                            else CC_EXPRESSION_EVALUATOR_LOG_OPTION_ERROR("renderer", "renderer:atom");
                        }
                        
                        else if (CCStringEqual(CCExpressionGetAtom(Option), CC_STRING("vsync:")))
                        {
                            if (ArgCount == 1)
                            {
                                CCExpression Fullscreen = *(CCExpression*)CCCollectionEnumeratorNext(&Enumerator);
                                
                                if (CCExpressionGetType(Fullscreen) == CCExpressionValueTypeInteger)
                                {
                                    Config.renderer.vsync = CCExpressionGetInteger(Fullscreen);
                                }
                                
                                else CC_EXPRESSION_EVALUATOR_LOG_OPTION_ERROR("vsync", "boolean");
                            }
                            
                            else CC_EXPRESSION_EVALUATOR_LOG_OPTION_ERROR("vsync", "boolean");
                        }
                        
                        else if (CCStringHasPrefix(CCExpressionGetAtom(Option), CC_STRING("dir-")))
                        {
                            CCString Dir = CCExpressionGetAtom(Option);
                            
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
                                { CC_STRING("save:"), &Config.directory.save, TRUE },
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
                                            if (*(FSPath*)Commands[Loop].attribute) FSPathDestroy(*(FSPath*)Commands[Loop].attribute);
                                            
                                            *(FSPath*)Commands[Loop].attribute = B2ProjectExpressionPathFromExpression(*(CCExpression*)CCCollectionEnumeratorNext(&Enumerator), Result);
                                        }
                                        
                                        else
                                        {
                                            CC_STRING_TEMP_BUFFER(Buffer, CCExpressionGetString(Result)) CC_EXPRESSION_EVALUATOR_LOG_OPTION_ERROR(Buffer, "path:string|list");
                                        }
                                    }
                                    
                                    else
                                    {
                                        CCOrderedCollection Directories = *(CCOrderedCollection*)Commands[Loop].attribute;
                                        
                                        for (Expr = NULL; (Expr = CCCollectionEnumeratorNext(&Enumerator)); )
                                        {
                                            FSPath Dir = B2ProjectExpressionPathFromExpression(*Expr, Expression);
                                            CCOrderedCollectionAppendElement(Directories, &Dir);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    TempTitle = NULL;
    
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

CCExpression B2ProjectExpressionAppDataDir(CCExpression Expression)
{
    FSPath AppDataPath = FSPathCreateAppData(TempTitle ? TempTitle : B2EngineConfiguration.title);
    CCString Path = CCStringCreate(CC_STD_ALLOCATOR, CCStringEncodingUTF8 | CCStringHintCopy, FSPathGetPathString(AppDataPath));
    FSPathDestroy(AppDataPath);
    
    return CCExpressionCreateString(CC_STD_ALLOCATOR, Path, FALSE);
}
