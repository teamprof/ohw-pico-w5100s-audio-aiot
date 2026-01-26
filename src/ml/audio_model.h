/* Copyright 2026 teamprof.net@gmail.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#pragma once

// define before include TFLM headers to disable TFLM DebugLog(const char* s), which is conflict with ArduPorf DebugLog
#define TENSORFLOW_LITE_MICRO_DEBUG_LOG_H_
#include <Chirale_TensorFlowLite.h>
#include <tensorflow/lite/micro/micro_mutable_op_resolver.h>
#include <tensorflow/lite/micro/all_ops_resolver.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/schema/schema_generated.h>
#include <tensorflow/lite/micro/tflite_bridge/micro_error_reporter.h>

const int kSpectrogramWidth = 124;
const int kSpectrogramHeight = 129;

using AudioOpResolver = tflite::MicroMutableOpResolver<7>;

class AudioModel
{
public:
    AudioModel(uint8_t *tensor_arena, int tensor_arena_size);
    virtual ~AudioModel();

    static AudioModel *getInstance(void);
    TfLiteStatus init(void);

    float inference(void);

    void *input_data();
    float input_scale() const;
    int32_t input_zero_point() const;
    int32_t input_width(void) const;
    int32_t input_height(void) const;

private:
    static AudioModel *_instance;
    uint8_t *_tensor_arena;
    int _tensor_arena_size;

    const unsigned char *_tflite_model;

    tflite::ErrorReporter *_error_reporter;
    const tflite::Model *_model;
    tflite::MicroInterpreter *_interpreter;
    TfLiteTensor *_input_tensor;
    TfLiteTensor *_output_tensor;
    AudioOpResolver _opsResolver;
    TfLiteStatus initOpsResolver(void);
};