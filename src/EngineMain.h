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
 * @brief Set this to the amount of time (in seconds) the update thread should occur.
 * @description The default value is set at 1/60 update cycles.
 */
extern double B2UpdateThreadTimeSlice;

/*!
 * @brief The engine asset path.
 * @note This should only be set if using a different project structure.
 */
extern FSPath B2EnginePath;

#endif
