#pragma once

#include <ini.h>

namespace Config
{
    inline bool IntroSkip;
    inline bool UnlockFPS;

    inline bool FixResolution;
    inline bool FixAspect;
    inline bool FixHUD;

    inline bool LODTweaks;
    inline int TerrainDistance;
    inline int ModelDistance;
    inline int GrassDistance;

    inline bool ChangeFOV;
    inline float FOV;

    inline mINI::INIStructure ini;

    template <typename T>
    inline T ParseConfig(const std::string& section, const std::string& key, T defaultValue)
    {
        if (!ini.has(section) || !ini[section].has(key)) {
            LOG_INFO("Config Parse: [{}]: {} not found, using default: {}", section, key, defaultValue);
            return defaultValue;
        }

        std::string raw = ini[section][key];
        T value;

        if constexpr (std::is_same_v<T, bool>) {
            std::transform(raw.begin(), raw.end(), raw.begin(), ::tolower);
            value = (raw == "true" || raw == "1");
        }
        else if constexpr (std::is_same_v<T, int> || std::is_same_v<T, float>) {
            if (std::from_chars(raw.data(), raw.data() + raw.size(), value).ec != std::errc{}) {
                LOG_WARN("Config Parse: [{}]: {}: \"{}\" is not a valid value, using default: {}", section, key, raw, defaultValue);
                return defaultValue;
            }
        }
        else if constexpr (std::is_same_v<T, std::string>) { value = raw; }
        else { static_assert(false, "ParseConfig: unsupported type"); }

        LOG_INFO("Config Parse: [{}]: {} = {}", section, key, value);
        return value;
    }

    void Init();
    int ParseConfig(const std::string& section, const std::string& key, int defaultValue, int min, int max);
    float ParseConfig(const std::string& section, const std::string& key, float defaultValue, float min, float max);
}
