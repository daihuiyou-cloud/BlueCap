cmake_minimum_required(VERSION 3.16)

project(BlueCap VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 REQUIRED COMPONENTS Widgets)

add_executable(BlueCap WIN32
    src/main.cpp
    src/ui/MainWindow.cpp
    src/ui/MainWindow.h
    src/ui/ModeSwitch.cpp
    src/ui/ModeSwitch.h
    src/ui/RecordButton.cpp
    src/ui/RecordButton.h
    src/ui/RecordPage.cpp
    src/ui/RecordPage.h
    src/ui/PlaceholderPage.cpp
    src/ui/PlaceholderPage.h
    src/recorder/RecorderController.cpp
    src/recorder/RecorderController.h
    src/storage/VideoLibrary.cpp
    src/storage/VideoLibrary.h
    resources/resources.qrc
)

target_link_libraries(BlueCap PRIVATE Qt5::Widgets)

target_compile_definitions(BlueCap PRIVATE
    BLUECAP_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

if(MSVC)
    target_compile_options(BlueCap PRIVATE /utf-8)
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
