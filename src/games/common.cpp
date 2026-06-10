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
            // Unlock windowed/borderless resolution list
            MAKE_MIDHOOK(WindowedResolutions_sh, std::format("{}: Unlock Resolutions: Windowed/Borderless", CurrentGame->ShortName).c_str(), "3B ?? ?? 0F 87 ?? ?? ?? ?? 3B ?? ?? 0F 87 ?? ?? ?? ?? 3B ?? ??", 0, [](SafetyHookContext& ctx) {
                ctx.rflags |= (1ULL << 6);
                ctx.rip = WindowedResolutions_sh.target_address() + 0x2C;
            });
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

    void HUD()
    {
        // Nothing shared yet
    }

    void Framerate()
    {
        if (Config::UnlockFPS) {
            // Force "variable" framerate setting
            if (auto* FramerateSetting = Memory::FindPattern(std::format("{}: Framerate: Setting", CurrentGame->ShortName).c_str(), "48 33 ?? ?? ?? ?? ?? 49 85 ?? 48 0F ?? ?? ?? ?? ?? ?? 48 89 ?? ?? ?? ??"))
                Memory::PatchBytes(FramerateSetting, "\x48\x31\xC0\x90\x90\x90\x90"); // xor rax, rax
            
            // Force "variable" framerate target
            if (auto* FramerateTarget = Memory::FindPattern(std::format("{}: Framerate: Target", CurrentGame->ShortName).c_str(), "49 85 ?? 75 ?? F2 0F 10 0D ?? ?? ?? ??"))
                Memory::PatchBytes(FramerateTarget + 0x3, "\xEB");

            // Reduce sleep duration for main thread
            MAKE_MIDHOOK(MainThreadSleep_sh, std::format("{}: Main Thread Sleep", CurrentGame->ShortName).c_str(), "48 ?? ?? 48 85 ?? 75 ?? 8D ?? 01 48 8D ?? ?? ??", 0xB, [](SafetyHookContext& ctx) {
                if (ctx.rbp == 1) ctx.rdx = 0; // Sleep(0)
            });

            // Set timer resolution
            if (HMODULE ntdll = GetModuleHandleA("ntdll.dll")) {
                using _NtSetTimerResolution = NTSTATUS(NTAPI*)(ULONG, BOOLEAN, PULONG);
                if (auto NtSetTimerResolution = reinterpret_cast<_NtSetTimerResolution>(GetProcAddress(ntdll, "NtSetTimerResolution"))) {
                    ULONG currentRes;
                    if (NtSetTimerResolution(5000, TRUE, &currentRes) == 0)
                        LOG_INFO("{}: Framerate: Timer resolution set to 0.5ms", CurrentGame->ShortName);
                }
            }
        }
    }

    void Graphics()
    {
        if (Config::LODTweaks) {
            // Adjust model/grass draw distance
            MAKE_MIDHOOK(ModelQuality_sh, std::format("{}: LOD: Model/Grass Distance", CurrentGame->ShortName).c_str(), "89 ?? 64 B0 01 C3 8B ?? ?? C6 ?? ?? 00 89 ?? ?? B0 01 C3", 0, [](SafetyHookContext& ctx) {
                const float grassDist = static_cast<float>(Config::GrassDistance);
                const float modelDist = static_cast<float>(Config::ModelDistance);

                if (ctx.rbx == 9)
                    ctx.rax = *reinterpret_cast<const uint32_t*>(&grassDist);
                else
                    ctx.rax = *reinterpret_cast<const uint32_t*>(&modelDist);
            });
        }
    }
}