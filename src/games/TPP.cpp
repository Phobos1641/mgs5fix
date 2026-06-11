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
            // Unlock fullscreen resolutions
            if (auto* FullscreenResolutions = Memory::FindPattern(std::format("{}: Unlock Resolutions: Fullscreen", CurrentGame->ShortName).c_str(), "EB ?? F3 0F ?? ?? F3 48 ?? ?? ?? B8 ?? ?? ?? ??"))
                Memory::PatchBytes(FullscreenResolutions + 0x5, "\xD3"); // mulss xmm2, xmm0 -> mulss xmm2, xmm3 to multiply by the actual aspect ratio
        }
    }

    static void AspectRatio()
    {
        if (Config::FixAspect) {
            // Fix depth of field strength
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

            // Increase bounds at which markers are constrained horizontally
            MAKE_MIDHOOK(MarkerBoundsHor_sh, std::format("{}: HUD: Marker Bounds (Horizontal)", CurrentGame->ShortName).c_str(), "F3 0F ?? ?? ?? ?? ?? ?? 0F ?? ?? 77 ?? F3 0F ?? ?? ?? ?? ?? ?? 0F ?? ?? 73 ?? 0F ?? ?? E8 ?? ?? ?? ??", 0, [](SafetyHookContext& ctx) {
                if (Screen::AspectRatio > Screen::NativeAspect) {
                    const float bounds = 51.5f * Screen::AspectMultiplier;
                    ctx.xmm7.f32[0] = std::clamp(ctx.xmm7.f32[0], -bounds, bounds);
                    ctx.rip = MarkerBoundsHor_sh.target_address() + 0x1D;
                }
            });

            // Fix various overlays
            auto overlayHook = [](SafetyHookContext& ctx) {
                if (Screen::AspectRatio > Screen::NativeAspect)
                    ctx.xmm5.f32[0] *= Screen::AspectMultiplier;
            };

            MAKE_MIDHOOK(Overlays1_sh, std::format("{}: HUD: Overlays: 1", CurrentGame->ShortName).c_str(), "F3 0F ?? ?? ?? ?? ?? ?? C7 44 ?? ?? 00 00 80 BF C7 44 ?? ?? 00 00 80 3F", 0, overlayHook);
            MAKE_MIDHOOK(Overlays2_sh, std::format("{}: HUD: Overlays: 2", CurrentGame->ShortName).c_str(), "F3 0F ?? ?? ?? ?? ?? ?? C7 44 ?? ?? 00 00 80 BF C7 44 ?? ?? 00 00 80 3F", 0, overlayHook);
            MAKE_MIDHOOK(Overlays3_sh, std::format("{}: HUD: Overlays: 3", CurrentGame->ShortName).c_str(), "F3 0F ?? ?? ?? ?? ?? ?? C7 44 ?? ?? 00 00 80 BF C7 44 ?? ?? 00 00 80 3F", 0, overlayHook);

            // Fix sonar markers
            MAKE_MIDHOOK(SonarMarkers_sh, std::format("{}: HUD: Sonar Markers", CurrentGame->ShortName).c_str(), "F3 0F ?? ?? ?? ?? ?? ?? F3 0F ?? ?? ?? ?? ?? ?? F3 0F ?? ?? F3 0F ?? ?? ?? 48 83 ?? ??", 0, [](SafetyHookContext& ctx) {
                if (Screen::AspectRatio > Screen::NativeAspect)
                    ctx.xmm0.f32[0] *= Screen::AspectMultiplier;
            });

            // Get movie playback status
            MAKE_MIDHOOK(MovieStatus_sh, std::format("{}: HUD: Movie Status", CurrentGame->ShortName).c_str(), "8B ?? ?? ?? ?? ?? FF ?? 0F 84 ?? ?? ?? ?? FF ?? 0F 84 ?? ?? ?? ?? FF ?? 74 ?? 48 8D ?? ?? ?? ?? ?? 33 ??", 0, [](SafetyHookContext& ctx) {
                IsMoviePlaying = (ctx.rax == 1 || ctx.rax == 2);
            });

            // Scale viewport size when movies are playing
            MAKE_MIDHOOK(Viewport_sh, std::format("{}: HUD: Viewport", CurrentGame->ShortName).c_str(), "F3 0F ?? ?? F3 0F ?? ?? 0F ?? ?? 73 ?? 41 0F ?? ?? 41 ?? ?? 44 ?? ?? F3 0F ?? ?? F3 0F ?? ??", 0, [](SafetyHookContext& ctx) {
                if (IsMoviePlaying && Screen::AspectRatio > Screen::NativeAspect)
                    ctx.xmm1.f32[0] = Screen::HUDWidth;
            });
        }
    }

    static void Framerate()
    {
        if (Config::UnlockFPS) {
            // Fix loading softlock at >200fps
            MAKE_MIDHOOK(LoadingSoftlock_sh, std::format("{}: Framerate: Loading Softlock", CurrentGame->ShortName).c_str(), "F3 0F ?? ?? ?? ?? ?? ?? 48 85 ?? 74 ?? 48 8B ?? ?? ?? ?? ?? 48 85 ??", 0x8, [](SafetyHookContext& ctx) {
                // Force the first frametime stored to be 60fps (this should only fire once per object)
                if (ctx.rdi && *reinterpret_cast<uint64_t*>(ctx.rdi + 0xE8) == 0) {
                    constexpr float ft = 1.0f / 60.0f;
                    if (ctx.xmm6.f32[0] < ft)
                        ctx.xmm6.f32[0] = ft;
                }
            });
        }
    }

    static void Graphics()
    {
        if (Config::LODTweaks) {
            // Adjust LOD factor resolution
            if (auto* LODFactorResolution = Memory::FindPattern(std::format("{}: LOD: LOD Factor Resolution", CurrentGame->ShortName).c_str(), "8B ?? ?? ?? ?? ?? 4C 8B ?? ?? ?? ?? ?? 85 ?? 75 ?? 8B ?? ?? ?? ?? ?? F3 0F ?? ?? ?? ?? ?? ?? F3 0F ?? ?? ?? ?? ?? ??"))
                Memory::Write(Memory::GetRelativeAddr(LODFactorResolution + 0x2), Config::TerrainDistance);
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
