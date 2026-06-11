local name = "MGSVFix"

set_project(name)
add_rules("mode.debug", "mode.release")
set_languages("cxxlatest", "clatest")

-- ===================== Zycore =====================

target("zycore")
    on_load(function (target)
        local zycore_path = "external/zydis/dependencies/zycore/CMakeLists.txt"
        if not os.isfile(zycore_path) then
            print("[Zycore] Submodule missing or incomplete, fetching...")
            os.exec("git submodule update --init --recursive")
            if not os.isfile(zycore_path) then
                raise("[Zycore] Failed to fetch Zycore submodule")
            end
        end
    end)

    set_kind("static")
    add_defines("ZYCORE_STATIC_BUILD", {public = true})

    add_includedirs("external/zydis/dependencies/zycore/include", {public = true})
    add_files("external/zydis/dependencies/zycore/src/*.c")

-- ===================== Zydis =====================

target("zydis")
    add_deps("zycore")
    set_kind("static")
    add_defines("ZYDIS_STATIC_BUILD", {public = true})

    add_includedirs("external/zydis/include", "external/zydis/src", {public = true})
    add_files("external/zydis/src/*.c")

-- ===================== ASI | safetyhook | mINI =====================

target(name)
    set_kind("shared")
    set_prefixname("")
    set_extension(".asi")

    add_includedirs("src", "src/resources", "external/safetyhook/include", "external/mINI/src/mini")
    add_headerfiles("src/**.h")
    add_files("src/**.cpp", "src/resources/version.rc", "external/safetyhook/src/**.cpp")

    add_deps("zydis")
    add_syslinks("user32")

    if is_plat("windows") then
        set_toolchains("msvc")
        if is_mode("release") then
            set_optimize("fastest")
            set_strip("all")
            set_runtimes("MT")
            add_cxflags("/utf-8", "/GL")
            add_ldflags("/LTCG", "/OPT:REF", "/OPT:ICF")
        else
            set_strip("none")
            set_runtimes("MDd")
            add_cxflags("/utf-8", "/Zi")
            add_defines("_DEBUG")
        end
    end