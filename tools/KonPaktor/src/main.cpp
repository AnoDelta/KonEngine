#include <QApplication>
#include <QIcon>
#include <QFileInfo>
#include "MainWindow.hpp"

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setApplicationName("KonPaktor");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("AnoDelta");

    // Set window icon from logo.png if available
    QStringList logoPaths = {
        QCoreApplication::applicationDirPath() + "/logo.png",
        QCoreApplication::applicationDirPath() + "/../logo.png",
        QCoreApplication::applicationDirPath() + "/../../logo.png",
        QCoreApplication::applicationDirPath() + "/../../../logo.png",
        "logo.png",
    };
    for (auto& p : logoPaths) {
        if (QFileInfo(p).exists()) { app.setWindowIcon(QIcon(p)); break; }
    }

    MainWindow w;
    w.show();
    return app.exec();
}
