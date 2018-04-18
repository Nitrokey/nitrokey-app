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

#ifndef LOG_H
#define LOG_H

#include <string>
#include <functional>

namespace nitrokey {
  namespace log {

//for MSVC
#ifdef ERROR
#undef ERROR
#endif


    enum class Loglevel : int {
      ERROR,
      WARNING,
      INFO,
      DEBUG_L1,
      DEBUG,
      DEBUG_L2
    };

    class LogHandler {
    public:
      virtual void print(const std::string &, Loglevel lvl) = 0;
    protected:
      std::string loglevel_to_str(Loglevel);
      std::string format_message_to_string(const std::string &str, const Loglevel &lvl);

    };

    class StdlogHandler : public LogHandler {
    public:
      virtual void print(const std::string &, Loglevel lvl);
    };

    class FunctionalLogHandler : public LogHandler {
      using log_function_type = std::function<void(std::string)>;
      log_function_type log_function;
    public:
      FunctionalLogHandler(log_function_type _log_function);
      virtual void print(const std::string &, Loglevel lvl);

    };

    extern StdlogHandler stdlog_handler;

    class Log {
    public:
      Log() : mp_loghandler(&stdlog_handler), m_loglevel(Loglevel::WARNING) {}

      static Log &instance() {
        if (mp_instance == nullptr) mp_instance = new Log;
        return *mp_instance;
      }

      void operator()(const std::string &, Loglevel);
      void set_loglevel(Loglevel lvl) { m_loglevel = lvl; }
      void set_handler(LogHandler *handler) { mp_loghandler = handler; }

    private:
      LogHandler *mp_loghandler;
      Loglevel m_loglevel;
      static std::string prefix;
    public:
      static void setPrefix(std::string prefix = std::string());

    private:

      static Log *mp_instance;
    };
  }
}


#ifdef NO_LOG
#define LOG(string, level) while(false){}
#define LOGD(string) while(false){}
#else
#define LOG(string, level) nitrokey::log::Log::instance()((string), (level))
#define LOGD1(string) nitrokey::log::Log::instance()((string), (nitrokey::log::Loglevel::DEBUG_L1))
#define LOGD(string) nitrokey::log::Log::instance()((string), (nitrokey::log::Loglevel::DEBUG_L2))
#endif

#endif
