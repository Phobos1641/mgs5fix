#include "stdafx.h"
#include "common.h"
#include "globals.h"
#include "config.h"
#include "screen.h"
#include "memory.h"

// Shared code in both TPP/GZ

namespace Common
{
    void CurrentResolution()
    {
        // Get current resolution
        MAKE_MIDHOOK(CurrentResolution_sh, std::format("{}: Current Resolution", CurrentGame->ShortName).c_str(), "48 89 ?? ?? 48 8B ?? ?? 48 ?? ?? ?? ?? ?? ?? ?? ?? B8 01 00 00 00 48 ?? ?? ??", 0, [](SafetyHookContext& ctx) {
            int resX = static_cast<int>(ctx.rax & 0xFFFFFFFF);
            int resY = static_cast<int>((ctx.rax >> 32) & 0xFFFFFFFF);

            Screen::CalculateAspectRatio(resX, resY);
        });
    }

    void Resolution()
    {
        // Unlock windowed/borderless resolutions
        if (auto* WindowedResolutions = Memory::FindPattern(std::format("{}: Unlock Resolutions: Windowed", CurrentGame->ShortName).c_str(), "72 ?? 0F ?? ?? 73 ?? 80 ?? ?? 00 74 ?? 0F ?? ?? 73 ?? F3 0F ?? ??"))
                Memory::PatchBytes(WindowedResolutions, "\xEB\x24"); // jmp over restrictions
    }
}