#include "game/objects/general/lift.h"

#include "game/gamebuf.h"
#include "game/items.h"
#include "game/math.h"
#include "game/room.h"
#include "global/vars.h"

#define LIFT_WAIT_TIME (3 * FRAMES_PER_SECOND) // = 90
#define LIFT_SHIFT 16
#define LIFT_HEIGHT (STEP_L * 5) // = 1280
#define LIFT_TRAVEL_DIST (STEP_L * 22)

typedef enum {
    LIFT_STATE_DOOR_CLOSED = 0,
    LIFT_STATE_DOOR_OPEN = 1,
} LIFT_STATE;

void Lift_Setup(void)
{
    OBJECT *const obj = Object_GetObject(O_LIFT);
    obj->initialise = Lift_Initialise;
    obj->control = Lift_Control;
    obj->ceiling = Lift_Ceiling;
    obj->floor = Lift_Floor;
    obj->save_position = 1;
    obj->save_flags = 1;
    obj->save_anim = 1;
}

void __cdecl Lift_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    LIFT_INFO *const lift_data =
        GameBuf_Alloc(sizeof(LIFT_INFO), GBUF_TEMP_ALLOC);
    lift_data->start_height = item->pos.y;
    lift_data->wait_time = 0;

    item->data = lift_data;
}

void __cdecl Lift_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    LIFT_INFO *const lift_data = item->data;

    if (Item_IsTriggerActive(item)) {
        if (item->pos.y
            < lift_data->start_height + LIFT_TRAVEL_DIST - LIFT_SHIFT) {
            if (lift_data->wait_time < LIFT_WAIT_TIME) {
                item->goal_anim_state = LIFT_STATE_DOOR_OPEN;
                lift_data->wait_time++;
            } else {
                item->goal_anim_state = LIFT_STATE_DOOR_CLOSED;
                item->pos.y += LIFT_SHIFT;
            }
        } else {
            item->goal_anim_state = LIFT_STATE_DOOR_OPEN;
            lift_data->wait_time = 0;
        }
    } else {
        if (item->pos.y > lift_data->start_height + LIFT_SHIFT) {
            if (lift_data->wait_time < LIFT_WAIT_TIME) {
                item->goal_anim_state = LIFT_STATE_DOOR_OPEN;
                lift_data->wait_time++;
            } else {
                item->goal_anim_state = LIFT_STATE_DOOR_CLOSED;
                item->pos.y -= LIFT_SHIFT;
            }
        } else {
            item->goal_anim_state = LIFT_STATE_DOOR_OPEN;
            lift_data->wait_time = 0;
        }
    }

    Item_Animate(item);

    int16_t room_num = item->room_num;
    Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
    if (item->room_num != room_num) {
        Item_NewRoom(item_num, room_num);
    }
}

void __cdecl Lift_FloorCeiling(
    const ITEM *const item, const int32_t x, const int32_t y, const int32_t z,
    int32_t *const out_floor, int32_t *const out_ceiling)
{
    const XZ_32 lift_tile = {
        .x = item->pos.x >> WALL_SHIFT,
        .z = item->pos.z >> WALL_SHIFT,
    };

    const XZ_32 lara_tile = {
        .x = g_LaraItem->pos.x >> WALL_SHIFT,
        .z = g_LaraItem->pos.z >> WALL_SHIFT,
    };

    const XZ_32 test_tile = {
        .x = x >> WALL_SHIFT,
        .z = z >> WALL_SHIFT,
    };

    const DIRECTION direction = Math_GetDirection(item->rot.y);
    int32_t dx = 0;
    int32_t dz = 0;
    switch (direction) {
    case DIR_NORTH:
        dx = -1;
        dz = 1;
        break;
    case DIR_EAST:
        dx = 1;
        dz = 1;
        break;
    case DIR_SOUTH:
        dx = 1;
        dz = -1;
        break;
    case DIR_WEST:
        dx = -1;
        dz = -1;
        break;
    default:
        break;
    }

    // clang-format off
    const bool point_in_shaft =
        (test_tile.x == lift_tile.x || test_tile.x + dx == lift_tile.x) &&
        (test_tile.z == lift_tile.z || test_tile.z + dz == lift_tile.z);

    const bool lara_in_shaft =
        (lara_tile.x == lift_tile.x || lara_tile.x + dx == lift_tile.x) &&
        (lara_tile.z == lift_tile.z || lara_tile.z + dz == lift_tile.z);
    // clang-format on

    const int32_t lift_floor = item->pos.y;
    const int32_t lift_ceiling = item->pos.y - LIFT_HEIGHT;

    *out_floor = 0x7FFF;
    *out_ceiling = -0x7FFF;

    if (lara_in_shaft) {
        if (item->current_anim_state == LIFT_STATE_DOOR_CLOSED
            && g_LaraItem->pos.y < lift_floor + STEP_L
            && g_LaraItem->pos.y > lift_ceiling + STEP_L) {
            if (point_in_shaft) {
                *out_floor = lift_floor;
                *out_ceiling = lift_ceiling + STEP_L;
            } else {
                *out_floor = NO_HEIGHT;
                *out_ceiling = 0x7FFF;
            }
        } else if (point_in_shaft) {
            if (g_LaraItem->pos.y < lift_ceiling + STEP_L) {
                *out_floor = lift_ceiling;
            } else if (g_LaraItem->pos.y < lift_floor + STEP_L) {
                *out_floor = lift_floor;
                *out_ceiling = lift_ceiling + STEP_L;
            } else {
                *out_ceiling = lift_floor + STEP_L;
            }
        }
    } else if (point_in_shaft) {
        if (y <= lift_ceiling) {
            *out_floor = lift_ceiling;
        } else if (y >= lift_floor + STEP_L) {
            *out_ceiling = lift_floor + STEP_L;
        } else if (item->current_anim_state == LIFT_STATE_DOOR_OPEN) {
            *out_floor = lift_floor;
            *out_ceiling = lift_ceiling + STEP_L;
        } else {
            *out_floor = NO_HEIGHT;
            *out_ceiling = 0x7FFF;
        }
    }
}

void __cdecl Lift_Floor(
    const ITEM *const item, const int32_t x, const int32_t y, const int32_t z,
    int32_t *const out_height)
{
    int32_t floor;
    int32_t height;
    Lift_FloorCeiling(item, x, y, z, &floor, &height);
    if (floor < *out_height) {
        *out_height = floor;
    }
}

void __cdecl Lift_Ceiling(
    const ITEM *const item, const int32_t x, const int32_t y, const int32_t z,
    int32_t *const out_height)
{
    int32_t floor;
    int32_t height;
    Lift_FloorCeiling(item, x, y, z, &floor, &height);
    if (height > *out_height) {
        *out_height = height;
    }
}
