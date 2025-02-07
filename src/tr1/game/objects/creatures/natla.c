#include "game/objects/creatures/natla.h"

#include "game/creature.h"
#include "game/effects.h"
#include "game/items.h"
#include "game/lot.h"
#include "game/music.h"
#include "game/random.h"
#include "game/room.h"
#include "game/sound.h"
#include "game/spawn.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/math.h>

#define NATLA_SHOT_DAMAGE 100
#define NATLA_NEAR_DEATH 200
#define NATLA_FLY_MODE 0x8000
#define NATLA_TIMER 0x7FFF
#define NATLA_FIRE_ARC (DEG_1 * 30) // = 5460
#define NATLA_FLY_TURN (DEG_1 * 5) // = 910
#define NATLA_RUN_TURN (DEG_1 * 6) // = 1092
#define NATLA_LAND_CHANCE 256
#define NATLA_DIE_TIME (LOGIC_FPS * 16) // = 480
#define NATLA_GUN_SPEED 400
#define NATLA_HITPOINTS 400
#define NATLA_RADIUS (WALL_L / 5) // = 204
#define NATLA_SMARTNESS 0x7FFF

typedef enum {
    NATLA_STATE_EMPTY = 0,
    NATLA_STATE_STOP = 1,
    NATLA_STATE_FLY = 2,
    NATLA_STATE_RUN = 3,
    NATLA_STATE_AIM = 4,
    NATLA_STATE_SEMIDEATH = 5,
    NATLA_STATE_SHOOT = 6,
    NATLA_STATE_FALL = 7,
    NATLA_STATE_STAND = 8,
    NATLA_STATE_DEATH = 9,
} NATLA_STATE;

static BITE m_NatlaGun = { 5, 220, 7, 4 };

void Natla_Setup(OBJECT *obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->collision = Creature_Collision;
    obj->initialise = Creature_Initialise;
    obj->control = Natla_Control;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->hit_points = NATLA_HITPOINTS;
    obj->radius = NATLA_RADIUS;
    obj->smartness = NATLA_SMARTNESS;
    obj->intelligent = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;

    Object_GetBone(obj, 2)->rot_x = true;
    Object_GetBone(obj, 2)->rot_z = true;
}

