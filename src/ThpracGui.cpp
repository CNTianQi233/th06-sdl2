// ThpracGui.cpp — Ported from thprac by Ack. Compiled with modern MSVC (/std:c++14).
// Hardcoded memory addresses replaced with source-level struct/field references.
//
// Address → source reference substitutions applied throughout this file:
//   *(int8_t*)(0x69bcb0)  → (int)g_GameManager.difficulty
//   *(int8_t*)(0x69d4ba)  → g_GameManager.livesRemaining
//   *(int8_t*)(0x69d4bb)  → g_GameManager.bombsRemaining
//   *(int16_t*)(0x69d4b0) → (int)g_GameManager.currentPower
//   *(int32_t*)(0x69bca0) → g_GameManager.guiScore
//   *(int32_t*)(0x69bcb4) → g_GameManager.grazeInStage
//   *(int16_t*)(0x69d4b4) → (int)g_GameManager.pointItemsCollectedInStage
//   *(int32_t*)(0x69d710) → g_GameManager.rank
//   *(int32_t*)(0x69d714) → g_GameManager.maxRank
//   *(int32_t*)(0x69d718) → g_GameManager.minRank
//   *(int32_t*)(0x69d71C) → g_GameManager.subRank
//   *(int8_t*)(0x69d4bd)  → (int)g_GameManager.character
//   *(int8_t*)(0x69d4be)  → (int)g_GameManager.shotType

#include "ThpracGui.hpp"
#include "GameManager.hpp"
#include "Player.hpp"
#include "Supervisor.hpp"
#include "Controller.hpp"

#ifdef TH06_IMGUI_ENABLED
#include <imgui.h>
#include <SDL.h>

namespace th06 {

// ===========================================================================
// GuiSlider<T,type> — ported verbatim from THPrac::Gui::GuiSlider
//                     (thprac_gui_components.h)
// GuiDrag<T,type>   — ported verbatim from THPrac::Gui::GuiDrag
//
// Changes from original:
//   - Locale system removed; constructor takes plain const char* label.
//   - InGameInputGet() implemented with SDL_GetKeyboardState
//     (edge-triggered: returns true only on the first frame of a keypress).
// ===========================================================================

static bool s_prevKeyState[512] = {};

static inline bool InGameInputGet(SDL_Scancode sc)
{
    const Uint8 *state = SDL_GetKeyboardState(NULL);
    bool cur = state[sc] != 0;
    bool prev = s_prevKeyState[sc];
    s_prevKeyState[sc] = cur;
    return cur && !prev;
}

template <typename T, ImGuiDataType type>
class GuiSlider {
    const char* mLabel;
    T mValue;
    T mValueMin;
    T mValueMax;
    T mStep;
    T mStepMin;
    T mStepMax;
    T mStepX;

public:
    GuiSlider(const char* label, T minimum, T maximum,
              T step_min = (T)1, T step_max = (T)1, T step_x = (T)10)
        : mLabel(label)
        , mValue(minimum)
        , mValueMin(minimum)
        , mValueMax(maximum)
        , mStep(step_min)
        , mStepMin(step_min)
        , mStepMax(step_max)
        , mStepX(step_x)
    {
    }

    inline void SetValue(T value) { mValue = value; }
    inline T&   operator*()       { return mValue; }
    inline T    GetValue() const  { return mValue; }

    inline void SetBound(T minimum, T maximum)
    {
        mValueMin = minimum;
        mValueMax = maximum;
        if (mValue < mValueMin) mValue = mValueMin;
        else if (mValue > mValueMax) mValue = mValueMax;
    }

    inline void RoundDown(T x)
    {
        if (mValue % x) mValue -= (mValue % x);
    }

