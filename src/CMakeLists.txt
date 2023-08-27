cmake_minimum_required(VERSION 3.7)
enable_language(ASM)
project(WireGuardKeyMine)

if(UNIX AND NOT APPLE)
  link_libraries(m pthread bsd)
else()
  link_libraries(m pthread)
endif()

if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64")
  message("Building for x86_64")
  set(SOURCE_FILES_X25519 mx25519/src/amd64/scalarmult_gnu.S)
else(${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64")
  message("Building for aarch64")
  set(SOURCE_FILES_X25519 X25519-AArch64/X25519-AArch64.s)
endif()

set(CMAKE_C_STANDARD 99)
include_directories(base64-config/lib)
set(SOURCE_FILES
    base64/lib/lib.c
    base64/lib/tables/tables.c
    base64/lib/arch/avx/codec.c
    base64/lib/arch/neon64/codec.c
    base64-config/lib/codec_choose.c
    ${SOURCE_FILES_X25519}
    main.c)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native")

add_executable(wgmine ${SOURCE_FILES})