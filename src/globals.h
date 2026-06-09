#pragma once

#include <string>
#include <filesystem>

struct GameInfo {
    std::string Exe;
    std::string Name;
    std::string ShortName;
    void(*Init)();
};

inline HMODULE ExeModule  = nullptr;
inline HMODULE ThisModule = nullptr;

inline std::string ExeName;
inline std::filesystem::path ExePath;
inline std::filesystem::path FixPath;

inline const GameInfo* CurrentGame = nullptr;