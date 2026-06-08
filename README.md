# BlueCap

BlueCap 是一个基于 Qt5 的 Windows 屏幕录制工具。轻量、清爽、接近 Windows 11 视觉风格的桌面录屏应用，支持全屏、区域、窗口录制、系统托盘、全局快捷键及视频库管理。

![主界面](docs/images/light.png)

## 技术栈

- UI 框架：Qt5 Widgets
- 开发语言：C++17
- 构建工具：CMake
- 首要平台：Windows
- 录制后端：FFmpeg 可执行文件调用

## 当前功能

- 卡片式紧凑 UI 设计，无边框主窗口
- 录制模式：全屏、区域（选区浮层）、窗口（非模态选择器）
- 录制控制：开始/停止、倒计时（可取消）、脉冲动画录制指示器、计时器
- 系统托盘：录制状态图标、快速录制、退出
- 全局快捷键（需注册）
- 区域选择：尺寸实时显示、Enter 确认、窗口位置记忆
- 视频库：缩略图、重命名、搜索、空状态、点击打开/播放
- 设置页：保存路径、自动保存反馈、鼠标光标选项
- 录制文件保留策略、部分文件保留
- 完成通知（显示文件大小）、状态栏文件可点击、底部栏交互区分
- 阴影渲染优化、UI 线程异步化、设置贯通

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

## 备注

当前 `3rd/ffmpeg` 只包含 FFmpeg 可执行文件，没有 `include` 和 `lib`。因此现阶段采用进程调用方式完成录制 MVP。后续如果补充 FFmpeg 开发库，可以再把录制层升级为直接调用 libavcodec/libavformat。
