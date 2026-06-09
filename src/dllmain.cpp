#include "stdafx.h"
#include "version.h"
#include "logger.h"
#include "globals.h"
#include "config.h"

#include "games/TPP.h"
#include "games/GZ.h"

const std::vector<GameInfo> SupportedGames = {
    { "mgsvtpp.exe", "METAL GEAR SOLID V: THE PHANTOM PAIN", "TPP", TPP::Init },
    { "MgsGroundZeroes.exe", "METAL GEAR SOLID V: GROUND ZEROES", "GZ", GZ::Init }
};

DWORD __stdcall Main(void*)
{
    Logger::Init();
    Config::Init();

    for (const auto& game : SupportedGames) {
        if (_stricmp(ExeName.c_str(), game.Exe.c_str()) == 0) {
            CurrentGame = &game;
            LOG_INFO("Detected: {} ({})", CurrentGame->Name, CurrentGame->Exe);
            CurrentGame->Init();
            LOG_INFO("----------");
            return 0;
        }
    }

    LOG_ERROR("Unrecognised game executable: {}", ExeName);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH: { 
        DisableThreadLibraryCalls(hModule);
        
        ThisModule = hModule;
        ExeModule = GetModuleHandle(NULL);

        WCHAR path[MAX_PATH]{};

        GetModuleFileNameW(ThisModule, path, MAX_PATH);
        FixPath = std::filesystem::path(path).remove_filename();

        GetModuleFileNameW(ExeModule, path, MAX_PATH);
        std::filesystem::path exePath(path);
        ExeName = exePath.filename().string();
        ExePath = exePath.remove_filename();

        HANDLE mainHandle = CreateThread(NULL, 0, Main, 0, NULL, 0);
        if (mainHandle) {
            SetThreadPriority(mainHandle, THREAD_PRIORITY_HIGHEST);
            CloseHandle(mainHandle);
        }
        else {
            return FALSE;
        }
        break;
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}