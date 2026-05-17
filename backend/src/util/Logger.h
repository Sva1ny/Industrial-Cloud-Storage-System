#pragma once
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <mutex>

enum LogLevel { LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR };

class Logger {
public:
    static Logger& instance() {
        static Logger inst;
        return inst;
    }

    void set_level(LogLevel lvl) { m_level = lvl; }
    void set_file(const std::string &path) {
        if (m_file.is_open()) m_file.close();
        m_file.open(path, std::ios::app);
    }
    void set_trace_id(const std::string &id) { m_trace_id = id; }
    std::string trace_id() const { return m_trace_id; }

    void log(LogLevel lvl, const char *file, int line, const std::string &msg) {
        if (lvl < m_level) return;
        std::lock_guard<std::mutex> lock(m_mutex);
        std::string line_str = format(lvl, file, line, msg);
        std::cout << line_str << std::endl;
        if (m_file.is_open()) m_file << line_str << std::endl;
    }

private:
    LogLevel m_level = LOG_DEBUG;
    std::string m_trace_id;
    std::ofstream m_file;
    std::mutex m_mutex;

    std::string format(LogLevel lvl, const char *file, int line, const std::string &msg) {
        const char *labels[] = {"DEBUG", "INFO", "WARN", "ERROR"};
        time_t now = time(nullptr);
        char ts[20];
        strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&now));

        std::ostringstream oss;
        oss << "[" << ts << "] [" << labels[lvl] << "]";
        if (!m_trace_id.empty()) oss << " [" << m_trace_id << "]";
        oss << " " << file << ":" << line << " - " << msg;
        return oss.str();
    }
};

#define LOG_DEBUG(msg)  Logger::instance().log(LOG_DEBUG, __FILE__, __LINE__, msg)
#define LOG_INFO(msg)   Logger::instance().log(LOG_INFO,  __FILE__, __LINE__, msg)
#define LOG_WARN(msg)   Logger::instance().log(LOG_WARN,  __FILE__, __LINE__, msg)
#define LOG_ERROR(msg)  Logger::instance().log(LOG_ERROR, __FILE__, __LINE__, msg)
