#include "game/ui/widgets/stats_dialog.h"

#include "game/game_string.h"
#include "game/stats.h"
#include "global/vars.h"

#include <libtrx/game/ui/common.h>
#include <libtrx/game/ui/widgets/label.h>
#include <libtrx/game/ui/widgets/stack.h>
#include <libtrx/game/ui/widgets/window.h>
#include <libtrx/memory.h>

#include <stdio.h>

typedef struct {
    UI_WIDGET *stack;
    UI_WIDGET *left_label;
    UI_WIDGET *right_label;
} M_ROW;

typedef struct {
    UI_WIDGET_VTABLE vtable;
    UI_WIDGET *window;
    UI_WIDGET *outer_stack;
    int32_t row_count;
    M_ROW *rows;
} UI_STATS_DIALOG;

static void M_AddRow(
    UI_STATS_DIALOG *self, const char *left_text, const char *right_text);
static void M_AddLevelStatsRows(UI_STATS_DIALOG *self);
static void M_AddFinalStatsRows(UI_STATS_DIALOG *self);
static void M_DoLayout(UI_STATS_DIALOG *self);

static int32_t M_GetWidth(const UI_STATS_DIALOG *self);
static int32_t M_GetHeight(const UI_STATS_DIALOG *self);
static void M_SetPosition(UI_STATS_DIALOG *self, int32_t x, int32_t y);
static void M_Control(UI_STATS_DIALOG *self);
static void M_Draw(UI_STATS_DIALOG *self);
static void M_Free(UI_STATS_DIALOG *self);

static void M_AddRow(
    UI_STATS_DIALOG *const self, const char *const left_text,
    const char *const right_text)
{
    self->row_count++;
    self->rows = Memory_Realloc(self->rows, sizeof(M_ROW) * self->row_count);
    M_ROW *const row = &self->rows[self->row_count - 1];

    row->left_label = UI_Label_Create(left_text, UI_LABEL_AUTO_SIZE, 18);
    row->right_label = UI_Label_Create(right_text, UI_LABEL_AUTO_SIZE, 18);
    row->stack =
        UI_Stack_Create(UI_STACK_LAYOUT_HORIZONTAL, 290, UI_STACK_AUTO_SIZE);
    UI_Stack_SetHAlign(row->stack, UI_STACK_H_ALIGN_DISTRIBUTE);
    UI_Stack_AddChild(row->stack, row->left_label);
    UI_Stack_AddChild(row->stack, row->right_label);
    UI_Stack_AddChild(self->outer_stack, row->stack);
}

static void M_AddLevelStatsRows(UI_STATS_DIALOG *const self)
{
    const STATISTICS_INFO stats = g_SaveGame.statistics;
    char buf[32];

    // time
    const int32_t sec = stats.timer / FRAMES_PER_SECOND;
    sprintf(buf, "%02d:%02d:%02d", (sec / 60) / 60, (sec / 60) % 60, sec % 60);
    M_AddRow(self, g_GF_GameStrings[GF_S_GAME_MISC_TIME_TAKEN], buf);

    // secrets
    if (g_GF_NumSecrets != 0) {
        char *ptr = buf;
        int32_t num_secrets = 0;
        for (int32_t i = 0; i < 3; i++) {
            if (stats.secrets & (1 << i)) {
                sprintf(ptr, "\\{secret %d}", i + 1);
                num_secrets++;
            } else {
                strcpy(ptr, "   ");
            }
            ptr += strlen(ptr);
        }
        *ptr++ = '\0';
        if (num_secrets == 0) {
            sprintf(buf, g_GF_GameStrings[GF_S_GAME_MISC_NONE]);
        }
        M_AddRow(self, g_GF_GameStrings[GF_S_GAME_MISC_SECRETS_FOUND], buf);
    }

    // kills
    sprintf(buf, "%d", stats.kills);
    M_AddRow(self, g_GF_GameStrings[GF_S_GAME_MISC_KILLS], buf);

    // ammo used
    sprintf(buf, "%d", stats.shots);
    M_AddRow(self, g_GF_GameStrings[GF_S_GAME_MISC_AMMO_USED], buf);

    // ammo hits
    sprintf(buf, "%d", stats.hits);
    M_AddRow(self, g_GF_GameStrings[GF_S_GAME_MISC_HITS], buf);

    // medipacks
    if ((stats.medipacks & 1) != 0) {
        sprintf(buf, "%d.5", stats.medipacks >> 1);
    } else {
        sprintf(buf, "%d.0", stats.medipacks >> 1);
    }
    M_AddRow(self, g_GF_GameStrings[GF_S_GAME_MISC_HEALTH_PACKS_USED], buf);

    // distance traveled
    const int32_t distance = stats.distance / 445;
    if (distance < 1000) {
        sprintf(buf, "%dm", distance);
    } else {
        sprintf(buf, "%d.%02dkm", distance / 1000, distance % 100);
    }
    M_AddRow(self, g_GF_GameStrings[GF_S_GAME_MISC_DISTANCE_TRAVELLED], buf);
}

