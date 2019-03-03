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

#import <XCTest/XCTest.h>
#import "ProjectExpressions.h"
#import "Configuration.h"

@interface ExpressionProjectTests : XCTestCase

@end

@implementation ExpressionProjectTests

+(void) setUp
{
    [super setUp];
    
    CCExpressionEvaluatorRegister(CC_STRING("game"), B2ProjectExpressionGame);
}

-(void) testProject
{
    CCExpression Expression = CCExpressionCreateFromSource("(game \"Test Game\""
                                                           "    (default-resolution: 640 480)\n"
                                                           "    (default-fullscreen: #f)\n"
                                                           "\n"
                                                           "    (renderer: :opengl)\n"
                                                           "    (vsync: #t)\n"
                                                           "\n"
                                                           "    (dir-fonts: \"font/\")\n"
                                                           "    (dir-levels: \"logic/levels/\")\n"
                                                           "    (dir-rules: \"logic/rules/\")\n"
                                                           "    (dir-textures: \"graphics/textures/\")\n"
                                                           "    (dir-shaders: \"graphics/shaders/\")\n"
                                                           "    (dir-sounds: \"audio/\")\n"
                                                           "    (dir-layouts: \"ui/\")\n"
                                                           "    (dir-entities: \"logic/entities/\")\n"
                                                           "    (dir-save: \"save/\")\n"
                                                           "    (dir-logs: \"logs/\")\n"
                                                           "    (dir-tmp: \"tmp/\")\n"
                                                           ")\n");
    
    CCExpression Result = CCExpressionEvaluate(Expression);
    XCTAssertEqual(CCExpressionGetType(Result), B2ProjectExpressionValueTypeGameConfig, @"Should be a project config");
    XCTAssertTrue(!strcmp(((B2EngineConfig*)CCExpressionGetData(Result))->title, "Test Game"), @"Should be initialized");
    XCTAssertEqual(((B2EngineConfig*)CCExpressionGetData(Result))->window.fullscreen, FALSE, @"Should be initialized");
    XCTAssertEqual(((B2EngineConfig*)CCExpressionGetData(Result))->window.width, 640, @"Should be initialized");
    XCTAssertEqual(((B2EngineConfig*)CCExpressionGetData(Result))->window.height, 480, @"Should be initialized");
    XCTAssertEqual(((B2EngineConfig*)CCExpressionGetData(Result))->renderer.pipeline, B2EngineRenderPipelineOpenGL, @"Should be initialized");
    XCTAssertTrue(((B2EngineConfig*)CCExpressionGetData(Result))->renderer.vsync, @"Should be initialized");
    XCTAssertTrue(!strcmp(FSPathGetPathString(*(FSPath*)CCOrderedCollectionGetElementAtIndex(((B2EngineConfig*)CCExpressionGetData(Result))->directory.fonts, 0)), "font/"), @"Should be initialized");
    XCTAssertTrue(!strcmp(FSPathGetPathString(*(FSPath*)CCOrderedCollectionGetElementAtIndex(((B2EngineConfig*)CCExpressionGetData(Result))->directory.levels, 0)), "logic/levels/"), @"Should be initialized");
    XCTAssertTrue(!strcmp(FSPathGetPathString(*(FSPath*)CCOrderedCollectionGetElementAtIndex(((B2EngineConfig*)CCExpressionGetData(Result))->directory.rules, 0)), "logic/rules/"), @"Should be initialized");
    XCTAssertTrue(!strcmp(FSPathGetPathString(*(FSPath*)CCOrderedCollectionGetElementAtIndex(((B2EngineConfig*)CCExpressionGetData(Result))->directory.textures, 0)), "graphics/textures/"), @"Should be initialized");
    XCTAssertTrue(!strcmp(FSPathGetPathString(*(FSPath*)CCOrderedCollectionGetElementAtIndex(((B2EngineConfig*)CCExpressionGetData(Result))->directory.shaders, 0)), "graphics/shaders/"), @"Should be initialized");
    XCTAssertTrue(!strcmp(FSPathGetPathString(*(FSPath*)CCOrderedCollectionGetElementAtIndex(((B2EngineConfig*)CCExpressionGetData(Result))->directory.sounds, 0)), "audio/"), @"Should be initialized");
    XCTAssertTrue(!strcmp(FSPathGetPathString(*(FSPath*)CCOrderedCollectionGetElementAtIndex(((B2EngineConfig*)CCExpressionGetData(Result))->directory.layouts, 0)), "ui/"), @"Should be initialized");
    XCTAssertTrue(!strcmp(FSPathGetPathString(*(FSPath*)CCOrderedCollectionGetElementAtIndex(((B2EngineConfig*)CCExpressionGetData(Result))->directory.entities, 0)), "logic/entities/"), @"Should be initialized");
    XCTAssertTrue(!strcmp(FSPathGetPathString(((B2EngineConfig*)CCExpressionGetData(Result))->directory.save), "save/"), @"Should be initialized");
    XCTAssertTrue(!strcmp(FSPathGetPathString(((B2EngineConfig*)CCExpressionGetData(Result))->directory.logs), "logs/"), @"Should be initialized");
    XCTAssertTrue(!strcmp(FSPathGetPathString(((B2EngineConfig*)CCExpressionGetData(Result))->directory.tmp), "tmp/"), @"Should be initialized");
    
    CCExpressionDestroy(Expression);
}

