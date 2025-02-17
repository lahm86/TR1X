#pragma once

#include "global/types.h"

#include <libtrx/game/savegame.h>

#include <stdint.h>

// Loading a saved game is divided into two phases. First, the game reads the
// savegame file contents to look for the level number. The rest of the save
// data is stored in a special buffer in the g_GameInfo. Then the engine
// continues to execute the normal game flow and loads the specified level.
// Second phase occurs after everything finishes loading, e.g. items,
// creatures, triggers etc., and is what actually sets Lara's health, creatures
// status, triggers, inventory etc.

#define SAVEGAME_CURRENT_VERSION 6

typedef enum {
    VERSION_LEGACY = -1,
    VERSION_0 = 0,
    VERSION_1 = 1,
    VERSION_2 = 2,
    VERSION_3 = 3,
    VERSION_4 = 4,
    VERSION_5 = 5,
    VERSION_6 = 6,
} SAVEGAME_VERSION;

typedef enum {
    SAVEGAME_FORMAT_LEGACY = 1,
    SAVEGAME_FORMAT_BSON = 2,
} SAVEGAME_FORMAT;

typedef struct {
    SAVEGAME_FORMAT format;
    char *full_path;
    int32_t counter;
    int32_t level_num;
    char *level_title;
    int16_t initial_version;
    struct {
        bool restart;
        bool select_level;
    } features;
} SAVEGAME_INFO;

void Savegame_Init(void);
void Savegame_Shutdown(void);
bool Savegame_IsInitialised(void);

void Savegame_InitCurrentInfo(void);

int32_t Savegame_GetLevelNumber(int32_t slot_num);

bool Savegame_Load(int32_t slot_num);
bool Savegame_Save(int32_t slot_num);
bool Savegame_UpdateDeathCounters(int32_t slot_num, GAME_INFO *game_info);
bool Savegame_LoadOnlyResumeInfo(int32_t slot_num, GAME_INFO *game_info);

void Savegame_ScanSavedGames(void);
void Savegame_ScanAvailableLevels(REQUEST_INFO *req);
void Savegame_HighlightNewestSlot(void);
bool Savegame_RestartAvailable(int32_t slot_num);

RESUME_INFO *Savegame_GetCurrentInfo(const GF_LEVEL *level);

void Savegame_ApplyLogicToCurrentInfo(const GF_LEVEL *level);
void Savegame_ResetCurrentInfo(const GF_LEVEL *level);
void Savegame_CarryCurrentInfoToNextLevel(
    const GF_LEVEL *src_level, const GF_LEVEL *dst_level);

// Persist Lara's inventory to the current info.
// Used to carry over Lara's inventory between levels.
void Savegame_PersistGameToCurrentInfo(const GF_LEVEL *level);

void Savegame_ProcessItemsBeforeLoad(void);
void Savegame_ProcessItemsBeforeSave(void);
