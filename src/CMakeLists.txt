cmake_minimum_required(VERSION 3.7)
enable_language(ASM)
project(WireGuardKeyMine)

if(UNIX AND NOT APPLE)
  link_libraries(m pthread bsd)
else()
  link_libraries(m pthread)
endif()

set(CMAKE_C_STANDARD 99)
include_directories(base64-neon64/lib)
set(SOURCE_FILES
    base64/lib/lib.c
    base64/lib/tables/tables.c
    base64/lib/arch/neon64/codec.c
    base64-neon64/lib/codec_choose.c
    X25519-AArch64/X25519-AArch64.s
    main.c)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native")

add_executable(wgmine ${SOURCE_FILES})