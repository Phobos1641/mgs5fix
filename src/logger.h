#pragma once

namespace Logger
{
    void Init();
    void Shutdown();
    void Log(std::string_view level, std::string_view msg);
}

#define LOG_INFO(fmt, ...)  Logger::Log("Info",  std::format(fmt, ##__VA_ARGS__))
#define LOG_WARN(fmt, ...)  Logger::Log("Warn",  std::format(fmt, ##__VA_ARGS__))
#define LOG_ERROR(fmt, ...) Logger::Log("Error", std::format(fmt, ##__VA_ARGS__))

#ifdef _DEBUG
    #define LOG_DEBUG(fmt, ...) Logger::Log("Debug", std::format(fmt, ##__VA_ARGS__))
#else
    #define LOG_DEBUG(fmt, ...) do {} while(0)
#endif