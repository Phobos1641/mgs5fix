#include "stdafx.h"
#include "common.h"
#include "globals.h"
#include "config.h"
#include "screen.h"
#include "memory.h"

#include "GZ.h"

// METAL GEAR SOLID V: GROUND ZEROES

namespace GZ
{
    static void Resolution() 
    {
        if (Config::FixResolution) {
            // Unlock fullscreen/borderless resolutions
            if (auto* FullscreenResolutions = Memory::FindPattern(std::format("{}: Unlock Resolutions: Fullscreen/Borderless", CurrentGame->ShortName).c_str(), "F3 0F ?? ?? F3 48 ?? ?? ?? 8B ?? 41 ?? ?? ?? ?? ?? ?? 0F ?? ?? 44 ?? ?? 41 ?? ?? ?? 41 ?? ?? 33 ??"))
                Memory::PatchBytes(FullscreenResolutions + 0x3, "\xD1"); // divss xmm2, xmm0 -> divss xmm2, xmm1 to divide by the actual aspect ratio

            // Remove HWND_TOPMOST flag for borderless mode
            MAKE_MIDHOOK(BorderlessTopMost_sh, std::format("{}: Borderless TopMost", CurrentGame->ShortName).c_str(), "C7 44 ?? ?? ?? ?? ?? ?? 89 ?? ?? ?? 8B ?? ?? 89 ?? ?? ?? FF ?? ?? ?? ?? ?? E9 ?? ?? ?? ??", 0x8, [](SafetyHookContext& ctx) {
                if (ctx.rdx == reinterpret_cast<uintptr_t>(HWND_TOPMOST))
                    ctx.rdx = reinterpret_cast<uintptr_t>(HWND_NOTOPMOST);
            });
        }
    }

    static void AspectRatio()
    {
        if (Config::FixAspect) {
            // Fix depth of field strength
            MAKE_MIDHOOK(DepthOfField_sh, std::format("{}: Depth of Field", CurrentGame->ShortName).c_str(), "F3 0F ?? ?? ?? ?? ?? ?? 0F ?? ?? 0F ?? ?? ?? ?? ?? ?? 44 0F ?? ?? F3 44 ?? ?? ??", 0, [](SafetyHookContext& ctx) {
                if (Screen::AspectRatio > Screen::NativeAspect) {
                    ctx.xmm6.f32[0] = (Screen::HUDWidth * 0.85f) * (1.0f / 1920.0f);
                    ctx.xmm4.f32[0] = Screen::HUDWidth;
                }
            });
        }
    }

    static void HUD()
    {
        if (Config::FixHUD) {
            // Span backgrounds
            MAKE_MIDHOOK(HUDBackgrounds_sh, std::format("{}: HUD: Backgrounds", CurrentGame->ShortName).c_str(), "41 0F ?? ?? 8B ?? ?? F6 ?? ?? 0F 84 ?? ?? ?? ?? 44 ?? ?? 41 ?? ?? ?? 41 ?? ?? ?? 74 ??", 0, [](SafetyHookContext& ctx) {
                const float width = *reinterpret_cast<const float*>(ctx.rcx + 0x30);
                const float height = *reinterpret_cast<const float*>(ctx.rcx + 0x34);

                if (Screen::AspectRatio > Screen::NativeAspect) {
                    // TPP/GZ: ui_sys_cmn_bg
                    if (width == 2048.0f && height == 1152.0f)
                        ctx.xmm0.f32[0] *= Screen::AspectMultiplier;

                    // TPP/GZ: Cutscene skip BG
                    if (width == 2000.0f && height == 1125.0f)
                        ctx.xmm0.f32[0] *= Screen::AspectMultiplier;

                    // TPP/GZ: Mission failed BGs
                    if (width == 1500.0f && height == 1500.0f)
                        ctx.xmm0.f32[0] *= Screen::AspectMultiplier;
                    if (width == 2000.0f && height == 2000.0f)
                        ctx.xmm0.f32[0] *= Screen::AspectMultiplier;
                    if ((width > 1882.0f && width < 1884.0f) && (height > 1059.0f && height < 1061.0f))
                        ctx.xmm0.f32[0] *= Screen::AspectMultiplier;
                    if ((width > 1770.0f && width < 1772.0f) && (height > 995.0f && height < 997.0f))
                        ctx.xmm0.f32[0] *= Screen::AspectMultiplier;

                    // TPP/GZ: Scope fade
                    if (width == 1400.0f && height == 1280.0f)
                        ctx.xmm0.f32[0] *= Screen::AspectMultiplier;

                    // GZ: Scope frame
                    if (width == 600.0f && (height > 1230.0f && height < 1231.0f))
                        ctx.xmm0.f32[0] *= Screen::AspectMultiplier;
                }
            });
        }
    }

    static void Framerate()
    {
        if (Config::UnlockFPS) {
            // Fix freezing bug with throwables when using variable framerate
            if (auto* ThrowableFreezeBug = Memory::FindPattern(std::format("{}: Framerate: Throwable Freeze Bug", CurrentGame->ShortName).c_str(), "F2 0F 59 ?? ?? ?? ?? ?? 66 0F ?? ?? F7 ?? ?? ?? ?? ?? 00 01 00 00 74 ??"))
                Memory::PatchBytes(ThrowableFreezeBug, "\xF2\x0F\x59\x40\x30\x90\x90\x90"); // mulsd xmm0,[7FF677CA9C00] (fixed 60fps frametime) -> mulsd xmm0, [rax+30] (current frametime)
        
            // Force controller input to be read per-frame
            MAKE_MIDHOOK(ControllerInput_sh, std::format("{}: Framerate: Controller Input", CurrentGame->ShortName).c_str(), "44 89 ?? ?? ?? ?? ?? 4C 8B ?? ?? ?? 48 8B ?? ?? 48 33 ?? E8 ?? ?? ?? ??", 0, [](SafetyHookContext& ctx) {
                if (ctx.r15 == 0) ctx.r15 = 1;
            });
        }
    }

    static void Graphics()
    {
        if (Config::LODTweaks) {
            // Adjust LOD factor resolution
            MAKE_MIDHOOK(LODFactorResolution_sh, std::format("{}: LOD: LOD Factor Resolution", CurrentGame->ShortName).c_str(), "66 0F ?? ?? ?? ?? ?? ?? 0F 29 ?? ?? 0F 28 ?? F3 0F ?? ?? ?? ?? ?? ?? 0F 5B ??", 0x8, [](SafetyHookContext& ctx) {
                ctx.xmm3.u16[0] = Config::TerrainDistance;
            });
        }
    }

    void Init()
    {
        Common::CurrentResolution();

        Common::Resolution();
        Resolution();

        Common::AspectRatio();
        AspectRatio();

        Common::HUD();
        HUD();

        Common::Framerate();
        Framerate();

        Common::Graphics();
        Graphics();
    }
}
