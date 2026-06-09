#include "stdafx.h"
#include "memory.h"
#include "globals.h"

namespace Memory
{
    std::uint8_t* PatternScan(void* module, const char* signature, bool scanAll)
    {
        // https://github.com/0x1F9F1/mem/blob/b0e2652ff01c9364f69324592ecc7d62b532486f/include/mem/simd_scanner.h#L143-L168
        static constexpr uint8_t frequencies[256] = {
            0xFF,0xFB,0xF2,0xEE,0xEC,0xE7,0xDC,0xC8,0xED,0xB7,0xCC,0xC0,0xD3,0xCD,0x89,0xFA,
            0xF3,0xD6,0x8D,0x83,0xC1,0xAA,0x7A,0x72,0xC6,0x60,0x3E,0x2E,0x98,0x69,0x39,0x7C,
            0xEB,0x76,0x24,0x34,0xF9,0x50,0x04,0x07,0xE5,0xAC,0x53,0x65,0x9B,0x4D,0x6D,0x5C,
            0xDA,0x93,0x7F,0xCB,0x92,0x49,0x43,0x09,0xBA,0x8E,0x1E,0x91,0x8A,0x5B,0x11,0xA1,
            0xE8,0xF5,0x9E,0xAD,0xEF,0xE6,0x79,0x7B,0xFE,0xE0,0x1F,0x54,0xE4,0xBD,0x7D,0x6A,
            0xDF,0x67,0x7E,0xA4,0xB6,0xAF,0x88,0xA0,0xC3,0xA9,0x26,0x77,0xD1,0x71,0x61,0xC2,
            0x9A,0xCA,0x29,0x9F,0xD8,0xE2,0xD0,0x6E,0xB4,0xB8,0x25,0x3C,0xBF,0x73,0xB5,0xCF,
            0xD4,0x01,0xCE,0xBE,0xF1,0xDB,0x52,0x37,0x9D,0x63,0x02,0x6B,0x80,0x45,0x2B,0x95,
            0xE1,0xC4,0x36,0xF0,0xD5,0xE3,0x57,0x9C,0xB1,0xF7,0x82,0xFC,0x42,0xF6,0x18,0x33,
            0xD2,0x48,0x05,0x0F,0x41,0x1D,0x03,0x27,0x70,0x10,0x00,0x08,0x55,0x16,0x2F,0x0E,
            0x94,0x35,0x2C,0x40,0x6F,0x3B,0x1C,0x28,0x90,0x68,0x81,0x4B,0x56,0x30,0x2A,0x3D,
            0x97,0x17,0x06,0x13,0x32,0x0B,0x5A,0x75,0xA5,0x86,0x78,0x4F,0x2D,0x51,0x46,0x5F,
            0xE9,0xDE,0xA2,0xDD,0xC9,0x4C,0xAB,0xBB,0xC7,0xB9,0x74,0x8F,0xF8,0x6C,0x85,0x8B,
            0xC5,0x84,0x8C,0x66,0x21,0x23,0x64,0x59,0xA3,0x87,0x44,0x58,0x3A,0x0D,0x12,0x19,
            0xAE,0x5E,0x3F,0x38,0x31,0x22,0x0A,0x14,0xF4,0xD9,0x20,0xB0,0xB2,0x1A,0x0C,0x15,
            0xB3,0x47,0x5D,0xEA,0x4A,0x1B,0x99,0xBC,0xD7,0xA6,0x62,0x4E,0xA8,0x96,0xA7,0xFD,
        };

        std::vector<int> patternBytes;
        const char* current = signature;
        const char* end = signature + strlen(signature);

        for (; current < end; ++current) {
            if (*current == '?') {
                ++current;
                if (*current == '?')
                    ++current;
                patternBytes.push_back(-1);
            }
            else {
                patternBytes.push_back(strtoul(current, const_cast<char**>(&current), 16));
            }
        }

        const size_t s = patternBytes.size();
        const int* d = patternBytes.data();

        size_t anchorIdx = SIZE_MAX;
        size_t minFreq = SIZE_MAX;

        for (size_t i = 0; i < s; ++i) {
            if (d[i] == -1) continue;
            size_t freq = frequencies[static_cast<uint8_t>(d[i])];
            if (freq < minFreq) {
                minFreq = freq;
                anchorIdx = i;
            }
        }

        if (anchorIdx == SIZE_MAX)
            return nullptr;

        auto dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(module);
        auto ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<std::uint8_t*>(module) + dosHeader->e_lfanew);
        auto scanBytes = reinterpret_cast<std::uint8_t*>(module);

