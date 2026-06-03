#pragma once

#include <string>

enum class OverlayPage {
    None,
    Settings,
    Analytics,
    Library,
    ImportExport,
    OCRImport,
    Shortcuts,
    Generator,
    About
};

std::string overlayTitle(OverlayPage page);
