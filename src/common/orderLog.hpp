#ifndef YINHE_SRC_COMMON_ORDERLOG_H
#define YINHE_SRC_COMMON_ORDERLOG_H

#include <atomic>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

#include "SPSCQueue.hpp"
#include "types.hpp"

namespace fs = std::filesystem;

enum class LogEntryType : uint8_t { TRADE, MESSAGE, ERROR };

struct LogEntry {
  LogEntryType type;
  SimTick tick;
  OrderID id1;       // bid_id for TRADE, err_order_id for ERROR
  OrderID id2;       // ask_id for TRADE
  Price price;
  Quantity quantity;
  char message[128]; // MESSAGE type only
};

class OrderbookLogger {
public:
  OrderbookLogger() = default;

  ~OrderbookLogger() { close_Log(); }

  void set_logfile_save_location(std::string filepath) {
    if (!fs::is_directory(filepath)) {
      std::cerr << "Invalid filepath: provided filepath is not a directory\n";
      std::exit(1);
    }
    CUSTOM_LOGFILE_SAVE_LOCATION = filepath;
  }

  void init_Log() {
    auto log_dir = CUSTOM_LOGFILE_SAVE_LOCATION.empty()
                       ? DEFAULT_LOGFILE_SAVE_LOCATION
                       : CUSTOM_LOGFILE_SAVE_LOCATION;
    if (!fs::exists(log_dir))
      fs::create_directories(log_dir);
    logfile_location = log_dir + logfile_name;
    logFile.open(logfile_location, std::ios::app);
    if (!logFile.is_open()) {
      std::cerr << "Error opening logger file, exiting program" << std::endl;
      std::exit(1);
    }
    lastLogTick = 0;
    std::cout << "Opened: " << logfile_name << " at " << logfile_location
              << std::endl;

    // Start consumer thread
    stop_flag_.store(false, std::memory_order_relaxed);
    consumer_thread_ = std::thread(&OrderbookLogger::consumer_loop, this);
  }

  void log_Trade(SimTick tick, OrderID bid_id, OrderID ask_id, Price price,
                 Quantity quantity) {
    LogEntry entry{};
    entry.type = LogEntryType::TRADE;
    entry.tick = tick;
    entry.id1 = bid_id;
    entry.id2 = ask_id;
    entry.price = price;
    entry.quantity = quantity;
    push_entry(entry);
  }

  void log_message(std::string message, SimTick simulation_tick_time) {
    LogEntry entry{};
    entry.type = LogEntryType::MESSAGE;
    entry.tick = simulation_tick_time;
    std::strncpy(entry.message, message.c_str(), sizeof(entry.message) - 1);
    entry.message[sizeof(entry.message) - 1] = '\0';
    push_entry(entry);
  }

  void log_order_Error(OrderID err_order_id) {
    LogEntry entry{};
    entry.type = LogEntryType::ERROR;
    entry.id1 = err_order_id;
    push_entry(entry);
  }

  void close_Log() {
    if (!consumer_thread_.joinable())
      return;

    stop_flag_.store(true, std::memory_order_release);
    consumer_thread_.join();

    // Final drain — consumer is dead, single-thread pop is safe
    LogEntry entry;
    while (queue_.try_pop(entry)) {
      write_entry(entry);
    }

    logFile << "End logger" << std::endl;
    logFile << "Tick: " << std::to_string(lastLogTick) << std::endl;
    logFile.close();
    std::cout << "Closed logger" << std::endl;
  }

  void flush_log_Dir() {
    auto LOG_DIRECTORY = CUSTOM_LOGFILE_SAVE_LOCATION.empty()
                             ? DEFAULT_LOGFILE_SAVE_LOCATION
                             : CUSTOM_LOGFILE_SAVE_LOCATION;
    if (!fs::exists(LOG_DIRECTORY))
      return;
    try {
      for (const auto &entry : fs::directory_iterator(LOG_DIRECTORY)) {
        if (entry.is_regular_file()) {
          fs::remove(entry.path());
        }
      }
      std::cout << "Removed all files in directory \"" << LOG_DIRECTORY << "\""
                << std::endl;
    } catch (const fs::filesystem_error &e) {
      std::cerr << "Error: " << e.what() << " while flushing log files"
                << std::endl;
    }
  }

  std::string get_logfile_location() { return logfile_location; }

private:
  const std::string DEFAULT_LOGFILE_SAVE_LOCATION = "logs/";
  std::string CUSTOM_LOGFILE_SAVE_LOCATION;
  const std::string logfile_name = generate_logfile_name();
  std::string logfile_location;
  std::ofstream logFile;
  SimTick lastLogTick = 0;

  SPSCQueue<LogEntry> queue_;
  std::thread consumer_thread_;
  std::atomic<bool> stop_flag_{false};

  void push_entry(const LogEntry &entry) {
    while (!queue_.try_push(entry)) {
      std::this_thread::yield();
    }
  }

  void consumer_loop() {
    constexpr int kMaxSpins = 256;
    LogEntry entry;
    int idle_spins = 0;
    while (true) {
      if (queue_.try_pop(entry)) {
        write_entry(entry);
        idle_spins = 0;
        // Drain burst — keep popping without yielding
        while (queue_.try_pop(entry)) {
          write_entry(entry);
        }
      } else if (stop_flag_.load(std::memory_order_acquire)) {
        break;
      } else {
        if (++idle_spins >= kMaxSpins) {
          std::this_thread::yield();
          idle_spins = 0;
        }
      }
    }
    // Final drain after stop
    while (queue_.try_pop(entry)) {
      write_entry(entry);
    }
    logFile.flush();
  }

  void write_entry(const LogEntry &e) {
    switch (e.type) {
    case LogEntryType::TRADE:
      logFile << e.tick << " | " << e.id1 << " | " << e.id2 << " | "
              << e.price << " | " << e.quantity << "\n";
      lastLogTick = e.tick;
      break;
    case LogEntryType::MESSAGE:
      logFile << "\n-----------------------------------------------------------\n"
              << e.tick << " | MESSAGE:\n"
              << e.message
              << "\n-----------------------------------------------------------\n\n";
      break;
    case LogEntryType::ERROR:
      logFile << "Error with order: " << e.id1 << "\n";
      break;
    }
  }

  const std::string generate_logfile_name() const {
    std::time_t t = std::time(0);
    std::tm *tm_now = std::localtime(&t);
    return "log" + std::to_string(tm_now->tm_year) +
           std::to_string(tm_now->tm_mon) + std::to_string(tm_now->tm_mday) +
           std::to_string(tm_now->tm_hour) + std::to_string(tm_now->tm_min) +
           std::to_string(tm_now->tm_sec) + ".log";
  }
};

#endif
