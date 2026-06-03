#pragma once

#include "Board.h"
#include "Layout.h"
#include "OCRReviewState.h"
#include "OverlayPages.h"
#include "StepRecorder.h"

#include <SDL.h>
#include <SDL_ttf.h>

#include <array>
#include <opencv2/core.hpp>

#include <string>
#include <vector>

struct UIButton {
    std::string id;
    std::string label;
    SDL_Rect rect;
    bool enabled = true;
    bool hovered = false;
    bool pressed = false;
};

enum class CandidateDisplayMode {
    Off,
    Focused,
    All
};

struct RenderInfo {
    std::string versionText;
    std::string solverModeText;
    std::string statusText;
    std::string resultText;
    std::string puzzleName;
    std::string selectedCellText;
    std::string selectedCandidatesText;
    std::string candidateModeText;
    std::string mistakeModeText;
    std::string generatorText;
    std::string hintText;
    std::string difficultyText;
    std::string ioText;
    std::string libraryText;
    std::string focusText;
    std::string shortStatusText;
    std::string commandLabel;
    std::string commandDescription;
    std::string overlayTitle;
    std::string overlayInputText;
    std::string ocrStatusText;
    std::string ocrValidationText;
    std::string ocrImagePath;
    std::vector<std::string> overlayLines;
    std::array<OCRCell, 81> ocrCells{};
    int commandIndex = 0;
    int commandTotal = 1;
    int solvingTimeMs = 0;
    int selectedRow = -1;
    int selectedCol = -1;
    int ocrGivens = 0;
    int ocrLowConfidenceCount = 0;
    int ocrConflictCount = 0;
    int ocrSelectedRow = -1;
    int ocrSelectedCol = -1;
    int ocrPreviewVersion = 0;
    bool paused = false;
    bool playing = false;
    bool commandEnabled = true;
    bool overlayInputActive = false;
    bool ocrDebug = false;
    bool ocrCanConfirm = false;
    CandidateDisplayMode candidateMode = CandidateDisplayMode::Focused;
    OverlayPage overlayPage = OverlayPage::None;
    double speedMultiplier = 1.0;
    Uint32 stepAgeMs = 0;
    std::vector<CellRef> hintCells;
    std::vector<CellRef> mistakeCells;
    const cv::Mat* ocrOriginalPreview = nullptr;
    const cv::Mat* ocrWarpedGrid = nullptr;
};

SDL_Texture* createTextureFromMat(SDL_Renderer* renderer, const cv::Mat& mat);

class Renderer {
public:
    static constexpr int WindowWidth = 1280;
    static constexpr int WindowHeight = 860;

    Renderer();
    ~Renderer();

    bool initialize();
    void shutdown();

    void render(const Board& board,
                const std::vector<SolveStep>& steps,
                int currentStep,
                const RenderInfo& info,
                const std::vector<UIButton>& buttons);

    bool cellFromPoint(int x, int y, int& row, int& col) const;
    bool ocrCellFromPoint(int x, int y, int& row, int& col) const;
    void layoutButtons(std::vector<UIButton>& buttons,
                       int mouseX = -1,
                       int mouseY = -1,
                       const std::string& pressedId = "") const;
    void toggleFullscreen();
    bool isFullscreen() const;

private:
    SDL_Rect cellRect(int row, int col) const;
    void refreshLayout() const;
    void drawBackground(const SolveStep* current);
    void drawBoard(const Board& board, const SolveStep* current, const RenderInfo& info);
    void drawPanel(const std::vector<SolveStep>& steps,
                   int currentStep,
                   const RenderInfo& info,
                   const std::vector<UIButton>& buttons);
    void drawTimeline(const std::vector<SolveStep>& steps, int currentStep, const RenderInfo& info);
    void drawOverlay(const RenderInfo& info, const std::vector<UIButton>& buttons);
    void drawOCROverlay(const RenderInfo& info, const std::vector<UIButton>& buttons);
    void drawOCRImageCard(const std::string& title,
                          const std::string& emptyText,
                          const cv::Mat* image,
                          SDL_Texture*& texture,
                          int& textureVersion,
                          int sourceVersion,
                          const SDL_Rect& rect);
    void drawOCRReviewBoard(const RenderInfo& info, const SDL_Rect& rect);
    SDL_Rect ocrModalRect() const;
    SDL_Rect ocrReviewBoardRect() const;
    void drawCandidates(const Board& board,
                        int row,
                        int col,
                        const SolveStep* current,
                        const RenderInfo& info);
    bool shouldShowCandidates(int row, int col, const SolveStep* current, const RenderInfo& info) const;
    void drawButtons(const std::vector<UIButton>& buttons);
    void drawLabelValue(const std::string& label,
                        const std::string& value,
                        const SDL_Rect& rect,
                        SDL_Color valueColor);
    void drawText(const std::string& text, int x, int y, TTF_Font* font, SDL_Color color, int wrap = 0);
    int drawWrappedText(const std::string& text,
                        const SDL_Rect& rect,
                        TTF_Font* font,
                        SDL_Color color,
                        int lineSpacing = 4,
                        int maxLines = 0);
    void drawCenteredText(const std::string& text, const SDL_Rect& rect, TTF_Font* font, SDL_Color color);
    void fillRect(const SDL_Rect& rect, SDL_Color color);
    void strokeRect(const SDL_Rect& rect, SDL_Color color, int thickness = 1);
    void drawLine(int x1, int y1, int x2, int y2, SDL_Color color, int thickness = 1);
    SDL_Color accentForStep(StepType type) const;
    std::string describeStep(const SolveStep& step, int index, int total) const;
    std::string stepTypeName(StepType type) const;
    bool openFonts();
    void closeFonts();

    SDL_Window* window = nullptr;
    SDL_Renderer* sdlRenderer = nullptr;
    TTF_Font* fontSmall = nullptr;
    TTF_Font* fontTiny = nullptr;
    TTF_Font* fontCandidate = nullptr;
    TTF_Font* fontBody = nullptr;
    TTF_Font* fontMedium = nullptr;
    TTF_Font* fontNumber = nullptr;
    TTF_Font* fontNumberLarge = nullptr;
    bool fullscreen = false;
    mutable LayoutState layout;
    mutable SDL_Rect cachedOCRReviewRect{0, 0, 0, 0};
    SDL_Texture* ocrOriginalTexture = nullptr;
    SDL_Texture* ocrWarpedTexture = nullptr;
    int ocrOriginalTextureVersion = -1;
    int ocrWarpedTextureVersion = -1;
};
