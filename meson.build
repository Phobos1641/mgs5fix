project('MGSVFix', ['c', 'cpp'],
  version: '0.0.2',
  meson_version: '>= 1.1.0',
  default_options: [
    'buildtype=release',
    'optimization=2',
    'b_ndebug=if-release',
    'strip=true',
    'warning_level=1',
    # NOTE: b_lto is deliberately NOT enabled. Using -flto triggers an internal compiler
    #       error in the GCC 13 mingw-w64 toolchain shipped on the Ubuntu 24.04 runner.
    #       Enable only with a newer GCC (14+) or with llvm-mingw: -Db_lto=true
  ],
)

cc  = meson.get_compiler('c')
cpp = meson.get_compiler('cpp')

if cpp.get_argument_syntax() == 'msvc'
  add_project_arguments('/std:c++latest', language: 'cpp')
  add_project_arguments('/std:clatest', language: 'c')
  add_project_arguments('/utf-8', language: ['c', 'cpp'])
  add_project_arguments('/DUNICODE', '/D_UNICODE', '/D_WIN32_WINNT=0x0601', language: ['c', 'cpp'])
  add_project_arguments('/DZYCORE_STATIC_BUILD', '/DZYDIS_STATIC_BUILD', language: ['c', 'cpp'])
  add_project_link_arguments('/OPT:REF', '/OPT:ICF', '/MT', '/GL', '/LTCG', language: ['c', 'cpp'])
else
  add_project_arguments('-std=c++23', language: 'cpp')
  add_project_arguments('-std=gnu2x', '-DUNICODE', '-D_UNICODE', '-D_WIN32_WINNT=0x0601', language: 'c')
  add_project_arguments('-DUNICODE', '-D_UNICODE', '-D_WIN32_WINNT=0x0601', language: ['c', 'cpp'])
  add_project_arguments('-DZYCORE_STATIC_BUILD', '-DZYDIS_STATIC_BUILD', language: ['c', 'cpp'])
endif

sources = files(
  'src/games/TPP.cpp',
  'src/games/GZ.cpp',
  'src/games/MGO.cpp',
  'src/games/common.cpp',
  'src/screen.cpp',
  'src/config.cpp',
  'src/logger.cpp',
  'src/dllmain.cpp',
  'src/memory.cpp',

  'external/zydis/src/Decoder.c',
  'external/zydis/src/DecoderData.c',
  'external/zydis/src/Disassembler.c',
  'external/zydis/src/Encoder.c',
  'external/zydis/src/EncoderData.c',
  'external/zydis/src/Formatter.c',
  'external/zydis/src/FormatterATT.c',
  'external/zydis/src/FormatterBase.c',
  'external/zydis/src/FormatterBuffer.c',
  'external/zydis/src/FormatterIntel.c',
  'external/zydis/src/MetaInfo.c',
  'external/zydis/src/Mnemonic.c',
  'external/zydis/src/Register.c',
  'external/zydis/src/Segment.c',
  'external/zydis/src/SharedData.c',
  'external/zydis/src/String.c',
  'external/zydis/src/Utils.c',
  'external/zydis/src/Zydis.c',

  'external/safetyhook/src/allocator.cpp',
  'external/safetyhook/src/easy.cpp',
  'external/safetyhook/src/inline_hook.cpp',
  'external/safetyhook/src/mid_hook.cpp',
  'external/safetyhook/src/os.windows.cpp',
  'external/safetyhook/src/utility.cpp',
  'external/safetyhook/src/vmt_hook.cpp',
)

incdirs = include_directories(
  'src',
  'external/mINI/src/mini',
  'external/zydis/dependencies/zycore/include',
  'external/zydis/src',
  'external/zydis/include',
  'external/safetyhook/include',
)

deps = [
  cpp.find_library('user32')
]

cargs = []
linkargs = []
if cpp.get_argument_syntax() != 'msvc'
  cargs += ['-ffunction-sections', '-fdata-sections', '-fno-ident']
  linkargs += [
    '-static', '-static-libgcc', '-static-libstdc++',
    '-Wl,--gc-sections',
    # NOTE: Work around mingw-w64 + binutils 2.41 duplicate-symbol error,
    #       "multiple definitions of std::type_info::operator==" when statically linking libstdc++.
    '-Wl,--allow-multiple-definition',
    '-Wl,--enable-stdcall-fixup',
    '-s', # NOTE: Explicit strip since meson on strips on install.
  ]
endif

shared_library('MGSVFix', sources,
  name_prefix: '',
  name_suffix: 'asi',
  include_directories: incdirs,
  dependencies: deps,
  c_args: cargs,
  cpp_args: cargs,
  link_args: linkargs,
  gnu_symbol_visibility: 'hidden',
)
