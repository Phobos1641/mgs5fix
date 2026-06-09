#include "stdafx.h"
#include "screen.h"

namespace Screen 
{
    void CalculateAspectRatio(int resX, int resY, bool log)
    {
        if (resX <= 0 || resY <= 0) return;
        if (ResX == resX && ResY == resY) return;

        ResX = resX;
        ResY = resY;

        const float x = static_cast<float>(ResX);
        const float y = static_cast<float>(ResY);

        AspectRatio = x / y;
        AspectMultiplier = AspectRatio / NativeAspect;

        if (AspectRatio >= NativeAspect) {
            HUDWidth = y * NativeAspect;
            HUDHeight = y;
            HUDWidthOffset = (x - HUDWidth) / 2.0f;
            HUDHeightOffset = 0.0f;
        } 
        else {
            HUDWidth = x;
            HUDHeight = x / NativeAspect;
            HUDWidthOffset = 0.0f;
            HUDHeightOffset = (y - HUDHeight) / 2.0f;
        }

        if (log) {
            LOG_INFO("Current Resolution: {}x{} | Aspect Ratio: {:.5f} ({:.5f}) | HUD: {:.2f}x{:.2f} ({:.2f}, {:.2f})", 
                ResX, ResY, 
                AspectRatio, AspectMultiplier, 
                HUDWidth, HUDHeight, 
                HUDWidthOffset, HUDHeightOffset);
            LOG_INFO("----------");
        }
    }

    std::pair<int, int> GetPhysicalDesktopDimensions() 
    {
        if (DEVMODE devMode{ .dmSize = sizeof(DEVMODE) }; EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &devMode))
            return { devMode.dmPelsWidth, devMode.dmPelsHeight };

        return {};
    }
}