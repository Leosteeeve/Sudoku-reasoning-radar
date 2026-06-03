#include "OverlayPages.h"

std::string overlayTitle(OverlayPage page) {
    switch (page) {
    case OverlayPage::Settings:
        return "Settings";
    case OverlayPage::Analytics:
        return "Analytics";
    case OverlayPage::Library:
        return "Puzzle Library";
    case OverlayPage::ImportExport:
        return "Import / Export";
    case OverlayPage::OCRImport:
        return "OCR Import Assistant";
    case OverlayPage::Shortcuts:
        return "Shortcuts";
    case OverlayPage::Generator:
        return "Generator";
    case OverlayPage::About:
        return "About";
    case OverlayPage::None:
        return "";
    }
    return "";
}
