#include <QGuiApplication>
#include <QRegExp>
#include <QClipboard>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDebug>
#include <QTimer>

#include "transmission.h"

constexpr static int MAX_RETRIES = 3;

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QString host = QStringLiteral("localhost");
    QString port = QStringLiteral("9091");
    QString user;
    QString pwd;

    if (qEnvironmentVariableIsSet("TRANSMISSION_HOST")) host = qgetenv("TRANSMISSION_HOST");
    if (qEnvironmentVariableIsSet("TRANSMISSION_PORT")) port = qgetenv("TRANSMISSION_PORT");
    if (qEnvironmentVariableIsSet("TRANSMISSION_USER")) user = qgetenv("TRANSMISSION_USER");
    if (qEnvironmentVariableIsSet("TRANSMISSION_PWD"))  pwd  = qgetenv("TRANSMISSION_PWD");

    qCritical()<< "Connecting to "<< host<< port;

    Transmission tr{host, port, user, pwd};

    QCommandLineParser cliParser;
    cliParser.process(app);

    auto interactive = cliParser.positionalArguments().length() == 0;
    int position{0};
    int retries {0};

    auto addTorrent = [&] (const QString &initial) {
        QString clip = initial;
        QString text = clip;
        QRegExp reg("(magnet:[^ \t\n]+)");

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
    };


    if (interactive) {
        qCritical()<< "Running interactively";
        auto *cp = QGuiApplication::clipboard();

        auto worker = [&] (QClipboard::Mode m) {
            if (m == QClipboard::Clipboard) {
                addTorrent(cp->text(m));
            }
        };

        QObject::connect(cp,  &QClipboard::changed, worker);
        QObject::connect(&tr, &Transmission::retry, [&] () {
            worker(QClipboard::Clipboard);
        });
    } else {
        QTimer::singleShot(500, [&] {
            if (position < cliParser.positionalArguments().length())
                addTorrent(cliParser.positionalArguments()[position]);
        });
        QObject::connect(&tr, &Transmission::added, [&] {
            ++position;
            retries = 0;
            if (position < cliParser.positionalArguments().length()) {
                addTorrent(cliParser.positionalArguments()[position]);
            } else {
                QCoreApplication::quit();
            }
        });
        QObject::connect(&tr, &Transmission::retry, [&] {
            qCritical()<< "Retrying";
            if (retries >= MAX_RETRIES) ++position;

            if (position < cliParser.positionalArguments().length()) {
                addTorrent(cliParser.positionalArguments()[position]);
            } else {
                QCoreApplication::quit();
            }

            ++retries;
        });
    }

    return QGuiApplication::exec();
}
