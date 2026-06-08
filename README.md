# BlueCap

BlueCap 是一个基于 Qt5 的 Windows 屏幕录制工具。项目目标是做一个轻量、清爽、接近 Windows 11 视觉风格的桌面录屏应用，支持全屏、区域、窗口录制以及最近视频管理。

当前仓库处于初步开发阶段，已经建立 Qt5 + CMake 工程骨架，并接入本地 `3rd/ffmpeg/ffmpeg.exe` 作为第一版录制后端。

## 技术栈

- UI 框架：Qt5 Widgets
- 开发语言：C++17
- 构建工具：CMake
- 首要平台：Windows
- 录制后端：FFmpeg 可执行文件调用

## 当前功能

- 无边框主窗口
- 截图风格的录制主页
- 左侧导航：录制、视频库、设置
- 录制模式入口：全屏、区域、窗口
- 圆形开始/停止录制按钮
- 最近视频记录
- 通过 `ffmpeg.exe` 执行全屏录制 MVP

目前区域录制、窗口录制、音频采集、全局快捷键仍在后续计划中。

## 目录结构

```text
BlueCap/
  CMakeLists.txt
  LICENSE
  README.md
  3rd/
    ffmpeg/
      ffmpeg.exe
      ffplay.exe
      ffprobe.exe
  src/
    main.cpp
    ui/
      MainWindow.*
      Sidebar.*
      RecordPage.*
      RecordButton.*
      ModeSwitch.*
      PlaceholderPage.*
    recorder/
      RecorderController.*
    storage/
      VideoLibrary.*
```

## 环境要求

- Windows
- CMake 3.16+
- Qt5，当前本地验证环境为 Qt 5.13.2
- Visual Studio C++ 工具链
- `3rd/ffmpeg/ffmpeg.exe`

当前项目使用 Qt5 Widgets：

```cmake
find_package(Qt5 REQUIRED COMPONENTS Widgets)
```

如果 CMake 找不到 Qt5，可以在配置时指定 Qt5 路径，例如：

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="D:\Qt5.13.2\5.13.2\msvc2017"
```

## 构建

```powershell
cmake -S . -B build
cmake --build build --config Debug
```

生成的程序位于：

```text
build/Debug/BlueCap.exe
```

CMake 会在构建后尝试把 `3rd/ffmpeg/ffmpeg.exe` 复制到程序输出目录：

```text
build/Debug/3rd/ffmpeg/ffmpeg.exe
```

## 录制说明

第一版录制通过 `QProcess` 调用 FFmpeg：

```text
ffmpeg -y -f gdigrab -framerate 30 -i desktop -c:v libx264 -preset ultrafast -pix_fmt yuv420p output.mp4
```

录制文件默认保存到系统视频目录下的 `BlueCap` 文件夹，例如：

```text
Videos/BlueCap/BlueCap_yyyyMMdd_HHmmss.mp4
```

## 开发计划

1. 完成 Qt 主界面细节打磨
2. 修复并验证 Debug/Release 构建流程
3. 接入全局快捷键 `Ctrl + Shift + R`
4. 增加区域录制选择浮层
5. 增加窗口录制选择器
6. 增加系统音频和麦克风采集
7. 完善视频库页面
8. 增加设置页：保存路径、帧率、画质、快捷键
9. 优化打包和 FFmpeg 分发策略

## 备注

当前 `3rd/ffmpeg` 只包含 FFmpeg 可执行文件，没有 `include` 和 `lib`。因此现阶段采用进程调用方式完成录制 MVP。后续如果补充 FFmpeg 开发库，可以再把录制层升级为直接调用 libavcodec/libavformat。
