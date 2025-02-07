#include "game/objects/creatures/cutscene_player.h"

#include "game/camera.h"
#include "game/items.h"
#include "global/vars.h"

void CutscenePlayer_Setup(OBJECT *obj)
{
    obj->initialise = CutscenePlayer_Initialise;
    obj->control = CutscenePlayer_Control;
    obj->hit_points = 1;
}

void CutscenePlayer_Initialise(int16_t item_num)
{
    Item_AddActive(item_num);

    ITEM *const item = Item_Get(item_num);
    if (item->object_id == O_PLAYER_1) {
        g_Camera.pos.room_num = item->room_num;
        CINE_DATA *const cine_data = Camera_GetCineData();
        cine_data->position.pos = item->pos;
        cine_data->position.rot.y = 0;
    }
    item->rot.y = 0;
}

void CutscenePlayer_Control(int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (item->object_id != O_PLAYER_4) {
        CINE_DATA *const cine_data = Camera_GetCineData();
        item->rot.y = cine_data->position.rot.y;
        item->pos = cine_data->position.pos;
    }
    Item_Animate(item);
}