static void M_AddFinalStatsRows(UI_STATS_DIALOG *const self)
{
    const FINAL_STATS stats = Stats_ComputeFinalStats();
    char buf[32];

    // time
    const int32_t sec = stats.timer / FRAMES_PER_SECOND;
    sprintf(buf, "%02d:%02d:%02d", (sec / 60) / 60, (sec / 60) % 60, sec % 60);
    M_AddRow(self, g_GF_GameStrings[GF_S_GAME_MISC_TIME_TAKEN], buf);

    // secrets
    sprintf(
        buf, "%d %s %d", stats.found_secrets,
        g_GF_GameStrings[GF_S_GAME_MISC_OF], stats.total_secrets);
    M_AddRow(self, g_GF_GameStrings[GF_S_GAME_MISC_SECRETS_FOUND], buf);

    // kills
    sprintf(buf, "%d", stats.kills);
    M_AddRow(self, g_GF_GameStrings[GF_S_GAME_MISC_KILLS], buf);

    // ammo used
    sprintf(buf, "%d", stats.ammo_used);
    M_AddRow(self, g_GF_GameStrings[GF_S_GAME_MISC_AMMO_USED], buf);

    // ammo hits
    sprintf(buf, "%d", stats.ammo_hits);
    M_AddRow(self, g_GF_GameStrings[GF_S_GAME_MISC_HITS], buf);

    // medipacks
    if ((stats.medipacks & 1) != 0) {
        sprintf(buf, "%d.5", stats.medipacks >> 1);
    } else {
        sprintf(buf, "%d.0", stats.medipacks >> 1);
    }
    M_AddRow(self, g_GF_GameStrings[GF_S_GAME_MISC_HEALTH_PACKS_USED], buf);

    // distance traveled
    const int32_t distance = stats.distance / 445;
    if (distance < 1000) {
        sprintf(buf, "%dm", distance);
    } else {
        sprintf(buf, "%d.%02dkm", distance / 1000, distance % 100);
    }
    M_AddRow(self, g_GF_GameStrings[GF_S_GAME_MISC_DISTANCE_TRAVELLED], buf);
}

static void M_DoLayout(UI_STATS_DIALOG *const self)
{
    M_SetPosition(
        self, (UI_GetCanvasWidth() - M_GetWidth(self)) / 2,
        (UI_GetCanvasHeight() - M_GetHeight(self)) - 50);
}

static int32_t M_GetWidth(const UI_STATS_DIALOG *const self)
{
    return self->window->get_width(self->window);
}

static int32_t M_GetHeight(const UI_STATS_DIALOG *const self)
{
    return self->window->get_height(self->window);
}

static void M_SetPosition(
    UI_STATS_DIALOG *const self, const int32_t x, const int32_t y)
{
    return self->window->set_position(self->window, x, y);
}

static void M_Control(UI_STATS_DIALOG *const self)
{
    if (self->window->control != NULL) {
        self->window->control(self->window);
    }
}

static void M_Draw(UI_STATS_DIALOG *const self)
{
    if (self->window->draw != NULL) {
        self->window->draw(self->window);
    }
}

static void M_Free(UI_STATS_DIALOG *const self)
{
    for (int32_t i = 0; i < self->row_count; i++) {
        self->rows[i].left_label->free(self->rows[i].left_label);
        self->rows[i].right_label->free(self->rows[i].right_label);
        self->rows[i].stack->free(self->rows[i].stack);
    }
    self->outer_stack->free(self->outer_stack);
    self->window->free(self->window);
    Memory_Free(self);
}

UI_WIDGET *UI_StatsDialog_Create(const bool show_final_stats)
{
    UI_STATS_DIALOG *const self = Memory_Alloc(sizeof(UI_STATS_DIALOG));
    self->vtable = (UI_WIDGET_VTABLE) {
        .get_width = (UI_WIDGET_GET_WIDTH)M_GetWidth,
        .get_height = (UI_WIDGET_GET_HEIGHT)M_GetHeight,
        .set_position = (UI_WIDGET_SET_POSITION)M_SetPosition,
        .control = (UI_WIDGET_CONTROL)M_Control,
        .draw = (UI_WIDGET_DRAW)M_Draw,
        .free = (UI_WIDGET_FREE)M_Free,
    };

    self->outer_stack = UI_Stack_Create(
        UI_STACK_LAYOUT_VERTICAL, UI_STACK_AUTO_SIZE, UI_STACK_AUTO_SIZE);

    self->window = UI_Window_Create(self->outer_stack, 8, 8, 8, 8);

    if (show_final_stats) {
        UI_Window_SetTitle(
            self->window, g_GF_GameStrings[GF_S_GAME_MISC_FINAL_STATISTICS]);
        M_AddFinalStatsRows(self);
    } else {
        UI_Window_SetTitle(self->window, g_GF_LevelNames[g_CurrentLevel]);
        M_AddLevelStatsRows(self);
    }

    M_DoLayout(self);
    return (UI_WIDGET *)self;
}
