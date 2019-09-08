#include <QJsonObject>
#include <QJsonDocument>

#include <QDebug>

#include "transmission.h"

void Transmission::onSessionResponse()
{
    auto *reply   = dynamic_cast<QNetworkReply*>(sender());
    auto  session = reply->rawHeader("X-Transmission-Session-Id");

    this->m_session = session;
}

QNetworkReply* Transmission::rpcMethod(const QString &name, const QJsonObject &arguments)
{
    QNetworkRequest request{QStringLiteral("http://%1:%2/transmission/rpc").arg(host, port)};

    if (!user.isEmpty()) {
        QString cred = QString("%1:%2").arg(user,pwd);
        request.setRawHeader("Authentication",
                             QString("Basic %1").arg(QString(cred.toLatin1().toBase64())).toLatin1());
    }
    QNetworkReply *reply{nullptr};

    if (name.isEmpty() || m_session.isEmpty()) {
        reply = manager.get(request);

        connect(reply, &QNetworkReply::finished, this, &Transmission::onSessionResponse);
        connect(reply, &QNetworkReply::finished, this, &Transmission::retry);
    } else {
        QJsonObject body;

        body.insert("method",    name);
        body.insert("arguments", arguments);

        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        request.setRawHeader("X-Transmission-Session-Id", m_session.toLatin1());

        reply = manager.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));

        connect(reply, &QNetworkReply::finished, [this, reply] () {
            auto r = reply->readAll();
            auto document = QJsonDocument::fromJson(r);

            if (document.isNull()) {
                QRegExp reg("X-Transmission-Session-Id: *([0-9a-zA-Z]+)");

                reg.indexIn(r);
                if (reg.captureCount() == 1) {
                    this->m_session = reg.cap(1);

                    Q_EMIT(this->retry());
                }
            } else {
                auto response = document.object();

                if (response.value("result") != "success") { Q_EMIT(this->retry()); return; }

                if (response.value("arguments").toObject().contains("torrent-duplicate"))
                    qCritical()<< "torrent duplicate"
                                << response.value("arguments").toObject()
                                       .value("torrent-duplicate").toObject().value("name").toString();
                else if (response.value("arguments").toObject().contains("torrent-added"))
                    qCritical()<< "torrent added"
                                << response.value("arguments").toObject().value("torrent-added")
                                       .toObject().value("name").toString();
            }
        });

        connect(reply, static_cast<void(QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
                [reply] (QNetworkReply::NetworkError) {
                    auto r = reply->readAll();
                    //QJsonObject response = QJsonDocument::fromJson(r).object();

                    qCritical()<< "torrent failed: "<< r;
                });
    }

    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onErrorResponse(QNetworkReply::NetworkError)));
    connect(reply, &QNetworkReply::finished, reply, &QObject::deleteLater);

    return reply;
}

void Transmission::addTorrent(const QString &torrent)
{
    QJsonObject args;

    args.insert("filename", torrent);

    rpcMethod("torrent-add", args);
}

void Transmission::addTorrentFile(const QString &url)
{
    qCritical()<< "Downloading torrent '"<< url<< "'";
    QNetworkRequest req(url);
    QNetworkReply *reply = manager.get(req);

    connect(reply, &QNetworkReply::finished, [this, reply]{
        QByteArray data = reply->readAll();

        this->addTorrent(data.toBase64());
    });
    connect(reply, static_cast<void(QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
            [] (QNetworkReply::NetworkError error) {
        qCritical()<< "torrent failed: "<< error;
    });
    connect(reply, SIGNAL(finished()), reply, SLOT(deleteLater()));
}

void Transmission::onErrorResponse(QNetworkReply::NetworkError)
{
    auto *reply = dynamic_cast<QNetworkReply*>(sender());
    qCritical()<< ":: response error: "<< reply->errorString();
    Q_EMIT(retry());
}
