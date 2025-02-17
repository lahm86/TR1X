#include "game/items.h"
#include "game/lara/common.h"
#include "game/random.h"
#include "game/sound.h"
#include "game/spawn.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/utils.h>

#define DAMOCLES_SWORD_ACTIVATE_DIST ((WALL_L * 3) / 2)
#define DAMOCLES_SWORD_DAMAGE 100

static void M_Setup(OBJECT *obj);
static void M_Initialise(int16_t item_num);
static void M_Control(int16_t item_num);
static void M_Collision(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);

static void M_Setup(OBJECT *const obj)
{
    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;
    obj->collision_func = M_Collision;
    obj->shadow_size = UNIT_SHADOW;
    obj->save_position = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;
}

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    item->rot.y = Random_GetControl();
    item->required_anim_state = (Random_GetControl() - 0x4000) / 16;
    item->fall_speed = 50;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (item->gravity) {
        item->rot.y += item->required_anim_state;
        item->fall_speed += item->fall_speed < FAST_FALL_SPEED ? GRAVITY : 1;
        item->pos.y += item->fall_speed;
        item->pos.x += item->current_anim_state;
        item->pos.z += item->goal_anim_state;

        if (item->pos.y > item->floor) {
            Sound_Effect(SFX_DAMOCLES_SWORD, &item->pos, SPM_NORMAL);
            item->pos.y = item->floor + 10;
            item->gravity = 0;
            item->status = IS_DEACTIVATED;
            Item_RemoveActive(item_num);
        }
    } else if (item->pos.y != item->floor) {
        item->rot.y += item->required_anim_state;
        int32_t x = g_LaraItem->pos.x - item->pos.x;
        int32_t y = g_LaraItem->pos.y - item->pos.y;
        int32_t z = g_LaraItem->pos.z - item->pos.z;
        if (ABS(x) <= DAMOCLES_SWORD_ACTIVATE_DIST
            && ABS(z) <= DAMOCLES_SWORD_ACTIVATE_DIST && y > 0
            && y < WALL_L * 3) {
            item->current_anim_state = x / 32;
            item->goal_anim_state = z / 32;
            item->gravity = 1;
        }
    }
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    if (!Lara_TestBoundsCollide(item, coll->radius)) {
        return;
    }
    if (coll->enable_baddie_push) {
        Lara_Push(item, coll, false, true);
    }
    if (item->gravity) {
        lara_item->hit_points -= DAMOCLES_SWORD_DAMAGE;
        int32_t x = lara_item->pos.x + (Random_GetControl() - 0x4000) / 256;
        int32_t z = lara_item->pos.z + (Random_GetControl() - 0x4000) / 256;
        int32_t y = lara_item->pos.y - Random_GetControl() / 44;
        int32_t d = lara_item->rot.y + (Random_GetControl() - 0x4000) / 8;
        Spawn_Blood(x, y, z, lara_item->speed, d, lara_item->room_num);
    }
}

REGISTER_OBJECT(O_DAMOCLES_SWORD, M_Setup)
