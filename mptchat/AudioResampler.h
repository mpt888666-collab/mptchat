//
// Created by mpt on 2026/9/27.
//

#ifndef MPTCHAT_AUDIORESAMPLER_H
#define MPTCHAT_AUDIORESAMPLER_H


#include <QByteArray>
#include <vector>

#include <QByteArray>
#include <vector>
#include <samplerate.h>
class AudioResampler{
public:
    AudioResampler(int inRate, int channels);
    ~AudioResampler();

    QByteArray push(const QByteArray& pcmInt16);

    void reset();

    bool isValid() const { return _srcState != nullptr; }

private:
    int _inRate{0};
    int _channels{1};
    int _targetRate{16000};

    SRC_STATE* _srcState{nullptr};
    double _ratio{1.0};
};


#endif //MPTCHAT_AUDIORESAMPLER_H