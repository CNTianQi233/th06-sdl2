###############################################################################
# th06 - 東方紅魔郷 SDL2 移植版构建脚本
#
# 用法 (Windows x86):
#   cmake -B build_sdl2 -A Win32
#   cmake --build build_sdl2 --config Release
#
# 用法 (Windows x64):
#   cmake -B build_sdl2 -A x64
#   cmake --build build_sdl2 --config Release
#
# 用法 (Android, 需要 NDK):
#   cmake -B build_android -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a
#   cmake --build build_android
###############################################################################

cmake_minimum_required(VERSION 3.20)
project(th06_sdl2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================================
# 选项
# ============================================================================
option(TH06_USE_SDL2 "使用 SDL2 作为平台后端" ON)
option(TH06_BUILD_ORIGINAL "构建原始 D3D8/Win32 版本 (仅限 Windows)" OFF)

# ============================================================================
# SDL2 查找 (优先使用项目内置的 3rdparty 版本)
# ============================================================================
if(TH06_USE_SDL2)
    # 设置 SDL2 搜索路径为项目内置的开发库
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(SDL2_ARCH "x64")
    else()
        set(SDL2_ARCH "x86")
    endif()

    set(SDL2_DIR "${CMAKE_SOURCE_DIR}/3rdparty/SDL2")
    set(SDL2_IMAGE_DIR "${CMAKE_SOURCE_DIR}/3rdparty/SDL2_image")
    set(SDL2_MIXER_DIR "${CMAKE_SOURCE_DIR}/3rdparty/SDL2_mixer")

    # --- SDL2 核心 ---
    add_library(SDL2::SDL2 SHARED IMPORTED)
    set_target_properties(SDL2::SDL2 PROPERTIES
        IMPORTED_LOCATION "${SDL2_DIR}/lib/${SDL2_ARCH}/SDL2.dll"
        IMPORTED_IMPLIB   "${SDL2_DIR}/lib/${SDL2_ARCH}/SDL2.lib"
        INTERFACE_INCLUDE_DIRECTORIES "${SDL2_DIR}/include"
    )

    add_library(SDL2::SDL2main STATIC IMPORTED)
    set_target_properties(SDL2::SDL2main PROPERTIES
        IMPORTED_LOCATION "${SDL2_DIR}/lib/${SDL2_ARCH}/SDL2main.lib"
        INTERFACE_INCLUDE_DIRECTORIES "${SDL2_DIR}/include"
    )

    # --- SDL2_image ---
    add_library(SDL2_image::SDL2_image SHARED IMPORTED)
    set_target_properties(SDL2_image::SDL2_image PROPERTIES
        IMPORTED_LOCATION "${SDL2_IMAGE_DIR}/lib/${SDL2_ARCH}/SDL2_image.dll"
        IMPORTED_IMPLIB   "${SDL2_IMAGE_DIR}/lib/${SDL2_ARCH}/SDL2_image.lib"
        INTERFACE_INCLUDE_DIRECTORIES "${SDL2_IMAGE_DIR}/include"
    )

    # --- SDL2_mixer ---
    add_library(SDL2_mixer::SDL2_mixer SHARED IMPORTED)
    set_target_properties(SDL2_mixer::SDL2_mixer PROPERTIES
        IMPORTED_LOCATION "${SDL2_MIXER_DIR}/lib/${SDL2_ARCH}/SDL2_mixer.dll"
        IMPORTED_IMPLIB   "${SDL2_MIXER_DIR}/lib/${SDL2_ARCH}/SDL2_mixer.lib"
        INTERFACE_INCLUDE_DIRECTORIES "${SDL2_MIXER_DIR}/include"
    )

    message(STATUS "SDL2 路径: ${SDL2_DIR}")
    message(STATUS "SDL2_image 路径: ${SDL2_IMAGE_DIR}")
    message(STATUS "SDL2_mixer 路径: ${SDL2_MIXER_DIR}")
    message(STATUS "目标架构: ${SDL2_ARCH}")
endif()

# ============================================================================
# SDL2 验证测试程序
# ============================================================================
add_executable(th06_sdl2_test src/sdl2_test.cpp)

target_link_libraries(th06_sdl2_test PRIVATE
    SDL2::SDL2main
    SDL2::SDL2
    SDL2_image::SDL2_image
    SDL2_mixer::SDL2_mixer
)

# 复制 DLL 到输出目录
add_custom_command(TARGET th06_sdl2_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL2_DIR}/lib/${SDL2_ARCH}/SDL2.dll"
        $<TARGET_FILE_DIR:th06_sdl2_test>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL2_IMAGE_DIR}/lib/${SDL2_ARCH}/SDL2_image.dll"
        $<TARGET_FILE_DIR:th06_sdl2_test>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL2_MIXER_DIR}/lib/${SDL2_ARCH}/SDL2_mixer.dll"
        $<TARGET_FILE_DIR:th06_sdl2_test>
    COMMENT "复制 SDL2 DLL 到输出目录"
)

