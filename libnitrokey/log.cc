#include <iostream>
#include <string>
#include <ctime>
#include <iomanip>
#include "log.h"

#include <sstream>

namespace nitrokey {
  namespace log {

    Log *Log::mp_instance = nullptr;
    StdlogHandler stdlog_handler;

    std::string LogHandler::loglevel_to_str(Loglevel lvl) {
      switch (lvl) {
        case Loglevel::DEBUG_L1:
          return std::string("DEBUG_L1");
        case Loglevel::DEBUG_L2:
          return std::string("DEBUG_L2");
        case Loglevel::DEBUG:
          return std::string("DEBUG");
        case Loglevel::INFO:
          return std::string("INFO");
        case Loglevel::WARNING:
          return std::string("WARNING");
        case Loglevel::ERROR:
          return std::string("ERROR");
      }
      return std::string("");
    }

    void Log::operator()(const std::string &logstr, Loglevel lvl) {
      if (mp_loghandler != nullptr)
        if ((int) lvl <= (int) m_loglevel) mp_loghandler->print(logstr, lvl);
    }

    void StdlogHandler::print(const std::string &str, Loglevel lvl) {
      std::string s = format_message_to_string(str, lvl);
      std::clog << s;
    }

    void FunctionalLogHandler::print(const std::string &str, Loglevel lvl) {
      std::string s = format_message_to_string(str, lvl);
      log_function(s);
    }

    std::string LogHandler::format_message_to_string(const std::string &str, const Loglevel &lvl) {
      static bool last_short = false;
      if (str.length() == 1){
        last_short = true;
        return str;
      }
      time_t t = time(nullptr);
      tm tm = *localtime(&t);

      std::stringstream s;
      s
          << (last_short? "\n" : "")
          << "[" << std::put_time(&tm, "%c") << "]"
          << "[" << loglevel_to_str(lvl) << "]\t"
          << str << std::endl;
      last_short = false;
      return s.str();
    }

    FunctionalLogHandler::FunctionalLogHandler(log_function_type _log_function) {
      log_function = _log_function;
    }
  }
}
