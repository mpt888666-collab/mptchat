//
// Created by mpt on 2026/9/26.
//

#ifndef MPTCHAT_AUDIORECORDER_H
#define MPTCHAT_AUDIORECORDER_H


#include <QObject>
#include <QAudioSource>
#include <QAudioFormat>
#include <QAudioDevice>
#include <QMediaDevices>

class AudioRecorder : public QObject {
    Q_OBJECT
public:
    explicit AudioRecorder(QObject *parent = nullptr);
    ~AudioRecorder() override;

    bool isRecording() const { return _recording; }
    bool isUsable()    const { return _usable; }

public slots:
    void slotStart();
    void slotStop();

    signals:
        void sigAudioReady(QByteArray pcm);   // 16kHz / 单声道 / Int16
    void sigError(QString msg);

private:
    QAudioSource *_source{nullptr};
    QIODevice    *_io{nullptr};
    bool _recording{false};
    bool _usable{false};
};


#endif //MPTCHAT_AUDIORECORDER_H