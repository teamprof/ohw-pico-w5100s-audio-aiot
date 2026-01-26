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
#include <math.h>

#include "audio_model.h"
#include "tflite_model.h"
#include "../AppDef.h"

static ML_DATA __ALIGNED(8) uint8_t tensor_arena[64 * 1024];

////////////////////////////////////////////////////////////////////////////////////////////
AudioModel *AudioModel::_instance = nullptr;

////////////////////////////////////////////////////////////////////////////////////////////

AudioModel::AudioModel(uint8_t *tensor_arena,
                       int tensor_arena_size) : _tensor_arena(tensor_arena),
                                                _tensor_arena_size(tensor_arena_size),
                                                _model(NULL),
                                                _interpreter(NULL),
                                                _input_tensor(NULL),
                                                _output_tensor(NULL)
{
    static tflite::MicroErrorReporter micro_error_reporter;
    _error_reporter = &micro_error_reporter;
}

AudioModel::~AudioModel()
{
    if (_interpreter != NULL)
    {
        delete _interpreter;
        _interpreter = NULL;
    }
}

AudioModel *AudioModel::getInstance(void)
{
    if (!_instance)
    {
        static ML_DATA AudioModel _model(tensor_arena, sizeof(tensor_arena));
        _instance = &_model;
    }
    return _instance;
}

TfLiteStatus AudioModel::initOpsResolver(void)
{
    TF_LITE_ENSURE_STATUS(_opsResolver.AddConv2D());
    TF_LITE_ENSURE_STATUS(_opsResolver.AddMaxPool2D());
    TF_LITE_ENSURE_STATUS(_opsResolver.AddFullyConnected());
    TF_LITE_ENSURE_STATUS(_opsResolver.AddReshape());
    TF_LITE_ENSURE_STATUS(_opsResolver.AddSoftmax());
    TF_LITE_ENSURE_STATUS(_opsResolver.AddResizeNearestNeighbor());
    TF_LITE_ENSURE_STATUS(_opsResolver.AddLogistic());
    return kTfLiteOk;
}

TfLiteStatus AudioModel::init(void)
{
    _model = tflite::GetModel(tflite_model);
    if (_model->version() != TFLITE_SCHEMA_VERSION)
    {
        TF_LITE_REPORT_ERROR(_error_reporter,
                             "Model provided is schema version %d not equal "
                             "to supported version %d.",
                             _model->version(), TFLITE_SCHEMA_VERSION);

        return kTfLiteError;
    }

    if (initOpsResolver() != kTfLiteOk)
    {
        TF_LITE_REPORT_ERROR(_error_reporter,
                             "Failed to initOpsResolver");
        return kTfLiteUnresolvedOps;
    }
    MicroPrintf("initOpsResolver() success");

    _interpreter = new tflite::MicroInterpreter(
        _model, _opsResolver,
        _tensor_arena, _tensor_arena_size);
    if (_interpreter == NULL)
    {
        TF_LITE_REPORT_ERROR(_error_reporter,
                             "Failed to allocate interpreter");
        return kTfLiteError;
    }
    MicroPrintf("new tflite::MicroInterpreter() success");

    TfLiteStatus allocate_status = _interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk)
    {
        TF_LITE_REPORT_ERROR(_error_reporter, "AllocateTensors() failed");
        return kTfLiteError;
    }
    MicroPrintf("_interpreter->AllocateTensors() success");

    _input_tensor = _interpreter->input(0);
    _output_tensor = _interpreter->output(0);

    return kTfLiteOk;
}

void *AudioModel::input_data()
{
    return (_input_tensor == NULL) ? NULL : _input_tensor->data.data;
}

float AudioModel::inference(void)
{
    if (!_interpreter)
    {
        TF_LITE_REPORT_ERROR(_error_reporter, "_interpreter is null");
        return NAN;
    }

    TfLiteStatus invoke_status = _interpreter->Invoke();
    if (invoke_status != kTfLiteOk)
    {
        TF_LITE_REPORT_ERROR(_error_reporter, "_interpreter->Invoke() failed");
        return NAN;
    }

    float y_quantized = _output_tensor->data.int8[0];
    float y = (y_quantized - _output_tensor->params.zero_point) * _output_tensor->params.scale;
    return y;
}

float AudioModel::input_scale() const
{
    return (_input_tensor == NULL) ? NAN : _input_tensor->params.scale;
}

int32_t AudioModel::input_zero_point() const
{
    return (_input_tensor == NULL) ? 0 : _input_tensor->params.zero_point;
}

int32_t AudioModel::input_width(void) const
{
    return (_input_tensor->dims->size > 1) ? _input_tensor->dims->data[1] : -1;
}
int32_t AudioModel::input_height(void) const
{
    return (_input_tensor->dims->size > 2) ? _input_tensor->dims->data[2] : -1;
}
