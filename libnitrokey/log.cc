/*
 * Copyright (c) 2015-2018 Nitrokey UG
 *
 * This file is part of libnitrokey.
 *
 * libnitrokey is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * libnitrokey is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libnitrokey. If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: LGPL-3.0
 */

#include "log.h"
#include <iostream>
#include <ctime>
#include <iomanip>

#include <sstream>

namespace nitrokey {
  namespace log {

    Log *Log::mp_instance = nullptr;
    StdlogHandler stdlog_handler;

    std::string Log::prefix = "";


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
      if (mp_loghandler != nullptr){
        if ((int) lvl <= (int) m_loglevel) mp_loghandler->print(prefix+logstr, lvl);
      }
    }

    void Log::setPrefix(const std::string prefix) {
      if (!prefix.empty()){
        Log::prefix = "["+prefix+"]";
      } else {
        Log::prefix = "";
      }
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
