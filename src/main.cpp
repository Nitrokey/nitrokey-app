/*
 * Author: Copyright (C) Andrzej Surowiec 2012
 * Copyright (c) 2012-2018 Nitrokey UG
 *
 * This file is part of Nitrokey App.
 *
 * Nitrokey App is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * Nitrokey App is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Nitrokey App. If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0
 */

#include "mainwindow.h"
#include <QApplication>
#include <QDebug>
#include <QFileInfo>
#include <QLibraryInfo>
#include <QSettings>
#include <QStyleFactory>
#include <QTranslator>
#include <QtWidgets>
#include <libnitrokey/log.h>
//#include <QSharedMemory>
#include "src/version.h"
#include "src/utils/bool_values.h"

enum {DEBUG_STATUS_NO_DEBUGGING = 0, DEBUG_STATUS_LOCAL_DEBUG, DEBUG_STATUS_DEBUG_ALL};


bool configureParser(const QApplication &a, QCommandLineParser &parser);
void configureApplicationName();
void configureBasicTranslator(const QApplication &a, QTranslator &qtTranslator);
void issue_43_workaround();

void configureTranslator(const QApplication &a, const QCommandLineParser &parser, const QString &settings_language,
                         QTranslator &myappTranslator);

void set_dark_theme();

#include <libnitrokey/NK_C_API.h>
#ifndef LIBNK_MIN_VERSION
#define LIBNK_MIN_VERSION
#endif


int main(int argc, char *argv[]) {
  qInfo() << "Nitrokey App " CMAKE_BUILD_TYPE " " GUI_VERSION " (git: " GIT_VERSION ")";
  qInfo() << "Qt versions - built with: " QTWIDGETS_VERSION_STR << ", connected:" << qVersion();
  qInfo() << "libnitrokey versions - connected:" << NK_get_library_version() << ", required:" << LIBNK_MIN_VERSION;

  qRegisterMetaType<QMap<QString, QVariant>>();
  issue_43_workaround();

  QApplication a(argc, argv);
  configureApplicationName();
  QCommandLineParser parser;
  auto shouldQuit = configureParser(a, parser);
  if(shouldQuit)
    return 0;

  auto debug_enabled = parser.isSet("debug");
  if(debug_enabled) {
    qDebug() << a.arguments();
  }

  // initialize i18n
//  QTranslator qtTranslator;
//  configureBasicTranslator(a, qtTranslator);


  QSettings settings;
  const auto language_key = "main/language";
  if (parser.isSet("language")){
    qDebug() << "Setting default language to " << parser.value("language");
    settings.setValue(language_key, parser.value("language"));
  }
  QString settings_language = settings.value(language_key).toString();
  if(debug_enabled) {
    qDebug() << "Language saved in settings: " << settings_language << settings.fileName();
  }

  QTranslator myappTranslator;
  configureTranslator(a, parser, settings_language, myappTranslator);


  // Check for multiple instances
  // GUID from http://www.guidgenerator.com/online-guid-generator.aspx
  /*
     QSharedMemory shared("6b50960df-f5f3-4562-bbdc-84c3bc004ef4");

     if( !shared.create( 512, QSharedMemory::ReadWrite) ) { // An instance is already running. Quit
     the current instance QMessageBox msgBox;
     msgBox.setText( QObject::tr("Can't start more than one instance of the application.") );
     msgBox.setIcon( QMessageBox::Critical );
     msgBox.exec(); exit(0); } else { */
  if(debug_enabled) {
    qDebug() << "Application started successfully.";
  }
  // }

  /*
     SplashScreen *splash = 0; splash = new SplashScreen; splash->show();

     QFile qss( ":/qss/default.qss" ); if( ! qss.open( QIODevice::ReadOnly ) ) { qss.setFileName(
     ":/qss/default.qss" ); qss.open(
     QIODevice::ReadOnly ); }

     if( qss.isOpen() ) { a.setStyleSheet( qss.readAll() ); }

     QTimer::singleShot(3000,splash,SLOT(deleteLater())); */


  a.setQuitOnLastWindowClosed(false);


  MainWindow w;
  w.enable_admin_commands();

  QString df = settings.value("debug/file", "").toString().trimmed();
  if (!df.isEmpty() && settings.value("debug/enabled", false).toBool() && df != "console"){
    w.set_debug_file(df);
  }
  auto debug_file_option_name = "debug-file";
  if (parser.isSet(debug_file_option_name) && !parser.value(debug_file_option_name).isEmpty()){
    w.set_debug_file(parser.value(debug_file_option_name));
  }
  if (parser.isSet("debug-window")){
    w.set_debug_window();
  }
  if(debug_enabled){
    w.set_debug_mode();
  }

  w.set_commands_delay(40);
  if (parser.isSet("delay")){
    auto delay_in_ms = parser.value("delay").toInt();
    w.set_commands_delay(delay_in_ms);
  }

  w.set_debug_level(2);
  if (settings.value("debug/enabled", false).toBool()){
    w.set_debug_level(settings.value("debug/level", 2).toInt());
  }
  if(parser.isSet("debug-level")){
    w.set_debug_level(parser.value("debug-level").toInt());
  }

  if(parser.isSet("no-window") || !settings.value("main/show_on_start", true).toBool() ) {
    w.hideOnStartup();
  }

  if(parser.isSet("dark-theme")){
      set_dark_theme();
  }

//  csApplet()->setParent(&w);
  int retcode = -1;
#ifndef _DEBUG
  try{
    retcode = a.exec();
  }
  catch (std::exception &e){
    auto message = QApplication::tr("Critical error encountered. Please restart application.\nMessage: ") + e.what();
    nitrokey::log::Log::instance()(message.toStdString(), nitrokey::log::Loglevel::ERROR);
    csApplet()->warningBox(message);
    qDebug() << message;
  }
#else
  retcode = a.exec();
  qDebug() << "Normal application exit";
#endif
  return retcode;
}


