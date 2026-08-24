/**
 * @file logger.hpp
 * @brief Thread-safe logger
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include <mutex>
#include <string>

namespace simple_ldapd {

enum class LogLevel { Debug = 0, Info = 1, Warning = 2, Error = 3, Fatal = 4 };

class Logger {
public:
  static Logger &instance();

  void setLevel(LogLevel level);
  LogLevel level() const;
  void setLogFile(const std::string &path);

  void debug(const std::string &message);
  void info(const std::string &message);
  void warning(const std::string &message);
  void error(const std::string &message);
  void fatal(const std::string &message);

private:
  Logger() = default;
  void log(LogLevel level, const std::string &message);

  LogLevel level_{LogLevel::Info};
  std::string log_file_;
  mutable std::mutex mutex_;
};

}  // namespace simple_ldapd
