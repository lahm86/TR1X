#include "game/objects/creatures/wolf.h"

#include "game/creature.h"
#include "game/items.h"
#include "game/lara/common.h"
#include "game/lot.h"
#include "game/random.h"
#include "game/spawn.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/utils.h>

#define WOLF_SLEEP_FRAME 96
#define WOLF_BITE_DAMAGE 100
#define WOLF_POUNCE_DAMAGE 50
#define WOLF_DIE_ANIM 20
#define WOLF_WALK_TURN (2 * DEG_1) // = 364
#define WOLF_RUN_TURN (5 * DEG_1) // = 910
#define WOLF_STALK_TURN (2 * DEG_1) // = 364
#define WOLF_ATTACK_RANGE SQUARE(WALL_L * 3 / 2) // = 2359296
#define WOLF_STALK_RANGE SQUARE(WALL_L * 3) // = 9437184
#define WOLF_BITE_RANGE SQUARE(345) // = 119025
#define WOLF_WAKE_CHANCE 32
#define WOLF_SLEEP_CHANCE 32
#define WOLF_HOWL_CHANCE 384
#define WOLF_TOUCH 0x774F
#define WOLF_HITPOINTS 6
#define WOLF_RADIUS (WALL_L / 3) // = 341
#define WOLF_SMARTNESS 0x2000

typedef enum {
    WOLF_STATE_EMPTY = 0,
    WOLF_STATE_STOP = 1,
    WOLF_STATE_WALK = 2,
    WOLF_STATE_RUN = 3,
    WOLF_STATE_JUMP = 4,
    WOLF_STATE_STALK = 5,
    WOLF_STATE_ATTACK = 6,
    WOLF_STATE_HOWL = 7,
    WOLF_STATE_SLEEP = 8,
    WOLF_STATE_CROUCH = 9,
    WOLF_STATE_FAST_TURN = 10,
    WOLF_STATE_DEATH = 11,
    WOLF_STATE_BITE = 12,
} WOLF_STATE;

static BITE m_WolfJawBite = { 0, -14, 174, 6 };

void Wolf_Setup(OBJECT *obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise = Wolf_Initialise;
    obj->control = Wolf_Control;
    obj->collision = Creature_Collision;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->hit_points = WOLF_HITPOINTS;
    obj->pivot_length = 375;
    obj->radius = WOLF_RADIUS;
    obj->smartness = WOLF_SMARTNESS;
    obj->intelligent = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;

    Object_GetBone(obj, 2)->rot_y = true;
}

void Wolf_Initialise(int16_t item_num)
{
    Item_Get(item_num)->frame_num = WOLF_SLEEP_FRAME;
    Creature_Initialise(item_num);
}

