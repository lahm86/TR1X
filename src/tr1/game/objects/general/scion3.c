#include "game/objects/general/scion3.h"

#include "game/camera.h"
#include "game/effects.h"
#include "game/items.h"
#include "game/random.h"
#include "game/room.h"
#include "game/sound.h"
#include "global/vars.h"

void Scion3_Setup(OBJECT *obj)
{
    obj->control = Scion3_Control;
    obj->hit_points = 5;
    obj->save_flags = 1;
    obj->save_hitpoints = 1;
}

void Scion3_Control(int16_t item_num)
{
    static int32_t counter = 0;
    ITEM *const item = Item_Get(item_num);

    if (item->hit_points > 0) {
        counter = 0;
        Item_Animate(item);
        return;
    }

    if (counter == 0) {
        item->status = IS_INVISIBLE;
        item->hit_points = DONT_TARGET;
        Room_TestTriggers(item);
        Item_RemoveDrawn(item_num);
    }

    if (counter % 10 == 0) {
        int16_t effect_num = Effect_Create(item->room_num);
        if (effect_num != NO_EFFECT) {
            EFFECT *effect = Effect_Get(effect_num);
            effect->pos.x = item->pos.x + (Random_GetControl() - 0x4000) / 32;
            effect->pos.y =
                item->pos.y + (Random_GetControl() - 0x4000) / 256 - 500;
            effect->pos.z = item->pos.z + (Random_GetControl() - 0x4000) / 32;
            effect->speed = 0;
            effect->frame_num = 0;
            effect->object_id = O_EXPLOSION_1;
            effect->counter = 0;
            Sound_Effect(SFX_ATLANTEAN_EXPLODE, &effect->pos, SPM_NORMAL);
            g_Camera.bounce = -200;
        }
    }

    counter++;
    if (counter >= LOGIC_FPS * 3) {
        Item_Kill(item_num);
    }
}
