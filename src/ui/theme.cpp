#include "theme.h"
#include "win95_widgets.h"

namespace theme {

void applyDarkMode(bool enabled) {
    DARK_MODE_ENABLED = enabled;
    if (enabled) {
        WINDOW_BG = {40, 40, 40, 255};
        TITLE_BAR = {32, 32, 64, 255};
        TITLE_TEXT = {255, 255, 255, 255};

        TEXT_AREA_BG = {22, 22, 22, 255};
        TEXT_COLOR = {230, 230, 230, 255};
        CARET_COLOR = {230, 230, 230, 255};
        SELECTION_BG = {64, 64, 128, 255};
        SELECTION_TEXT = {255, 255, 255, 255};

        BORDER_LIGHT = {80, 80, 80, 255};
        BORDER_DARK = {20, 20, 20, 255};

        STATUS_BAR = {48, 48, 48, 255};
        STATUS_ERROR = {200, 80, 80, 255};
        STATUS_SUCCESS = {80, 160, 80, 255};

        MENU_BG = {48, 48, 48, 255};
        MENU_HOVER = {64, 64, 128, 255};
        MENU_TEXT = {230, 230, 230, 255};
        MENU_TEXT_HOVER = {255, 255, 255, 255};
        MENU_DISABLED = {120, 120, 120, 255};
        MENU_SEPARATOR = {90, 90, 90, 255};

        DIALOG_BG = {48, 48, 48, 255};
        DIALOG_TITLE_BG = {32, 32, 64, 255};
        DIALOG_TITLE_TEXT = {255, 255, 255, 255};

        BUTTON_BG = {64, 64, 64, 255};
        BUTTON_TEXT = {230, 230, 230, 255};
        BUTTON_PRESSED_BG = {32, 32, 32, 255};
    } else {
        WINDOW_BG = {192, 192, 192, 255};
        TITLE_BAR = {0, 0, 128, 255};
        TITLE_TEXT = {255, 255, 255, 255};

        TEXT_AREA_BG = {255, 255, 255, 255};
        TEXT_COLOR = {0, 0, 0, 255};
        CARET_COLOR = {0, 0, 0, 255};
        SELECTION_BG = {0, 0, 128, 255};
        SELECTION_TEXT = {255, 255, 255, 255};

        BORDER_LIGHT = {255, 255, 255, 255};
        BORDER_DARK = {128, 128, 128, 255};

        STATUS_BAR = {192, 192, 192, 255};
        STATUS_ERROR = {200, 0, 0, 255};
        STATUS_SUCCESS = {0, 100, 0, 255};

        MENU_BG = {192, 192, 192, 255};
        MENU_HOVER = {0, 0, 128, 255};
        MENU_TEXT = {0, 0, 0, 255};
        MENU_TEXT_HOVER = {255, 255, 255, 255};
        MENU_DISABLED = {128, 128, 128, 255};
        MENU_SEPARATOR = {128, 128, 128, 255};

        DIALOG_BG = {192, 192, 192, 255};
        DIALOG_TITLE_BG = {0, 0, 128, 255};
        DIALOG_TITLE_TEXT = {255, 255, 255, 255};

        BUTTON_BG = {192, 192, 192, 255};
        BUTTON_TEXT = {0, 0, 0, 255};
        BUTTON_PRESSED_BG = {128, 128, 128, 255};
    }

    win95::applyDarkMode(enabled);
}

}  // namespace theme

