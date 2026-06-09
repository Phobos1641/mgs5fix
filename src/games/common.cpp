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
        if (Config::FixResolution) {
            // Unlock windowed resolutions
            if (auto* WindowedResolutions = Memory::FindPattern(std::format("{}: Unlock Resolutions: Windowed", CurrentGame->ShortName).c_str(), "72 ?? 0F ?? ?? 73 ?? 80 ?? ?? 00 74 ?? 0F ?? ?? 73 ?? F3 0F ?? ??"))
                Memory::PatchBytes(WindowedResolutions, "\xEB\x24"); // jmp over restrictions
        }
    }

    void AspectRatio()
    {
        if (Config::FixAspect) {
            // Fix throwable arc marker
            MAKE_MIDHOOK(ThrowableMarker_sh, std::format("{}: Throwable Marker", CurrentGame->ShortName).c_str(), "E8 ?? ?? ?? ?? F3 0F ?? ?? ?? ?? 66 0F ?? ?? 66 0F ?? ?? 41 ?? ?? ?? 4C ?? ?? ?? ?? BA 01 00 00 00", 0x5, [](SafetyHookContext& ctx) {
                if (Screen::AspectRatio > Screen::NativeAspect)
                    ctx.xmm7.f32[0] = Screen::AspectMultiplier;
            });

            // Fix lens effects (flares, dirt etc)
            MAKE_MIDHOOK(LensEffects_sh, std::format("{}: Lens Effects", CurrentGame->ShortName).c_str(), "0F 28 ?? F3 ?? 0F ?? ?? ?? ?? ?? ?? F3 45 ?? ?? ?? ?? F3 45 ?? ?? ?? F3 44 ?? ?? ?? ?? E8 ?? ?? ?? ??", 0x3, [](SafetyHookContext& ctx) {
                if (Screen::AspectRatio > Screen::NativeAspect) {
                    ctx.xmm13.f32[0] = Screen::NativeAspect;
                    ctx.xmm9.f32[0] /= Screen::AspectMultiplier;
                }
            });
        }
    }
}