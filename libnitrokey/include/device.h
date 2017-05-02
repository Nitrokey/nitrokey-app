#ifndef DEVICE_H
#define DEVICE_H
#include <chrono>
#include "hidapi/hidapi.h"
#include <cstdint>
#include <string>

#define HID_REPORT_SIZE 65

#include <atomic>

namespace nitrokey {
namespace device {
    using namespace std::chrono_literals;
    using std::chrono::milliseconds;

    struct EnumClassHash
    {
        template <typename T>
        std::size_t operator()(T t) const
        {
          return static_cast<std::size_t>(t);
        }
    };

enum class DeviceModel{
    PRO,
    STORAGE
};

#include <atomic>

class Device {

public:

  struct ErrorCounters{
    using cnt = std::atomic_int;
    cnt wrong_CRC;
    cnt CRC_other_than_awaited;
    cnt busy;
    cnt total_retries;
    cnt sending_error;
    cnt receiving_error;
    cnt total_comm_runs;
    cnt successful_storage_commands;
    cnt command_successful_recv;
    cnt recv_executed;
    cnt sends_executed;
    cnt busy_progressbar;
    cnt command_result_not_equal_0_recv;
    cnt communication_successful;
    cnt low_level_reconnect;
    std::string get_as_string();

  } m_counters = {};


    Device(const uint16_t vid, const uint16_t pid, const DeviceModel model,
                   const milliseconds send_receive_delay, const int retry_receiving_count,
                   const milliseconds retry_timeout);

    virtual ~Device();

  // lack of device is not actually an error,
  // so it doesn't throw
  virtual bool connect();
  virtual bool disconnect();

  /*
   *	Sends packet of HID_REPORT_SIZE.
   */
  virtual int send(const void *packet);

  /*
   *	Gets packet of HID_REPORT_SIZE.
   *	Can sleep. See below.
   */
  virtual int recv(void *packet);

  /***
   * Returns true if some device is visible by OS with given VID and PID
   * whether the device is connected through HID API or not.
   * @return true if visible by OS
   */
  bool could_be_enumerated();

  void show_stats();
//  ErrorCounters get_stats(){ return m_counters; }
  int get_retry_receiving_count() const { return m_retry_receiving_count; };
  int get_retry_sending_count() const { return m_retry_sending_count; };
  std::chrono::milliseconds get_retry_timeout() const { return m_retry_timeout; };
  std::chrono::milliseconds get_send_receive_delay() const {return m_send_receive_delay;}

  int get_last_command_status() {int a = std::atomic_exchange(&last_command_status, static_cast<uint8_t>(0)); return a;};
  void set_last_command_status(uint8_t _err) { last_command_status = _err;} ;
  bool last_command_sucessfull() const {return last_command_status == 0;};
  DeviceModel get_device_model() const {return m_model;}
  void set_receiving_delay(std::chrono::milliseconds delay);
  void set_retry_delay(std::chrono::milliseconds delay);
  static void set_default_device_speed(int delay);
  void setDefaultDelay();

private:
  std::atomic<uint8_t> last_command_status;
  void _reconnect();
  bool _connect();
  bool _disconnect();

protected:
  const uint16_t m_vid;
  const uint16_t m_pid;
  const DeviceModel m_model;

  /*
   *	While the project uses Signal11 portable HIDAPI
   *	library, there's no way of doing it asynchronously,
   *	hence polling.
   */
  const int m_retry_sending_count;
  const int m_retry_receiving_count;
  std::chrono::milliseconds m_retry_timeout;
  std::chrono::milliseconds m_send_receive_delay;
  std::atomic<hid_device *>mp_devhandle;

  static std::atomic_int instances_count;
  static std::chrono::milliseconds default_delay ;
};

class Stick10 : public Device {
 public:
  Stick10();

};

class Stick20 : public Device {
 public:
  Stick20();
};
}
}
#endif
