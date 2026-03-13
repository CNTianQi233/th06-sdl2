#pragma once
#include "diffbuild.hpp"
#include "inttypes.hpp"

namespace th06
{
// -----------------------------------------------------------------------
// ThpracGui — thprac-style in-game overlay for th06 source build
//
// F1          : 无敌
// F2          : 锁残
// F3          : 锁Bomb
// F4          : 锁火力
// F5          : 锁时 (rank timer freeze)
// F6          : 自动B  [TODO: needs Player hook]
// F7          : 永续BGM [TODO: needs MidiOutput hook]
// F8          : 游戏信息 overlay
// BACKSPACE   : 开/关主菜单 (练习选项 / 高级选项)
//
// Ported from thprac by Ack. Hardcoded memory addresses replaced with
// source-level references into g_GameManager / g_Player / g_Supervisor.
// -----------------------------------------------------------------------

struct ThpracGui
{
    static void Init();
    static void Render();
    static void ApplyCheats();

    // --- In-game cheat flags (toggled via F1-F8) ---
    // In thprac these are GuiHotKey + HOTKEY_DEFINE/PATCH_HK members of THOverlay.
    // Here they are plain bool flags; ApplyCheats() implements the effects.
    static bool invincible;    // F1  (thprac: mMuteki)
    static bool infLives;      // F2  (thprac: mInfLives)
    static bool infBombs;      // F3  (thprac: mInfBombs)
    static bool infPower;      // F4  (thprac: mInfPower)
    static bool timeLock;      // F5  (thprac: mTimeLock)
    static bool autoBomb;      // F6  (thprac: mAutoBomb)   [TODO]
    static bool persistentBGM; // F7  (thprac: mElBgm)      [TODO]
    static bool showGameInfo;  // F8  (thprac: mShowSpellCapture)

    // --- Advanced display options (thprac: g_adv_igi_options / THAdvOptWnd) ---
    static bool showRank;   // show rank in game-info overlay
    static bool keyDisplay; // show input display at bottom of screen
    static i32  fpsMode;    // 0=60fps  1=30fps  2=15fps
};

DIFFABLE_EXTERN(ThpracGui, g_ThpracGui)
}; // namespace th06
