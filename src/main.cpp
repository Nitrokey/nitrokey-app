/*
 * Author: Copyright (C) Andrzej Surowiec 2012
 *
 * This file is part of Nitrokey.
 *
 * Nitrokey is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * Nitrokey is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Nitrokey. If not, see <http://www.gnu.org/licenses/>.
 */
#include "src/core/ThreadWorker.h"
#include "mainwindow.h"
//#include "mcvs-wrapper.h"
//#include "nitrokey-applet.h"
//#include "splash.h"
#include <QApplication>
#include <QTranslator>
#include <QLibraryInfo>
#include <QSettings>
#include <QFileInfo>
#include <QDebug>
//#include <QSharedMemory>
//#include <src/ui/mainwindow.h>
#include "src/version.h"

#include "src/utils/bool_values.h"

#ifdef Q_OS_LINUX
//#include <libintl.h>
//#include <locale.h>
//#define _(String) gettext(String)
#endif //Q_OS_LINUX

enum {DEBUG_STATUS_NO_DEBUGGING = 0, DEBUG_STATUS_LOCAL_DEBUG, DEBUG_STATUS_DEBUG_ALL};

StartUpParameter_tst &
parseCommandLine(int argc, char *const *argv, StartUpParameter_tst &StartupInfo_st);

void initialize_startup_info(StartUpParameter_tst &StartupInfo_st);

void HelpInfos(void) {
  puts("Nitrokey App "
  GUI_VERSION
           "\n\n"
       "-h, --help        display this help and exit\n"
       "-a, --admin       enable extra administrativefunctions\n"
       "-d, --debug       enable debug options\n"
       "--debugAll       enable extensive debug options\n"
       "--lock-hardware   enable hardware lock option\n"
       /* Disable password matrix printf ("--PWM Enable PIN entry via matrix\n"); */
       "--cmd ...         start a command line session\n"
       "--language ...    load translation file with name i18n/nitrokey_xxx and store this choice "
           "in settings file (use --debug for more details)\n"
       "                  Use --language with empty parameter to clear the choice"
       "\n");
}

int main(int argc, char *argv[]) {
// workaround for issue https://github.com/Nitrokey/nitrokey-app/issues/43
#if QT_VERSION > QT_VERSION_CHECK(5, 0, 0)
  if (qgetenv("QT_QPA_PLATFORMTHEME") == "appmenu-qt5") {
    qputenv("QT_QPA_PLATFORMTHEME", "generic");
  }
#endif

  qRegisterMetaType<QMap<QString, QVariant>>();

  QApplication a(argc, argv);
  StartUpParameter_tst StartupInfo_st;
  initialize_startup_info(StartupInfo_st);
  qDebug() << "Command line arguments disabled";
//  parseCommandLine(argc, argv, StartupInfo_st);

  // initialize i18n
  QTranslator qtTranslator;
#if defined(Q_WS_WIN)
  qtTranslator.load("qt_" + QLocale::system().name());
#else
  qtTranslator.load("qt_" + QLocale::system().name(),
                    QLibraryInfo::location(QLibraryInfo::TranslationsPath));
#endif
  a.installTranslator(&qtTranslator);

  QCoreApplication::setOrganizationName("Nitrokey");
  QCoreApplication::setOrganizationDomain("nitrokey.com");
  QCoreApplication::setApplicationName("Nitrokey App");

  QSettings settings;
  const auto language_key = "main/language";
  if (StartupInfo_st.language_set){
    qDebug() << "Setting default language to " << StartupInfo_st.language_string;
    settings.setValue(language_key, StartupInfo_st.language_string);
  }
  QString settings_language = settings.value(language_key).toString();
  if(StartupInfo_st.FlagDebug) {
    qDebug() << settings_language << settings.fileName();
  }

  QTranslator myappTranslator;
  bool success = false;

#if QT_VERSION >= 0x040800 && !defined(Q_WS_MAC)
  QLocale loc = QLocale::system();
  QString lang = QLocale::languageToString(loc.language());
  if(StartupInfo_st.FlagDebug) {
    qDebug() << loc << lang << loc.name();
  }
  if(!StartupInfo_st.language_set && settings_language.isEmpty()){
    success = myappTranslator.load(QLocale::system(), // locale
                                 "",                // file name
                                 "nitrokey_",       // prefix
                                 ":/i18n/",         // folder
                                 ".qm");            // suffix
  }

  if(!success){
    auto translation_paths = {
        QString("/i18n/nitrokey_%1.qm").arg(settings_language),
        QString("/i18n/nitrokey_%1.qm").arg(QLocale::system().name()),
        QString("/i18n/nitrokey_%1.qm").arg(lang.toLower()),
        QString("/i18n/nitrokey_%1.qm").arg("en"),
    };

    for (auto path : translation_paths ){
      for(auto p : {QString(':'), QString('.')}){
        auto path2 = p + path;
        success = myappTranslator.load(path2);
        QFileInfo fileInfo(path2);
        if(StartupInfo_st.FlagDebug){
          qDebug() << path2 << success << fileInfo.exists();
        }
        if (success) break;
      }
      if (success) break;
    }
  }
#else
  success = myappTranslator.load(QString(":/i18n/nitrokey_%1.qm").arg(QLocale::system().name()));
#endif

  if (success){
    a.installTranslator(&myappTranslator);
  }
#ifdef Q_OS_LINUX
//  setlocale(LC_ALL, "");
//  bindtextdomain("nitrokey-app", "/usr/share/locale");
//  textdomain("nitrokey-app");
#endif

  // Check for multiple instances
  // GUID from http://www.guidgenerator.com/online-guid-generator.aspx
  /*
     QSharedMemory shared("6b50960df-f5f3-4562-bbdc-84c3bc004ef4");

     if( !shared.create( 512, QSharedMemory::ReadWrite) ) { // An instance is already running. Quit
     the current instance QMessageBox msgBox;
     msgBox.setText( QObject::tr("Can't start more than one instance of the application.") );
     msgBox.setIcon( QMessageBox::Critical );
     msgBox.exec(); exit(0); } else { */
  qDebug() << "Application started successfully.";
  // }

  /*
     SplashScreen *splash = 0; splash = new SplashScreen; splash->show();

     QFile qss( ":/qss/default.qss" ); if( ! qss.open( QIODevice::ReadOnly ) ) { qss.setFileName(
     ":/qss/default.qss" ); qss.open(
     QIODevice::ReadOnly ); }

     if( qss.isOpen() ) { a.setStyleSheet( qss.readAll() ); }

     QTimer::singleShot(3000,splash,SLOT(deleteLater())); */

//  MainWindow w(&StartupInfo_st);
//    csApplet()->setParent(&w);

  {
    QDateTime local(QDateTime::currentDateTime());
    qsrand(static_cast<uint> (local.currentMSecsSinceEpoch() % 2000000000));
  }

  a.setQuitOnLastWindowClosed(false);



  MainWindow w;
  const auto retcode = a.exec();
  qDebug() << "normal exit";
  return retcode;
}