        const uint8_t anchor = static_cast<uint8_t>(d[anchorIdx]);

        auto section = IMAGE_FIRST_SECTION(ntHeaders);
        for (int si = 0; si < ntHeaders->FileHeader.NumberOfSections; ++si, ++section) {
            if (!scanAll && !(section->Characteristics & IMAGE_SCN_CNT_CODE))
                continue;

            auto sectionStart = scanBytes + section->VirtualAddress;
            size_t sectionSize = section->Misc.VirtualSize;

            if (sectionSize < s) continue;

            const size_t scanEnd = sectionSize - s;
            const size_t simdEnd = (scanEnd > anchorIdx) ? (scanEnd - anchorIdx) : 0;

            size_t i = 0;

            const __m128i needle = _mm_set1_epi8(static_cast<char>(anchor));
            constexpr size_t CHUNK = 16;

            for (; i + CHUNK <= simdEnd; i += CHUNK) {
                __m128i haystack = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&sectionStart[i + anchorIdx]));
                __m128i cmp = _mm_cmpeq_epi8(haystack, needle);
                uint32_t mask = static_cast<uint32_t>(_mm_movemask_epi8(cmp));

                while (mask != 0) {
                    uint32_t bit = _tzcnt_u32(mask);
                    size_t matchPos = i + bit;

                    bool found = true;
                    for (size_t j = 0; j < s; ++j) {
                        if (d[j] != -1 && sectionStart[matchPos + j] != static_cast<uint8_t>(d[j])) {
                            found = false;
                            break;
                        }
                    }

                    if (found)
                        return &sectionStart[matchPos];

                    mask &= mask - 1;
                }
            }

            for (; i < scanEnd; ++i) {
                if (sectionStart[i + anchorIdx] != anchor) continue;

                bool found = true;
                for (size_t j = 0; j < s; ++j) {
                    if (d[j] != -1 && sectionStart[i + j] != static_cast<uint8_t>(d[j])) {
                        found = false;
                        break;
                    }
                }

                if (found)
                    return &sectionStart[i];
            }
        }

        return nullptr;
    }

    std::uint8_t* FindPattern(const char* label, HMODULE module, const char* moduleName, const char* pattern, bool scanAll)
    {
        if (!module) return nullptr;

        std::uint8_t* addr = Memory::PatternScan(module, pattern, scanAll);
        if (!addr) {
            LOG_ERROR("{}: Pattern scan failed. Pattern: {}", label, pattern);
            return nullptr;
        }

        LOG_INFO("{}: Address is {:s}+{:x}", label, moduleName, addr - reinterpret_cast<std::uint8_t*>(module));
            
        return addr;
    }

    std::uint8_t* GetRelativeAddr(std::uint8_t* address)
    {
        if (!address) return nullptr;

        std::int32_t offset = *reinterpret_cast<std::int32_t*>(address);
        return address + sizeof(offset) + offset;
    }
}

namespace Hook
{
    BOOL HookIAT(HMODULE callerModule, char const* targetModule, const void* targetFunction, void* detourFunction)
    {
        auto* base = reinterpret_cast<std::uint8_t*>(callerModule);
        const auto* dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
        const auto* ntHeaders = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dosHeader->e_lfanew);
        const auto* imports = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(base + ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

        for (int i = 0; imports[i].Characteristics; i++)
        {
            const char* name = reinterpret_cast<const char*>(base + imports[i].Name);
            if (lstrcmpiA(name, targetModule) != 0)
                continue;

            void** thunk = reinterpret_cast<void**>(base + imports[i].FirstThunk);

            for (; *thunk; thunk++)
            {
                const void* import = *thunk;

                if (import != targetFunction)
                    continue;

                DWORD oldState;
                if (!VirtualProtect(thunk, sizeof(void*), PAGE_READWRITE, &oldState))
                    return false;

                *thunk = detourFunction;

                VirtualProtect(thunk, sizeof(void*), oldState, &oldState);

                return true;
            }
        }
        return false;
    }
}
