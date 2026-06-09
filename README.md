# BlueCap

![浅色主题](docs/images/light.png) &nbsp; ![深色主题](docs/images/dark.png)

BlueCap 是一个基于 Qt5 的 Windows 屏幕录制工具。轻量、清爽、接近 Windows 11 视觉风格，支持全屏 / 区域 / 窗口三种录制模式，配备**深色/浅色主题**自动切换、系统托盘、全局快捷键及视频库管理。

## 技术栈

- UI 框架：Qt5 Widgets + Qt5 SVG
- 开发语言：C++17
- 构建工具：CMake 3.16+
- 首要平台：Windows（VS 工具链）
- 录制后端：FFmpeg（`gdigrab`，进程调用），GPU 编码器自动检测

## 功能

- **深色/浅色主题**：跟随 Windows 系统主题自动切换，支持手动设置
- **无边框窗口**，卡片式 UI + 圆角阴影 + 自定义标题栏
- **三种录制模式**：全屏 / 区域（选区浮层，实时尺寸） / 窗口（搜索 + 筛选选择器）
- **录制控制**：倒计时（可 Esc 取消）、脉冲动画指示器、计时器、录制覆盖层
- **编码器自动选择**：优先使用 GPU 硬件编码器（h264_mf / h264_amf / h264_nvenc），回退至 libx264
- **系统托盘**：录制状态图标、快速全屏录制、显示/隐藏、退出
- **全局快捷键**：`Ctrl + Shift + R` 开始/停止
- **视频库**：缩略图预览、重命名、搜索筛选、回收站删除、打开文件位置
- **设置页**：帧率、画质（ultrafast → slow）、保存路径验证、鼠标光标选项、启停超时
- **设置持久化**：`QSettings` 保存窗口位置、录制参数、主题偏好
- **exe 应用程序图标**（内嵌多尺寸 ICO）

## 目录结构

```text
BlueCap/
  CMakeLists.txt          # CMake 构建配置
  LICENSE
  README.md
  3rd/ffmpeg/             # 录制后端 FFmpeg
    ffmpeg.exe
    ffprobe.exe
  resources/
    bluecap.qss           # 浅色主题全局样式表
    bluecap-dark.qss      # 深色主题全局样式表
    resources.qrc         # Qt 资源文件
    app-icon.rc           # Windows 资源脚本（exe 图标）
    generate-icon.ps1     # ICO 图标生成脚本
    icons/                # SVG 图标（支持 currentColor 着色）+ 生成的 .ico
  src/
    main.cpp
    RecordMode.h          # 录制模式枚举
    utils/                # 工具层
      Theme.h             # 主题检测与样式表加载
      Format.h            # 文件大小格式化工具
    ui/                   # 用户界面层
      MainWindow.*
      Sidebar.*
      RecordPage.*
      RecordButton.*
      ModeSwitch.*
      RegionSelector.*
      WindowPicker.*
      RecordingOverlay.*  # 录制中屏幕覆盖层
      SettingsPage.*
      VideoLibraryPage.*
      IconHelper.h        # SVG currentColor 着色与缓存
    recorder/             # 录制控制层
      RecorderController.*
    storage/              # 数据持久化层
      VideoLibrary.*
```

## 环境要求

- Windows 10+
- CMake 3.16+
- Qt 5.12+（当前验证环境 5.13.2）
- Visual Studio 2017+ 工具链
- `3rd/ffmpeg/ffmpeg.exe`

```cmake
find_package(Qt5 REQUIRED COMPONENTS Widgets Svg)
```

如果 CMake 找不到 Qt5，配置时指定路径：

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="D:\Qt5.13.2\5.13.2\msvc2017"
```

## 构建

```powershell
cmake -S . -B build
cmake --build build --config Debug     # 或 --config Release
```

生成的可执行文件：

```text
build/Debug/BlueCap.exe                # Debug
build/Release/BlueCap.exe              # Release
```

构建后会自动复制 FFmpeg 到输出目录：

```text
build/Debug/3rd/ffmpeg/ffmpeg.exe
```

## 录制说明

通过 `QProcess` 调用 FFmpeg，使用 Windows `gdigrab` 设备抓取屏幕。编码器按优先级自动选择：`h264_mf`（Media Foundation）→ `h264_amf`（AMD）→ `h264_nvenc`（NVIDIA）→ `libx264`（软件回退）：

```text
ffmpeg -y -f gdigrab -framerate 30 -i desktop ^
       -c:v h264_mf -preset fast -pix_fmt yuv420p output.mp4
```

- 区域录制：`-offset_x x -offset_y y -video_size wxh -vf crop=w:h:x:y`
- 窗口录制：`-i title=<窗口标题>`
- 默认保存路径：`Videos/BlueCap/BlueCap_yyyyMMdd_HHmmss.mp4`

## 图标

SVG 图标使用 `currentColor` 属性，通过 `IconHelper` 在运行时动态着色，支持 Normal / Active / Disabled 三态。运行 `resources/generate-icon.ps1` 可重新生成应用图标（基于 `app-logo.svg` 设计，输出 16/32/48/256 四尺寸 ICO）：

```powershell
powershell -ExecutionPolicy Bypass -File resources/generate-icon.ps1
```
