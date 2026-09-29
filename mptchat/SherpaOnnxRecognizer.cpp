//
// Created by mpt on 2026/9/26.
//

#include "SherpaOnnxRecognizer.h"
#include "sherpa-onnx/c-api/c-api.h"
#include <QThread>
#include <QDebug>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStringList>
#include "ConfigMgr.h"

namespace {

// 采集 / 重采样 / 特征提取必须一致的量：不做成运行时配置。
// 改这里必须同步 AudioRecorder 的采集格式和 AudioResampler 的目标采样率。
constexpr int kFeatSampleRate = 16000;
constexpr int kFeatDim = 80;

} // namespace

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
    const ConfigMgr &cfg = ConfigMgr::Inst();

    QString model_dir = cfg.value(QStringLiteral("ASR"), QStringLiteral("model_dir"),
                                  QStringLiteral("models/sherpa-onnx-streaming-zipformer-zh-int8-2025-06-30"));
    if (!QFileInfo(model_dir).isAbsolute()) {
        model_dir = QDir(QCoreApplication::applicationDirPath()).filePath(model_dir);
    }

    const auto model_file = [&cfg, &model_dir](const QString &key, const QString &defaultName) {
        return QDir(model_dir).filePath(cfg.value(QStringLiteral("ASR"), key, defaultName));
    };

    const std::string enc = model_file(QStringLiteral("encoder"), QStringLiteral("encoder.int8.onnx")).toStdString();
    const std::string dec = model_file(QStringLiteral("decoder"), QStringLiteral("decoder.onnx")).toStdString();
    const std::string joi = model_file(QStringLiteral("joiner"),  QStringLiteral("joiner.int8.onnx")).toStdString();
    const std::string tok = model_file(QStringLiteral("tokens"),  QStringLiteral("tokens.txt")).toStdString();

    for (const std::string *f : {&enc, &dec, &joi, &tok}) {
        if (!SherpaOnnxFileExists(f->c_str())) {
            qWarning() << "[ASR] 模型文件不存在:" << QString::fromStdString(*f);
            return false;
        }
    }

    int threads = cfg.value(QStringLiteral("ASR"), QStringLiteral("num_threads"), QStringLiteral("2")).toInt();
    threads = qBound(1, threads, QThread::idealThreadCount());

    QString provider = cfg.value(QStringLiteral("ASR"), QStringLiteral("provider"), QStringLiteral("cpu"));
    static const QStringList kProviders{QStringLiteral("cpu"), QStringLiteral("cuda"),
                                        QStringLiteral("directml")};
    if (!kProviders.contains(provider)) {
        qWarning() << "[ASR] provider 不在白名单，回退 cpu:" << provider;
        provider = QStringLiteral("cpu");
    }
    
    const QByteArray provider_utf8 = provider.toUtf8();
    const QByteArray method_utf8 =
        cfg.value(QStringLiteral("ASR"), QStringLiteral("decoding_method"),
                  QStringLiteral("greedy_search")).toUtf8();

    const bool endpoint_on =
        cfg.value(QStringLiteral("ASR"), QStringLiteral("enable_endpoint"), QStringLiteral("1")).toInt() != 0;
    const float rule1 = cfg.value(QStringLiteral("ASR"), QStringLiteral("rule1_min_trailing_silence"),
                                  QStringLiteral("2.4")).toFloat();
    const float rule2 = cfg.value(QStringLiteral("ASR"), QStringLiteral("rule2_min_trailing_silence"),
                                  QStringLiteral("0.7")).toFloat();
    const float rule3 = cfg.value(QStringLiteral("ASR"), QStringLiteral("rule3_min_utterance_length"),
                                  QStringLiteral("20")).toFloat();

    SherpaOnnxOnlineRecognizerConfig config{};

    config.feat_config.sample_rate = kFeatSampleRate;
    config.feat_config.feature_dim = kFeatDim;

    config.model_config.transducer.encoder = enc.c_str();
    config.model_config.transducer.decoder = dec.c_str();
    config.model_config.transducer.joiner = joi.c_str();
    config.model_config.tokens = tok.c_str();

    config.model_config.num_threads = threads;
    config.model_config.provider = provider_utf8.constData();
    config.decoding_method = method_utf8.constData();

    config.enable_endpoint = endpoint_on ? 1 : 0;
    config.rule1_min_trailing_silence = rule1;
    config.rule2_min_trailing_silence = rule2;
    config.rule3_min_utterance_length = rule3;

    _recognizer.reset(const_cast<SherpaOnnxOnlineRecognizer*>(SherpaOnnxCreateOnlineRecognizer(&config)), DestroyRecognizer);

    if (!_recognizer) {
        qWarning() << "[ASR] SherpaOnnxCreateOnlineRecognizer 返回空，检查模型文件与 onnxruntime";
        return false;
    }

    _ready = true;
    qDebug().noquote() << "[ASR] 就绪 | model_dir:" << model_dir
                       << "| threads:" << threads
                       << "| provider:" << provider
                       << "| method:" << QString::fromUtf8(method_utf8)
                       << "| endpoint:" << endpoint_on
                       << "| rule: " << rule1 << rule2 << rule3
                       << "| asr thread:" << QThread::currentThread()->objectName();
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



