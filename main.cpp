// Copyright (c) 2014 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#include <QtCore/QDebug>
#include <QApplication>
#include <QTranslator>

#include "main.h"
#include "mainwindow.h"


const QString Company = "c't";
const QString CompanyDomain = "http://www.ct.de/";
const QString AppName = "EyeX Demo";
const QString AppUrl = "http://code.google.com/p/eyex/";
const QString AppAuthor = "Oliver Lau";
const QString AppAuthorMail = "ola@ct.de";
const QString AppVersionNoDebug = "0.1";
const QString AppMinorVersion = "-PREALPHA";
#ifdef QT_NO_DEBUG
const QString AppVersion = AppVersionNoDebug + AppMinorVersion;
#else
const QString AppVersion = AppVersionNoDebug + AppMinorVersion + " [DEBUG]";
#endif


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setOrganizationName(Company);
    a.setOrganizationDomain(CompanyDomain);
    a.setApplicationName(AppName);
    a.setApplicationVersion(AppVersionNoDebug);
    qApp->addLibraryPath("plugins");
    qApp->addLibraryPath("./plugins");

#ifdef Q_OS_MAC
    qApp->addLibraryPath("../plugins");
#endif

#ifndef QT_NO_DEBUG
    qDebug() << qApp->libraryPaths();
#endif

    QTranslator translator;
    bool ok = translator.load(":/translations/eyex_" + QLocale::system().name());
#ifndef QT_NO_DEBUG
    if (!ok)
        qWarning() << "Could not load translations for" << QLocale::system().name() << "locale";
#endif
    if (ok)
        a.installTranslator(&translator);

    MainWindow w;
    w.show();
    return a.exec();
}