    bool operator()(const char* format = nullptr)
    {
        bool hasChanged = ImGui::SliderScalar(mLabel, type, &mValue, &mValueMin, &mValueMax, format);
        bool isFocused  = ImGui::IsItemFocused();

        if (isFocused) {
            if (InGameInputGet(SDL_SCANCODE_LSHIFT)) {
                if (mStep == mStepMax) mStep = mStepMin;
                else {
                    mStep *= mStepX;
                    if (mStep > mStepMax) mStep = mStepMax;
                }
            }
            if (InGameInputGet(SDL_SCANCODE_LEFT)) {
                T old = mValue;
                mValue -= mStep;
                if (mValue < mValueMin) mValue = mValueMin;
                hasChanged |= (mValue != old);
            } else if (InGameInputGet(SDL_SCANCODE_RIGHT)) {
                T old = mValue;
                mValue += mStep;
                if (mValue > mValueMax) mValue = mValueMax;
                hasChanged |= (mValue != old);
            }
        }
        return hasChanged;
    }
};

template <typename T, ImGuiDataType type>
class GuiDrag {
    const char* mLabel;
    T mValue;
    T mValueMin;
    T mValueMax;
    T mStep;
    T mStepMin;
    T mStepMax;
    T mStepX;

public:
    GuiDrag(const char* label, T minimum, T maximum,
            T step_min = (T)1, T step_max = (T)1, T step_x = (T)10)
        : mLabel(label)
        , mValue(minimum)
        , mValueMin(minimum)
        , mValueMax(maximum)
        , mStep(step_min)
        , mStepMin(step_min)
        , mStepMax(step_max)
        , mStepX(step_x)
    {
    }

    inline void SetValue(T value) { mValue = value; }
    inline T&   operator*()       { return mValue; }
    inline T    GetValue() const  { return mValue; }

    inline void SetBound(T minimum, T maximum)
    {
        mValueMin = minimum;
        mValueMax = maximum;
        if (mValue < mValueMin) mValue = mValueMin;
        else if (mValue > mValueMax) mValue = mValueMax;
    }

    inline void RoundDown(T x)
    {
        if (mValue % x) mValue -= (mValue % x);
    }

