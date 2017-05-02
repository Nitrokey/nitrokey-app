#ifndef LOG_H
#define LOG_H

#include <string>
#include <cstddef>

#include <functional>

namespace nitrokey {
  namespace log {

//for MSVC
#ifdef ERROR
#undef ERROR
#endif


    enum class Loglevel : int {
      DEBUG_L2, DEBUG, INFO, WARNING, ERROR
    };

    class LogHandler {
    public:
      virtual void print(const std::string &, Loglevel lvl) = 0;
    protected:
      std::string loglevel_to_str(Loglevel);
    };

    class StdlogHandler : public LogHandler {
    public:
      virtual void print(const std::string &, Loglevel lvl);
    };

    class FunctionalLogHandler : public LogHandler {
      std::function<void(std::string)> log_function;
    public:
      FunctionalLogHandler(std::function<void(std::string)> _log_function);
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

      static Log *mp_instance;
    };
  }
}


#ifdef NO_LOG
#define LOG(string, level) while(false){}
#define LOGD(string, level) while(false){}
#else
#define LOG(string, level) nitrokey::log::Log::instance()((string), (level))
#define LOGD(string) nitrokey::log::Log::instance()((string), (nitrokey::log::Loglevel::DEBUG_L2))
#endif

#endif
