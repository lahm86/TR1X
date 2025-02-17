#include "game/creature.h"
#include "game/items.h"
#include "game/lara/common.h"
#include "game/lot.h"
#include "game/random.h"
#include "game/spawn.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/utils.h>

#define BEAR_CHARGE_DAMAGE 3
#define BEAR_SLAM_DAMAGE 200
#define BEAR_ATTACK_DAMAGE 200
#define BEAR_PAT_DAMAGE 400
#define BEAR_TOUCH 0x2406C
#define BEAR_ROAR_CHANCE 80
#define BEAR_REAR_CHANCE 768
#define BEAR_DROP_CHANCE 1536
#define BEAR_REAR_RANGE SQUARE(WALL_L * 2) // = 4194304
#define BEAR_ATTACK_RANGE SQUARE(WALL_L) // = 1048576
#define BEAR_PAT_RANGE SQUARE(600) // = 360000
#define BEAR_FIX_PAT_RANGE SQUARE(300) // = 90000
#define BEAR_RUN_TURN (5 * DEG_1) // = 910
#define BEAR_WALK_TURN (2 * DEG_1) // = 364
#define BEAR_EAT_RANGE SQUARE(WALL_L * 3 / 4) // = 589824
#define BEAR_HITPOINTS 20
#define BEAR_RADIUS (WALL_L / 3) // = 341
#define BEAR_SMARTNESS 0x4000

typedef enum {
    BEAR_STATE_STROLL = 0,
    BEAR_STATE_STOP = 1,
    BEAR_STATE_WALK = 2,
    BEAR_STATE_RUN = 3,
    BEAR_STATE_REAR = 4,
    BEAR_STATE_ROAR = 5,
    BEAR_STATE_ATTACK_1 = 6,
    BEAR_STATE_ATTACK_2 = 7,
    BEAR_STATE_EAT = 8,
    BEAR_STATE_DEATH = 9,
} BEAR_STATE;

static BITE m_BearHeadBite = { 0, 96, 335, 14 };

