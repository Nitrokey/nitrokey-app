/*
* Author: Copyright (C) Andrzej Surowiec 2012
*
*
* This file is part of GPF Crypto Stick.
*
* GPF Crypto Stick is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* GPF Crypto Stick is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with GPF Crypto Stick. If not, see <http://www.gnu.org/licenses/>.
*/

#include <QApplication>
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
    printf ("Crypto Stick GUI\n");
    printf ("\n");
    printf ("-h, --help     This help\n");
    printf ("-a, --admin    Enable extra functions in the GUI for administrators.\n");
    printf ("-d, --debug    Enable debug options\n");
    printf ("--debugAll     Enable extensive debug options\n");
    printf ("--lock-hardware Enable menu entry for hardware lock\n");
/* Disable password matrix
    printf ("--PWM          Enable PIN entry via matrix\n");
*/
    printf ("--cmd ....     Start a command line command\n");
    printf ("\n");
}

/*******************************************************************************

  main

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/


int main(int argc, char *argv[])
{

    Q_INIT_RESOURCE(stylesheet);

    csApplet = new CryptostickApplet;

    int i;
    //int DebugWindowActive;
    //int SpecialConfigActive;
    char *p;
    StartUpParameter_tst StartupInfo_st;

    QApplication a(argc, argv);

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