# ============================================================================
# 主游戏可执行文件 th06
# ============================================================================

# 收集源文件 (排除原版构建专用文件和测试文件)
set(TH06_GAME_SOURCES
    src/main.cpp
    src/AnmManager.cpp
    src/AsciiManager.cpp
    src/BombData.cpp
    src/BulletData.cpp
    src/BulletManager.cpp
    src/Chain.cpp
    src/CMyFont.cpp
    src/Controller.cpp
    src/EclManager.cpp
    src/EffectManager.cpp
    src/Ending.cpp
    src/EnemyEclInstr.cpp
    src/EnemyManager.cpp
    src/FileSystem.cpp
    src/GameErrorContext.cpp
    src/GameManager.cpp
    src/GameWindow.cpp
    src/Gui.cpp
    src/ItemManager.cpp
    src/MainMenu.cpp
    src/MidiOutput.cpp
    src/MusicRoom.cpp
    src/Player.cpp
    src/ReplayManager.cpp
    src/ResultScreen.cpp
    src/Rng.cpp
    src/ScreenEffect.cpp
    src/sdl2_renderer.cpp
    src/SoundPlayer.cpp
    src/Stage.cpp
    src/Supervisor.cpp
    src/TextHelper.cpp
    src/ThImGuiBridge.cpp
    src/ThpracGui.cpp
    src/utils.cpp
    src/ZunTimer.cpp
    src/zwave.cpp
    # pbg3 文件解析库
    src/pbg3/FileAbstraction.cpp
    src/pbg3/IPbg3Parser.cpp
    src/pbg3/Pbg3Archive.cpp
    src/pbg3/Pbg3Parser.cpp
)

add_executable(th06 ${TH06_GAME_SOURCES})

# ThpracGui.cpp 包含 UTF-8 中文字符串, 需要单独 /utf-8
if(MSVC)
    set_source_files_properties(src/ThpracGui.cpp PROPERTIES COMPILE_FLAGS "/utf-8")
endif()

# 包含目录
target_include_directories(th06 PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/build/autogenerated
)

# i18n.hpp 生成 (UTF-8 → Shift-JIS)
set(I18N_TPL "${CMAKE_SOURCE_DIR}/src/i18n.tpl")
set(I18N_HPP "${CMAKE_SOURCE_DIR}/build/autogenerated/i18n.hpp")

add_custom_command(
    OUTPUT "${I18N_HPP}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/build/autogenerated"
    COMMAND python -c
        "import sys; open(sys.argv[2], 'wb').write(open(sys.argv[1], 'rb').read().decode('utf8').encode('shift_jis'))"
        "${I18N_TPL}" "${I18N_HPP}"
    DEPENDS "${I18N_TPL}"
    COMMENT "生成 i18n.hpp (UTF-8 → Shift-JIS)"
)
add_custom_target(generate_i18n DEPENDS "${I18N_HPP}")
add_dependencies(th06 generate_i18n)

# 查找 OpenGL
find_package(OpenGL REQUIRED)

# 链接库
target_link_libraries(th06 PRIVATE
    SDL2::SDL2main
    SDL2::SDL2
    SDL2_image::SDL2_image
    SDL2_mixer::SDL2_mixer
    OpenGL::GL
)

# Windows 特定: GDI32 (TextHelper 文本渲染), Winmm (timeGetTime 兼容)
if(WIN32)
    target_link_libraries(th06 PRIVATE gdi32 winmm)
endif()

# 复制 DLL 到主游戏输出目录
add_custom_command(TARGET th06 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL2_DIR}/lib/${SDL2_ARCH}/SDL2.dll"
        $<TARGET_FILE_DIR:th06>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL2_IMAGE_DIR}/lib/${SDL2_ARCH}/SDL2_image.dll"
        $<TARGET_FILE_DIR:th06>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL2_MIXER_DIR}/lib/${SDL2_ARCH}/SDL2_mixer.dll"
        $<TARGET_FILE_DIR:th06>
    COMMENT "复制 SDL2 DLL 到游戏输出目录"
)