void Wolf_Control(int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    if (item->status == IS_INVISIBLE) {
        if (!LOT_EnableBaddieAI(item_num, 0)) {
            return;
        }
        item->status = IS_ACTIVE;
    }

    CREATURE *wolf = item->data;
    int16_t head = 0;
    int16_t angle = 0;
    int16_t tilt = 0;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != WOLF_STATE_DEATH) {
            item->current_anim_state = WOLF_STATE_DEATH;
            Item_SwitchToAnim(
                item, WOLF_DIE_ANIM + (int16_t)(Random_GetControl() / 11000),
                0);
        }
    } else {
        AI_INFO info;
        Creature_AIInfo(item, &info);

        if (info.ahead) {
            head = info.angle;
        }

        Creature_Mood(item, &info, false);

        angle = Creature_Turn(item, wolf->maximum_turn);

        switch (item->current_anim_state) {
        case WOLF_STATE_SLEEP:
            head = 0;
            if (wolf->mood == MOOD_ESCAPE || info.zone_num == info.enemy_zone) {
                item->required_anim_state = WOLF_STATE_CROUCH;
                item->goal_anim_state = WOLF_STATE_STOP;
            } else if (Random_GetControl() < WOLF_WAKE_CHANCE) {
                item->required_anim_state = WOLF_STATE_WALK;
                item->goal_anim_state = WOLF_STATE_STOP;
            }
            break;

        case WOLF_STATE_STOP:
            if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else {
                item->goal_anim_state = WOLF_STATE_WALK;
            }
            break;

        case WOLF_STATE_WALK:
            wolf->maximum_turn = WOLF_WALK_TURN;
            if (wolf->mood != MOOD_BORED) {
                item->goal_anim_state = WOLF_STATE_STALK;
                item->required_anim_state = WOLF_STATE_EMPTY;
            } else if (Random_GetControl() < WOLF_SLEEP_CHANCE) {
                item->required_anim_state = WOLF_STATE_SLEEP;
                item->goal_anim_state = WOLF_STATE_STOP;
            }
            break;

        case WOLF_STATE_CROUCH:
            if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (wolf->mood == MOOD_ESCAPE) {
                item->goal_anim_state = WOLF_STATE_RUN;
            } else if (info.distance < WOLF_BITE_RANGE && info.bite) {
                item->goal_anim_state = WOLF_STATE_BITE;
            } else if (wolf->mood == MOOD_STALK) {
                item->goal_anim_state = WOLF_STATE_STALK;
            } else if (wolf->mood == MOOD_BORED) {
                item->goal_anim_state = WOLF_STATE_STOP;
            } else {
                item->goal_anim_state = WOLF_STATE_RUN;
            }
            break;

        case WOLF_STATE_STALK:
            wolf->maximum_turn = WOLF_STALK_TURN;
            if (wolf->mood == MOOD_ESCAPE) {
                item->goal_anim_state = WOLF_STATE_RUN;
            } else if (info.distance < WOLF_BITE_RANGE && info.bite) {
                item->goal_anim_state = WOLF_STATE_BITE;
            } else if (info.distance > WOLF_STALK_RANGE) {
                item->goal_anim_state = WOLF_STATE_RUN;
            } else if (wolf->mood == MOOD_ATTACK) {
                if (!info.ahead || info.distance > WOLF_ATTACK_RANGE
                    || (info.enemy_facing < FRONT_ARC
                        && info.enemy_facing > -FRONT_ARC)) {
                    item->goal_anim_state = WOLF_STATE_RUN;
                }
            } else if (Random_GetControl() < WOLF_HOWL_CHANCE) {
                item->required_anim_state = WOLF_STATE_HOWL;
                item->goal_anim_state = WOLF_STATE_CROUCH;
            } else if (wolf->mood == MOOD_BORED) {
                item->goal_anim_state = WOLF_STATE_CROUCH;
            }
            break;

        case WOLF_STATE_RUN:
            wolf->maximum_turn = WOLF_RUN_TURN;
            tilt = angle;
            if (info.ahead && info.distance < WOLF_ATTACK_RANGE) {
                if (info.distance > (WOLF_ATTACK_RANGE / 2)
                    && (info.enemy_facing > FRONT_ARC
                        || info.enemy_facing < -FRONT_ARC)) {
                    item->required_anim_state = WOLF_STATE_STALK;
                    item->goal_anim_state = WOLF_STATE_CROUCH;
                } else {
                    item->goal_anim_state = WOLF_STATE_ATTACK;
                    item->required_anim_state = WOLF_STATE_EMPTY;
                }
            } else if (
                wolf->mood == MOOD_STALK && info.distance < WOLF_STALK_RANGE) {
                item->required_anim_state = WOLF_STATE_STALK;
                item->goal_anim_state = WOLF_STATE_CROUCH;
            } else if (wolf->mood == MOOD_BORED) {
                item->goal_anim_state = WOLF_STATE_CROUCH;
            }
            break;

        case WOLF_STATE_ATTACK:
            tilt = angle;
            if (item->required_anim_state == WOLF_STATE_EMPTY
                && (item->touch_bits & WOLF_TOUCH)) {
                Creature_Effect(item, &m_WolfJawBite, Spawn_Blood);
                Lara_TakeDamage(WOLF_POUNCE_DAMAGE, true);
                item->required_anim_state = WOLF_STATE_RUN;
            }
            item->goal_anim_state = WOLF_STATE_RUN;
            break;

        case WOLF_STATE_BITE:
            if (item->required_anim_state == WOLF_STATE_EMPTY
                && (item->touch_bits & WOLF_TOUCH) && info.ahead) {
                Creature_Effect(item, &m_WolfJawBite, Spawn_Blood);
                Lara_TakeDamage(WOLF_BITE_DAMAGE, true);
                item->required_anim_state = WOLF_STATE_CROUCH;
            }
            break;
        }
    }

    Creature_Tilt(item, tilt);
    Creature_Head(item, head);
    Creature_Animate(item_num, angle, tilt);
}