    void operator()(const char* format = nullptr)
    {
        ImGui::DragScalar(mLabel, type, &mValue, (float)(mStep * 2), &mValueMin, &mValueMax, format);
        if (mValue > mValueMax) mValue = mValueMax;
        if (mValue < mValueMin) mValue = mValueMin;

        bool isFocused = ImGui::IsItemFocused();
        if (isFocused) {
            if (InGameInputGet(SDL_SCANCODE_LSHIFT)) {
                mStep *= mStepX;
                if (mStep > mStepMax) mStep = mStepMin;
            }
            if (InGameInputGet(SDL_SCANCODE_LEFT)) {
                mValue -= mStep;
                if (mValue < mValueMin) mValue = mValueMin;
            } else if (InGameInputGet(SDL_SCANCODE_RIGHT)) {
                mValue += mStep;
                if (mValue > mValueMax) mValue = mValueMax;
            }
        }
    }
};

// ===========================================================================
// ThpracGui public static member definitions
// ===========================================================================

DIFFABLE_STATIC(ThpracGui, g_ThpracGui)

bool ThpracGui::invincible    = false;
bool ThpracGui::infLives      = false;
bool ThpracGui::infBombs      = false;
bool ThpracGui::infPower      = false;
bool ThpracGui::timeLock      = false;
bool ThpracGui::autoBomb      = false;
bool ThpracGui::persistentBGM = false;
bool ThpracGui::showGameInfo  = false;

bool ThpracGui::showRank   = false;
bool ThpracGui::keyDisplay  = false;
i32  ThpracGui::fpsMode     = 0;

// ===========================================================================
// Practice menu controls — ported from THGuiPrac (thprac_th06.cpp)
//
// Member variable declarations (see thprac_th06.cpp ~line 780):
//   Gui::GuiSlider<int, ImGuiDataType_S32> mLife  { TH_LIFE,  0, 8 };
//   Gui::GuiSlider<int, ImGuiDataType_S32> mBomb  { TH_BOMB,  0, 8 };
//   Gui::GuiSlider<int, ImGuiDataType_S32> mPower { TH_POWER, 0, 128 };
//   Gui::GuiDrag<int64_t, ImGuiDataType_S64> mScore { TH_SCORE, 0, 9999999990, 10, 100000000 };
//   Gui::GuiDrag<int, ImGuiDataType_S32> mGraze { TH_GRAZE, 0, 99999, 1, 10000 };
//   Gui::GuiDrag<int, ImGuiDataType_S32> mPoint { TH_POINT, 0, 9999, 1, 1000 };
//   Gui::GuiSlider<int, ImGuiDataType_S32> mRank { TH06_RANK, 0, 32, 1, 10, 10 };
//   Gui::GuiCheckBox mRankLock { TH06_RANKLOCK };
//
// All *(type*)(address) replaced with g_GameManager source fields.
// ===========================================================================

static int s_mode  = 0; // 0=正常游戏  1=自定义练习  (THGuiPrac::mMode)
static int s_stage = 0; // 0-5                       (THGuiPrac::mStage)

// Ported verbatim from THGuiPrac member declarations, labels localised to ZH_CN:
static GuiSlider<int,     ImGuiDataType_S32> mLife  { "残机",  0, 8 };
static GuiSlider<int,     ImGuiDataType_S32> mBomb  { "Bomb",  0, 8 };
static GuiSlider<int,     ImGuiDataType_S32> mPower { "火力",  0, 128 };
static GuiDrag<int64_t,   ImGuiDataType_S64> mScore { "分数",  (int64_t)0, (int64_t)9999999990LL, (int64_t)10, (int64_t)100000000 };
static GuiDrag<int,       ImGuiDataType_S32> mGraze { "擦弹",  0, 99999, 1, 10000 };
static GuiDrag<int,       ImGuiDataType_S32> mPoint { "点数",  0, 9999,  1, 1000  };
static GuiSlider<int,     ImGuiDataType_S32> mRank  { "Rank",  0, 32, 1, 10, 10 };
static bool                                  mRankLock = false;

// Read from game state on each frame — ported from THGuiPrac::State(1):
//   mDiffculty = (int)(*((int8_t*)0x69bcb0))             → g_GameManager.difficulty
//   mShotType  = (*((int8_t*)0x69d4bd) * 2) + ...        → character*2 + shotType
static int s_diffculty = 0;
static int s_shotType  = 0;

static bool s_menuOpen = false;

// ===========================================================================
// DrawOverlay() — ported from THOverlay::OnContentUpdate (thprac_th06.cpp)
//
// In thprac THOverlay::OnContentUpdate calls mMuteki(), mInfLives(), etc.
// Each GuiHotKey::operator()() renders a row with checkbox + coloured label
// and applies / removes the memory patches.  Here the patch effects are
// handled in ApplyCheats(); the UI renders a plain checkbox + coloured text.
// ===========================================================================

struct HkEntry { const char* key; const char* label; bool* flag; };
static HkEntry s_hkEntries[8] = {
    { "F1", "无敌",    &ThpracGui::invincible    },
    { "F2", "锁残",    &ThpracGui::infLives      },
    { "F3", "锁Bomb",  &ThpracGui::infBombs      },
    { "F4", "锁火力",  &ThpracGui::infPower      },
    { "F5", "锁时",    &ThpracGui::timeLock      },
    { "F6", "自动B",   &ThpracGui::autoBomb      },
    { "F7", "永续BGM", &ThpracGui::persistentBGM },
    { "F8", "游戏信息",&ThpracGui::showGameInfo   },
};

static void DrawOverlay()
{
    // THOverlay: SetPos(10.0f, 10.0f), SetFade(0.5f, 0.5f)
    ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.5f);
    ImGui::Begin("##modmenu", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav);

