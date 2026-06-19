#include "stdafx.h"
#include "resources/version.h"
#include "globals.h"
#include "config.h"

namespace Config
{
    void Init()
    {
        std::string configPath = (FixPath / (FixName + ".ini")).string();
        mINI::INIFile file(configPath);

        if (!file.read(ini)) {
            const auto err = GetLastError();
            AllocConsole();
            FILE* dummy = nullptr;
            freopen_s(&dummy, "CONOUT$", "w", stderr);
            std::cerr << "Fatal Error: " << FixName << ": Could not locate config file.\nMake sure " << FixName << ".ini is located in " << FixPath.string() << "\nReason: " << std::system_category().message(err) << std::endl;
            LOG_ERROR("Could not locate config file. Make sure {}.ini is located in {}. Reason: {}", FixName, FixPath.string(), std::system_category().message(err));            
            FreeLibraryAndExitThread(ThisModule, 1);
        }

        LOG_INFO("Config File: {}", configPath);

        IntroSkip       = ParseConfig("Intro Skip", "Enabled", false);
        UnlockFPS       = ParseConfig("Unlock Framerate", "Enabled", false);

        FixResolution   = ParseConfig("Fix Resolution", "Enabled", false);
        FixAspect       = ParseConfig("Fix Aspect Ratio", "Enabled", false);
        FixHUD          = ParseConfig("Fix HUD", "Enabled", false);

        LODTweaks       = ParseConfig("LOD Tweaks", "Enabled", false);
        if (LODTweaks) {
            TerrainDistance = ParseConfig("LOD Tweaks", "TerrainDistance", 8192);
            ModelDistance   = ParseConfig("LOD Tweaks", "ModelDistance", 512);
            GrassDistance   = ParseConfig("LOD Tweaks", "GrassDistance", 1000);
        }

        LOG_INFO("----------");
    }

    int ParseConfig(const std::string& section, const std::string& key, int defaultValue, int min, int max)
    {
        int value = ParseConfig(section, key, defaultValue);
        if (value < min || value > max) {
            int clamped = std::clamp(value, min, max);
            LOG_WARN("Config Parse: [{}]: {}: {} is out of range. Clamped to: {}", section, key, value, clamped);
            return clamped;
        }
        return value;
    }

    float ParseConfig(const std::string& section, const std::string& key, float defaultValue, float min, float max)
    {
        float value = ParseConfig(section, key, defaultValue);
        if (value < min || value > max) {
            float clamped = std::clamp(value, min, max);
            LOG_WARN("Config Parse: [{}]: {}: {} is out of range. Clamped to: {}", section, key, value, clamped);
            return clamped;
        }
        return value;
    }
}