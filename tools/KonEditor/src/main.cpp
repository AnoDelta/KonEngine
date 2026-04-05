#include <QApplication>
#include <QFileInfo>
#include "KonEditor.hpp"
#include "WelcomeScreen.hpp"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("KonEditor");
    app.setOrganizationName("AnoDelta");
    app.setApplicationVersion("0.1.0");

    // Dark theme before welcome screen
    qApp->setStyle("Fusion");
    QPalette dark;
    dark.setColor(QPalette::Window,          QColor(30, 30, 30));
    dark.setColor(QPalette::WindowText,      QColor(220, 220, 220));
    dark.setColor(QPalette::Base,            QColor(22, 22, 22));
    dark.setColor(QPalette::AlternateBase,   QColor(35, 35, 35));
    dark.setColor(QPalette::Text,            QColor(220, 220, 220));
    dark.setColor(QPalette::Button,          QColor(45, 45, 45));
    dark.setColor(QPalette::ButtonText,      QColor(220, 220, 220));
    dark.setColor(QPalette::Highlight,       QColor(0, 120, 215));
    dark.setColor(QPalette::HighlightedText, Qt::white);
    qApp->setPalette(dark);

    // Allow opening a .ks file directly from the command line
    QString directFile;
    if (argc > 1) {
        QString arg = QString::fromLocal8Bit(argv[1]);
        if (QFileInfo(arg).suffix().toLower() == "ks" && QFileInfo(arg).exists())
            directFile = QFileInfo(arg).absoluteFilePath();
    }

    if (directFile.isEmpty()) {
        // Show welcome screen
        WelcomeScreen welcome;
        if (welcome.exec() != QDialog::Accepted)
            return 0; // user closed welcome screen
        directFile = welcome.selectedProject();
    }

    KonEditor editor;
    editor.show();

    if (!directFile.isEmpty())
        editor.openProject(directFile);

    return app.exec();
}
