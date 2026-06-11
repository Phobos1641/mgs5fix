#pragma once

#include "globals.h"

namespace Memory
{
    template<typename T>
    void Write(std::uint8_t* writeAddress, T value)
    {
        DWORD oldProtect;
        VirtualProtect((LPVOID)(writeAddress), sizeof(T), PAGE_EXECUTE_READWRITE, &oldProtect);
        *(reinterpret_cast<T*>(writeAddress)) = value;
        VirtualProtect((LPVOID)(writeAddress), sizeof(T), oldProtect, &oldProtect);
    }

    template <size_t N>
    void PatchBytes(std::uint8_t* address, const char (&pattern)[N])
    {
        DWORD oldProtect;
        VirtualProtect(address, N - 1, PAGE_EXECUTE_READWRITE, &oldProtect);
        memcpy(address, pattern, N - 1);
        VirtualProtect(address, N - 1, oldProtect, &oldProtect);
    }

    std::uint8_t* PatternScan(void* module, const char* signature, bool scanAll = false);
    std::uint8_t* FindPattern(const char* label, HMODULE module, const char* moduleName, const char* pattern, bool scanAll = false);
    std::uint8_t* GetRelativeAddr(std::uint8_t* address);

    inline std::uint8_t* FindPattern(const char* label, const char* pattern, bool scanAll = false) 
    {
        return FindPattern(label, ExeModule, ExeName.c_str(), pattern, scanAll);
    }
}

namespace Hook
{
    template <typename F>
    bool CreateMidHook(SafetyHookMid& hook, const char* label, HMODULE module, const char* moduleName, const char* pattern, ptrdiff_t offset, F&& callback)
    {
        std::uint8_t* addr = Memory::FindPattern(label, module, moduleName, pattern);
        if (!addr) return false;

        hook = safetyhook::create_mid(addr + offset, std::forward<F>(callback));
        return true;
    }

    template <typename F>
    bool CreateMidHook(SafetyHookMid& hook, const char* label, const char* pattern, ptrdiff_t offset, F&& callback)
    {
        return CreateMidHook(hook, label, ExeModule, ExeName.c_str(), pattern, offset, std::forward<F>(callback));
    }

    template <typename F>
    bool CreateInlineHook(SafetyHookInline& hook, const char* label, HMODULE module, const char* moduleName, const char* pattern, ptrdiff_t offset, F&& detour)
    {
        std::uint8_t* addr = Memory::FindPattern(label, module, moduleName, pattern);
        if (!addr) return false;
        
        hook = safetyhook::create_inline(addr + offset, std::forward<F>(detour));
        return true;
    }

    template <typename F>
    bool CreateInlineHook(SafetyHookInline& hook, const char* label, const char* pattern, ptrdiff_t offset, F&& detour)
    {
        return CreateInlineHook(hook, label, ExeModule, ExeName.c_str(), pattern, offset, std::forward<F>(detour));
    }

    #define MAKE_MIDHOOK(hook_name, label, pattern, offset, lambda) \
        static SafetyHookMid hook_name{}; \
        Hook::CreateMidHook(hook_name, label, pattern, offset, lambda);

    #define MAKE_MIDHOOK_DLL(hook_name, label, module, moduleName, pattern, offset, lambda) \
        static SafetyHookMid hook_name{}; \
        Hook::CreateMidHook(hook_name, label, module, moduleName, pattern, offset, lambda);

    #define MAKE_INLINEHOOK(hook_name, label, pattern, offset, detour) \
        static SafetyHookInline hook_name{}; \
        Hook::CreateInlineHook(hook_name, label, pattern, offset, detour);

    #define MAKE_INLINEHOOK_DLL(hook_name, label, module, moduleName, pattern, offset, detour) \
        static SafetyHookInline hook_name{}; \
        Hook::CreateInlineHook(hook_name, label, module, moduleName, pattern, offset, detour);
    
    BOOL HookIAT(HMODULE callerModule, char const* targetModule, const void* targetFunction, void* detourFunction);
}
