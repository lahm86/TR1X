#pragma once

#include "../anims.h"
#include "../collision.h"
#include "../items.h"
#include "../math.h"
#include "ids.h"
#include "types.h"

typedef struct {
    const GAME_OBJECT_ID key_id;
    const GAME_OBJECT_ID value_id;
} GAME_OBJECT_PAIR;

extern OBJECT *Object_GetObject(GAME_OBJECT_ID object_id);

bool Object_IsObjectType(
    GAME_OBJECT_ID object_id, const GAME_OBJECT_ID *test_arr);

GAME_OBJECT_ID Object_GetCognate(
    GAME_OBJECT_ID key_id, const GAME_OBJECT_PAIR *test_map);

GAME_OBJECT_ID Object_GetCognateInverse(
    GAME_OBJECT_ID value_id, const GAME_OBJECT_PAIR *test_map);

void Object_InitialiseMeshes(int32_t mesh_count);
void Object_StoreMeshes(OBJECT_MESH **mesh_ptrs, int32_t mesh_count);

OBJECT_MESH *Object_GetMesh(int32_t index);
void Object_SwapMesh(
    GAME_OBJECT_ID object1_id, GAME_OBJECT_ID object2_id, int32_t mesh_num);
