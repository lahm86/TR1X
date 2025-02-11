#include "game/objects/traps/spike_wall.h"

#include "game/items.h"
#include "game/lara/control.h"
#include "game/math.h"
#include "game/objects/common.h"
#include "game/room.h"
#include "game/sound.h"
#include "game/spawn.h"
#include "global/vars.h"

#include <libtrx/game/gamebuf.h>

#define SPIKE_WALL_DAMAGE 20
#define SPIKE_WALL_SPEED 1

static void M_Initialise(int16_t item_num);
static void M_Move(int16_t item_num);
static void M_Reset(int16_t item_num);
static void M_HitLara(ITEM *item);

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    GAME_VECTOR *const data =
        GameBuf_Alloc(sizeof(GAME_VECTOR), GBUF_ITEM_DATA);
    data->pos.x = item->pos.x;
    data->pos.y = item->pos.y;
    data->pos.z = item->pos.z;
    data->room_num = item->room_num;

    item->data = data;
}

static void M_Move(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    const int32_t z =
        item->pos.z + (SPIKE_WALL_SPEED * Math_Cos(item->rot.y) >> WALL_SHIFT);
    const int32_t x =
        item->pos.x + (SPIKE_WALL_SPEED * Math_Sin(item->rot.y) >> WALL_SHIFT);

    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(x, item->pos.y, z, &room_num);

    if (Room_GetHeight(sector, x, item->pos.y, z) != item->pos.y) {
        item->status = IS_DEACTIVATED;
    } else {
        item->pos.z = z;
        item->pos.x = x;
        if (room_num != item->room_num) {
            Item_NewRoom(item_num, room_num);
        }
    }

    Sound_Effect(SFX_DOOR_SLIDE, &item->pos, SPM_NORMAL);
}

static void M_Reset(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    item->status = IS_INACTIVE;
    const GAME_VECTOR *const data = item->data;
    item->pos.x = data->x;
    item->pos.y = data->y;
    item->pos.z = data->z;

    if (item->room_num != data->room_num) {
        Item_RemoveDrawn(item_num);
        ROOM *const room = Room_Get(data->room_num);
        item->next_item = room->item_num;
        room->item_num = item_num;
        item->room_num = data->room_num;
    }

    Item_RemoveActive(item_num);
}

static void M_HitLara(ITEM *const item)
{
    Lara_TakeDamage(SPIKE_WALL_DAMAGE, true);

    Spawn_BloodBath(
        g_LaraItem->pos.x, g_LaraItem->pos.y - WALL_L / 2, g_LaraItem->pos.z,
        SPIKE_WALL_SPEED, item->rot.y, g_LaraItem->room_num, 3);
    item->touch_bits = 0;

    Sound_Effect(SFX_LARA_FLESH_WOUND, &item->pos, SPM_NORMAL);
}

void SpikeWall_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    if (!Item_IsTriggerActive(item)) {
        M_Reset(item_num);
    } else if (item->status != IS_DEACTIVATED) {
        M_Move(item_num);
    }

    if (item->touch_bits) {
        M_HitLara(item);
    }
}

void SpikeWall_Setup(void)
{
    OBJECT *const obj = Object_GetObject(O_SPIKE_WALL);
    obj->control = SpikeWall_Control;
    obj->collision = Object_Collision;
    obj->initialise = M_Initialise;
    obj->save_position = 1;
    obj->save_flags = 1;
}
