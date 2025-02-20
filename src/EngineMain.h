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

#ifndef Blob2D_EngineMain_h
#define Blob2D_EngineMain_h

#include <Blob2D/Base.h>

/*!
 * @brief Called when the engine has started setting up.
 * @description Use this callback to handle custom pre-setup.
 */
extern void (*B2EngineSetupBegin)(void);

/*!
 * @brief Called when the engine has finished setting up.
 * @description Use this callback to handle custom setup.
 */
extern void (*B2EngineSetupComplete)(void);

/*!
 * @brief The main entrypoint into the engine.
 * @param argc The number of entries in argv.
 * @param argv The program inputs.
 * @result The error code upon completion/exit of your program.
 */
int B2EngineRun(int argc, const char *argv[]);

/*!
 * @brief Set this to the amount of time (in nanoseconds) the update thread should occur.
 * @description The default value is set at 1/60 update cycles.
 */
extern uint64_t B2UpdateThreadTimeSlice;

/*!
 * @brief The engine asset path.
 * @note This should only be set if using a different project structure.
 */
extern FSPath B2EnginePath;

typedef struct {
    ECSContext *context;
    size_t groupCount;
    ECSGroup *groups;
    ECSExecutionGroup *state;
} B2ECSInstance;

typedef struct {
    uint32_t numerator;
    uint32_t denominator;
} B2TimeScale;

typedef struct {
    size_t count;
    B2ECSInstance *instances;
    struct {
        _Atomic(B2TimeScale) *scale;
        _Atomic(uint64_t) *wait;
        _Atomic(_Bool) changed;
    } tick;
} B2ECSConfig;

/*!
 * @brief Set this to the ECS configuration to be used by the engine.
 * @note This should be set and the contexts initialised prior to engine setup.
 */
extern B2ECSConfig B2EngineECS;

typedef enum {
    B2ThreadMain,
    B2ThreadUpdate,
    B2ThreadRender
} B2Thread;

/*!
 * @brief A callback a waiting thread calls.
 * @param Thread The calling thread. Note that there are no guarantees that any one thread will ever call this callback.
 * @param Available The amount of available execution time. Execution should return before this amount of time elapses.
 * @param ReturnBy Is the timestamp the function should return by.
 */
typedef void (*B2WaitingCallback)(B2Thread Thread, uint64_t Available, uint64_t ReturnBy);

/*!
 * @brief Set this to the waiting callback.
 * @note This should be set by @b B2EngineSetupComplete at the latest.
 */
extern B2WaitingCallback B2Waiting;

/*!
 * @brief The refresh rate of the input thread in seconds. Defaults to 1ms (0.001).
 * @description This mainly influences the refresh rate of the controller state and clipboard.
 * @note This may run faster than the specified rate, but will not run slower.
 */
extern double B2InputFrequency;

#endif