void configureTranslator(const QApplication &a, const QCommandLineParser &parser, const QString &settings_language,
                         QTranslator &myappTranslator) {
  bool success = false;

#if QT_VERSION >= 0x040800 && !defined(Q_WS_MAC)
  QLocale loc = QLocale::system();
  QString lang = QLocale::languageToString(loc.language());
  if (parser.isSet("debug")) {
    qDebug() << loc << lang << loc.name();
  }

  if (!success) {
    auto translation_paths = {
        QString(settings_language),
        QString(QLocale::system().name()),
        QString(lang.toLower()),
        QString("English"),
    };

    if (parser.isSet("debug")) {
      qDebug() << "Loading translation files";
    }
    QString working_translation_file;
    for (auto path : translation_paths) {
      for (auto p : {QString(':'), QString('.')}) {
        if(path.isEmpty()) continue;
        auto path2 = p + QString("/i18n/nitrokey_%1.qm").arg(path);
        success = myappTranslator.load(path2);
        QFileInfo fileInfo(path2);
        if (parser.isSet("debug")) {
          qDebug() << path2 << " - file loaded successfully: " <<success
                   << ", file exists on disk: "<< fileInfo.exists();
        }
        if (success) break;
      }
      if (success){
        working_translation_file = path;
        break;
      }
    }

    QSettings settings;
    const auto language_key = "main/language";    
    if (settings.value(language_key).toString().isEmpty()){
        qDebug() << "Setting working language to " << working_translation_file;
        settings.setValue(language_key, working_translation_file);
    }

  }
#else
  success = myappTranslator.load(QString(":/i18n/nitrokey_%1.qm").arg(QLocale::system().name()));
#endif

  if (success) {
    a.installTranslator(&myappTranslator);
  }
}

void issue_43_workaround() {
// workaround for issue https://github.com/Nitrokey/nitrokey-app/issues/43
#if QT_VERSION > QT_VERSION_CHECK(5, 0, 0)
  if (qgetenv("QT_QPA_PLATFORMTHEME") == "appmenu-qt5") {
    qputenv("QT_QPA_PLATFORMTHEME", "generic");
  }
#endif

}

