#pragma once

#include <QObject>

#include <QNetworkRequest>
#include <QNetworkReply>


class Transmission : public QObject
{
    Q_OBJECT
public:
    Transmission(QString host, QString port, QString user = "", QString pwd = "") :
        mHost{std::move(host)}, mPort{std::move(port)}, mUser{std::move(user)}, mPassword{std::move(pwd)}
    {
        QNetworkRequest req(QStringLiteral("http://%1:%2/transmission/rpc").arg(mHost, mPort));
        if (!mUser.isEmpty()) {
            QString cred = QString("%1:%2").arg(mUser, mPassword);
            req.setRawHeader("Authentication", QString("Basic %1").arg(QString(cred.toLatin1().toBase64())).toLatin1());
        }

        auto reply = manager.get(req);

        connect(reply, SIGNAL(finished()), this, SLOT(onSessionResponse()));
        connect(reply, SIGNAL(error(QNetworkReply::NetworkError)),
                this, SLOT(onErrorResponse(QNetworkReply::NetworkError)));
        connect(reply, &QNetworkReply::finished, reply, &QObject::deleteLater);
    }

Q_SIGNALS:
    void retry();
    void added();

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
