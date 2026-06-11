#pragma once

namespace paint {

struct Metrics {
    static constexpr int windowRadius = 14;
    static constexpr int pageRadius = 12;
    static constexpr int cardRadius = 10;
    static constexpr int controlRadius = 8;
    static constexpr int focusRadius = 8;
    static constexpr int sidebarWidth = 204;
    static constexpr int titleBarHeight = 48;
    static constexpr int pagePanelRadius = 16;
    static constexpr int modeCardHeight = 112;
    static constexpr int audioCardWidth = 136;
    static constexpr int audioCardHeight = 64;
    static constexpr int recentRowHeight = 54;

    // Form controls
    static constexpr int inputHeight = 46;
    static constexpr int inputMinWidth = 300;
    static constexpr int settingsPanelMaxWidth = 760;
    static constexpr int sidebarButtonMinHeight = 56;

    // RecordButton
    static constexpr int recordButtonSize = 112;
    static constexpr int recordButtonMargin = 5;

    // Window
    static constexpr int windowMinWidth = 740;
    static constexpr int windowMinHeight = 460;
    static constexpr int shellMargin = 8;
};

}
