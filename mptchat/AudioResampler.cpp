//
// Created by mpt on 2026/9/27.
//

#include "AudioResampler.h"
#include <QByteArray>
#include <QDebug>

AudioResampler::AudioResampler(int inRate, int channels) : _inRate(inRate), _channels(channels){
    _ratio = static_cast<double>(_targetRate) / _inRate;

    int err = 0;
    // SRC_SINC_FASTEST：速度与音质平衡；SRC_SINC_BEST_QUALITY音质更好更慢
    _srcState = src_new(SRC_SINC_FASTEST, _channels, &err);
}

AudioResampler::~AudioResampler(){
    if (_srcState){
        src_delete(_srcState);
        _srcState = nullptr;
    }
}

void AudioResampler::reset(){
    if (_srcState){
        src_reset(_srcState);
    }
}

QByteArray AudioResampler::push(const QByteArray& pcmInt16)
{
    if (!_srcState || pcmInt16.isEmpty() || _channels <= 0)
        return {};
    qDebug() << "come here=============================================";
    const int inSampleCountAll = static_cast<int>(pcmInt16.size() / sizeof(int16_t));
    const int16_t* pIn16 = reinterpret_cast<const int16_t*>(pcmInt16.constData());

    // 1. int16 → float [-1.0, 1.0]
    std::vector<float> inFloat(inSampleCountAll);
    src_short_to_float_array(pIn16, inFloat.data(), inSampleCountAll);

    // 预估输出样本上限
    const size_t outEstimate = static_cast<size_t>(inSampleCountAll * _ratio) + 512;
    std::vector<float> outFloat(outEstimate);

    SRC_DATA srcData{};
    srcData.data_in = inFloat.data();
    srcData.input_frames = inSampleCountAll / _channels;
    // frames：帧，1帧 = channels个样本；立体声1帧包含左+右两个样本
    srcData.data_out = outFloat.data();
    srcData.output_frames = static_cast<int>(outEstimate / _channels);
    srcData.src_ratio = _ratio;
    srcData.end_of_input = 0; // 流式，不是文件结束

    int ret = src_process(_srcState, &srcData);
    if (ret != 0 || srcData.output_frames_gen <= 0)
        return {};

    int outFramesGen = srcData.output_frames_gen;

    std::vector<int16_t> outInt16;
    outInt16.resize(static_cast<size_t>(outFramesGen * _channels));
    src_float_to_short_array(outFloat.data(), outInt16.data(), outFramesGen * _channels);

    // 如果是多声道，取左声道，转为单声道给 sherpa‑onnx
    std::vector<int16_t> mono16;
    mono16.reserve(static_cast<size_t>(outFramesGen));
    for (int i = 0; i < outFramesGen * _channels; i += _channels)
    {
        mono16.push_back(outInt16[static_cast<size_t>(i)]);
    }

    QByteArray outBytes(reinterpret_cast<const char*>(mono16.data()),
                        static_cast<int>(mono16.size() * sizeof(int16_t)));
    return outBytes;
}

