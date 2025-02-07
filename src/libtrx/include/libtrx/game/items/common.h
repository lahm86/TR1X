#pragma once

#include "../anims.h"
#include "./types.h"

ITEM *Item_Get(int16_t num);
int16_t Item_GetIndex(const ITEM *item);
ITEM *Item_Find(GAME_OBJECT_ID obj_id);
int32_t Item_GetTotalCount(void);
int32_t Item_GetDistance(const ITEM *item, const XYZ_32 *target);
void Item_TakeDamage(ITEM *item, int16_t damage, bool hit_status);

// Mesh_bits: which meshes to affect.
// Damage:
// * Positive values - deal damage, enable body part explosions.
// * Negative values - deal damage, disable body part explosions.
// * Zero - don't deal any damage, disable body part explosions.
int32_t Item_Explode(int16_t item_num, int32_t mesh_bits, int16_t damage);

ANIM *Item_GetAnim(const ITEM *item);
bool Item_TestAnimEqual(const ITEM *item, int16_t anim_idx);
int16_t Item_GetRelativeAnim(const ITEM *item);
int16_t Item_GetRelativeObjAnim(const ITEM *item, GAME_OBJECT_ID obj_id);
int16_t Item_GetRelativeFrame(const ITEM *item);
void Item_SwitchToAnim(ITEM *item, int16_t anim_idx, int16_t frame);
void Item_SwitchToObjAnim(
    ITEM *item, int16_t anim_idx, int16_t frame, GAME_OBJECT_ID obj_id);
bool Item_TestFrameEqual(const ITEM *item, int16_t frame);
bool Item_TestFrameRange(const ITEM *item, int16_t start, int16_t end);
bool Item_GetAnimChange(ITEM *item, const ANIM *anim);

void Item_Translate(ITEM *item, int32_t x, int32_t y, int32_t z);

void Item_Animate(ITEM *item);
void Item_PlayAnimSFX(const ITEM *item, const ANIM_COMMAND_EFFECT_DATA *data);
