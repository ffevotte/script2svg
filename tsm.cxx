#include "tsm.hxx"

namespace TSM {

inline Log::Level level (unsigned int sev) {
  switch (sev) {
  case 0: // FATAL
  case 1: // ALERT
  case 2: // CRITICAL
  case 3: // ERROR
    return Log::ERROR;
  case 4: // WARNING
    return Log::WARNING;
  case 5: // NOTICE
    return Log::NOTICE;
  case 6: // INFO
    return Log::INFO;
  case 7: // DEBUG
  default:
    return Log::DEBUG;
  }
}

inline void log (void *data, const char *file, int line, const char *fn,
                 const char *subs, unsigned int sev, const char *format,
                 va_list args) {
  Log::Logger & log {*static_cast<Log::Logger*>(data)};

  char buf[1024];
  vsnprintf(buf, 1024, format, args);

  log.write (level(sev), [&](auto&&out) {
      out << "[" << subs << "] "
          << buf << std::endl;
    });
}

inline void log_term_write (struct tsm_vte *vte, const char *u8, size_t len,
                            void *data)
{
  Log::Logger & log {*static_cast<Log::Logger*>(data)};
  log.write<Log::DEBUG> ([&](auto&&out){
      out << "[tsm-vte] write_cb (\"" << u8 << "\")" << std::endl;
    });
}

Screen::Screen (Log::Logger & logger) {
  tsm_screen_new (&screen_,
                  log, &logger);
}

Screen::~Screen () {
  tsm_screen_unref (screen_);
}

VTE::VTE (Log::Logger & logger,
          tsm_screen * screen) {
  tsm_vte_new (&vte_, screen,
               log_term_write, &logger,
               log,            &logger);
}

VTE::~VTE () {
  tsm_vte_unref (vte_);
}

}
