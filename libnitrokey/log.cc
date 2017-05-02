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
        if ((int) lvl >= (int) m_loglevel) mp_loghandler->print(logstr, lvl);
    }

    void StdlogHandler::print(const std::string &str, Loglevel lvl) {
      std::time_t t = std::time(nullptr);
      std::tm tm = *std::localtime(&t);

      std::clog << "[" << loglevel_to_str(lvl) << "] ["
                << std::put_time(&tm, "%c %z")
                << "]\t" << str << std::endl;
    }

    void FunctionalLogHandler::print(const std::string &str, Loglevel lvl) {
      std::time_t t = std::time(nullptr);
      std::tm tm = *std::localtime(&t);

      std::stringstream s;
      s << "[" << loglevel_to_str(lvl) << "] ["
                << std::put_time(&tm, "%c %z")
                << "]\t" << str << std::endl;

      log_function(s.str());
    }

    FunctionalLogHandler::FunctionalLogHandler(std::function<void(std::string)> _log_function) {
      log_function = _log_function;
    }
  }
}
