#pragma once

#include <QObject>

#include <QNetworkRequest>
#include <QNetworkReply>


class Transmission : public QObject
{
    Q_OBJECT
public:
    Transmission(const QString &host, const QString &port, const QString &user="", const QString &pwd ="") :
        mHost(host), mPort(port), mUser(user), mPassword(pwd)
    {
        QNetworkRequest req(QStringLiteral("http://%1:%2/transmission/rpc").arg(host, port));
        if(!user.isEmpty() && !pwd.isEmpty()) {
            QString cred = QString("%1:%2").arg(user,pwd);
            req.setRawHeader("Authentication", QString("Basic %1").arg(QString(cred.toLatin1().toBase64())).toLatin1());
        }

        auto reply = manager.get(req);

        connect(reply, SIGNAL(finished()), this, SLOT(onSessionResponse()));
        connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onErrorResponse(QNetworkReply::NetworkError)));
        connect(reply, SIGNAL(finished()), reply, SLOT(deleteLater()));
    }

Q_SIGNALS:
    void retry();

public slots:
    void addTorrent(const QString &torrent);
    void addTorrentFile(const QString &url);

private slots:
    void onSessionResponse();
    void onErrorResponse(QNetworkReply::NetworkError);
    //void onTorrentAdded();

private:
    QString mSession;
    QString mHost;
    QString mPort;
    QString mUser;
    QString mPassword;

    QNetworkAccessManager manager;

    QNetworkReply* rpcMethod(const QString &name, const QJsonObject &arguments);
};