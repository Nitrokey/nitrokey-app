/*
 * Copyright (c) 2017-2018 Nitrokey UG
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

#include "StorageActions.h"
#include "src/ui/pindialog.h"
#include "src/utils/bool_values.h"
#include <libnitrokey/NitrokeyManager.h>

#ifdef Q_OS_LINUX
#include "systemutils.h"
#include <sys/mount.h> // for unmounting on linux
#endif                 // Q_OS_LINUX

#if defined(Q_OS_LINUX) || defined(Q_OS_MAC)
#include <unistd.h> //for sync syscall
#endif              // Q_OS_LINUX || Q_OS_MAC
#include <OwnSleep.h>
#include <src/ui/stick20lockfirmwaredialog.h>
#include <src/ui/stick20hiddenvolumedialog.h>

#define LOCAL_PASSWORD_SIZE 40
#include <memory>
#include <src/core/ThreadWorker.h>
#include <libnitrokey/stick20_commands.h>


void unmountEncryptedVolumes() {
    return;
#if defined(Q_OS_LINUX)
  std::string endev = systemutils::getEncryptedDevice();
  if (endev.size() < 1)
    return;
  std::string mntdir = systemutils::getMntPoint(endev);
  qDebug() << "Unmounting " << mntdir.c_str();
  // TODO polling with MNT_EXPIRE? test which will suit better
  // int err = umount2("/dev/nitrospace", MNT_DETACH);
  int err = umount(mntdir.c_str());
  if (err != 0) {
    qDebug() << "Unmount error: " << strerror(errno);
  }
#endif // Q_OS_LINUX
}

void local_sync() {
  // TODO TEST unmount during/after big data transfer
  fflush(NULL); // for windows, not necessarly needed or working
#if defined(Q_OS_LINUX) || defined(Q_OS_MAC)
  sync();
#endif // Q_OS_LINUX || Q_OS_MAC
  // manual says sync blocks until it's done, but they
  // are not guaranteeing will this save data integrity anyway,
  // additional sleep might help
  OwnSleep::sleep(1);
  // unmount does sync on its own additionally (if successful)
  unmountEncryptedVolumes();
}


void StorageActions::startStick20EnableCryptedVolume() {
  bool answer;

  if (TRUE == HiddenVolumeActive) {
    answer = csApplet()->yesOrNoBox(tr("This activity locks your hidden volume. Do you want to "
                                           "proceed?\nTo avoid data loss, please unmount the partitions before "
                                           "proceeding."), false);
    if (!answer)
      return;
  }

  PinDialog dialog(PinType::USER_PIN, nullptr);

  const auto user_wants_to_proceed = QDialog::Accepted == dialog.exec();
  if (user_wants_to_proceed) {
    startProgressFunc(tr("Enabling encrypted volume")); //FIXME use existing translation
    const auto s = dialog.getPassword();

    new ThreadWorker(
    [s]() -> Data { // FIXME make s shared_ptr to delete after use //or secure string
      Data data;
      data["error"] = 0;

      auto m = nitrokey::NitrokeyManager::instance();
      auto l = libada::i();
      try{
        local_sync();
        bool enable_storage_v045_workaround = l->getMinorFirmwareVersion() <= 45;
        m->unlock_encrypted_volume(s.c_str());
        if (enable_storage_v045_workaround)
          OwnSleep::sleep(5); //workaround for https://github.com/Nitrokey/nitrokey-storage-firmware/issues/31

        data["success"] = true;
      }
      catch (CommandFailedException &e){
        data["error"] = e.last_command_status;
        if (e.reason_wrong_password()){
          data["wrong_password"] = true;
        }
      }
      catch (DeviceCommunicationException &e){
        data["error"] = -1;
        data["comm_error"] = true;
      }

      return data;
    },
    [this](Data data){
      if(data["success"].toBool()){
        CryptedVolumeActive = true;
        emit storageStatusChanged();
        show_message_function(tr("Encrypted volume enabled"));
      } else if (data["wrong_password"].toBool()){
        csApplet()->warningBox(tr("Could not enable encrypted volume.") + " " //FIXME use existing translation
                               + tr("Wrong password."));

      } else {
        csApplet()->warningBox(tr("Could not enable encrypted volume.") + " "
                               + tr("Status code: %1").arg(data["error"].toInt())); //FIXME use existing translation
      }
      end_progress_function();
    }, this);

  }
}

void StorageActions::startStick20DisableCryptedVolume() {
  if (TRUE == CryptedVolumeActive) {
    const bool user_wants_to_proceed = csApplet()->yesOrNoBox(tr("This activity locks your encrypted volume. Do you want to "
                                                "proceed?\nTo avoid data loss, please unmount the partitions before "
                                                "proceeding."), false);
    if (!user_wants_to_proceed)
      return;

    startProgressFunc(tr("Disabling encrypted volume")); //FIXME use existing translation

    new ThreadWorker(
    []() -> Data {
      Data data;

      try{
        local_sync();
        auto m = nitrokey::NitrokeyManager::instance();
        m->lock_encrypted_volume();
        data["success"] = true;
      }
      catch (CommandFailedException &e){
        data["error"] = e.last_command_status;
      }
      catch (DeviceCommunicationException &e){
        data["error"] = -1;
        data["comm_error"] = true;
      }

      return data;
    },
    [this](Data data){
      if(data["success"].toBool()) {
        CryptedVolumeActive = false;
        emit storageStatusChanged();
        show_message_function(tr("Encrypted volume disabled"));
      }
       else {
        csApplet()->warningBox(tr("Could not lock encrypted volume.") + " "
                               + tr("Status code: %1").arg(data["error"].toInt())); //FIXME use existing translation
      }
      end_progress_function();
    }, this);
  }
}

void StorageActions::startStick20EnableHiddenVolume() {
  if (!CryptedVolumeActive) {
    csApplet()->warningBox(tr("Please enable the encrypted volume first."));
    return;
  }

  const bool user_wants_to_proceed =
      csApplet()->yesOrNoBox(tr("This activity locks your encrypted volume. Do you want to "
                                    "proceed?\nTo avoid data loss, please unmount the partitions before "
                                    "proceeding."), true);
  if (!user_wants_to_proceed)
    return;

  PinDialog dialog(PinType::HIDDEN_VOLUME, nullptr);
  const auto user_gives_password = dialog.exec() == QDialog::Accepted ;

  if (!user_gives_password) {
    return;
  }

  startProgressFunc(tr("Enabling hidden volume")); //FIXME use existing translation

  auto s = dialog.getPassword();

  new ThreadWorker(
  [s]() -> Data { //FIXME transport throuugh shared_ptr or secure string
    Data data;

    auto m = nitrokey::NitrokeyManager::instance();
    auto l = libada::i();
    try {
      local_sync();
      bool enable_storage_v045_workaround = l->getMinorFirmwareVersion() <= 45;
      m->unlock_hidden_volume(s.c_str());

      if (enable_storage_v045_workaround)
        OwnSleep::sleep(5); //workaround for https://github.com/Nitrokey/nitrokey-storage-firmware/issues/31

      data["success"] = true;
    }
    catch (CommandFailedException &e){
      data["error"] = e.last_command_status;
      if (e.reason_wrong_password()){
        data["wrong_password"] = true;
      }
    }
    catch (DeviceCommunicationException &e){
      data["error"] = -1;
      data["comm_error"] = true;
    }

    return data;
  },
  [this](Data data){

    if(data["success"].toBool()){
      HiddenVolumeActive = true;
      emit storageStatusChanged();
      show_message_function(tr("Hidden volume enabled"));//FIXME use existing translation
    } else if (data["wrong_password"].toBool()){
      csApplet()->warningBox(tr("Could not enable hidden volume.") + " " //FIXME use existing translation
                             + tr("Wrong password."));

    } else {
      csApplet()->warningBox(tr("Could not enable hidden volume.") + " "
                             + tr("Status code: %1").arg(data["error"].toInt())); //FIXME use existing translation
    }
    end_progress_function();
  }, this);

}

void StorageActions::startStick20DisableHiddenVolume() {
  const bool user_wants_to_proceed =
      csApplet()->yesOrNoBox(tr("This activity locks your hidden volume. Do you want to proceed?\nTo "
                                    "avoid data loss, please unmount the partitions before proceeding."), true);
  if (!user_wants_to_proceed)
    return;

  startProgressFunc(tr("Disabling hidden volume")); //FIXME use existing translation


  new ThreadWorker(
      []() -> Data {
        Data data;

        try{
          local_sync();
          auto m = nitrokey::NitrokeyManager::instance();
          m->lock_hidden_volume();
          data["success"] = true;
        }
        catch (CommandFailedException &e){
          data["error"] = e.last_command_status;
        }
        catch (DeviceCommunicationException &e){
          data["error"] = -1;
          data["comm_error"] = true;
        }

        return data;
      },
      [this](Data data){
        if(data["success"].toBool()) {
          HiddenVolumeActive = false;
          emit storageStatusChanged();
          show_message_function(tr("Hidden volume disabled")); //FIXME use existing translation
        }
        else {
          csApplet()->warningBox(tr("Could not lock hidden volume.") + " "
                                 + tr("Status code: %1").arg(data["error"].toInt())); //FIXME use existing translation
        }
        end_progress_function();
      }, this);

}

void StorageActions::startLockDeviceAction(bool ask_for_confirmation) {
  bool user_wants_to_proceed;

  if ((ask_for_confirmation) && ((TRUE == CryptedVolumeActive) || (TRUE == HiddenVolumeActive))) {
    user_wants_to_proceed = csApplet()->yesOrNoBox(tr("This activity locks your encrypted volume. Do you want to "
                                           "proceed?\nTo avoid data loss, please unmount the partitions before "
                                           "proceeding."), true);
    if (!user_wants_to_proceed) {
      return;
    }
  }

  startProgressFunc(tr("Locking device")); //FIXME use existing translation


  new ThreadWorker(
    []() -> Data {
      Data data;
      try {
        local_sync();
        auto m = nitrokey::NitrokeyManager::instance();
        m->lock_device();
        data["success"] = true;
      }
      catch (CommandFailedException &e){
        data["error"] = e.last_command_status;
      }
      catch (DeviceCommunicationException &e){
        data["error"] = -1;
        data["comm_error"] = true;
      }
      return data;
    },
    [this](Data data){
      if(data["success"].toBool()) {
        HiddenVolumeActive = false;
        CryptedVolumeActive = false;
        emit storageStatusChanged();
        show_message_function(tr("Device locked")); //FIXME use existing translation
      }
      else {
        csApplet()->warningBox(tr("Could not lock device.") + " "
                               + tr("Status code: %1").arg(data["error"].toInt())); //FIXME use existing translation
      }

      end_progress_function();
    }, this);

}

#include "stick20updatedialog.h"

void StorageActions::startStick20EnableFirmwareUpdate() {
  UpdateDialog dialogUpdate(nullptr);

  bool user_wants_to_proceed = dialogUpdate.exec() == QDialog::Accepted;
  if (!user_wants_to_proceed) {
    return;
  }

  auto successMessage = tr("Device set in update mode");
  auto failureMessage = tr("Device could not be set in update mode.");

  bool success = false;
  // try with default password
  // ask for password when fails
  runAndHandleErrorsInUI(successMessage, "",
                         [&success](){ //FIXME use secure string
                           local_sync();
                           auto m = nitrokey::NitrokeyManager::instance();
                           m->enable_firmware_update("12345678");
                           success = true;
                         },[](){});

  if(success) return;

  PinDialog dialog(PinType::FIRMWARE_PIN);
  user_wants_to_proceed = QDialog::Accepted == dialog.exec();

  if (!user_wants_to_proceed) {
    return;
  }
  auto s = dialog.getPassword();

  runAndHandleErrorsInUI(successMessage, failureMessage,
     [s](){ //FIXME use secure string
         local_sync();
         auto m = nitrokey::NitrokeyManager::instance();
         m->enable_firmware_update(s.c_str());
       },[](){});
}


void StorageActions::startStick20ExportFirmwareToFile() {
  PinDialog dialog(PinType::ADMIN_PIN);
  bool user_provided_PIN = QDialog::Accepted == dialog.exec();
  if (!user_provided_PIN) {
    return;
  }
  auto s = dialog.getPassword(); //FIXME use secure string

  //FIXME use existing translation
  runAndHandleErrorsInUI(tr("Firmware exported"), tr("Could not export firmware."), [s](){
    auto m = nitrokey::NitrokeyManager::instance();
    m->export_firmware(s.c_str());
  }, [](){});
}

void StorageActions::startStick20DestroyCryptedVolume(int fillSDWithRandomChars) {
  bool user_entered_PIN;
  bool answer;

  answer = csApplet()->yesOrNoBox(tr("WARNING: Generating new AES keys will destroy the encrypted volumes, "
                                         "hidden volumes, and password safe! Continue?"), false);
  if (answer) {
    PinDialog dialog(PinType::ADMIN_PIN);
    user_entered_PIN = QDialog::Accepted == dialog.exec();
    if (!user_entered_PIN) {
      return;
    }
    auto s = dialog.getPassword();

    startProgressFunc(tr("Generating new AES keys")); //FIXME use existing translation

    new ThreadWorker(
    [s]() -> Data { //FIXME use secure string
      Data data;
      try{
        auto m = nitrokey::NitrokeyManager::instance();
        m->lock_device(); //lock device to reset its state
        m->build_aes_key(s.c_str());
        data["success"] = true;
      }
      catch (CommandFailedException &e){
        data["error"] = e.last_command_status;
        if (e.reason_wrong_password()){
          data["wrong_password"] = true;
        }
      }
      catch (DeviceCommunicationException &e){
        data["error"] = -1;
        data["comm_error"] = true;
      }
      return data;
    },
    [this, fillSDWithRandomChars, s](Data data){ // FIXME use secure string
      if(data["success"].toBool()) {
        emit FactoryReset();
        show_message_function(tr("New AES keys generated")); //FIXME use existing translation
        if (fillSDWithRandomChars != 0) {
          _execute_SD_clearing(s);
        }
      } else if (data["wrong_password"].toBool()){
        csApplet()->warningBox(tr("Keys could not be generated.") + " " //FIXME use existing translation
                               + tr("Wrong password."));

      } else {
        csApplet()->warningBox(tr("Keys could not be generated.") + " "
                               + tr("Status code: %1").arg(data["error"].toInt())); //FIXME use existing translation
      }
      end_progress_function();
    }, this);
  }
}

void StorageActions::_execute_SD_clearing(const std::string &s) {
//does not need long operation indicator
  new ThreadWorker(
    [s]() -> Data { //FIXME use secure string
      Data data;
      data["success"] = false;
      try{
        auto m = nitrokey::NitrokeyManager::instance();
        m->fill_SD_card_with_random_data(s.c_str());
      }
      catch (LongOperationInProgressException &l){
        //expected
        data["success"] = true;
      }
      catch (CommandFailedException &e){
        data["error"] = e.last_command_status;
        if (e.reason_wrong_password()){
          data["wrong_password"] = true;
        }
      }
      catch (DeviceCommunicationException &e){
        data["error"] = -1;
        data["comm_error"] = true;
      }

      return data;
    },
    [this](Data data){
      if(data["success"].toBool()) {
        QTimer::singleShot(1000, [this](){
          emit storageStatusUpdated();
          emit longOperationStarted();
          emit storageStatusChanged();
        });
      } else if (data["wrong_password"].toBool()){
        csApplet()->warningBox(tr("Could not clear SD card.") + " " //FIXME use existing translation
                               + tr("Wrong password."));

      } else {
        csApplet()->warningBox(tr("Could not clear SD card.") + " "
                               + tr("Status code: %1").arg(data["error"].toInt())); //FIXME use existing translation
      }
    }, this);
}

void StorageActions::startStick20FillSDCardWithRandomChars() {
  PinDialog dialog(PinType::ADMIN_PIN);
  bool user_provided_PIN = QDialog::Accepted == dialog.exec();
  if (user_provided_PIN) {
    auto s = dialog.getPassword();
    _execute_SD_clearing(s);
  }
}

void StorageActions::runAndHandleErrorsInUI(QString successMessage, QString operationFailureMessage,
                                            std::function<void(void)> codeToRunInDeviceThread,
                                            std::function<void(void)> onSuccessInGuiThread) {
//  startProgressFunc(successMessage); //TODO enable after moving blocking code to separate thread
  try{
    //TODO run in separate thread
    codeToRunInDeviceThread();
    onSuccessInGuiThread();
    show_message_function(successMessage);
  }
  catch (CommandFailedException &e){
    if (!operationFailureMessage.isEmpty()){
      if (e.reason_wrong_password()){
        csApplet()->warningBox(operationFailureMessage + " "
                               + tr("Wrong password."));
      } else {
        csApplet()->warningBox(operationFailureMessage + " "
                               + tr("Status code: %1").arg(e.last_command_status));
      }
    }
  }
  catch (DeviceCommunicationException &e){
    if (!operationFailureMessage.isEmpty()){
      csApplet()->warningBox(operationFailureMessage + " " +
                           tr("Communication issue."));
    }
  }
//  end_progress_function();
}


void StorageActions::startStick20ClearNewSdCardFound() {
  PinDialog dialog(PinType::ADMIN_PIN);

  bool user_provided_pin = QDialog::Accepted == dialog.exec();
  if (!user_provided_pin) {
    return;
  }

  auto s = dialog.getPassword();
  auto operationFailureMessage = tr("Flag cannot be cleared."); //FIXME use existing translation
  auto operationSuccessMessage = tr("Flag cleared.");

  runAndHandleErrorsInUI(QString(), operationFailureMessage, [s]() { //FIXME use secure string
    auto m = nitrokey::NitrokeyManager::instance();
    m->clear_new_sd_card_warning(s.c_str());
  }, [this]() {
    emit storageStatusUpdated();
  });
}


void StorageActions::startStick20SetReadOnlyUncryptedVolume() {
  using nm = nitrokey::NitrokeyManager;
  auto type = PinType::ADMIN_PIN;
  if(nm::instance()->set_unencrypted_volume_rorw_pin_type_user()){
    type = PinType::USER_PIN;
  }
  PinDialog dialog(type);

  bool user_provided_pin = QDialog::Accepted == dialog.exec();
  if (!user_provided_pin) {
    return;
  }

  auto operationFailureMessage = tr("Cannot set unencrypted volume read-only"); //FIXME use existing translation
  auto operationSuccessMessage = tr("Unencrypted volume set read-only"); //FIXME use existing translation
  auto s = dialog.getPassword();

  runAndHandleErrorsInUI(operationSuccessMessage, operationFailureMessage, [s]() {
    auto m = nitrokey::NitrokeyManager::instance();
    m->set_unencrypted_read_only(s.c_str()); //FIXME use secure string
  }, [this]() {
    emit storageStatusChanged();
  });

}

void StorageActions::startStick20SetReadWriteUncryptedVolume() {
  using nm = nitrokey::NitrokeyManager;
  auto type = PinType::ADMIN_PIN;
  if(nm::instance()->set_unencrypted_volume_rorw_pin_type_user()){
    type = PinType::USER_PIN;
  }
  PinDialog dialog(type);

  bool user_provided_pin = QDialog::Accepted == dialog.exec();
  if (!user_provided_pin) {
    return;
  }

  auto operationFailureMessage = tr("Cannot set unencrypted volume read-write"); //FIXME use existing translation
  auto operationSuccessMessage = tr("Unencrypted volume set read-write"); //FIXME use existing translation
  auto s = dialog.getPassword();

  runAndHandleErrorsInUI(operationSuccessMessage, operationFailureMessage, [s]() {
    auto m = nitrokey::NitrokeyManager::instance();
    m->set_unencrypted_read_write(s.c_str()); //FIXME use secure string
  }, [this]() {
    emit storageStatusChanged();
  });

}

void StorageActions::startStick20SetReadOnlyEncryptedVolume() {
  PinDialog dialog(PinType::ADMIN_PIN);

  bool user_provided_pin = QDialog::Accepted == dialog.exec();
  if (!user_provided_pin) {
    return;
  }

  auto operationFailureMessage = tr("Cannot set encrypted volume read-only"); //FIXME use existing translation
  auto operationSuccessMessage = tr("Encrypted volume set read-only"); //FIXME use existing translation
  auto s = dialog.getPassword();

  runAndHandleErrorsInUI(operationSuccessMessage, operationFailureMessage, [s]() {
    auto m = nitrokey::NitrokeyManager::instance();
    m->set_encrypted_volume_read_only(s.c_str()); //FIXME use secure string
  }, [this]() {
    emit storageStatusChanged();
  });

}

void StorageActions::startStick20SetReadWriteEncryptedVolume() {
  PinDialog dialog(PinType::ADMIN_PIN);

  bool user_provided_pin = QDialog::Accepted == dialog.exec();
  if (!user_provided_pin) {
    return;
  }

  auto operationFailureMessage = tr("Cannot set encrypted volume read-write"); //FIXME use existing translation
  auto operationSuccessMessage = tr("Encrypted volume set read-write"); //FIXME use existing translation
  auto s = dialog.getPassword();

  runAndHandleErrorsInUI(operationSuccessMessage, operationFailureMessage, [s]() {
    auto m = nitrokey::NitrokeyManager::instance();
    m->set_encrypted_volume_read_write(s.c_str()); //FIXME use secure string
  }, [this]() {
    emit storageStatusChanged();
  });

}

void StorageActions::startStick20LockStickHardware() {
  csApplet()->messageBox(tr("Functionality not implemented in current version")); //FIXME use existing translation
  return;

  stick20LockFirmwareDialog firmwareDialog(nullptr);
  bool user_wants_to_proceed = QDialog::Accepted == firmwareDialog.exec();
  if (user_wants_to_proceed) {
    PinDialog pinDialog(PinType::ADMIN_PIN);

    bool user_provided_PIN = pinDialog.exec() == QDialog::Accepted;
    if (!user_provided_PIN) {
      return;
    }
    const auto pass = pinDialog.getPassword();
// TODO     stick20SendCommand(STICK20_CMD_SEND_LOCK_STICK_HARDWARE, password);
  }
}

void StorageActions::startStick20SetupHiddenVolume() {
  stick20HiddenVolumeDialog HVDialog(nullptr);

  if (FALSE == CryptedVolumeActive) {
    csApplet()->warningBox(tr("Please enable the encrypted volume first."));
    return;
  }

  auto operationSuccessMessage = tr("Hidden volume created");
  auto operationFailureMessage = tr("Hidden volume could not be created."); //FIXME use existing translation

  bool user_wants_to_proceed = HVDialog.exec() == QDialog::Accepted;
  if (!user_wants_to_proceed) {
    return;
  }
  const auto d = HVDialog.HV_Setup_st; //FIXME clear securely struct
  auto p = std::string( reinterpret_cast< char const* >(d.HiddenVolumePassword_au8));

  runAndHandleErrorsInUI(operationSuccessMessage, operationFailureMessage, [d, p](){ //FIXME use secure string
    auto m = nitrokey::NitrokeyManager::instance();
    m->create_hidden_volume(d.SlotNr_u8, d.StartBlockPercent_u8,
                            d.EndBlockPercent_u8, p.c_str());
  }, [](){});

}

StorageActions::StorageActions(QObject *parent, Authentication *auth_admin, Authentication *auth_user) : QObject(
    parent), auth_admin(auth_admin), auth_user(auth_user) {
  connect(this, SIGNAL(storageStatusChanged()), this, SLOT(on_StorageStatusChanged()));
}

#include <QDebug>


void StorageActions::on_StorageStatusChanged() {
  if (!libada::i()->isStorageDeviceConnected())
    return;

  new ThreadWorker(
    []() -> Data {
        bool interrupt = QThread::currentThread()->isInterruptionRequested();
        static bool first_run = true;
        int times = first_run? 5 : 3;
        first_run = false;

        for (int i = 0; i < times && !interrupt; ++i) {
            QThread::currentThread()->msleep(500);
            interrupt = QThread::currentThread()->isInterruptionRequested();
        }
      Data data;
        if(interrupt) {
            return data;
        }

        auto m = nitrokey::NitrokeyManager::instance();
        auto s = m->get_status_storage();
        data["encrypted_active"] = s.VolumeActiceFlag_st.encrypted;
        data["hidden_active"] = s.VolumeActiceFlag_st.hidden;
        return data;
    },
    [this](Data data){
        CryptedVolumeActive = data["encrypted_active"].toBool();
        HiddenVolumeActive = data["hidden_active"].toBool();
        emit storageStatusUpdated();
    }, this, "update storage status");
}

void StorageActions::set_start_progress_window(std::function<void(QString)> _start_progress_function) {
  startProgressFunc = _start_progress_function;
}

void StorageActions::set_end_progress_window(std::function<void()> _end_progress_function) {
  end_progress_function = _end_progress_function;
}

void StorageActions::set_show_message(std::function<void(QString)> _show_message) {
  show_message_function = _show_message;
}
