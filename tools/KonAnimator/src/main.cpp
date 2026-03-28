#include <QApplication>
#include <QStyleFactory>
#include <QPalette>
#include "MainWindow.hpp"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("KonAnimator");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("KonEngine");

    // Dark Fusion theme
    app.setStyle(QStyleFactory::create("Fusion"));
    QPalette pal;
    pal.setColor(QPalette::Window,          QColor(45,45,45));
    pal.setColor(QPalette::WindowText,      QColor(220,220,220));
    pal.setColor(QPalette::Base,            QColor(30,30,30));
    pal.setColor(QPalette::AlternateBase,   QColor(40,40,40));
    pal.setColor(QPalette::ToolTipBase,     QColor(55,55,55));
    pal.setColor(QPalette::ToolTipText,     QColor(220,220,220));
    pal.setColor(QPalette::Text,            QColor(220,220,220));
    pal.setColor(QPalette::Button,          QColor(55,55,55));
    pal.setColor(QPalette::ButtonText,      QColor(220,220,220));
    pal.setColor(QPalette::BrightText,      Qt::red);
    pal.setColor(QPalette::Highlight,       QColor(0,150,220));
    pal.setColor(QPalette::HighlightedText, Qt::white);
    pal.setColor(QPalette::Disabled, QPalette::Text,       QColor(110,110,110));
    pal.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(110,110,110));
    app.setPalette(pal);

    MainWindow win;
    win.show();

    // Open file passed as argument (e.g. double-click on .anim file)
    if (argc >= 2)
        win.openFile(QString::fromLocal8Bit(argv[1]));

    return app.exec();
}
