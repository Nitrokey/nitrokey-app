/*
* Author: Copyright (C) Andrzej Surowiec 2012
*						Parts Rudolf Boeddeker  Date: 2013-08-13
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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include "device.h"

#define GUI_VERSION      "0.6"

namespace Ui {
class MainWindow;
}

typedef struct {
    int  FlagDebug;
    int  ExtendedConfigActive;
    int  PasswordMatrix;
    int  LockHardware;
    int  Cmd;
    char *CmdLine;
} StartUpParameter_tst;


class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(StartUpParameter_tst *StartupInfo_st,QWidget *parent = 0);
    void startTimer();
    ~MainWindow();

    bool DebugWindowActive;
    bool ExtendedConfigActive;
    bool MatrixInputActive;
    bool LockHardware;

protected:
     void closeEvent(QCloseEvent *event);

private:
    Ui::MainWindow *ui;
    QSystemTrayIcon *trayIcon;
    QMenu *trayMenu;
    QMenu *trayMenuSubConfigure;
    QMenu *trayMenuSubSpecialConfigure;

    unsigned char HOTP_SlotCount;
    unsigned char TOTP_SlotCount;

    Device *cryptostick;
    void getSlotNames();

    uint64_t currentTime;
    bool     CryptedVolumeActive;
    bool     HiddenVolumeActive;
    bool     NormalVolumeRWActive;
    bool     HiddenVolumeAccessable;
    bool     StickNotInitated;
    bool     SdCardNotErased;
    bool     SdCardNotErased_DontAsk;
    bool     StickNotInitated_DontAsk;

    QAction *quitAction;
    QAction *restoreAction;
    QAction *Stick20Action;
    QAction *DebugAction;
    QAction *ActionAboutDialog;
    QAction *SecPasswordAction;
    QAction *Stick20SetupAction;


    QAction *Stick20ActionEnableCryptedVolume;
    QAction *Stick20ActionDisableCryptedVolume;
    QAction *Stick20ActionEnableHiddenVolume;
    QAction *Stick20ActionDisableHiddenVolume;
    QAction *Stick20ActionChangeUserPIN;
    QAction *Stick20ActionChangeAdminPIN;
    QAction *Stick20ActionEnableFirmwareUpdate;
    QAction *Stick20ActionExportFirmwareToFile;
    QAction *Stick20ActionDestroyCryptedVolume;
    QAction *Stick20ActionInitCryptedVolume;
    QAction *Stick20ActionFillSDCardWithRandomChars;
    QAction *Stick20ActionGetStickStatus;
    QAction *Stick20ActionSetReadonlyUncryptedVolume;
    QAction *Stick20ActionSetReadWriteUncryptedVolume;
    QAction *Stick20ActionDebugAction;
    QAction *Stick20ActionSetupHiddenVolume;
    QAction *Stick20ActionClearNewSDCardFound;
    QAction *Stick20ActionSetupPasswordMatrix;
    QAction *Stick20ActionLockStickHardware;

    QString DebugText;


    int ExecStickCmd(char *Cmdline);

    void generateMenuOTP();
    void generateMenuForStick10();
    void initActionsForStick20();
    int  stick20SendCommand (uint8_t stick20Command, uint8_t *password);

    //QAction *totp1Action;

    void generateComboBoxEntrys();
    void generateMenu();
    void generateHOTPConfig(HOTPSlot *slot);
    void generateTOTPConfig(TOTPSlot *slot);
    void generateAllConfigs();

    void generateMenuForStick20();
    int  UpdateDynamicMenuEntrys (void);
    void AnalyseProductionInfos();

private slots:
    void checkConnection();
    void startConfiguration();
    void getCode(uint8_t slotNo);
//    void on_pushButton_clicked();
//    void on_pushButton_2_clicked();
    void on_writeButton_clicked();
    void displayCurrentSlotConfig();
    void displayCurrentGeneralConfig();
    void on_slotComboBox_currentIndexChanged(int index);
    void on_resetButton_clicked();
    void on_hexRadioButton_toggled(bool checked);
    void on_base32RadioButton_toggled(bool checked);
    void on_setToZeroButton_clicked();
    void on_setToRandomButton_clicked();
//    void on_checkBox_2_toggled(bool checked);
    void on_tokenIDCheckBox_toggled(bool checked);
    void on_writeGeneralConfigButton_clicked();

bool eventFilter (QObject *obj, QEvent *event);
void iconActivated(QSystemTrayIcon::ActivationReason reason);

    void getHOTPDialog (int slot);
    void getHOTP1();
    void getHOTP2();
    void getHOTP3();

    void getTOTPDialog (int slot);
    void getTOTP1();
    void getTOTP2();
    void getTOTP3();
    void getTOTP4();
    void getTOTP5();
    void getTOTP6();
    void getTOTP7();
    void getTOTP8();
    void getTOTP9();
    void getTOTP10();
    void getTOTP11();
    void getTOTP12();
    void getTOTP13();
    void getTOTP14();
    void getTOTP15();
    void on_eraseButton_clicked();
    void on_resetGeneralConfigButton_clicked();
    void on_randomSecretButton_clicked();
    void on_checkBox_toggled(bool checked);

    void startStick20Configuration();
    void startStickDebug();
    void startAboutDialog ();
    void startMatrixPasswordDialog();
    void startStick20Setup() ;



    void startStick20EnableCryptedVolume();
    void startStick20DisableCryptedVolume();
    void startStick20EnableHiddenVolume();
    void startStick20DisableHiddenVolume();
    void startStick20ActionChangeUserPIN ();
    void startStick20ActionChangeAdminPIN ();
    void startStick20EnableFirmwareUpdate();
    void startStick20ExportFirmwareToFile();
    void startStick20DestroyCryptedVolume();
    void startStick20FillSDCardWithRandomChars();
    void startStick20GetStickStatus();
    void startStick20SetReadonlyUncryptedVolume();
    void startStick20SetReadWriteUncryptedVolume();
    void startStick20DebugAction();
    void startStick20SetupHiddenVolume();
    void startStick20ClearNewSdCardFound();
    void startStick20SetupPasswordMatrix();
    void startStick20LockStickHardware();
};

#endif // MAINWINDOW_H
