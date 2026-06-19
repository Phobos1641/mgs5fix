#include "stdafx.h"
#include "common.h"
#include "globals.h"
#include "config.h"
#include "screen.h"
#include "memory.h"

#include <cmath>

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
            // Unlock fullscreen resolution list
            MAKE_MIDHOOK(FullscreenResolutions_sh, std::format("{}: Unlock Resolutions: Fullscreen", CurrentGame->ShortName).c_str(), "74 ?? 0F ?? ?? F3 ?? 0F ?? ?? 0F ?? ?? F3 ?? 0F ?? ?? 0F ?? ??", 0x2, [](SafetyHookContext& ctx) {
                ctx.rflags |= (1ULL << 6); // ZF = 1
                ctx.rip = FullscreenResolutions_sh.target_address() - 0x2;
            });

            // Unlock windowed/borderless resolution list
            MAKE_MIDHOOK(WindowedResolutions_sh, std::format("{}: Unlock Resolutions: Windowed/Borderless", CurrentGame->ShortName).c_str(), "3B ?? ?? 0F 87 ?? ?? ?? ?? 3B ?? ?? 0F 87 ?? ?? ?? ?? 3B ?? ??", 0, [](SafetyHookContext& ctx) {
                ctx.rflags |= (1ULL << 6); // ZF = 1
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

            // Reduce sleep duration for critical threads
            MAKE_MIDHOOK(ThreadSleep_sh, std::format("{}: Framerate: Thread Sleep", CurrentGame->ShortName).c_str(), "48 ?? ?? 48 85 ?? 75 ?? 8D ?? 01 48 8D ?? ?? ??", 0xB, [](SafetyHookContext& ctx) {
                const int threadID = *reinterpret_cast<const int*>(ctx.rsi + 0x8);
                if (threadID <= 4) ctx.rdx = 0; // Sleep(0)
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

    void FOV()
    {
        if (!Config::ChangeFOV)
            return;

        const auto deg2rad     = 3.1415926F / 180.F;
        const auto frame_width = 36.F;
        const auto tpp_fov_tan = std::tan(Config::FOV * deg2rad / 2.F);
        Screen::fFOVNewTPP      = frame_width / tpp_fov_tan / 2.F;
        Screen::fFOVNewShoulder = frame_width / (tpp_fov_tan * (Screen::fFOVDefaultTPP / Screen::fFOVDefaultShoulder)) / 2.F;
        Screen::fFOVNewHiding   = frame_width / (tpp_fov_tan * (Screen::fFOVDefaultTPP / Screen::fFOVDefaultHiding)) / 2.F;
        Screen::fFOVNewCQC      = frame_width / (tpp_fov_tan * (Screen::fFOVDefaultTPP / Screen::fFOVDefaultCQC)) / 2.F;

        std::uint8_t* UpdateFOVLerpCallSiteScanResult = Memory::PatternScan(ExeModule, "48 8B 8F ?? ?? ?? ?? 48 8B 01 FF 50 18 48 8D 4F E0 E8");
        if (!UpdateFOVLerpCallSiteScanResult) {
            LOG_INFO("{}: Graphics: FOV: Pattern scan failed.", CurrentGame->ShortName);
            return;
        }

        LOG_INFO("{}: Graphics: FOV: update_fov_lerp: Call site is {:s}+{:x}", CurrentGame->ShortName, CurrentGame->Exe, UpdateFOVLerpCallSiteScanResult - (std::uint8_t*)ExeModule);

        // Resolve the rel32 displacement at UpdateFOVLerpCallSiteScanResult+18 to find update_fov_lerp itself
        auto* rel32_ptr = reinterpret_cast<std::int32_t*>(UpdateFOVLerpCallSiteScanResult + 18);
        std::uint8_t* UpdateFovLerpAddress = reinterpret_cast<std::uint8_t*>(rel32_ptr) + 4 + *rel32_ptr;

        LOG_INFO("{}: Graphics: FOV: update_fov_lerp: Address is {:s}+{:x}", CurrentGame->ShortName, CurrentGame->Exe, UpdateFovLerpAddress - (std::uint8_t*)ExeModule);

        static const std::uintptr_t fov_offset = (CurrentGame->ShortName == "MGO") ? 0x2EC : 0x2FC;

        static SafetyHookMid FOVMidHook{};
        FOVMidHook = safetyhook::create_mid(UpdateFovLerpAddress, [](SafetyHookContext& ctx) {
            auto* target_fov = reinterpret_cast<float*>(ctx.rcx + fov_offset);

            if (*target_fov == Screen::fFOVDefaultTPP)
                *target_fov = Screen::fFOVNewTPP;
            else if (*target_fov == Screen::fFOVDefaultShoulder)
                *target_fov = Screen::fFOVNewShoulder;
            else if (*target_fov == Screen::fFOVDefaultHiding)
                *target_fov = Screen::fFOVNewHiding;
            else if (*target_fov == Screen::fFOVDefaultCQC)
                *target_fov = Screen::fFOVNewCQC;
        });
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

        FOV();
    }
}
