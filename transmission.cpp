#include <QJsonObject>
#include <QJsonDocument>

#include <QDebug>

#include "transmission.h"

void Transmission::onSessionResponse()
{
    QNetworkReply * reply = static_cast<QNetworkReply*>(sender());

    QByteArray session = reply->rawHeader("X-Transmission-Session-Id");
    this->m_session = session;
}

void Transmission::addTorrent(const QString &torrent)
{
    if(m_session.isEmpty()) {

        qCritical()<< "unable to send to transmission, does not have valid session key";
        QNetworkRequest req(QStringLiteral("http://%1:%2/transmission/rpc").arg(host, port));
        if(!user.isEmpty() && !pwd.isEmpty()) {
            QString cred = QString("%1:%2").arg(user,pwd);
            req.setRawHeader("Authentication", QString("Basic %1").arg(QString(cred.toLatin1().toBase64())).toLatin1());
        }

        auto reply = manager.get(req);


        connect(reply, SIGNAL(finished()), this, SLOT(onSessionResponse()));
        connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onErrorResponse(QNetworkReply::NetworkError)));
        connect(reply, SIGNAL(finished()), reply, SLOT(deleteLater()));
        return ;
    }

    QNetworkRequest req(QStringLiteral("http://%1:%2/transmission/rpc").arg(host, port));
    req.setRawHeader("X-Transmission-Session-Id", m_session.toLatin1());
    req.setRawHeader("Content-Type", "application/json");
    if(!user.isEmpty() && !pwd.isEmpty()) {
        QString cred = QString("%1:%2").arg(user,pwd);
        req.setRawHeader("Authentication", QString("Basic %1").arg(QString(cred.toLatin1().toBase64())).toLatin1());
    }

    QJsonObject obj;
    obj.insert("method", "torrent-add");
    QJsonObject obj2;
    obj2.insert("filename", torrent);
    obj.insert("arguments", obj2);
    auto reply = manager.post(req, QJsonDocument(obj).toJson());

    connect(reply, &QNetworkReply::finished, [this, reply]() {
        auto r = reply->readAll();

        QByteArray session = reply->rawHeader("X-Transmission-Session-Id");
        if(!session.isEmpty())
            m_session = session;

        qCritical()<< "torrent added "<< r;
    });

    connect(reply, SIGNAL(finished()), reply, SLOT(deleteLater()));
    connect(reply, static_cast<void(QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error), [this, reply](QNetworkReply::NetworkError error) {
        QByteArray session = reply->rawHeader("X-Transmission-Session-Id");
        if(!session.isEmpty())
            m_session = session;

        qCritical()<< "torrent failed: "<< error;
    });
}

void Transmission::addTorrentFile(const QString &url)
{
    QNetworkRequest req(url);
    QNetworkReply *reply = manager.get(req);
    qCritical()<< "Downloading torrent '"<< url<< "'";
    connect(reply, &QNetworkReply::finished, [this, reply]{
        QByteArray data = reply->readAll();

        this->addTorrent(data.toBase64());
    });

    connect(reply, static_cast<void(QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error), [this, reply](QNetworkReply::NetworkError error) {
        qCritical()<< "torrent failed: "<< error;
    });
    connect(reply, SIGNAL(finished()), reply, SLOT(deleteLater()));
}

void Transmission::onErrorResponse(QNetworkReply::NetworkError error)
{
    qCritical()<< "Got error: "<< error;
}