StartUpParameter_tst &
parseCommandLine(int argc, char *const *argv, StartUpParameter_tst &StartupInfo_st) {
  //TODO rewrite with QCommandLineParser (does not support qt4)
  initialize_startup_info(StartupInfo_st);

  int i;
  char *p;

  // Check for commandline parameter
  for (i = 2; i <= argc; i++) {
    p = argv[i - 1];
    if ((0 == strcmp(p, "--help")) || (0 == strcmp(p, "-h"))) {
      HelpInfos();
      exit(0);
    }

    if ((0 == strcmp(p, "--debug")) || (0 == strcmp(p, "-d"))) {
      StartupInfo_st.FlagDebug = DEBUG_STATUS_LOCAL_DEBUG;
    }
    if (0 == strcmp(p, "--debugAll")) {
      StartupInfo_st.FlagDebug = DEBUG_STATUS_DEBUG_ALL;
    }

    if ((0 == strcmp(p, "--admin")) || (0 == strcmp(p, "-a"))) {
      StartupInfo_st.ExtendedConfigActive = TRUE;
    }
    /* Disable password matrix if (0 == strcmp (p,"--PWM")) { StartupInfo_st.PasswordMatrix = TRUE;
     * } */
    if (0 == strcmp(p, "--lock-hardware"))
      StartupInfo_st.LockHardware = TRUE;

    if (0 == strcmp(p, "--cmd")) {
      StartupInfo_st.Cmd = TRUE;
      i++;
      if (i > argc) {
        fprintf(stderr, "ERROR: Can't get command\n");
        fflush(stderr);
        exit(1);
      } else {
        p = argv[i - 1];
        StartupInfo_st.CmdLine = p;
      }
    }
    if (0 == strcmp(p, "--language")) {
      StartupInfo_st.language_set = TRUE;
      i++;
      if (i > argc) {
        StartupInfo_st.language_string = "";
      } else {
        p = argv[i - 1];
        StartupInfo_st.language_string = p;
      }
    }
  }
  return StartupInfo_st;
}

void initialize_startup_info(StartUpParameter_tst &StartupInfo_st) {
  StartupInfo_st.ExtendedConfigActive = FALSE;
//  StartupInfo_st.FlagDebug = DEBUG_STATUS_NO_DEBUGGING;
  StartupInfo_st.FlagDebug = DEBUG_STATUS_DEBUG_ALL;
  StartupInfo_st.PasswordMatrix = FALSE;
  StartupInfo_st.LockHardware = FALSE;
  StartupInfo_st.Cmd = FALSE;
  StartupInfo_st.language_set = FALSE;
  StartupInfo_st.language_string = "";
}

//extern "C" char *GetTimeStampForLog(void);
//
//char *GetTimeStampForLog(void) {
//  static QDateTime LastTimeStamp(QDateTime::currentDateTime());
//
//  QDateTime ActualTimeStamp(QDateTime::currentDateTime());
//
//  static char DateString[40];
//
//  if (ActualTimeStamp.toTime_t() != LastTimeStamp.toTime_t()) {
//    LastTimeStamp = ActualTimeStamp;
//    STRCPY(DateString, sizeof(DateString) - 1,
//           LastTimeStamp.toString("dd.MM.yyyy hh:mm:ss").toLatin1());
//  } else
//    DateString[0] = 0;
//
//  return (DateString);
//}
