//
// Created by mpt on 2026/9/26.
//

#ifndef MPTCHAT_SHERPAONNXRECOGNIZER_H
#define MPTCHAT_SHERPAONNXRECOGNIZER_H
#include "singleton.h"
#include <memory>
#include <QObject>

class RecognizerThread : public QObject{
    Q_OBJECT
public:
    RecognizerThread();

    ~RecognizerThread() override;

private:
    QThread* _recognizer_thread;
};

class SherpaOnnxOnlineRecognizer;
class SherpaOnnxOnlineStream;
class SherpaOnnxOnlineRecognizerResult;
class SherpaOnnxRecognizer : public QObject, public Singleton<SherpaOnnxRecognizer>{
    Q_OBJECT
    friend Singleton<SherpaOnnxRecognizer>;
public:
    bool Init();
    [[nodiscard]] bool isReady() const { return _ready; }

    bool CreateStream();
public slots:
    void slotInit();
    void slotAcceptWaveform(QByteArray pcm);
    void slotFinishInput();
    void slotCreateStream();
signals:
    void sigPartialText(QString text);
    void sigSentence(QString text);
    void sigInitFinished(bool ok);
private:

    static void DestroyRecognizer(SherpaOnnxOnlineRecognizer* p);

    static void DestroyStream(SherpaOnnxOnlineStream* p);


    SherpaOnnxRecognizer();

    std::shared_ptr<const SherpaOnnxOnlineRecognizer> _recognizer;
    std::shared_ptr<const SherpaOnnxOnlineStream> _stream;
    QString _lastPartial{};
    std::vector<float> _f32buf;
    std::atomic<bool> _ready;

};


#endif //MPTCHAT_SHERPAONNXRECOGNIZER_H