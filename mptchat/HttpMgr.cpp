//
// Created by mpt on 2026/6/28.
//

#include "HttpMgr.h"
#include "HttpMgr.h"

#include <utility>

void HttpMgr::PostHttpReq(QUrl url, QJsonObject json, ReqId req_id, Modules mod) {
    QByteArray data = QJsonDocument(json).toJson();

    QNetworkRequest req = QNetworkRequest(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(data.length()));

    auto self = shared_from_this();
    QNetworkReply *reply = _manager.post(req, data);

    connect(reply, &QNetworkReply::finished, this, [self, reply, req_id, mod] {
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "HttpMgr::PostHttpReq failed:" << reply->error();
            emit self->sig_http_finish(req_id, "", ErrorCodes::ERR_NETWORK, mod);
            reply->deleteLater();
            return;
        }

        QString msg = reply->readAll();
        emit self->sig_http_finish(req_id, msg, ErrorCodes::SUCCESS, mod);
        reply->deleteLater();
        return;
    });
}
HttpMgr::HttpMgr() {
    connect(this, &HttpMgr::sig_http_finish, this, &HttpMgr::slot_http_finish);
}

void HttpMgr::slot_http_finish(ReqId id, QString res, ErrorCodes err, Modules mod)
{
    if(mod == Modules::REGISTERMOD){
        //发送信号通知指定模块http响应结束
        emit sig_reg_mod_finish(id, res, err);
    }
    if (mod == Modules::RESETMOD) {
        emit sig_reset_mod_finish(id, res, err);
    }
    if (mod == Modules::LOGINMOD) {
        emit sig_login_mod_finish(id, res, err);
    }
}
