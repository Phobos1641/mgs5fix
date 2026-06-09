#include "stdafx.h"
#include "common.h"
#include "globals.h"
#include "config.h"
#include "screen.h"
#include "memory.h"

#include "TPP.h"

// METAL GEAR SOLID V: THE PHANTOM PAIN

namespace TPP
{
    static void Resolution() 
    {
        if (Config::FixResolution) {
            // Unlock fullscreen/borderless resolutions
            if (auto* FullscreenResolutions = Memory::FindPattern(std::format("{}: Unlock Resolutions: Fullscreen/Borderless", CurrentGame->ShortName).c_str(), "EB ?? F3 0F ?? ?? F3 48 ?? ?? ?? B8 ?? ?? ?? ??"))
                Memory::PatchBytes(FullscreenResolutions + 0x5, "\xD3"); // mulss xmm2, xmm0 -> mulss xmm2, xmm3 to multiply by the actual aspect ratio
        }
    }

    static void AspectRatio()
    {
        if (Config::FixAspect) {
            MAKE_MIDHOOK(DepthOfField_sh, std::format("{}: Depth of Field", CurrentGame->ShortName).c_str(), "F3 0F ?? ?? ?? ?? ?? ?? 0F ?? ?? 0F ?? ?? 0F ?? ?? 0F ?? ?? ?? 0F ?? ?? ?? 0F ?? ?? ?? 44 0F ?? ??", 0, [](SafetyHookContext& ctx) {
                if (Screen::AspectRatio > Screen::NativeAspect) {
                    ctx.xmm6.f32[0] = (Screen::HUDWidth * 0.85f) * (1.0f / 1920.0f);
                    ctx.xmm4.f32[0] = Screen::HUDWidth;
                }
            });
        }
    }

    static void HUD()
    {
        static bool IsMoviePlaying = false;

        if (Config::FixHUD) {
            // Span backgrounds
            MAKE_MIDHOOK(HUDBackgrounds_sh, std::format("{}: HUD: Backgrounds", CurrentGame->ShortName).c_str(), "F6 41 ?? 01 74 ?? 0F ?? ?? ?? 0F ?? ?? ?? 44 0F ?? ?? ?? 41 ?? ?? ??", 0, [](SafetyHookContext& ctx) {
                const float width = *reinterpret_cast<const float*>(ctx.rcx + 0x40);
                const float height = *reinterpret_cast<const float*>(ctx.rcx + 0x44);

                // Resize HUD to counteract viewport scaling when a movie plays
                if (IsMoviePlaying) {
                    if (Screen::AspectRatio > Screen::NativeAspect && width > 1.0f)
                        ctx.xmm0.f32[0] *= Screen::AspectMultiplier;
                }

                if (Screen::AspectRatio > Screen::NativeAspect) {
                    // TPP/GZ: ui_sys_cmn_bg
                    if (width == 2048.0f && height == 1152.0f)
                        ctx.xmm0.f32[0] *= Screen::AspectMultiplier;

                    // TPP/GZ: Cutscene skip BG
                    if (width == 2000.0f && height == 1125.0f)
                        ctx.xmm0.f32[0] *= Screen::AspectMultiplier;

                    // TPP: Loadout BG
                    if (width == 1400.0f && height == 1400.0f)
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

                    // TPP: Scope frame
                    if (width == 1500.0f && height == 1000.0f)
                        *reinterpret_cast<float*>(ctx.rcx + 0x740) = Screen::AspectRatio / 2.0f; // Set the width scale
                }
                else {
                    // TPP: Scope frame
                    if (width == 1500.0f && height == 1000.0f)
                        *reinterpret_cast<float*>(ctx.rcx + 0x740) = 1.0f; // Reset in-case the resolution has changed.
                }
            });

            // Fix incorrectly positioned markers
            MAKE_MIDHOOK(Markers_sh, std::format("{}: HUD: Markers", CurrentGame->ShortName).c_str(), "48 81 ?? ?? ?? ?? ?? E9 ?? ?? ?? ?? 48 8B ?? ?? ?? ?? ?? F3 0F ?? ?? ?? ?? ?? ?? F3 0F ?? ?? ?? ?? ?? ??", 0, [](SafetyHookContext& ctx) {
                if (Screen::AspectRatio > Screen::NativeAspect) {
                    *reinterpret_cast<float*>(ctx.rdx + 0x120) = 64.0f * Screen::AspectMultiplier;
                    ctx.xmm1.f32[0] = 64.0f;
                }
                else {
                    *reinterpret_cast<float*>(ctx.rdx + 0x120) = 64.0f;
                    ctx.xmm1.f32[0] = 64.0f;
                }
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
        
        HUD();
    }
}
