#include "game/objects/common.h"

#include "game/gamebuf.h"

static OBJECT_MESH **m_MeshPointers = NULL;
static int32_t m_MeshCount = 0;

bool Object_IsObjectType(
    GAME_OBJECT_ID object_id, const GAME_OBJECT_ID *test_arr)
{
    for (int i = 0; test_arr[i] != NO_OBJECT; i++) {
        if (test_arr[i] == object_id) {
            return true;
        }
    }
    return false;
}

GAME_OBJECT_ID Object_GetCognate(
    GAME_OBJECT_ID key_id, const GAME_OBJECT_PAIR *test_map)
{
    const GAME_OBJECT_PAIR *pair = &test_map[0];
    while (pair->key_id != NO_OBJECT) {
        if (pair->key_id == key_id) {
            return pair->value_id;
        }
        pair++;
    }

    return NO_OBJECT;
}

GAME_OBJECT_ID Object_GetCognateInverse(
    GAME_OBJECT_ID value_id, const GAME_OBJECT_PAIR *test_map)
{
    const GAME_OBJECT_PAIR *pair = &test_map[0];
    while (pair->key_id != NO_OBJECT) {
        if (pair->value_id == value_id) {
            return pair->key_id;
        }
        pair++;
    }

    return NO_OBJECT;
}

void Object_InitialiseMeshes(const int32_t mesh_count)
{
    m_MeshPointers =
        GameBuf_Alloc(sizeof(OBJECT_MESH *) * mesh_count, GBUF_MESH_POINTERS);
    m_MeshCount = 0;
}

void Object_StoreMeshes(OBJECT_MESH **mesh_ptrs, const int32_t mesh_count)
{
    for (int32_t i = 0; i < mesh_count; i++) {
        m_MeshPointers[m_MeshCount + i] = mesh_ptrs[i];
    }
    m_MeshCount += mesh_count;
}

OBJECT_MESH *Object_GetMesh(const int32_t index)
{
    return m_MeshPointers[index];
}

void Object_SwapMesh(
    const GAME_OBJECT_ID object1_id, const GAME_OBJECT_ID object2_id,
    const int32_t mesh_num)
{
    const OBJECT *const obj1 = Object_GetObject(object1_id);
    const OBJECT *const obj2 = Object_GetObject(object2_id);

    OBJECT_MESH *const temp = m_MeshPointers[obj1->mesh_idx + mesh_num];
    m_MeshPointers[obj1->mesh_idx + mesh_num] =
        m_MeshPointers[obj2->mesh_idx + mesh_num];
    m_MeshPointers[obj2->mesh_idx + mesh_num] = temp;
}
