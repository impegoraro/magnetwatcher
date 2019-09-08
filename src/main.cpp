#include <QGuiApplication>
#include <QRegExp>
#include <QClipboard>
#include <QDebug>

#include "transmission.h"

int main(int argc, char *argv[])
{
    QGuiApplication a(argc, argv);
    QClipboard *cp = QGuiApplication::clipboard();
    QString host = QStringLiteral("localhost");
    QString port = QStringLiteral("9091");
    QString user;
    QString pwd;

    if (qEnvironmentVariableIsSet("TRANSMISSION_HOST")) host = qgetenv("TRANSMISSION_HOST");
    if (qEnvironmentVariableIsSet("TRANSMISSION_PORT")) port = qgetenv("TRANSMISSION_PORT");
    if (qEnvironmentVariableIsSet("TRANSMISSION_USER")) user = qgetenv("TRANSMISSION_USER");
    if (qEnvironmentVariableIsSet("TRANSMISSION_PWD"))  pwd  = qgetenv("TRANSMISSION_PWD");

    Transmission tr{host, port, user, pwd};

    auto worker = [&] (QClipboard::Mode m) {
        if (m == QClipboard::Clipboard) {
            QString clip= cp->text(m);
            QString text = clip;
            QRegExp reg("(magnet:[^ \t\n]+)");

            while (!text.isEmpty()) {

                auto a = reg.indexIn(text);
                if(a == -1) break;
                int i = 0;

                for (const QString &str : reg.capturedTexts()) {
                    if (i) {
                        tr.addTorrent(str);
                        text = text.mid(str.length());
                    }
                    i++;
                }

            }

            text = clip;

            reg = QRegExp("(https?://[^ \t\n]+\\.torrent)");
            while (!text.isEmpty()) {

                auto a = reg.indexIn(text);
                if (a == -1) break;
                int i = 0;

                for (const QString &str : reg.capturedTexts()) {
                    if (i) {
                        tr.addTorrent(str);
                        text = text.mid(str.length());
                    }
                    i++;
                }

            }

        }
    };

    QObject::connect(cp,  &QClipboard::changed, worker);
    QObject::connect(&tr, &Transmission::retry, [&] () {
        worker(QClipboard::Clipboard);
    });

    return QGuiApplication::exec();
}