    // mMuteki() / mInfLives() / ... — each renders one row
    for (int i = 0; i < 8; ++i) {
        const HkEntry& e = s_hkEntries[i];
        char cbId[16];
        snprintf(cbId, sizeof(cbId), "##hk%d", i);
        ImGui::Checkbox(cbId, e.flag);
        ImGui::SameLine();
        bool on = *e.flag;
        if (on)
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "%s %s [ON]", e.key, e.label);
        else
            ImGui::TextDisabled("%s %s", e.key, e.label);
    }
    ImGui::Separator();
    // mMenu toggle hint (thprac: Gui::GuiHotKeyChord mMenu BACKSPACE)
    ImGui::TextDisabled("BACKSPACE: %s", s_menuOpen ? "关闭菜单" : "打开菜单");
    ImGui::End();
}

// ===========================================================================
// DrawGameInfoOverlay() — ported from TH06InGameInfo::OnContentUpdate
//   *(int8_t*)(0x69bcb0)  → g_GameManager.difficulty
//   *(int8_t*)(0x69D4BD)  → g_GameManager.character
//   *(int8_t*)(0x69D4BE)  → g_GameManager.shotType
//   *(int8_t*)(0x0069BCC4)→ g_GameManager.bombsRemaining (bomb count in thprac)
//   *(int32_t*)(0x69d710) → g_GameManager.rank
//   *(int32_t*)(0x69d71C) → g_GameManager.subRank
// ===========================================================================

static void DrawGameInfoOverlay()
{
    ImVec2 disp = ImGui::GetIO().DisplaySize;
    // TH06InGameInfo: SetPosRel(433/640, 245/480), SetSizeRel(180/640, 0)
    ImGui::SetNextWindowPos(ImVec2(disp.x - 8.0f, 8.0f), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.9f);
    ImGui::Begin("##igi", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoInputs);

    static const char* s_diffNames[] = { "Easy", "Normal", "Hard", "Lunatic", "Extra" };
    static const char* s_charNames[] = { "ReimuA", "ReimuB", "MarisaA", "MarisaB" };

    // byte cur_diff         = *(byte*)(0x69bcb0)  → g_GameManager.difficulty
    // byte cur_player_type  = (character<<1)|shotType → GameManager::CharacterShotType()
    int diff     = (int)g_GameManager.difficulty;
    int charType = GameManager::CharacterShotType();
    if (diff     < 0 || diff     > 4) diff     = 0;
    if (charType < 0 || charType > 3) charType = 0;

    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%s (%s)",
        s_diffNames[diff], s_charNames[charType]);

    ImGui::Columns(2);
    ImGui::TextUnformatted("Score");  ImGui::NextColumn();
    ImGui::Text("%u", g_GameManager.score); ImGui::NextColumn();
    ImGui::TextUnformatted("Lives");  ImGui::NextColumn();
    ImGui::Text("%d", (int)g_GameManager.livesRemaining); ImGui::NextColumn();
    ImGui::TextUnformatted("Bombs");  ImGui::NextColumn();
    // *(int8_t*)(0x0069BCC4) in thprac is g_GameManager.bombsRemaining
    ImGui::Text("%d", (int)g_GameManager.bombsRemaining); ImGui::NextColumn();
    ImGui::TextUnformatted("Power");  ImGui::NextColumn();
    ImGui::Text("%d",  (int)g_GameManager.currentPower); ImGui::NextColumn();
    ImGui::TextUnformatted("Graze");  ImGui::NextColumn();
    ImGui::Text("%d",  g_GameManager.grazeInStage); ImGui::NextColumn();
    ImGui::TextUnformatted("Points"); ImGui::NextColumn();
    ImGui::Text("%d",  (int)g_GameManager.pointItemsCollectedInStage); ImGui::NextColumn();
    if (ThpracGui::showRank) {
        // *(int32_t*)(0x69d710) → rank, *(int32_t*)(0x69d71C) → subRank
        ImGui::TextUnformatted("Rank"); ImGui::NextColumn();
        ImGui::Text("%d.%02d", g_GameManager.rank, g_GameManager.subRank); ImGui::NextColumn();
    }
    ImGui::Columns(1);
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

    ImGui::End();
}

// ===========================================================================
// DrawKeyDisplay() — show current button inputs at bottom of screen
// ===========================================================================

