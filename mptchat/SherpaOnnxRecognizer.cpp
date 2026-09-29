//
// Created by mpt on 2026/9/26.
//

#include "SherpaOnnxRecognizer.h"
#include "sherpa-onnx/c-api/c-api.h"
#include <QThread>
#include <QDebug>
const std::string SherpaOnnxRecognizer::kModelDir =
    R"(C:\Users\mpt\ChatDemo\mptchat\models\sherpa-onnx-streaming-zipformer-zh-int8-2025-06-30)";

void SherpaOnnxRecognizer::DestroyRecognizer(SherpaOnnxOnlineRecognizer* p)
{
    if(p)
    {
        SherpaOnnxDestroyOnlineRecognizer(p);
    }
}

void SherpaOnnxRecognizer::DestroyStream(SherpaOnnxOnlineStream* p)
{
    if(p)
    {
        SherpaOnnxDestroyOnlineStream(p);
    }
}

RecognizerThread::RecognizerThread() {
    _recognizer_thread = new QThread();
    _recognizer_thread->setObjectName("recognizer_thread");

    auto mgr = SherpaOnnxRecognizer::instance();

    mgr->moveToThread(_recognizer_thread);


    _recognizer_thread->start();

    QMetaObject::invokeMethod(mgr.get(), "slotInit", Qt::QueuedConnection);

    qDebug() << "asr thread object:" << _recognizer_thread
             << "worker thread name:" << QThread::currentThread()->objectName()
             << "ready:" << mgr->isReady();
}

RecognizerThread::~RecognizerThread() {
    if (!_recognizer_thread)
        return;

    auto mgr = SherpaOnnxRecognizer::instance();
    if (mgr && _recognizer_thread->isRunning()) {
        mgr->moveToThread(QThread::currentThread());
    }
    _recognizer_thread->quit();
    _recognizer_thread->wait();
    delete _recognizer_thread;
}

SherpaOnnxRecognizer::SherpaOnnxRecognizer() : _recognizer(nullptr), _ready(false){

}

bool SherpaOnnxRecognizer::Init() {
    std::string enc = kModelDir + "\\encoder.int8.onnx";
    std::string dec = kModelDir + "\\decoder.onnx";
    std::string joi = kModelDir + "\\joiner.int8.onnx";
    std::string tok = kModelDir + "\\tokens.txt";

    const std::string* files[] = {&enc, &dec, &joi, &tok};
    for (const std::string* f : files)
    {
        if (!SherpaOnnxFileExists(f->c_str()))
        {
            qWarning() << "模型文件不存在:" << QString::fromStdString(*f);
            //emit sigInitFailed(QString::fromStdString(*f) + QStringLiteral(" 不存在"));
            return false;
        }
    }
    SherpaOnnxOnlineRecognizerConfig config{};

    config.feat_config.sample_rate = 16000;
    config.feat_config.feature_dim = 80;

    config.model_config.transducer.encoder = enc.c_str();
    config.model_config.transducer.decoder = dec.c_str();
    config.model_config.transducer.joiner = joi.c_str();
    config.model_config.tokens = tok.c_str();

    config.model_config.num_threads = 2;
    config.model_config.provider = "cpu";
    config.decoding_method = "greedy_search";

    config.enable_endpoint = 1;
    config.rule2_min_trailing_silence = 0.7f;

    _recognizer.reset(const_cast<SherpaOnnxOnlineRecognizer*>(SherpaOnnxCreateOnlineRecognizer(&config)), DestroyRecognizer);

    if (!_recognizer)
    {
        qWarning() << "SherpaOnnxCreateOnlineRecognizer 返回空";
        return false;
    }

    _ready = true;
    qDebug() << "sherpa-onnx 识别器就绪，所在线程:" << QThread::currentThread()->objectName();
    return true;
}

void SherpaOnnxRecognizer::slotInit()
{
    bool ok = Init();
    emit sigInitFinished(ok);
}

void SherpaOnnxRecognizer::slotCreateStream() {
    CreateStream();
}

bool SherpaOnnxRecognizer::CreateStream() {
    if (!_recognizer) return false;
    if (_stream) return true;
    _stream.reset(const_cast<SherpaOnnxOnlineStream*>(SherpaOnnxCreateOnlineStream(_recognizer.get())), DestroyStream);
    if (!_stream)
    {
        qWarning() << "SherpaOnnxCreateOnlineStream 返回空";
        return false;
    }
    return true;
}


void SherpaOnnxRecognizer::slotAcceptWaveform(QByteArray pcm) {
    if (!_ready || !_recognizer || !_stream) return;
    const int n = pcm.size() / 2;
    if (n <= 0) return;
    _f32buf.resize(n);

    const auto *raw = reinterpret_cast<const int16_t *>(pcm.constData());
    for (int i = 0; i < n; ++i) _f32buf[i] = raw[i] / 32768.0f;

    SherpaOnnxOnlineStreamAcceptWaveform(_stream.get(), 16000, _f32buf.data(), n);

    while (SherpaOnnxIsOnlineStreamReady(_recognizer.get(), _stream.get())) {
        SherpaOnnxDecodeOnlineStream(_recognizer.get(), _stream.get());
    }

    if (const SherpaOnnxOnlineRecognizerResult *r = SherpaOnnxGetOnlineStreamResult(_recognizer.get(), _stream.get())) {
        const QString text = QString::fromUtf8(r->text ? r->text : "");
        if (text != _lastPartial) {
            _lastPartial = text;
            emit sigPartialText(text);
        }
        SherpaOnnxDestroyOnlineRecognizerResult(r);
    }

    if (SherpaOnnxOnlineStreamIsEndpoint(_recognizer.get(), _stream.get())) {
        if (!_lastPartial.isEmpty()) emit sigSentence(_lastPartial);
        _lastPartial.clear();
        SherpaOnnxOnlineStreamReset(_recognizer.get(), _stream.get());
    }
}

void SherpaOnnxRecognizer::slotFinishInput() {
    if (!_ready || !_stream) return;

    SherpaOnnxOnlineStreamInputFinished(_stream.get());
    while (SherpaOnnxIsOnlineStreamReady(_recognizer.get(), _stream.get())) {
        SherpaOnnxDecodeOnlineStream(_recognizer.get(), _stream.get());
    }

    if (const SherpaOnnxOnlineRecognizerResult *r =SherpaOnnxGetOnlineStreamResult(_recognizer.get(), _stream.get())) {
        const QString text = QString::fromUtf8(r->text ? r->text : "");
        if (!text.isEmpty()) emit sigSentence(text);
        SherpaOnnxDestroyOnlineRecognizerResult(r);
    }

    _lastPartial.clear();
    _stream.reset();
}



