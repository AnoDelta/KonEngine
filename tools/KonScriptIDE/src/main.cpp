#include <QApplication>
#include <QCommandLineParser>
#include "KonScriptIDE.hpp"

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setApplicationName("KonScript IDE");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("KonEngine");

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("file", "KonScript file(s) to open", "[file...]");
    parser.process(app);

    KonScriptIDE ide;
    ide.show();

    // Open any files passed on the command line
    for (auto& file : parser.positionalArguments())
        ide.openFile(file);

    return app.exec();
}