static void DrawKeyDisplay()
{
    ImVec2 disp = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(disp.x * 0.5f, disp.y - 8.0f), ImGuiCond_Always, ImVec2(0.5f, 1.0f));
    ImGui::SetNextWindowBgAlpha(0.70f);
    ImGui::Begin("##keydisp", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoInputs);

    u16 inp = g_CurFrameInput;

    auto showKey = [](bool on, const char* label) {
        if (on) ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "%s", label);
        else    ImGui::TextDisabled("%s", label);
    };

    ImGui::Dummy(ImVec2(22, 0)); ImGui::SameLine();
    showKey(!!(inp & TH_BUTTON_UP), "^");
    ImGui::NewLine();
    showKey(!!(inp & TH_BUTTON_LEFT), "<"); ImGui::SameLine();
    ImGui::Dummy(ImVec2(16, 0)); ImGui::SameLine();
    showKey(!!(inp & TH_BUTTON_RIGHT), ">");
    ImGui::NewLine();
    ImGui::Dummy(ImVec2(22, 0)); ImGui::SameLine();
    showKey(!!(inp & TH_BUTTON_DOWN), "v");

    ImGui::SameLine(0, 24);

    ImGui::BeginGroup();
    showKey(!!(inp & TH_BUTTON_FOCUS), "Shift"); ImGui::SameLine();
    showKey(!!(inp & TH_BUTTON_SHOOT), "Z");     ImGui::SameLine();
    showKey(!!(inp & TH_BUTTON_BOMB),  "X");
    ImGui::EndGroup();

    ImGui::End();
}

// ===========================================================================
// PracticeMenu() — ported from THGuiPrac::PracticeMenu (thprac_th06.cpp)
//
// Original (abridged):
//   void PracticeMenu(Gui::GuiNavFocus& nav_focus) {
//       mMode();
//       if (mStage()) *mSection = *mChapter = 0;
//       if (*mMode == 1) {
//           if (mWarp()) { ... }   // warp/section selection — omitted (needs hooks)
//           mLife();
//           mBomb();
//           mScore();  mScore.RoundDown(10);
//           mPower();
//           mGraze();
//           mPoint();
//           mRank();
//           if (mRankLock()) {
//               if (*mRankLock) mRank.SetBound(0, 99);
//               else            mRank.SetBound(0, 32);
//           }
//       }
//       nav_focus();
//   }
//
// Warp/section widget omitted: requires game hook infrastructure.
// ===========================================================================

static void PracticeMenu()
{
    // THGuiPrac::SetItemWidth(-60.0f) — label to the right of each widget
    ImGui::PushItemWidth(-60.0f);

    // mMode() — Gui::GuiCombo mMode { TH_MODE, TH_MODE_SELECT }
    static const char* s_modeItems[] = { "正常游戏", "自定义练习" };
    ImGui::Combo("模式", &s_mode, s_modeItems, 2);

    // mStage() — Gui::GuiCombo mStage { TH_STAGE, TH_STAGE_SELECT }
    static const char* s_stageItems[] = {
        "Stage 1", "Stage 2", "Stage 3",
        "Stage 4", "Stage 5", "Stage 6",
    };
    ImGui::Combo("关卡", &s_stage, s_stageItems, 6);

    if (s_mode == 1) {
        ImGui::Spacing();

        // mLife()  — GuiSlider<int,S32> mLife  { TH_LIFE,  0, 8 }
        mLife();
        // mBomb()  — GuiSlider<int,S32> mBomb  { TH_BOMB,  0, 8 }
        mBomb();
        // mScore() — GuiDrag<int64_t,S64> mScore { TH_SCORE, 0, 9999999990, 10, 100000000 }
        mScore();
        mScore.RoundDown(10); // thprac: mScore.RoundDown(10)
        // mPower() — GuiSlider<int,S32> mPower { TH_POWER, 0, 128 }
        mPower();
        // mGraze() — GuiDrag<int,S32> mGraze { TH_GRAZE, 0, 99999, 1, 10000 }
        mGraze();
        // mPoint() — GuiDrag<int,S32> mPoint { TH_POINT, 0, 9999, 1, 1000 }
        mPoint();
        // mRank()  — GuiSlider<int,S32> mRank { TH06_RANK, 0, 32, 1, 10, 10 }
        mRank();

        // mRankLock() — Gui::GuiCheckBox mRankLock { TH06_RANKLOCK }
        // thprac: if (mRankLock()) { if (*mRankLock) mRank.SetBound(0,99); else mRank.SetBound(0,32); }
        if (ImGui::Checkbox("锁Rank", &mRankLock)) {
            if (mRankLock) mRank.SetBound(0, 99);
            else           mRank.SetBound(0, 32);
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (g_GameManager.gameFrames == 0)
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "参数将在下次开始游戏时应用");
        else
            ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "游戏进行中 — 返回主菜单后生效");
    }

    ImGui::PopItemWidth();
}

