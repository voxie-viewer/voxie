#include <Main/root.hpp>

#include <QtCore/QCommandLineParser>
#include <QtCore/QDebug>
#include <QtCore/QFile>

#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
#ifdef QT_DEBUG
	qSetMessagePattern("%{message} (%{file}:%{line})");
#endif

    if (argc < 1) {
        qCritical("argc is smaller than 1");
        return 1;
    }

    QCommandLineParser parser;
    parser.setApplicationDescription("Test helper");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("files", "File to open.", "[files...]");
    
    // TODO: make sure this works really
    QCommandLineOption noDBusOption("no-dbus", "Do not use DBus (implies --new-instance)");
    parser.addOption(noDBusOption);

    QCommandLineOption newInstanceOption("new-instance", "Create a new instance, even if voxie is already runnung");
    parser.addOption(newInstanceOption);

    QCommandLineOption qtOptionOption("qt-option", "Pass option to Qt", "option");
    parser.addOption(qtOptionOption);

    QCommandLineOption isoOption("iso", "Create an isosurface view for every opened file");
    parser.addOption(isoOption);
    QCommandLineOption sliceOption("slice", "Create a slice view for every opened file");
    parser.addOption(sliceOption);

    QStringList args;
    for (char** arg = argv; *arg; arg++)
        args.push_back(*arg);

    {
        int argc0 = 1;
        char* args0[2] = { argv[0], NULL };
        QCoreApplication app0(argc0, args0);
        parser.process(args);
    }

    QStringList qtOptionsList = parser.values(qtOptionOption);
    int qtOptionsArgc = qtOptionsList.size() + 1;
    std::vector<std::string> qtOptions;
    std::vector<char*> qtOptionsChar;
    qtOptionsChar.push_back(argv[0]);
    for (const QString& opt : qtOptionsList) {
        qtOptions.push_back(opt.toUtf8().data());
        qtOptionsChar.push_back((char*) qtOptions.back().c_str());
    }
    qtOptionsChar.push_back(NULL);

	QApplication app(qtOptionsArgc, qtOptionsChar.data());
    if (qtOptionsArgc != 1) {
        qCritical("Got invalid Qt options:");
        for (char** arg = qtOptionsChar.data(); *arg; arg++)
            if (arg != qtOptionsChar.data())
                qCritical("  '%s'", *arg);
        return 1;
    }

	return voxie::Root::startVoxie(app, parser);
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
