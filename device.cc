#include <chrono>
#include <thread>
#include <cstddef>
#include <stdexcept>
#include "hidapi/hidapi.h"
#include "include/misc.h"
#include "include/device.h"
#include "include/log.h"
#include <mutex>
#include "DeviceCommunicationExceptions.h"
#include "device.h"

std::mutex mex_dev_com;

using namespace nitrokey::device;
using namespace nitrokey::log;
using namespace std::chrono;

std::atomic_int Device::instances_count{0};
std::chrono::milliseconds Device::default_delay {0} ;

Device::Device(const uint16_t vid, const uint16_t pid, const DeviceModel model,
               const milliseconds send_receive_delay, const int retry_receiving_count,
               const milliseconds retry_timeout)
    :
      last_command_status(0),
      m_vid(vid),
      m_pid(pid),
      m_model(model),
      m_retry_sending_count(3),
      m_retry_receiving_count(retry_receiving_count),
      m_retry_timeout(retry_timeout),
      m_send_receive_delay(send_receive_delay),
      mp_devhandle(nullptr)
{
  instances_count++;
}

bool Device::disconnect() {
  //called in object's destructor
  LOG(__FUNCTION__, Loglevel::DEBUG_L2);
  std::lock_guard<std::mutex> lock(mex_dev_com);
  return _disconnect();
}

bool Device::_disconnect() {
  LOG(std::string(__FUNCTION__) + std::string(m_model == DeviceModel::PRO ? "PRO" : "STORAGE"), Loglevel::DEBUG_L2);
  LOG(std::string(__FUNCTION__) +  std::string(" *IN* "), Loglevel::DEBUG_L2);

  LOG(std::string("Disconnection success: ") + std::to_string(mp_devhandle == nullptr), Loglevel::DEBUG_L2);
  if(mp_devhandle == nullptr) return false;

  hid_close(mp_devhandle);
  mp_devhandle = nullptr;
#ifndef __APPLE__
  if (instances_count == 1){
    LOG(std::string("Calling hid_exit"), Loglevel::DEBUG_L2);
    hid_exit();
  }
#endif
  return true;
}

bool Device::connect() {
  LOG(__FUNCTION__, Loglevel::DEBUG_L2);
  std::lock_guard<std::mutex> lock(mex_dev_com);
  return _connect();
}

bool Device::_connect() {
  LOG(std::string(__FUNCTION__) + std::string(" *IN* "), Loglevel::DEBUG_L2);

//   hid_init(); // done automatically on hid_open
  mp_devhandle = hid_open(m_vid, m_pid, nullptr);
  const bool success = mp_devhandle != nullptr;
  LOG(std::string("Connection success: ") + std::to_string(success), Loglevel::DEBUG_L2);
  return success;
}

int Device::send(const void *packet) {
  LOG(__FUNCTION__, Loglevel::DEBUG_L2);
  std::lock_guard<std::mutex> lock(mex_dev_com);
  LOG(std::string(__FUNCTION__) +  std::string(" *IN* "), Loglevel::DEBUG_L2);

  int send_feature_report = -1;

  for (int i = 0; i < 3 && send_feature_report < 0; ++i) {
    if (mp_devhandle == nullptr) {
      LOG(std::string("Connection fail") , Loglevel::DEBUG_L2);
      throw DeviceNotConnected("Attempted HID send on an invalid descriptor.");
    }
    send_feature_report = hid_send_feature_report(
        mp_devhandle, (const unsigned char *)(packet), HID_REPORT_SIZE);
    if (send_feature_report < 0) _reconnect();
    //add thread sleep?
    LOG(std::string("Sending attempt: ")+std::to_string(i) + " / 3" , Loglevel::DEBUG_L2);
  }
  return send_feature_report;
}

