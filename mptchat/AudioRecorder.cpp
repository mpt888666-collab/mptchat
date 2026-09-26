//
// Created by mpt on 2026/9/26.
//

#include "AudioRecorder.h"
#include <QDebug>

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
        qWarning() << "设备不支持 16k/单声道/Int16，preferred =" << dev.preferredFormat();
        return;
    }

    _source = new QAudioSource(dev, fmt, this);
    // 想压低延迟可以设 0.1 秒的缓冲，但太小会欠载出杂音，默认别动
    // _source->setBufferSize(3200);   // 16000Hz * 0.1s * 2字节

    _usable = true;
}

AudioRecorder::~AudioRecorder()
{
    slotStop();
}

void AudioRecorder::slotStart()
{
    if (_recording || !_usable || !_source) return;

    _io = _source->start();        // 拉模式：麦克风的数据流进这个 QIODevice
    if (!_io) {
        emit sigError(QStringLiteral("启动录音失败"));
        return;
    }

    connect(_io, &QIODevice::readyRead, this, [this]() {
        const QByteArray pcm = _io->readAll();
        if (!pcm.isEmpty()) emit sigAudioReady(pcm);
    });

    _recording = true;
    qDebug() << "开始录音, state =" << _source->state();
}

void AudioRecorder::slotStop()
{
    if (!_recording) return;
    _recording = false;

    if (_io) {
        // 把设备缓冲里剩的尾音也读出来，别丢最后一个字
        const QByteArray tail = _io->readAll();
        if (!tail.isEmpty()) emit sigAudioReady(tail);

        _io->disconnect(this);   // 先断开，避免 stop 过程里再回调
        _io = nullptr;           // 只置空，不要 delete
    }
    if (_source) _source->stop();
}