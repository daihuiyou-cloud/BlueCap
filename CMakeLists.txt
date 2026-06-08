cmake_minimum_required(VERSION 3.16)

project(BlueCap VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 REQUIRED COMPONENTS Widgets Svg)

add_executable(BlueCap WIN32
    src/main.cpp
    src/ui/MainWindow.cpp
    src/ui/Sidebar.cpp
    src/ui/ModeSwitch.cpp
    src/ui/RecordButton.cpp
    src/ui/RecordPage.cpp
    src/ui/RegionSelector.cpp
    src/ui/SettingsPage.cpp
    src/ui/WindowPicker.cpp
    src/ui/VideoLibraryPage.cpp
    src/recorder/RecorderController.cpp
    src/storage/VideoLibrary.cpp
    resources/resources.qrc
)

target_include_directories(BlueCap PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(BlueCap PRIVATE Qt5::Widgets Qt5::Svg)

target_compile_definitions(BlueCap PRIVATE
    BLUECAP_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

if(MSVC)
    target_compile_options(BlueCap PRIVATE /utf-8)
    target_compile_options(BlueCap PRIVATE
        $<$<CONFIG:Release>:/O2 /arch:AVX2 /GL>
    )
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
endif()

set(BLUECAP_FFMPEG_DIR "${CMAKE_SOURCE_DIR}/3rd/ffmpeg")
if(EXISTS "${BLUECAP_FFMPEG_DIR}/ffmpeg.exe")
    add_custom_command(TARGET BlueCap POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:BlueCap>/3rd/ffmpeg"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${BLUECAP_FFMPEG_DIR}/ffmpeg.exe"
            "$<TARGET_FILE_DIR:BlueCap>/3rd/ffmpeg/ffmpeg.exe"
        COMMENT "Copying bundled FFmpeg executable"
    )
endif()
