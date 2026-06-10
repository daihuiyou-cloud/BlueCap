#pragma once

namespace paint {

struct Metrics {
    static constexpr int windowRadius = 20;
    static constexpr int pageRadius = 18;
    static constexpr int cardRadius = 14;
    static constexpr int controlRadius = 8;
    static constexpr int focusRadius = 10;
    static constexpr int sidebarWidth = 220;
    static constexpr int titleBarHeight = 56;
    static constexpr int pagePanelRadius = 26;
    static constexpr int modeCardHeight = 128;
    static constexpr int audioCardWidth = 142;
    static constexpr int audioCardHeight = 74;
    static constexpr int recentRowHeight = 54;

    // Form controls
    static constexpr int inputHeight = 46;
    static constexpr int inputMinWidth = 300;
    static constexpr int settingsPanelMaxWidth = 760;
    static constexpr int sidebarButtonMinHeight = 64;

    // RecordButton
    static constexpr int recordButtonSize = 122;
    static constexpr int recordButtonMargin = 5;

    // Window
    static constexpr int windowMinWidth = 740;
    static constexpr int windowMinHeight = 460;
    static constexpr int shellMargin = 12;
};

}
