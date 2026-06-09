#include "stdafx.h"
#include "version.h"
#include "globals.h"
#include "logger.h"

namespace Logger
{
    static std::ofstream logFile;

    static uint32_t GetModuleTimestamp(void* module)
    {
        const auto* dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(module);
        const auto* ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<std::uint8_t*>(module) + dosHeader->e_lfanew);
        return ntHeaders->FileHeader.TimeDateStamp;
    }

    void Log(std::string_view level, std::string_view msg)
    {
        if (!logFile.is_open()) return;

        constexpr std::streamoff maxSize = 1 * 1024 * 1024;
        if (logFile.tellp() >= maxSize) return;

        const auto now = std::chrono::system_clock::now();
        const auto time = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
        localtime_s(&tm, &time);

        logFile << std::format("[{:04d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}] [{}] {}\n",
            tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
            tm.tm_hour, tm.tm_min, tm.tm_sec,
            level, msg);

        if (logFile.tellp() >= maxSize)
            logFile << std::format("[{:04d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}] [Warn] Log size limit (1MB) reached, stopping all logging.\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

        logFile.flush();
    }

    void Init()
    {
        const std::string logPath = (ExePath / (FixName + ".log")).string();
        logFile.open(logPath, std::ios::trunc);

        if (!logFile.is_open()) {
            AllocConsole();
            freopen_s(reinterpret_cast<FILE**>(stdout), "CONOUT$", "w", stdout);
            std::cerr << "Fatal Error: " << FixName << ": Failed to open log file: " << logPath << std::endl;
            FreeLibraryAndExitThread(ThisModule, 1);
        }

        LOG_INFO("----------");
        LOG_INFO("{} v{}", FixName, FixVersion);
        LOG_INFO("Build Date: {} {}", __DATE__, __TIME__);
        LOG_INFO("----------");
        LOG_INFO("Log File: {}", logPath);
        LOG_INFO("----------");
        LOG_INFO("Module Name: {}", ExeName);
        LOG_INFO("Module Path: {}", ExePath.string());
        LOG_INFO("Module Address: 0x{:x}", reinterpret_cast<uintptr_t>(ExeModule));
        LOG_INFO("Module Timestamp: {:d}", GetModuleTimestamp(ExeModule));
        LOG_INFO("----------");
    }

    void Shutdown()
    {
        if (logFile.is_open())
            logFile.close();
    }
}