// ===========================================================================
// DrawAdvancedOptions() — ported from THAdvOptWnd::ContentUpdate (thprac_th06.cpp)
//
// FPS control: thprac modifies vpatch DLL memory; here we set
//   g_Supervisor.cfg.frameskipConfig which the game reads each frame.
// Cheat checkboxes mirror the THOverlay hotkey flags for convenience.
// ===========================================================================

static void DrawAdvancedOptions()
{
    if (ImGui::CollapsingHeader("游戏速度", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();
        // THAdvOptWnd: FpsSet() writes to vpatch memory; here we use frameskipConfig
        static const char* s_fpsModes[] = { "x1.0 (60fps)", "x0.5 (30fps)", "x0.25 (15fps)" };
        ImGui::SetNextItemWidth(200.0f);
        if (ImGui::Combo("##fps", &ThpracGui::fpsMode, s_fpsModes, 3))
            g_Supervisor.cfg.frameskipConfig = (u8)ThpracGui::fpsMode;
        ImGui::Unindent();
    }

    ImGui::Spacing();

    if (ImGui::CollapsingHeader("游戏进行", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();

        // g_adv_igi_options.th06_showRank → ThpracGui::showRank
        ImGui::Checkbox("显示Rank", &ThpracGui::showRank);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("在游戏信息面板中显示当前Rank值");

        ImGui::Checkbox("按键显示", &ThpracGui::keyDisplay);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("在屏幕底部实时显示当前游戏输入");

        ImGui::Separator();

        // Mirrors THOverlay hotkeys — convenience toggles in the menu
        // (effects implemented in ApplyCheats, not via PATCH_HK)
        ImGui::Checkbox("[F1] 无敌",   &ThpracGui::invincible);
        ImGui::Checkbox("[F2] 锁残",   &ThpracGui::infLives);
        ImGui::Checkbox("[F3] 锁Bomb", &ThpracGui::infBombs);
        ImGui::Checkbox("[F4] 锁火力", &ThpracGui::infPower);
        ImGui::Checkbox("[F5] 锁时",   &ThpracGui::timeLock);

        ImGui::Separator();

        ImGui::BeginDisabled(true);
        ImGui::Checkbox("[F6] 自动B",   &ThpracGui::autoBomb);
        ImGui::SameLine(); ImGui::TextDisabled("(待实现)");
        ImGui::Checkbox("[F7] 永续BGM", &ThpracGui::persistentBGM);
        ImGui::SameLine(); ImGui::TextDisabled("(待实现)");
        ImGui::EndDisabled();

        ImGui::Unindent();
    }

    ImGui::Spacing();

    if (ImGui::CollapsingHeader("关于")) {
        ImGui::Indent();
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "th06 thprac overlay");
        ImGui::TextUnformatted("嵌入式练习工具 (源码版)");
        ImGui::TextDisabled("基于 thprac by Ack");
        ImGui::Unindent();
    }
}

// ===========================================================================
// DrawMainMenu() — main window with tab bar
//
// Layout based on THGuiPrac::OnLocaleChange for ZH_CN:
//   SetSize(330.f, 390.f), SetPos(260.f, 65.f), SetItemWidth(-60.0f)
// THGuiPrac::OnContentUpdate:
//   ImGui::TextUnformatted(S(TH_MENU));  ImGui::Separator();
//   PracticeMenu(mNavFocus);
// Advanced options tab added here (THAdvOptWnd rendered as a tab for simplicity).
// ===========================================================================

static void DrawMainMenu()
{
    // THGuiPrac ZH_CN: SetPos(260, 65), SetSize(330, 0)
    ImGui::SetNextWindowPos(ImVec2(260.0f, 65.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(330.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.8f); // SetFade(0.8f, ...)

    if (!ImGui::Begin("练习菜单##main", &s_menuOpen,
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
        ImGui::End();
        return;
    }

    // THGuiPrac::OnContentUpdate header
    ImGui::TextUnformatted("练习菜单");
    ImGui::Separator();

    if (ImGui::BeginTabBar("##tabs")) {
        if (ImGui::BeginTabItem("练习选项")) {
            PracticeMenu();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("高级选项")) {
            DrawAdvancedOptions();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}

// ===========================================================================
// Public API
// ===========================================================================

void ThpracGui::Init()
{
    // THGuiPrac constructor default values:
    //   *mLife = 8; *mBomb = 8; *mPower = 128; *mScore = 0; *mGraze = 0; *mRank = 32;
    *mLife  = 2;    // common practice start
    *mBomb  = 3;
    *mPower = 128;
    *mScore = 0LL;
    *mGraze = 0;
    *mPoint = 0;
    *mRank  = 32;
    mRankLock = false;
}

void ThpracGui::Render()
{
    // THOverlay::OnPreUpdate — BACKSPACE toggles overlay visibility
    if (InGameInputGet(SDL_SCANCODE_BACKSPACE)) s_menuOpen = !s_menuOpen;

    // THOverlay hotkeys: F1-F8 toggle cheat flags
    if (InGameInputGet(SDL_SCANCODE_F1))  invincible    = !invincible;
    if (InGameInputGet(SDL_SCANCODE_F2))  infLives      = !infLives;
    if (InGameInputGet(SDL_SCANCODE_F3))  infBombs      = !infBombs;
    if (InGameInputGet(SDL_SCANCODE_F4))  infPower      = !infPower;
    if (InGameInputGet(SDL_SCANCODE_F5))  timeLock      = !timeLock;
    if (InGameInputGet(SDL_SCANCODE_F6))  autoBomb      = !autoBomb;
    if (InGameInputGet(SDL_SCANCODE_F7))  persistentBGM = !persistentBGM;
    if (InGameInputGet(SDL_SCANCODE_F8))  showGameInfo  = !showGameInfo;

    // THGuiPrac::State(1) — sync current game state into practice menu context:
    //   mDiffculty = (int)(*((int8_t*)0x69bcb0))             → g_GameManager.difficulty
    //   mShotType  = (*((int8_t*)0x69d4bd) * 2) + ...        → CharacterShotType()
    s_diffculty = (int)g_GameManager.difficulty;
    s_shotType  = GameManager::CharacterShotType();

    // Apply cheat effects to live game state
    ApplyCheats();

    // Draw overlays
    DrawOverlay();

    if (showGameInfo)
        DrawGameInfoOverlay();

    if (keyDisplay)
        DrawKeyDisplay();

    if (s_menuOpen)
        DrawMainMenu();
}

void ThpracGui::ApplyCheats()
{
    // --- mMuteki PATCH_HK (0x4277c2, 0x42779a) ---
    // Effect: prevents bullet damage.  Source equivalent: keep grace period maxed.
    if (invincible && g_Player.bulletGracePeriod < 60)
        g_Player.bulletGracePeriod = 60;

    // --- mInfLives PATCH_HK (0x428DDB, 0x428AC6) ---
    // Effect: skip the life-decrement branches.  Source equivalent: clamp lives.
    if (infLives && g_GameManager.livesRemaining < 2)
        g_GameManager.livesRemaining = 2;

    // --- mInfBombs PATCH_HK (0x4289e3) ---
    // Effect: skip bomb-decrement.  Source equivalent: clamp bombs.
    if (infBombs && g_GameManager.bombsRemaining < 3)
        g_GameManager.bombsRemaining = 3;

    // --- mInfPower PATCH_HK (0x428B7D, 0x428B67) ---
    // Effect: skip power-decrement.  Source equivalent: keep power at max.
    if (infPower && g_GameManager.currentPower < 128)
        g_GameManager.currentPower = 128;

    // --- mTimeLock PATCH_HK (0x412DD1) ---
    // Effect: jump over rank-timer update.  Source equivalent: set isTimeStopped.
    //   *(int32_t*)(0x69d710) clamping → g_GameManager.rank is indirectly affected.
    if (timeLock)
        g_GameManager.isTimeStopped = 1;
    else if (g_GameManager.isTimeStopped == 1)
        g_GameManager.isTimeStopped = 0;

    // --- mRankLock (from THGuiPrac::PracticeMenu) ---
    // thprac: if (*mRankLock) mRank.SetBound(0,99); else mRank.SetBound(0,32);
    // Source equivalent: clamp min/max rank to the chosen rank value.
    //   *(int32_t*)(0x69d714) → g_GameManager.maxRank
    //   *(int32_t*)(0x69d718) → g_GameManager.minRank
    if (mRankLock) {
        g_GameManager.maxRank = *mRank;
        g_GameManager.minRank = *mRank;
    }

    // --- Practice mode: apply parameters at stage start ---
    // Equivalent to THGuiPrac::State(3) filling thPracParam + game hooks:
    //   thPracParam.life  = (float)*mLife  → g_GameManager.livesRemaining
    //   thPracParam.bomb  = (float)*mBomb  → g_GameManager.bombsRemaining
    //   thPracParam.power = (float)*mPower → g_GameManager.currentPower
    //   thPracParam.score = *mScore        → g_GameManager.guiScore / score
    //   thPracParam.graze = *mGraze        → g_GameManager.grazeInStage
    //   thPracParam.point = *mPoint        → g_GameManager.pointItemsCollectedInStage
    //   thPracParam.rank  = *mRank         → g_GameManager.rank
    if (s_mode == 1 &&
        g_GameManager.gameFrames == 0 &&
        g_GameManager.currentStage == s_stage)
    {
        g_GameManager.livesRemaining             = (i8)*mLife;
        g_GameManager.bombsRemaining             = (i8)*mBomb;
        g_GameManager.currentPower               = (u16)*mPower;
        g_GameManager.guiScore                   = (u32)*mScore;
        g_GameManager.score                      = (u32)*mScore;
        g_GameManager.grazeInStage               = *mGraze;
        g_GameManager.pointItemsCollectedInStage = (u16)*mPoint;
        g_GameManager.rank                       = *mRank;
        if (mRankLock) {
            g_GameManager.maxRank = *mRank;
            g_GameManager.minRank = *mRank;
        }
        g_GameManager.isInPracticeMode = 1;
    }
}

}; // namespace th06 (TH06_IMGUI_ENABLED implementation)

#else // !TH06_IMGUI_ENABLED — stub out the entire overlay

namespace th06 {

bool ThpracGui::invincible    = false;
bool ThpracGui::infLives      = false;
bool ThpracGui::infBombs      = false;
bool ThpracGui::infPower      = false;
bool ThpracGui::timeLock      = false;
bool ThpracGui::autoBomb      = false;
bool ThpracGui::persistentBGM = false;
bool ThpracGui::showGameInfo  = false;
bool ThpracGui::showRank      = false;
bool ThpracGui::keyDisplay    = false;
i32  ThpracGui::fpsMode       = 0;

void ThpracGui::Init() {}
void ThpracGui::Render() {}
void ThpracGui::ApplyCheats() {}

}; // namespace th06

#endif // TH06_IMGUI_ENABLED
