/*
* Author: Copyright (C) Andrzej Surowiec 2012
*
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

#include <QApplication>
#include <QSharedMemory>
#include <QDebug>
#include "mcvs-wrapper.h"
#include "splash.h"
#include "mainwindow.h"
#include "device.h"
#include "stick20hid.h"
#include "cryptostick-applet.h"

/*******************************************************************************

 External declarations

*******************************************************************************/


/*******************************************************************************

 Local defines

*******************************************************************************/


/*******************************************************************************

  HelpInfos

  Changes
  Date      Author        Info
  09.07.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void HelpInfos (void)
{
    puts (
            "Nitrokey App\n\n"
            "-h, --help        display this help and exit\n"
            "-a, --admin       enable extra administrativefunctions\n"
            "-d, --debug       enable debug options\n"
            "--debugAll       enable extensive debug options\n"
            "--lock-hardware   enable hardware lock option\n"
/* Disable password matrix
    printf ("--PWM          Enable PIN entry via matrix\n");
*/
            "--cmd ...         start a command line session\n"
            "\n");
}

/*******************************************************************************

  main

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/


int main(int argc, char *argv[])
{

    // Q_INIT_RESOURCE(stylesheet);

    csApplet = new CryptostickApplet;

    int i;
    //int DebugWindowActive;
    //int SpecialConfigActive;
    char *p;
    StartUpParameter_tst StartupInfo_st;

    QApplication a(argc, argv);

    // Check for multiple instances
    // GUID from http://www.guidgenerator.com/online-guid-generator.aspx
    /*
    QSharedMemory shared("6b50960df-f5f3-4562-bbdc-84c3bc004ef4");

    if( !shared.create( 512, QSharedMemory::ReadWrite) )
    {
      // An instance is already running. Quit the current instance
      QMessageBox msgBox;
      msgBox.setText( QObject::tr("Can't start more than one instance of the application.") );
      msgBox.setIcon( QMessageBox::Critical );
      msgBox.exec();
      exit(0);
    }
    else {
*/
        qDebug() << "Application started successfully.";
//    }

/*    
    SplashScreen *splash = 0;
    splash = new SplashScreen;
    splash->show();

    QFile qss( ":/qss/default.qss" );
    if( ! qss.open( QIODevice::ReadOnly ) ) {
        qss.setFileName( ":/qss/default.qss" );
        qss.open( QIODevice::ReadOnly );
    }

    if( qss.isOpen() )
    {
        a.setStyleSheet( qss.readAll() );
    }

    QTimer::singleShot(3000,splash,SLOT(deleteLater()));
*/

    StartupInfo_st.ExtendedConfigActive  = FALSE;
    StartupInfo_st.FlagDebug             = DEBUG_STATUS_NO_DEBUGGING;
    StartupInfo_st.PasswordMatrix        = FALSE;
    StartupInfo_st.LockHardware          = FALSE;
    StartupInfo_st.Cmd                   = FALSE;

    HID_Stick20Init ();

// Check for commandline parameter
    //    if (2 == argc)
    for (i=2;i<=argc;i++)
    {
        p = argv[i-1];
        if ((0 == strcmp (p,"--help")) || (0 == strcmp (p,"-h")))
        {
            HelpInfos ();
            exit (0);
        }

        if ((0 == strcmp (p,"--debug")) || (0 == strcmp (p,"-d")))
        {
            StartupInfo_st.FlagDebug = DEBUG_STATUS_LOCAL_DEBUG;
        }
        if (0 == strcmp (p,"--debugAll"))
        {
            StartupInfo_st.FlagDebug = DEBUG_STATUS_DEBUG_ALL;
        }     
        if ((0 == strcmp (p,"--admin")) || (0 == strcmp (p,"-a")))
        {
            StartupInfo_st.ExtendedConfigActive = TRUE;
        }
/* Disable password matrix
        if (0 == strcmp (p,"--PWM"))
        {
            StartupInfo_st.PasswordMatrix = TRUE;
        }
*/
        if (0 == strcmp (p,"--lock-hardware"))
        {
            StartupInfo_st.LockHardware = TRUE;
        }
        if (0 == strcmp (p,"--cmd"))
        {
            StartupInfo_st.Cmd = TRUE;
            i++;
            if (i > argc)
            {
                fprintf (stderr,"ERROR: Can't get command\n");
                fflush(stderr);
                exit (1);
            }
            else
            {
                p = argv[i-1];
                StartupInfo_st.CmdLine = p;
            }
        }
    }



    MainWindow w(&StartupInfo_st);
    //w.show();
/*
    QTime midnight(0, 0, 0);
    qsrand(midnight.secsTo(QTime::currentTime()));
*/
    QDateTime local(QDateTime::currentDateTime());
    qsrand (local.currentMSecsSinceEpoch() % 2000000000);

    a.setQuitOnLastWindowClosed(false);
    return a.exec();
}

/*******************************************************************************

  GetTimeStampForLog

  Changes
  Date      Author        Info
  09.12.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/
extern "C" char *GetTimeStampForLog (void);

char *GetTimeStampForLog (void)
{
    static QDateTime LastTimeStamp (QDateTime::currentDateTime());
    QDateTime ActualTimeStamp (QDateTime::currentDateTime());
    static char DateString[40];


    if (ActualTimeStamp.toTime_t() != LastTimeStamp.toTime_t())
    {
        LastTimeStamp = ActualTimeStamp;
        STRCPY (DateString,sizeof (DateString)-1,LastTimeStamp.toString("dd.MM.yyyy hh:mm:ss").toLatin1());
    }
    else
    {
        DateString[0] = 0;
    }
    return (DateString);
}