int Device::recv(void *packet) {
  LOG(__FUNCTION__, Loglevel::DEBUG_L2);
  std::lock_guard<std::mutex> lock(mex_dev_com);
  LOG(std::string(__FUNCTION__) +  std::string(" *IN* "), Loglevel::DEBUG_L2);
  int status;
  int retry_count = 0;

  for (;;) {
    if (mp_devhandle == nullptr){
      LOG(std::string("Connection fail") , Loglevel::DEBUG_L2);
      throw DeviceNotConnected("Attempted HID receive on an invalid descriptor.");
    }

    status = (hid_get_feature_report(mp_devhandle, (unsigned char *)(packet),
                                     HID_REPORT_SIZE));

    auto pwherr = hid_error(mp_devhandle);
    std::wstring wherr = (pwherr != nullptr) ? pwherr : L"No error message";
    std::string herr(wherr.begin(), wherr.end());
    LOG(std::string("libhid error message: ") + herr,
                    Loglevel::DEBUG_L2);

    if (status > 0) break;  // success
    if (retry_count++ >= m_retry_receiving_count) {
      LOG(
          "Maximum retry count reached: " + std::to_string(retry_count),
          Loglevel::WARNING);
      LOG(
          std::string("Counter stats: ") + m_counters.get_as_string(),
          Loglevel::DEBUG);
      break;
    }
    _reconnect();
    LOG("Retrying... " + std::to_string(retry_count),
                    Loglevel::DEBUG);
    std::this_thread::sleep_for(m_retry_timeout);
  }

  return status;
}

bool Device::could_be_enumerated() {
  LOG(__FUNCTION__, Loglevel::DEBUG_L2);
  std::lock_guard<std::mutex> lock(mex_dev_com);
  if (mp_devhandle==nullptr){
    return false;
  }
#ifndef __APPLE__
  auto pInfo = hid_enumerate(m_vid, m_pid);
  if (pInfo != nullptr){
    hid_free_enumeration(pInfo);
    return true;
  }
  return false;
#else
//  alternative for OSX
  unsigned char buf[1];
  return hid_read_timeout(mp_devhandle, buf, sizeof(buf), 20) != -1;
#endif
}

void Device::show_stats() {
  auto s = m_counters.get_as_string();
  LOG(s, Loglevel::DEBUG_L2);
}

void Device::_reconnect() {
  LOG(__FUNCTION__, Loglevel::DEBUG_L2);
  ++m_counters.low_level_reconnect;
  _disconnect();
  _connect();
}

Device::~Device() {
  show_stats();
  disconnect();
  instances_count--;
}

void Device::set_default_device_speed(int delay) {
  default_delay = std::chrono::duration<int, std::milli>(delay);
}


void Device::set_receiving_delay(const std::chrono::milliseconds delay){
  std::lock_guard<std::mutex> lock(mex_dev_com);
  m_send_receive_delay = delay;
}

void Device::set_retry_delay(const std::chrono::milliseconds delay){
  std::lock_guard<std::mutex> lock(mex_dev_com);
  m_retry_timeout = delay;
}

Stick10::Stick10():
  Device(0x20a0, 0x4108, DeviceModel::PRO, 100ms, 5, 100ms)
  {
    setDefaultDelay();
  }


Stick20::Stick20():
  Device(0x20a0, 0x4109, DeviceModel::STORAGE, 20ms, 20, 20ms)
  {
    setDefaultDelay();
  }

#include <sstream>
#define p(x) ss << #x << " " << x << ", ";
std::string Device::ErrorCounters::get_as_string() {
  std::stringstream ss;
  p(total_comm_runs);
  p(communication_successful);
  ss << "(";
  p(command_successful_recv);
  p(command_result_not_equal_0_recv);
  ss << "), ";
  p(sends_executed);
  p(recv_executed);
  p(successful_storage_commands);
  p(total_retries);
  ss << "(";
  p(busy);
  p(busy_progressbar);
  p(CRC_other_than_awaited);
  p(wrong_CRC);
  ss << "), ";
  p(low_level_reconnect);
  p(sending_error);
  p(receiving_error);
  return ss.str();
}

void Device::setDefaultDelay() {
  LOG(__FUNCTION__, Loglevel::DEBUG_L2);

  auto count = default_delay.count();
  if (count != 0){
    LOG("Setting default delay to " + std::to_string(count), Loglevel::DEBUG_L2);
      m_retry_timeout = default_delay;
      m_send_receive_delay = default_delay;
    }
}

#undef p
