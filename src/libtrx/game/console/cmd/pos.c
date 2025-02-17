#include "game/console/common.h"
#include "game/console/registry.h"
#include "game/const.h"
#include "game/game.h"
#include "game/game_flow/common.h"
#include "game/game_string.h"
#include "game/lara/common.h"
#include "game/objects/common.h"
#include "strings.h"

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *ctx);

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *const ctx)
{
    if (!String_IsEmpty(ctx->args)) {
        return CR_BAD_INVOCATION;
    }

    const OBJECT *const obj = Object_Get(O_LARA);
    if (!obj->loaded) {
        return CR_UNAVAILABLE;
    }

    const ITEM *const lara_item = Lara_GetItem();

    // clang-format off
    Console_Log(
        GS(OSD_POS_GET),
        GF_GetCurrentLevel()->num,
        GF_GetCurrentLevel()->title,
        lara_item->room_num,
        lara_item->pos.x / (float)WALL_L,
        lara_item->pos.y / (float)WALL_L,
        lara_item->pos.z / (float)WALL_L,
        lara_item->rot.x * 360.0f / (float)DEG_360,
        lara_item->rot.y * 360.0f / (float)DEG_360,
        lara_item->rot.z * 360.0f / (float)DEG_360);
    // clang-format on

    return CR_SUCCESS;
}

REGISTER_CONSOLE_COMMAND("pos", M_Entrypoint)
