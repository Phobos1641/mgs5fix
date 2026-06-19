#pragma once

namespace Screen
{
    inline int ResX = 0;
    inline int ResY = 0;
    inline float AspectRatio = 16.0f / 9.0f;
    inline float AspectMultiplier = 1.0f;
    inline float HUDWidth = 1920.0f;
    inline float HUDHeight = 1080.0f;
    inline float HUDWidthOffset = 0.0f;
    inline float HUDHeightOffset = 0.0f;

    inline constexpr float NativeAspect = 16.0f / 9.0f;

    // FOV is in focal length of a 24mm x 36mm camera lens and is locked horizontally
    const auto fFOVDefaultTPP      = 21.F;
    const auto fFOVDefaultShoulder = 22.F;
    const auto fFOVDefaultHiding   = 26.F;
    const auto fFOVDefaultCQC      = 32.F;
    inline float fFOVNewTPP;
    inline float fFOVNewShoulder;
    inline float fFOVNewHiding;
    inline float fFOVNewCQC;

    void CalculateAspectRatio(int resX, int resY, bool log = true);
    std::pair<int, int> GetPhysicalDesktopDimensions();
}
