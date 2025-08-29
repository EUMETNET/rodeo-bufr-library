#ifndef _LOG_ENTRY_H_
#define _LOG_ENTRY_H_

#include <string>

#if defined(_MSC_VER)
#include <Winsock2.h>
#else
#include <sys/time.h>
#endif

#include <vector>

#if defined(_MSC_VER)
enum class LogLevel { UNKNOWN = 0, TRACE, DEBUG, INFO, WARN, ERROR_, FATAL };
#else
enum LogLevel { UNKNOWN = 0, TRACE, DEBUG, INFO, WARN, ERROR_, FATAL };
#endif

static const std::vector<std::string> LogLevelStr = {
    "Off", "Trace", "Debug", "Info", "Warning", "Error", "Fatal"};

class LogEntry {

public:
  LogEntry(std::string msg, LogLevel l = LogLevel::TRACE,
           std::string mid = "Unknown", std::string bid = "");
  LogEntry();
  ~LogEntry();
  std::string toCsv(char delimiter = ',') const;
  std::string toJson() const;
  LogLevel getLogLevel() const;

private:
  std::string entryTime() const;
  struct timeval tv;
  std::string log_msg;
  LogLevel log_level;
  std::string module_id;
  std::string bufr_id;
};

static const char *const log_message_template = " { \
      \"datetime\" : null, \
      \"loglevel\" : null, \
      \"moduleid\" : null, \
      \"bufrid\" : null, \
      \"logmsg\" : null }";

#endif