void configureBasicTranslator(const QApplication &a, QTranslator &qtTranslator) {
#if defined(Q_WS_WIN)
  qtTranslator.load("qt_" + QLocale::system().name());
#else
  qtTranslator.load("qt_" + QLocale::system().name(),
                    QLibraryInfo::location(QLibraryInfo::TranslationsPath));
#endif
  a.installTranslator(&qtTranslator);
}

void configureApplicationName() {
  QCoreApplication::setOrganizationName("Nitrokey");
  QCoreApplication::setOrganizationDomain("nitrokey.com");
  QCoreApplication::setApplicationName("Nitrokey App");
  QCoreApplication::setApplicationVersion(GUI_VERSION);
}

bool configureParser(const QApplication &a, QCommandLineParser &parser) {
  parser.setApplicationDescription(
      QCoreApplication::translate("main", "Nitrokey App - Manage your Nitrokey sticks"));
  parser.addHelpOption();
  parser.addVersionOption();
  parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);

  //keep in sync with data/bash-autocomplete/nitrokey-app
  parser.addOptions({
      {"clear-settings",
          QCoreApplication::translate("main", "Clear all application's settings")},
      {{"d", "debug"},
          QCoreApplication::translate("main", "Enable debug messages")},                        
      {{"df","debug-file"},
          QCoreApplication::translate("main", "Save debug log to file with name <log-file-name> (experimental)"),
          "log-file-name"},
      {{"dw","debug-window"},
          QCoreApplication::translate("main", "Save debug log to App's window (experimental)") },
      {"dark-theme",
          QCoreApplication::translate("main", "Set dark theme")},
      {{"dl","debug-level"},
          QCoreApplication::translate("main", "Set debug level, 0-4"),
          "debug-level-int"},
      {{"s", "delay"},
          QCoreApplication::translate("main", "Set delay between commands sent to device (in ms) to <delay>"), "delay" },
      {"version-more",
          QCoreApplication::translate("main", "Show additional information about binary")},
      {{"a", "admin"},
          QCoreApplication::translate("main", "Enable extra administrative functions")},
//      {"lock-hardware", //TODO add functionality
//          QCoreApplication::translate("main", "Show hardware lock action in tray menu")},
      {"language-list",
          QCoreApplication::translate("main", "List available languages")},
      {{"l", "language"},
          QCoreApplication::translate("main",
                                      "Load translation file with given name "
                                      "and store this choice in settings file."),
                                      "English"},
      {"no-window",
          QCoreApplication::translate("main", "Display no window.")},
});

  parser.process(a);

  if(parser.isSet("language-list")){
    QDir langDir(":/i18n/");
    auto list = langDir.entryList();
    for (auto &&translationFile : list) {
      qDebug() << translationFile.remove("nitrokey_").remove(".qm");
    }
    return true;
  }

  if(parser.isSet("version-more")){
    qDebug() << CMAKE_BUILD_TYPE << GUI_VERSION << GIT_VERSION;
    qDebug() << CMAKE_CXX_COMPILER << CMAKE_CXX_FLAGS;
    return true;
  }

  if(parser.isSet("clear-settings")){
      qDebug() << "Clearing settings";
      QSettings settings;
      settings.clear();
  }

  return false;
}


void set_dark_theme() {
    //    https://stackoverflow.com/questions/48256772/dark-theme-for-qt-widgets
    qApp->setStyle(QStyleFactory::create("Fusion"));

    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53,53,53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25,25,25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53,53,53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53,53,53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));

    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);

    qApp->setPalette(darkPalette);

    qApp->setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");
}

void set_dark_theme2() {
    //    https://stackoverflow.com/questions/48256772/dark-theme-for-qt-widgets
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    QPalette p;
    p = qApp->palette();
    p.setColor(QPalette::Window, QColor(53,53,53));
    p.setColor(QPalette::Button, QColor(53,53,53));
    p.setColor(QPalette::Highlight, QColor(142,45,197));
    p.setColor(QPalette::ButtonText, QColor(255,255,255));
    qApp->setPalette(p);
}

void set_dark_theme3(){
//  further resources
//  https://github.com/ColinDuquesnoy/QDarkStyleSheet
//  https://github.com/albertosottile/darkdetect
}