void Natla_Control(int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    if (item->status == IS_INVISIBLE) {
        if (!LOT_EnableBaddieAI(item_num, 0)) {
            return;
        }
        item->status = IS_ACTIVE;
    }

    CREATURE *natla = item->data;
    int16_t head = 0;
    int16_t angle = 0;
    int16_t tilt = 0;
    int16_t gun = natla->head_rotation * 7 / 8;
    int16_t timer = natla->flags & NATLA_TIMER;
    int16_t facing = (int16_t)(intptr_t)item->priv;

    if (item->hit_points <= 0 && item->hit_points > DONT_TARGET) {
        item->goal_anim_state = NATLA_STATE_DEATH;
    } else if (item->hit_points <= NATLA_NEAR_DEATH) {
        natla->lot.step = STEP_L;
        natla->lot.drop = -STEP_L;
        natla->lot.fly = 0;

        AI_INFO info;
        Creature_AIInfo(item, &info);

        if (info.ahead && item->current_anim_state != NATLA_STATE_SEMIDEATH) {
            head = info.angle;
        }

        Creature_Mood(item, &info, true);

        angle = Creature_Turn(item, NATLA_RUN_TURN);

        int8_t shoot = info.angle > -NATLA_FIRE_ARC
            && info.angle < NATLA_FIRE_ARC
            && Creature_CanTargetEnemy(item, &info);

        if (facing) {
            item->rot.y += facing;
            facing = 0;
        }

        switch (item->current_anim_state) {
        case NATLA_STATE_FALL:
            if (item->pos.y < item->floor) {
                item->gravity = 1;
                item->speed = 0;
            } else {
                item->gravity = 0;
                item->goal_anim_state = NATLA_STATE_SEMIDEATH;
                item->pos.y = item->floor;
                timer = 0;
            }
            break;

        case NATLA_STATE_STAND:
            if (!shoot) {
                item->goal_anim_state = NATLA_STATE_RUN;
            }
            if (timer >= 20) {
                int16_t effect_num =
                    Creature_Effect(item, &m_NatlaGun, Spawn_ShardGun);
                if (effect_num != NO_EFFECT) {
                    EFFECT *effect = Effect_Get(effect_num);
                    gun = effect->rot.x;
                    Sound_Effect(
                        SFX_ATLANTEAN_NEEDLE, &effect->pos, SPM_NORMAL);
                }
                timer = 0;
            }
            break;

        case NATLA_STATE_RUN:
            tilt = angle;
            if (timer >= 20) {
                int16_t effect_num =
                    Creature_Effect(item, &m_NatlaGun, Spawn_ShardGun);
                if (effect_num != NO_EFFECT) {
                    EFFECT *effect = Effect_Get(effect_num);
                    gun = effect->rot.x;
                    Sound_Effect(
                        SFX_ATLANTEAN_NEEDLE, &effect->pos, SPM_NORMAL);
                }
                timer = 0;
            }
            if (shoot) {
                item->goal_anim_state = NATLA_STATE_STAND;
            }
            break;

        case NATLA_STATE_SEMIDEATH:
            if (timer == NATLA_DIE_TIME) {
                item->goal_anim_state = NATLA_STATE_STAND;
                natla->flags = 0;
                timer = 0;
                item->hit_points = NATLA_NEAR_DEATH;
                Music_Play(MX_NATLA_SPEECH, MPM_TRACKED);
            } else {
                if (g_Config.gameplay.target_mode == TLM_SEMI
                    || g_Config.gameplay.target_mode == TLM_NONE) {
                    g_Lara.target = nullptr;
                }
                item->hit_points = DONT_TARGET;
            }
            break;

        case NATLA_STATE_FLY:
            item->goal_anim_state = NATLA_STATE_FALL;
            timer = 0;
            break;

        case NATLA_STATE_STOP:
        case NATLA_STATE_AIM:
        case NATLA_STATE_SHOOT:
            item->goal_anim_state = NATLA_STATE_SEMIDEATH;
            item->flags = 0;
            timer = 0;
            break;
        }
    } else {
        natla->lot.step = STEP_L;
        natla->lot.drop = -STEP_L;
        natla->lot.fly = 0;

        AI_INFO info;
        Creature_AIInfo(item, &info);

        int8_t shoot = info.angle > -NATLA_FIRE_ARC
            && info.angle < NATLA_FIRE_ARC
            && Creature_CanTargetEnemy(item, &info);
        if (item->current_anim_state == NATLA_STATE_FLY
            && (natla->flags & NATLA_FLY_MODE)) {
            if (shoot && Random_GetControl() < NATLA_LAND_CHANCE) {
                natla->flags &= ~NATLA_FLY_MODE;
            }
            if (!(natla->flags & NATLA_FLY_MODE)) {
                Creature_Mood(item, &info, true);
            }
            natla->lot.step = WALL_L * 20;
            natla->lot.drop = -WALL_L * 20;
            natla->lot.fly = STEP_L / 8;
            Creature_AIInfo(item, &info);
        } else if (!shoot) {
            natla->flags |= NATLA_FLY_MODE;
        }

        if (info.ahead) {
            head = info.angle;
        }

        if (item->current_anim_state != NATLA_STATE_FLY
            || (natla->flags & NATLA_FLY_MODE)) {
            Creature_Mood(item, &info, false);
        }

        item->rot.y -= facing;
        angle = Creature_Turn(item, NATLA_FLY_TURN);

        if (item->current_anim_state == NATLA_STATE_FLY) {
            if (info.angle > NATLA_FLY_TURN) {
                facing += NATLA_FLY_TURN;
            } else if (info.angle < -NATLA_FLY_TURN) {
                facing -= NATLA_FLY_TURN;
            } else {
                facing += info.angle;
            }
            item->rot.y += facing;
        } else {
            item->rot.y += facing - angle;
            facing = 0;
        }

        switch (item->current_anim_state) {
        case NATLA_STATE_STOP:
            timer = 0;
            if (natla->flags & NATLA_FLY_MODE) {
                item->goal_anim_state = NATLA_STATE_FLY;
            } else {
                item->goal_anim_state = NATLA_STATE_AIM;
            }
            break;

        case NATLA_STATE_FLY:
            if (!(natla->flags & NATLA_FLY_MODE)
                && item->pos.y == item->floor) {
                item->goal_anim_state = NATLA_STATE_STOP;
            }
            if (timer >= 30) {
                int16_t effect_num =
                    Creature_Effect(item, &m_NatlaGun, Spawn_RocketGun);
                if (effect_num != NO_EFFECT) {
                    EFFECT *effect = Effect_Get(effect_num);
                    gun = effect->rot.x;
                    Sound_Effect(
                        SFX_ATLANTEAN_NEEDLE, &effect->pos, SPM_NORMAL);
                }
                timer = 0;
            }
            break;

        case NATLA_STATE_AIM:
            if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (shoot) {
                item->goal_anim_state = NATLA_STATE_SHOOT;
            } else {
                item->goal_anim_state = NATLA_STATE_STOP;
            }
            break;

        case NATLA_STATE_SHOOT:
            if (!item->required_anim_state) {
                int16_t effect_num =
                    Creature_Effect(item, &m_NatlaGun, Spawn_RocketGun);
                if (effect_num != NO_EFFECT) {
                    EFFECT *effect = Effect_Get(effect_num);
                    gun = effect->rot.x;
                }
                effect_num =
                    Creature_Effect(item, &m_NatlaGun, Spawn_RocketGun);
                if (effect_num != NO_EFFECT) {
                    EFFECT *effect = Effect_Get(effect_num);
                    effect->rot.y += (Random_GetControl() - 0x4000) / 4;
                }
                effect_num =
                    Creature_Effect(item, &m_NatlaGun, Spawn_RocketGun);
                if (effect_num != NO_EFFECT) {
                    EFFECT *effect = Effect_Get(effect_num);
                    effect->rot.y += (Random_GetControl() - 0x4000) / 4;
                }
                item->required_anim_state = NATLA_STATE_STOP;
            }
            break;
        }
    }

    Creature_Tilt(item, tilt);

    natla->neck_rotation = -head;
    if (gun) {
        natla->head_rotation = gun;
    }

    timer++;
    natla->flags &= ~NATLA_TIMER;
    natla->flags |= timer & NATLA_TIMER;

    item->rot.y -= facing;
    Creature_Animate(item_num, angle, 0);
    item->rot.y += facing;

    item->priv = (void *)(intptr_t)facing;
}
