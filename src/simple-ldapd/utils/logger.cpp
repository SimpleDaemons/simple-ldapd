/**
 * @file logger.cpp
 * @brief Logger implementation
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/utils/logger.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace simple_ldapd {

namespace {

const char *levelName(LogLevel level) {
  switch (level) {
  case LogLevel::Debug:
    return "DEBUG";
  case LogLevel::Info:
    return "INFO";
  case LogLevel::Warning:
    return "WARN";
  case LogLevel::Error:
    return "ERROR";
  case LogLevel::Fatal:
    return "FATAL";
  }
  return "INFO";
}

}  // namespace

bool parseLogLevel(const std::string &name, LogLevel &level) {
  std::string lower = name;
  std::transform(lower.begin(), lower.end(), lower.begin(),
                 [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
  if (lower == "debug") {
    level = LogLevel::Debug;
    return true;
  }
  if (lower == "info") {
    level = LogLevel::Info;
    return true;
  }
  if (lower == "warning" || lower == "warn") {
    level = LogLevel::Warning;
    return true;
  }
  if (lower == "error") {
    level = LogLevel::Error;
    return true;
  }
  if (lower == "fatal") {
    level = LogLevel::Fatal;
    return true;
  }
  return false;
}

Logger &Logger::instance() {
  static Logger logger;
  return logger;
}

void Logger::setLevel(LogLevel level) {
  std::lock_guard<std::mutex> lock(mutex_);
  level_ = level;
}

LogLevel Logger::level() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return level_;
}

void Logger::setLogFile(const std::string &path) { log_file_ = path; }

void Logger::debug(const std::string &message) { log(LogLevel::Debug, message); }
void Logger::info(const std::string &message) { log(LogLevel::Info, message); }
void Logger::warning(const std::string &message) { log(LogLevel::Warning, message); }
void Logger::error(const std::string &message) { log(LogLevel::Error, message); }
void Logger::fatal(const std::string &message) { log(LogLevel::Fatal, message); }

void Logger::log(LogLevel level, const std::string &message) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (level < level_) {
    return;
  }
  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);
  std::ostringstream line;
  line << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << " ["
       << levelName(level) << "] " << message;
  std::cerr << line.str() << std::endl;
  if (!log_file_.empty()) {
    std::ofstream out(log_file_, std::ios::app);
    if (out) {
      out << line.str() << std::endl;
    }
  }
}

}  // namespace simple_ldapd
