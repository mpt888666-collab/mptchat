//
// Created by mpt on 2026/9/26.
//

#include "AudioRecorder.h"
#include <QDebug>
#include "AudioResampler.h"
AudioRecorder::AudioRecorder(QObject *parent) : QObject(parent)
{
    const QAudioDevice dev = QMediaDevices::defaultAudioInput();
    if (dev.isNull()) {
        qWarning() << "没有找到可用的麦克风";
        return;
    }

    QAudioFormat fmt;
    fmt.setSampleRate(16000);
    fmt.setChannelCount(1);
    fmt.setSampleFormat(QAudioFormat::Int16);

    if (!dev.isFormatSupported(fmt)) {
        QAudioFormat rawFmt = dev.preferredFormat();
        rawFmt.setSampleFormat(QAudioFormat::Int16);
        if (!dev.isFormatSupported(rawFmt)) {
            qWarning() << "设备不支持 int16 采集格式";
            return;
        }
        _needResample = true;
        _resampler = new AudioResampler(rawFmt.sampleRate(), rawFmt.channelCount());
        _source = new QAudioSource(dev, rawFmt, this);
    }else {
        _source = new QAudioSource(dev, fmt, this);
    }
    _usable = true;
}

AudioRecorder::~AudioRecorder()
{
    slotStop();
}

void AudioRecorder::slotStart()
{
    if (_recording || !_usable || !_source) return;

    _io = _source->start();
    if (!_io) {
        emit sigError(QStringLiteral("启动录音失败"));
        return;
    }

    connect(_io, &QIODevice::readyRead, this, [this]() {
        QByteArray pcm = _io->readAll();
        if(pcm.isEmpty()) return;

        if(!_needResample){
            // 已经是16k单声道int16，直接转float喂sherpa
            emit sigAudioReady(pcm);
        }else{
            // 走重采样
            QByteArray out16k = _resampler->push(pcm);
            if(!out16k.isEmpty()) {
                emit sigAudioReady(out16k);
            }
        }

    });

    _recording = true;
    qDebug() << "开始录音, state =" << _source->state();
}

void AudioRecorder::slotStop()
{
    if (!_recording) return;
    _recording = false;

    if (_io) {
        const QByteArray tail = _io->readAll();
        if (!tail.isEmpty()) {
            if (_needResample && _resampler) {
                const QByteArray out16k = _resampler->push(tail);
                if (!out16k.isEmpty()) emit sigAudioReady(out16k);
            } else {
                emit sigAudioReady(tail);
            }
        }
        _io->disconnect(this);
        _io = nullptr;
    }
    if (_source) _source->stop();
}