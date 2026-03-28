#include <QApplication>
#include "MainWindow.hpp"

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setApplicationName("KonPaktor");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("AnoDelta");

    MainWindow w;
    w.show();
    return app.exec();
}
