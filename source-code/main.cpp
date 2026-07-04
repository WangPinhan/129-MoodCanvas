#include "mainwindow.h"
#include "datastore.h"
#include "logindialog.h"
#include "thememanager.h"
#include "userservice.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDialog>
#include <QDir>
#include <QFileInfo>
#include <QFont>

static void setupQtRuntimeEnvironment(const QString &appDir)
{
    const QStringList pluginCandidates = {
        QStringLiteral("C:/Qt/6.11.1/mingw_64/plugins"),
        appDir + QStringLiteral("/plugins"),
        appDir + QStringLiteral("/../plugins"),
    };

    for (const QString &path : pluginCandidates) {
        if (QDir(path).exists()) {
            QCoreApplication::addLibraryPath(path);
        }
    }

    const QString qtBin = QStringLiteral("C:/Qt/6.11.1/mingw_64/bin");
    if (QDir(qtBin).exists()) {
        const QByteArray path = (qtBin + QLatin1Char(';') + QString::fromLocal8Bit(qgetenv("PATH"))).toLocal8Bit();
        qputenv("PATH", path.constData());
    }
}

int main(int argc, char *argv[])
{
    const QString appDir = QFileInfo(QString::fromLocal8Bit(argv[0])).absolutePath();
    setupQtRuntimeEnvironment(appDir);

    QApplication app(argc, argv);

    QFont font(QStringLiteral("Microsoft YaHei UI"), 10);
    app.setFont(font);

    ThemeManager::instance();

    DataStore dataStore;
    UserService userService(&dataStore);
    LoginDialog loginDialog(&userService);
    if (loginDialog.exec() != QDialog::Accepted) {
        return 0;
    }

    MainWindow w(&dataStore, loginDialog.profile());
    w.show();

    return app.exec();
}
