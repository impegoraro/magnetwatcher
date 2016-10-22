#include <QGuiApplication>
#include <QRegExp>
#include <QClipboard>
#include <QDebug>

#include "transmission.h"

int main(int argc, char *argv[])
{
    QGuiApplication a(argc, argv);
    QClipboard *cp = QGuiApplication::clipboard();
    QString host = QStringLiteral("localhost"), port = QStringLiteral("9091"), user, pwd;

    if(qEnvironmentVariableIsSet("TRANSMISSION_HOST"))
        host = qgetenv("TRANSMISSION_HOST");
    if(qEnvironmentVariableIsSet("TRANSMISSION_PORT"))
        port = qgetenv("TRANSMISSION_PORT");
    if(qEnvironmentVariableIsSet("TRANSMISSION_USER"))
        user = qgetenv("TRANSMISSION_USER");
    if(qEnvironmentVariableIsSet("TRANSMISSION_PWD"))
        pwd= qgetenv("TRANSMISSION_PWD");

    Transmission tr(host, port, user, pwd);

    QObject::connect(cp, &QClipboard::changed, [&](QClipboard::Mode m) {
        if(m == QClipboard::Clipboard) {
            QString text = cp->text(m);

            QRegExp reg("(magnet:[^ \t\n]+)");
            while(!text.isEmpty()) {

                auto a = reg.indexIn(text);
                if(a == -1) return;
                int i = 0;

                for (QString str : reg.capturedTexts()) {
                    if(i) {
                        tr.addTorrent(str);
                        text = text.mid(str.length());
                    }
                    i++;
                }

            }

        }
    });
    return a.exec();
}
