cmake_minimum_required(VERSION 3.16)

project(BlueCap VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 REQUIRED COMPONENTS Widgets Svg Concurrent)

# Reconfigure when app icon changes, so the .rc is regenerated
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/app-icon.ico"
)

# Generate .rc file with absolute path to .ico for reliable resource compilation
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/resources")
set(BLUECAP_ICO_ABSOLUTE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/app-icon.ico")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/app-icon.rc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/resources/app-icon.rc"
)

add_executable(BlueCap WIN32
    src/main.cpp
    src/style/BlueCapStyle.cpp
    src/widgets/SurfaceWidget.cpp
    src/widgets/TitleBarButton.cpp
    src/widgets/SidebarButton.cpp
    src/widgets/RecordingIndicator.cpp
    src/widgets/BottomBar.cpp
    src/widgets/BottomNavSection.cpp
    src/widgets/SettingsPanel.cpp
    src/widgets/SettingsSeparator.cpp
    src/widgets/SaveFeedbackLabel.cpp
    src/widgets/ActionButton.cpp
    src/widgets/RecordModeCard.cpp
    src/widgets/AudioToggleCard.cpp
    src/widgets/RecentRecordingRow.cpp
    src/widgets/PaintedLineEdit.cpp
    src/widgets/PaintedComboBox.cpp
    src/widgets/PaintedSpinBox.cpp
    src/widgets/PaintedCheckBox.cpp
    src/widgets/PaintedDialog.cpp
    src/widgets/PageHeader.cpp
    src/widgets/ToastWidget.cpp
    src/widgets/PaintedScrollBar.cpp
    src/widgets/VideoListItemWidget.cpp
    src/widgets/SearchLineEdit.cpp
    src/widgets/StyledListWidget.cpp
    src/ui/NavigationController.cpp
    src/ui/MainWindow.cpp
    src/ui/Sidebar.cpp
    src/ui/ModeSwitch.cpp
    src/ui/RecordButton.cpp
    src/ui/RecordPage.cpp

    src/ui/RecordingOverlay.cpp
    src/ui/RegionSelector.cpp
    src/ui/SettingsPage.cpp
    src/ui/WindowPicker.cpp
    src/ui/VideoLibraryPage.cpp
    src/ui/TrayManager.cpp
    src/ui/HotkeyManager.cpp
    src/theme/Theme.h
    src/theme/ThemeColors.h
    src/theme/ThemeManager.cpp
    src/theme/ThemeManager.h
    src/theme/ThemePreference.h
    src/style/BlueCapStyle.h
    src/widgets/SurfaceWidget.h
    src/widgets/TitleBarButton.h
    src/widgets/SidebarButton.h
    src/widgets/RecordingIndicator.h
    src/widgets/BottomBar.h
    src/widgets/BottomNavSection.h
    src/widgets/SettingsPanel.h
    src/widgets/SettingsSeparator.h
    src/widgets/SaveFeedbackLabel.h
    src/widgets/ActionButton.h
    src/widgets/RecordModeCard.h
    src/widgets/AudioToggleCard.h
    src/widgets/RecentRecordingRow.h
    src/widgets/PaintedLineEdit.h
    src/widgets/PaintedComboBox.h
    src/widgets/PaintedSpinBox.h
    src/widgets/PaintedCheckBox.h
    src/widgets/PaintedDialog.h
    src/widgets/PageHeader.h
    src/widgets/ToastWidget.h
    src/widgets/PaintedScrollBar.h
    src/widgets/VideoListItemWidget.h
    src/widgets/SearchLineEdit.h
    src/widgets/StyledListWidget.h
    src/recorder/IRecorderService.h
    src/recorder/RecordingSession.h
    src/recorder/RecordingSession.cpp
    src/recorder/StderrMonitor.h
    src/recorder/StderrMonitor.cpp
    src/recorder/RecorderController.cpp
    src/storage/IVideoLibrary.h
    src/storage/VideoLibrary.cpp
    src/utils/FfmpegEncoderDetector.h
    src/utils/FfmpegError.h
    src/utils/IconHelper.h
    src/utils/OutputPath.h
    src/utils/win32/DiskSpaceChecker.h
    src/utils/win32/FileUtils.h
    src/utils/win32/Win32Icon.h
    src/utils/win32/WindowEnumerator.h
    src/utils/win32/WindowEnumerator.cpp
    resources/resources.qrc
    "${CMAKE_CURRENT_BINARY_DIR}/resources/app-icon.rc"
)

target_include_directories(BlueCap PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(BlueCap PRIVATE Qt5::Widgets Qt5::Svg Qt5::Concurrent)

target_compile_definitions(BlueCap PRIVATE
    BLUECAP_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING
)

if(MSVC)
    target_compile_options(BlueCap PRIVATE /utf-8 /W4)
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

find_program(CLANG_FORMAT NAMES clang-format)
if(CLANG_FORMAT)
    file(GLOB_RECURSE BLUECAP_ALL_SOURCE
        src/*.cpp src/*.h src/*.hpp
    )
    add_custom_target(format
        COMMAND ${CLANG_FORMAT} -i -style=file ${BLUECAP_ALL_SOURCE}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Formatting source files with clang-format"
    )
    add_custom_target(format-check
        COMMAND ${CLANG_FORMAT} --dry-run -Werror -style=file ${BLUECAP_ALL_SOURCE}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Checking code formatting"
    )
endif()