static void M_Setup(OBJECT *obj);
static void M_Control(int16_t item_num);

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise_func = Creature_Initialise;
    obj->control_func = M_Control;
    obj->collision_func = Creature_Collision;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->hit_points = BEAR_HITPOINTS;
    if (g_Config.gameplay.fix_bear_ai) {
        obj->pivot_length = 0;
    } else {
        obj->pivot_length = 500;
    }
    obj->radius = BEAR_RADIUS;
    obj->smartness = BEAR_SMARTNESS;
    obj->intelligent = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;

    Object_GetBone(obj, 13)->rot_y = true;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    if (item->status == IS_INVISIBLE) {
        if (!LOT_EnableBaddieAI(item_num, 0)) {
            return;
        }
        item->status = IS_ACTIVE;
    }

    CREATURE *bear = (CREATURE *)item->data;
    int16_t head = 0;
    PHD_ANGLE angle = 0;

    if (item->hit_points <= 0) {
        angle = Creature_Turn(item, DEG_1);

        switch (item->current_anim_state) {
        case BEAR_STATE_WALK:
            item->goal_anim_state = BEAR_STATE_REAR;
            break;

        case BEAR_STATE_RUN:
        case BEAR_STATE_STROLL:
            item->goal_anim_state = BEAR_STATE_STOP;
            break;

        case BEAR_STATE_REAR:
            bear->flags = 1;
            item->goal_anim_state = BEAR_STATE_DEATH;
            break;

        case BEAR_STATE_STOP:
            bear->flags = 0;
            item->goal_anim_state = BEAR_STATE_DEATH;
            break;

        case BEAR_STATE_DEATH:
            if (bear->flags && (item->touch_bits & BEAR_TOUCH)) {
                Lara_TakeDamage(BEAR_SLAM_DAMAGE, true);
                bear->flags = 0;
            }
            break;
        }
    } else {
        AI_INFO info;
        Creature_AIInfo(item, &info);

        if (info.ahead) {
            head = info.angle;
        }

        Creature_Mood(item, &info, true);

        angle = Creature_Turn(item, bear->maximum_turn);

        int dead_enemy = g_LaraItem->hit_points <= 0;
        if (item->hit_status) {
            bear->flags = 1;
        }

        switch ((int16_t)item->current_anim_state) {
        case BEAR_STATE_STOP:
            if (dead_enemy) {
                if (info.bite && info.distance < BEAR_EAT_RANGE) {
                    item->goal_anim_state = BEAR_STATE_EAT;
                } else {
                    item->goal_anim_state = BEAR_STATE_STROLL;
                }
            } else if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (bear->mood == MOOD_BORED) {
                item->goal_anim_state = BEAR_STATE_STROLL;
            } else {
                item->goal_anim_state = BEAR_STATE_RUN;
            }
            break;

        case BEAR_STATE_STROLL:
            bear->maximum_turn = BEAR_WALK_TURN;
            if (dead_enemy && (item->touch_bits & BEAR_TOUCH) && info.ahead) {
                item->goal_anim_state = BEAR_STATE_STOP;
            } else if (bear->mood != MOOD_BORED) {
                item->goal_anim_state = BEAR_STATE_STOP;
                if (bear->mood == MOOD_ESCAPE) {
                    item->required_anim_state = BEAR_STATE_STROLL;
                }
            } else if (Random_GetControl() < BEAR_ROAR_CHANCE) {
                item->required_anim_state = BEAR_STATE_ROAR;
                item->goal_anim_state = BEAR_STATE_STOP;
            }
            break;

        case BEAR_STATE_RUN:
            bear->maximum_turn = BEAR_RUN_TURN;
            if (item->touch_bits & BEAR_TOUCH) {
                Lara_TakeDamage(BEAR_CHARGE_DAMAGE, true);
            }
            if (bear->mood == MOOD_BORED || dead_enemy) {
                item->goal_anim_state = BEAR_STATE_STOP;
            } else if (info.ahead && !item->required_anim_state) {
                if (!bear->flags && info.distance < BEAR_REAR_RANGE
                    && Random_GetControl() < BEAR_REAR_CHANCE) {
                    item->required_anim_state = BEAR_STATE_REAR;
                    item->goal_anim_state = BEAR_STATE_STOP;
                } else if (info.distance < BEAR_ATTACK_RANGE) {
                    item->goal_anim_state = BEAR_STATE_ATTACK_1;
                }
            }
            break;

        case BEAR_STATE_REAR:
            if (bear->flags) {
                item->required_anim_state = BEAR_STATE_STROLL;
                item->goal_anim_state = BEAR_STATE_STOP;
            } else if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (bear->mood == MOOD_BORED || bear->mood == MOOD_ESCAPE) {
                item->goal_anim_state = BEAR_STATE_STOP;
            } else if (
                info.bite
                && info.distance
                    < (g_Config.gameplay.fix_bear_ai ? BEAR_FIX_PAT_RANGE
                                                     : BEAR_PAT_RANGE)) {
                item->goal_anim_state = BEAR_STATE_ATTACK_2;
            } else {
                item->goal_anim_state = BEAR_STATE_WALK;
            }
            break;

        case BEAR_STATE_WALK:
            if (bear->flags) {
                item->required_anim_state = BEAR_STATE_STROLL;
                item->goal_anim_state = BEAR_STATE_REAR;
            } else if (info.ahead && (item->touch_bits & BEAR_TOUCH)) {
                item->goal_anim_state = BEAR_STATE_REAR;
            } else if (bear->mood == MOOD_ESCAPE) {
                item->goal_anim_state = BEAR_STATE_REAR;
                item->required_anim_state = BEAR_STATE_STROLL;
            } else if (
                bear->mood == MOOD_BORED
                || Random_GetControl() < BEAR_ROAR_CHANCE) {
                item->required_anim_state = BEAR_STATE_ROAR;
                item->goal_anim_state = BEAR_STATE_REAR;
            } else if (
                info.distance > BEAR_REAR_RANGE
                || Random_GetControl() < BEAR_DROP_CHANCE) {
                item->required_anim_state = BEAR_STATE_STOP;
                item->goal_anim_state = BEAR_STATE_REAR;
            }
            break;

        case BEAR_STATE_ATTACK_1:
            if (!item->required_anim_state && (item->touch_bits & BEAR_TOUCH)) {
                Creature_Effect(item, &m_BearHeadBite, Spawn_Blood);
                Lara_TakeDamage(BEAR_ATTACK_DAMAGE, true);
                item->required_anim_state = BEAR_STATE_STOP;
            }
            break;

        case BEAR_STATE_ATTACK_2:
            if (!item->required_anim_state && (item->touch_bits & BEAR_TOUCH)) {
                Lara_TakeDamage(BEAR_PAT_DAMAGE, true);
                item->required_anim_state = BEAR_STATE_REAR;
            }
            break;
        }
    }

    Creature_Head(item, head);
    Creature_Animate(item_num, angle, 0);
}

REGISTER_OBJECT(O_BEAR, M_Setup)