-(void) testDefaultProject
{
    CCExpression Expression = CCExpressionCreateFromSource("(game)\n");
    
    CCExpression Result = CCExpressionEvaluate(Expression);
    XCTAssertEqual(CCExpressionGetType(Result), B2ProjectExpressionValueTypeGameConfig, @"Should be a project config");
    XCTAssertEqual(((B2EngineConfig*)CCExpressionGetData(Result))->title, NULL, @"Should be initialized");
    XCTAssertEqual(((B2EngineConfig*)CCExpressionGetData(Result))->window.fullscreen, FALSE, @"Should be initialized");
    XCTAssertEqual(((B2EngineConfig*)CCExpressionGetData(Result))->window.width, 0, @"Should be initialized");
    XCTAssertEqual(((B2EngineConfig*)CCExpressionGetData(Result))->window.height, 0, @"Should be initialized");
    XCTAssertEqual(((B2EngineConfig*)CCExpressionGetData(Result))->renderer.pipeline, B2EngineRenderPipelineNone, @"Should be initialized");
    XCTAssertTrue(((B2EngineConfig*)CCExpressionGetData(Result))->renderer.vsync, @"Should be initialized");
    XCTAssertEqual(CCCollectionGetCount(((B2EngineConfig*)CCExpressionGetData(Result))->directory.fonts), 0, @"Should be initialized");
    XCTAssertEqual(CCCollectionGetCount(((B2EngineConfig*)CCExpressionGetData(Result))->directory.levels), 0, @"Should be initialized");
    XCTAssertEqual(CCCollectionGetCount(((B2EngineConfig*)CCExpressionGetData(Result))->directory.rules), 0, @"Should be initialized");
    XCTAssertEqual(CCCollectionGetCount(((B2EngineConfig*)CCExpressionGetData(Result))->directory.textures), 0, @"Should be initialized");
    XCTAssertEqual(CCCollectionGetCount(((B2EngineConfig*)CCExpressionGetData(Result))->directory.shaders), 0, @"Should be initialized");
    XCTAssertEqual(CCCollectionGetCount(((B2EngineConfig*)CCExpressionGetData(Result))->directory.sounds), 0, @"Should be initialized");
    XCTAssertEqual(CCCollectionGetCount(((B2EngineConfig*)CCExpressionGetData(Result))->directory.layouts), 0, @"Should be initialized");
    XCTAssertEqual(CCCollectionGetCount(((B2EngineConfig*)CCExpressionGetData(Result))->directory.entities), 0, @"Should be initialized");
    XCTAssertEqual(((B2EngineConfig*)CCExpressionGetData(Result))->directory.save, NULL, @"Should be initialized");
    XCTAssertEqual(((B2EngineConfig*)CCExpressionGetData(Result))->directory.logs, NULL, @"Should be initialized");
    XCTAssertEqual(((B2EngineConfig*)CCExpressionGetData(Result))->directory.tmp, NULL, @"Should be initialized");
    
    CCExpressionDestroy(Expression);
}

@end
