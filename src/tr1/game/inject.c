#include "game/inject.h"

#include "game/camera.h"
#include "game/output.h"
#include "game/room.h"
#include "global/const.h"
#include "global/vars.h"
#include "items.h"

#include <libtrx/benchmark.h>
#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/game/game_buf.h>
#include <libtrx/game/level.h>
#include <libtrx/game/packer.h>
#include <libtrx/log.h>
#include <libtrx/memory.h>
#include <libtrx/utils.h>
#include <libtrx/virtual_file.h>

#include <stddef.h>

#define INJECTION_MAGIC MKTAG('T', '1', 'M', 'J')
#define INJECTION_CURRENT_VERSION 11
#define NULL_FD_INDEX ((uint16_t)(-1))

typedef struct {
    VFILE *fp;
    INJECTION_VERSION version;
    INJECTION_MAIN_TYPE type;
    INJECTION_INFO *info;
    bool relevant;
} INJECTION;

static int32_t m_NumInjections = 0;
static INJECTION *m_Injections = NULL;
static INJECTION_INFO *m_Aggregate = NULL;

static void M_LoadFromFile(INJECTION *injection, const char *filename);

static uint16_t M_RemapRGB(LEVEL_INFO *level_info, RGB_888 rgb);
static void M_AlignTextureReferences(
    const OBJECT *object, const uint16_t *palette_map, int32_t tex_info_base);

static void M_LoadTexturePages(
    INJECTION *injection, LEVEL_INFO *level_info, uint16_t *palette_map,
    RGBA_8888 *page_ptr);
static void M_TextureData(
    INJECTION *injection, LEVEL_INFO *level_info, int32_t page_base);
static void M_MeshData(const INJECTION *injection, LEVEL_INFO *level_info);
static void M_AnimData(INJECTION *injection, LEVEL_INFO *level_info);
static void M_AnimRangeEdits(INJECTION *injection);
static void M_ObjectData(
    const INJECTION *injection, const LEVEL_INFO *level_info,
    const uint16_t *palette_map);
static void M_SFXData(INJECTION *injection, LEVEL_INFO *level_info);

static uint16_t *M_GetMeshTexture(const FACE_EDIT *face_edit);

static void M_ApplyFace4Edit(
    const FACE_EDIT *edit, FACE4 *faces, uint16_t texture);
static void M_ApplyFace3Edit(
    const FACE_EDIT *edit, FACE3 *faces, uint16_t texture);
static void M_ApplyMeshEdit(
    const MESH_EDIT *mesh_edit, const uint16_t *palette_map);
static void M_MeshEdits(INJECTION *injection, uint16_t *palette_map);
static void M_TextureOverwrites(
    INJECTION *injection, LEVEL_INFO *level_info, uint16_t *palette_map);

static void M_FloorDataEdits(INJECTION *injection, LEVEL_INFO *level_info);
static void M_TriggerParameterChange(INJECTION *injection, SECTOR *sector);
static void M_SetMusicOneShot(SECTOR *sector);
static void M_InsertFloorData(
    INJECTION *injection, SECTOR *sector, LEVEL_INFO *level_info);
static void M_RoomShift(const INJECTION *injection, int16_t room_num);
static void M_TriggeredItem(INJECTION *injection, LEVEL_INFO *level_info);

static void M_RoomMeshEdits(const INJECTION *injection);
static void M_TextureRoomFace(const INJECTION *injection);
static void M_MoveRoomFace(const INJECTION *injection);
static void M_AlterRoomVertex(const INJECTION *injection);
static void M_RotateRoomFace(const INJECTION *injection);
static void M_AddRoomFace(const INJECTION *injection);
static void M_AddRoomVertex(const INJECTION *injection);
static void M_AddRoomSprite(const INJECTION *injection);

static uint16_t *M_GetRoomTexture(
    int16_t room_num, FACE_TYPE face_type, int16_t face_index);
static uint16_t *M_GetRoomFaceVertices(
    int16_t room_num, FACE_TYPE face_type, int16_t face_index);

static void M_RoomDoorEdits(INJECTION *injection);

static void M_ItemPositions(INJECTION *injection);
static void M_FrameEdits(
    const INJECTION *injection, const LEVEL_INFO *level_info);
static void M_CameraEdits(const INJECTION *injection);

static void M_LoadFromFile(INJECTION *injection, const char *filename)
{
    injection->relevant = false;
    injection->info = NULL;

    VFILE *const fp = VFile_CreateFromPath(filename);
    injection->fp = fp;
    if (!fp) {
        LOG_WARNING("Could not open %s", filename);
        return;
    }

    const uint32_t magic = VFile_ReadU32(fp);
    if (magic != INJECTION_MAGIC) {
        LOG_WARNING("Invalid injection magic in %s", filename);
        return;
    }

    injection->version = VFile_ReadS32(fp);
    if (injection->version < INJ_VERSION_1
        || injection->version > INJECTION_CURRENT_VERSION) {
        LOG_WARNING(
            "%s uses unsupported version %d", filename, injection->version);
        return;
    }

    injection->type = VFile_ReadS32(fp);

    switch (injection->type) {
    case IMT_GENERAL:
    case IMT_LARA_ANIMS:
        injection->relevant = true;
        break;
    case IMT_BRAID:
        injection->relevant = g_Config.visuals.enable_braid;
        break;
    case IMT_UZI_SFX:
        injection->relevant = g_Config.audio.enable_ps_uzi_sfx;
        break;
    case IMT_FLOOR_DATA:
        injection->relevant = g_Config.gameplay.fix_floor_data_issues;
        break;
    case IMT_TEXTURE_FIX:
        injection->relevant = g_Config.visuals.fix_texture_issues;
        break;
    case IMT_ITEM_POSITION:
        injection->relevant = g_Config.visuals.fix_item_rots;
        break;
    case IMT_PS1_ENEMY:
        injection->relevant = g_Config.gameplay.restore_ps1_enemies;
        break;
    case IMT_DISABLE_ANIM_SPRITE:
        injection->relevant = !g_Config.visuals.fix_animated_sprites;
        break;
    case IMT_SKYBOX:
        injection->relevant = g_Config.visuals.enable_skybox;
        break;
    case IMT_PS1_CRYSTAL:
        injection->relevant = g_Config.gameplay.enable_save_crystals
            && g_Config.visuals.enable_ps1_crystals;
        break;
    default:
        LOG_WARNING("%s is of unknown type %d", filename, injection->type);
        break;
    }

    if (!injection->relevant) {
        return;
    }

    injection->info = Memory_Alloc(sizeof(INJECTION_INFO));
    INJECTION_INFO *info = injection->info;

    info->texture_page_count = VFile_ReadS32(fp);
    info->texture_count = VFile_ReadS32(fp);
    info->sprite_info_count = VFile_ReadS32(fp);
    info->sprite_count = VFile_ReadS32(fp);
    info->mesh_count = VFile_ReadS32(fp);
    info->mesh_ptr_count = VFile_ReadS32(fp);
    info->anim_change_count = VFile_ReadS32(fp);
    info->anim_range_count = VFile_ReadS32(fp);
    info->anim_cmd_count = VFile_ReadS32(fp);
    info->anim_bone_count = VFile_ReadS32(fp) / ANIM_BONE_SIZE;
    info->anim_frame_data_count = VFile_ReadS32(fp);
    info->anim_count = VFile_ReadS32(fp);
    info->object_count = VFile_ReadS32(fp);
    info->sfx_count = VFile_ReadS32(fp);
    info->sfx_data_size = VFile_ReadS32(fp);
    info->sample_count = VFile_ReadS32(fp);
    info->mesh_edit_count = VFile_ReadS32(fp);
    info->texture_overwrite_count = VFile_ReadS32(fp);
    info->floor_edit_count = VFile_ReadS32(fp);

    if (injection->version < INJ_VERSION_8) {
        // Legacy value that stored the total injected floor data length.
        VFile_Skip(fp, sizeof(int32_t));
    }

    if (injection->version > INJ_VERSION_1) {
        // room_mesh_meta_count is a summary of the change in size of room mesh
        // properties, while room_mesh_edit_count indicates how many edits to
        // read and interpret (not all edits incur a size change).
        info->room_mesh_meta_count = VFile_ReadU32(fp);
        if (injection->version >= INJ_VERSION_9) {
            info->room_mesh_meta = Memory_Alloc(
                sizeof(INJECTION_MESH_META) * info->room_mesh_meta_count);
            for (int32_t i = 0; i < info->room_mesh_meta_count; i++) {
                INJECTION_MESH_META *const meta = &info->room_mesh_meta[i];
                meta->room_index = VFile_ReadS16(fp);
                meta->num_vertices = VFile_ReadS16(fp);
                meta->num_quads = VFile_ReadS16(fp);
                meta->num_triangles = VFile_ReadS16(fp);
                meta->num_sprites = VFile_ReadS16(fp);
            }
        } else {
            // Since the implementation of structured room meshes, older
            // injections without detailed meta are no longer supported.
            const int32_t legacy_size = sizeof(int16_t) + sizeof(uint32_t);
            VFile_Skip(fp, info->room_mesh_meta_count * legacy_size);
        }

        info->room_mesh_edit_count = VFile_ReadU32(fp);
        info->room_door_edit_count = VFile_ReadU32(fp);
    } else {
        info->room_mesh_meta = NULL;
    }

    if (injection->version > INJ_VERSION_2) {
        info->anim_range_edit_count = VFile_ReadS32(fp);
    } else {
        info->anim_range_edit_count = 0;
    }

    if (injection->version > INJ_VERSION_3) {
        info->item_position_count = VFile_ReadS32(fp);
    } else {
        info->item_position_count = 0;
    }

    if (injection->version > INJ_VERSION_9) {
        info->frame_edit_count = VFile_ReadS32(fp);
    } else {
        info->frame_edit_count = 0;
    }

    if (injection->version > INJ_VERSION_10) {
        info->camera_edit_count = VFile_ReadS32(fp);
    } else {
        info->camera_edit_count = 0;
    }

    m_Aggregate->texture_page_count += info->texture_page_count;
    m_Aggregate->texture_count += info->texture_count;
    m_Aggregate->sprite_info_count += info->sprite_info_count;
    m_Aggregate->sprite_count += info->sprite_count;
    m_Aggregate->mesh_count += info->mesh_count;
    m_Aggregate->mesh_ptr_count += info->mesh_ptr_count;
    m_Aggregate->anim_change_count += info->anim_change_count;
    m_Aggregate->anim_range_count += info->anim_range_count;
    m_Aggregate->anim_cmd_count += info->anim_cmd_count;
    m_Aggregate->anim_bone_count += info->anim_bone_count;
    m_Aggregate->anim_frame_data_count += info->anim_frame_data_count;
    m_Aggregate->anim_count += info->anim_count;
    m_Aggregate->object_count += info->object_count;
    m_Aggregate->sfx_count += info->sfx_count;
    m_Aggregate->sfx_data_size += info->sfx_data_size;
    m_Aggregate->sample_count += info->sample_count;

    LOG_INFO("%s queued for injection", filename);
}

static void M_LoadTexturePages(
    INJECTION *injection, LEVEL_INFO *level_info, uint16_t *palette_map,
    RGBA_8888 *page_ptr)
{
    BENCHMARK *const benchmark = Benchmark_Start();

    INJECTION_INFO *inj_info = injection->info;
    VFILE *const fp = injection->fp;

    palette_map[0] = 0;
    RGB_888 source_palette[256];
    for (int32_t i = 0; i < 256; i++) {
        source_palette[i].r = VFile_ReadU8(fp);
        source_palette[i].g = VFile_ReadU8(fp);
        source_palette[i].b = VFile_ReadU8(fp);
    }
    for (int32_t i = 1; i < 256; i++) {
        source_palette[i].r *= 4;
        source_palette[i].g *= 4;
        source_palette[i].b *= 4;
    }
    for (int32_t i = 0; i < 256; i++) {
        palette_map[i] = M_RemapRGB(level_info, source_palette[i]);
    }

    // Read in each page for this injection and realign the pixels
    // to the level's palette.
    const size_t pixel_count = TEXTURE_PAGE_SIZE * inj_info->texture_page_count;
    uint8_t *indices = Memory_Alloc(pixel_count);
    VFile_Read(fp, indices, pixel_count);
    uint8_t *input = indices;
    RGBA_8888 *output = page_ptr;
    for (size_t i = 0; i < pixel_count; i++) {
        const uint8_t index = *input++;
        if (index == 0) {
            output->a = 0;
        } else {
            output->r = source_palette[index].r;
            output->g = source_palette[index].g;
            output->b = source_palette[index].b;
            output->a = 255;
        }
        output++;
    }
    Memory_FreePointer(&indices);

    Benchmark_End(benchmark, NULL);
}

static void M_TextureData(
    INJECTION *injection, LEVEL_INFO *level_info, int32_t page_base)
{
    BENCHMARK *const benchmark = Benchmark_Start();

    INJECTION_INFO *inj_info = injection->info;
    VFILE *const fp = injection->fp;

    // Read the tex_infos and align them to the end of the page list.
    Level_ReadObjectTextures(
        level_info->texture_count, page_base, inj_info->texture_count, fp);
    Level_ReadSpriteTextures(
        level_info->sprite_info_count, page_base, inj_info->sprite_info_count,
        fp);

    for (int32_t i = 0; i < inj_info->sprite_count; i++) {
        const GAME_OBJECT_ID object_id = VFile_ReadS32(fp);
        const int16_t num_meshes = VFile_ReadS16(fp);
        const int16_t mesh_idx = VFile_ReadS16(fp);

        if (object_id < O_NUMBER_OF) {
            OBJECT *const object = Object_GetObject(object_id);
            object->mesh_count = num_meshes;
            object->mesh_idx = mesh_idx + level_info->sprite_info_count;
            object->loaded = true;
        } else if (object_id - O_NUMBER_OF < MAX_STATIC_OBJECTS) {
            STATIC_OBJECT_2D *const object =
                Object_GetStaticObject2D(object_id - O_NUMBER_OF);
            object->frame_count = ABS(num_meshes);
            object->texture_idx = mesh_idx + level_info->sprite_info_count;
            object->loaded = true;
        }
        level_info->sprite_info_count += ABS(num_meshes);
    }

    Benchmark_End(benchmark, NULL);
}

static void M_MeshData(const INJECTION *injection, LEVEL_INFO *const level_info)
{
    if (injection->info->mesh_count == 0) {
        return;
    }

    BENCHMARK *const benchmark = Benchmark_Start();

    const INJECTION_INFO *const inj_info = injection->info;
    VFILE *const fp = injection->fp;

    const int32_t mesh_base = level_info->mesh_count;
    const size_t data_start_pos = VFile_GetPos(fp);
    VFile_Skip(fp, inj_info->mesh_count * sizeof(int16_t));
    level_info->mesh_count += inj_info->mesh_count;

    const int32_t alloc_size = inj_info->mesh_ptr_count * sizeof(int32_t);
    int32_t *mesh_indices = Memory_Alloc(alloc_size);
    VFile_Read(fp, mesh_indices, alloc_size);

    const size_t end_pos = VFile_GetPos(fp);
    VFile_SetPos(fp, data_start_pos);

    Level_ReadObjectMeshes(inj_info->mesh_ptr_count, mesh_indices, fp);

    VFile_SetPos(fp, end_pos);
    Memory_FreePointer(&mesh_indices);

    Benchmark_End(benchmark, NULL);
}

static void M_AnimData(INJECTION *injection, LEVEL_INFO *level_info)
{
    BENCHMARK *const benchmark = Benchmark_Start();

    INJECTION_INFO *inj_info = injection->info;
    VFILE *const fp = injection->fp;

    Level_ReadAnimChanges(
        level_info->anim_change_count, inj_info->anim_change_count, fp);
    Level_ReadAnimRanges(
        level_info->anim_range_count, inj_info->anim_range_count, fp);
    Level_ReadAnimCommands(
        level_info->anim_command_count, inj_info->anim_cmd_count, fp);
    Level_ReadAnimBones(
        level_info->anim_bone_count, inj_info->anim_bone_count, fp);

    VFile_Read(
        fp, level_info->anim_frame_data + level_info->anim_frame_data_count,
        inj_info->anim_frame_data_count * sizeof(int16_t));

    Level_ReadAnims(level_info->anim_count, inj_info->anim_count, fp);
    for (int32_t i = 0; i < inj_info->anim_count; i++) {
        ANIM *const anim = Anim_GetAnim(level_info->anim_count + i);

        // Re-align to the level.
        anim->jump_anim_num += level_info->anim_count;
        anim->frame_ofs += level_info->anim_frame_data_count * 2;
        anim->change_idx += level_info->anim_change_count;
        anim->command_idx += level_info->anim_command_count;
    }

    // Re-align to the level.
    for (int32_t i = 0; i < inj_info->anim_change_count; i++) {
        ANIM_CHANGE *const change =
            Anim_GetChange(level_info->anim_change_count);
        change->range_idx += level_info->anim_range_count;
        level_info->anim_change_count++;
    }

    for (int32_t i = 0; i < inj_info->anim_range_count; i++) {
        ANIM_RANGE *const range = Anim_GetRange(level_info->anim_range_count);
        range->link_anim_num += level_info->anim_count;
        level_info->anim_range_count++;
    }

    Benchmark_End(benchmark, NULL);
}

static void M_AnimRangeEdits(INJECTION *injection)
{
    if (injection->version < INJ_VERSION_3) {
        return;
    }

    BENCHMARK *const benchmark = Benchmark_Start();

    INJECTION_INFO *inj_info = injection->info;
    VFILE *const fp = injection->fp;

    for (int32_t i = 0; i < inj_info->anim_range_edit_count; i++) {
        const GAME_OBJECT_ID object_id = VFile_ReadS32(fp);
        const int16_t anim_idx = VFile_ReadS16(fp);
        const int32_t edit_count = VFile_ReadS32(fp);

        if (object_id < 0 || object_id >= O_NUMBER_OF) {
            LOG_WARNING("Object %d is not recognised", object_id);
            VFile_Skip(fp, edit_count * sizeof(int16_t) * 4);
            continue;
        }

        OBJECT *object = &g_Objects[object_id];
        if (!object->loaded) {
            LOG_WARNING("Object %d is not loaded", object_id);
            VFile_Skip(fp, edit_count * sizeof(int16_t) * 4);
            continue;
        }

        const ANIM *const anim = Object_GetAnim(object, anim_idx);
        for (int32_t j = 0; j < edit_count; j++) {
            const int16_t change_idx = VFile_ReadS16(fp);
            const int16_t range_idx = VFile_ReadS16(fp);
            const int16_t low_frame = VFile_ReadS16(fp);
            const int16_t high_frame = VFile_ReadS16(fp);

            if (change_idx >= anim->num_changes) {
                LOG_WARNING(
                    "Change %d is invalid for animation %d", change_idx,
                    anim_idx);
                continue;
            }
            const ANIM_CHANGE *const change =
                Anim_GetChange(anim->change_idx + change_idx);

            if (range_idx >= change->num_ranges) {
                LOG_WARNING(
                    "Range %d is invalid for change %d, animation %d",
                    range_idx, change_idx, anim_idx);
                continue;
            }
            ANIM_RANGE *const range =
                Anim_GetRange(change->range_idx + range_idx);

            range->start_frame = low_frame;
            range->end_frame = high_frame;
        }
    }

    Benchmark_End(benchmark, NULL);
}

static void M_ObjectData(
    const INJECTION *const injection, const LEVEL_INFO *const level_info,
    const uint16_t *palette_map)
{
    BENCHMARK *const benchmark = Benchmark_Start();

    INJECTION_INFO *inj_info = injection->info;
    VFILE *const fp = injection->fp;

    for (int32_t i = 0; i < inj_info->object_count; i++) {
        const GAME_OBJECT_ID object_id = VFile_ReadS32(fp);
        OBJECT *const object = Object_GetObject(object_id);

        const int16_t num_meshes = VFile_ReadS16(fp);
        const int16_t mesh_idx = VFile_ReadS16(fp);
        const int32_t bone_idx = VFile_ReadS32(fp) / ANIM_BONE_SIZE;

        // Omitted mesh data indicates that we wish to retain what's already
        // defined in level data to avoid duplicate texture packing.
        if (!object->loaded || num_meshes != 0) {
            object->mesh_count = num_meshes;
            object->mesh_idx = mesh_idx + level_info->mesh_ptr_count;
            object->bone_idx = bone_idx + level_info->anim_bone_count;
        }

        object->frame_ofs = VFile_ReadU32(fp);
        object->frame_base = NULL;
        object->anim_idx = VFile_ReadS16(fp);
        if (object->anim_idx != -1) {
            object->anim_idx += level_info->anim_count;
        }
        object->loaded = true;

        if (num_meshes != 0) {
            M_AlignTextureReferences(
                object, palette_map, level_info->texture_count);
        }
    }

    Benchmark_End(benchmark, NULL);
}

static void M_SFXData(INJECTION *injection, LEVEL_INFO *level_info)
{
    BENCHMARK *const benchmark = Benchmark_Start();

    INJECTION_INFO *inj_info = injection->info;
    VFILE *const fp = injection->fp;

    for (int32_t i = 0; i < inj_info->sfx_count; i++) {
        const int16_t sfx_id = VFile_ReadS16(fp);
        g_SampleLUT[sfx_id] = level_info->sample_info_count;

        SAMPLE_INFO *sample_info =
            &g_SampleInfos[level_info->sample_info_count];
        sample_info->volume = VFile_ReadS16(fp);
        sample_info->randomness = VFile_ReadS16(fp);
        sample_info->flags = VFile_ReadS16(fp);
        sample_info->number = level_info->sample_count;

        int16_t num_samples = (sample_info->flags >> 2) & 15;
        for (int32_t j = 0; j < num_samples; j++) {
            const int32_t sample_length = VFile_ReadS32(fp);
            VFile_Read(
                fp, level_info->sample_data + level_info->sample_data_size,
                sizeof(char) * sample_length);

            level_info->sample_offsets[level_info->sample_count] =
                level_info->sample_data_size;
            level_info->sample_data_size += sample_length;
            level_info->sample_count++;
        }

        level_info->sample_info_count++;
    }

    Benchmark_End(benchmark, NULL);
}

static void M_AlignTextureReferences(
    const OBJECT *const object, const uint16_t *const palette_map,
    const int32_t tex_info_base)
{
    for (int32_t i = 0; i < object->mesh_count; i++) {
        OBJECT_MESH *const mesh = Object_GetMesh(object->mesh_idx + i);
        for (int32_t j = 0; j < mesh->num_tex_face4s; j++) {
            mesh->tex_face4s[j].texture_idx += tex_info_base;
        }

        for (int32_t j = 0; j < mesh->num_tex_face3s; j++) {
            mesh->tex_face3s[j].texture_idx += tex_info_base;
        }

        for (int32_t j = 0; j < mesh->num_flat_face4s; j++) {
            FACE4 *const face = &mesh->flat_face4s[j];
            face->palette_idx = palette_map[face->palette_idx];
        }

        for (int32_t j = 0; j < mesh->num_flat_face3s; j++) {
            FACE3 *const face = &mesh->flat_face3s[j];
            face->palette_idx = palette_map[face->palette_idx];
        }
    }
}

static uint16_t M_RemapRGB(LEVEL_INFO *level_info, RGB_888 rgb)
{
    // Find the index of the exact match to the given RGB
    for (int32_t i = 0; i < level_info->palette_size; i++) {
        const RGB_888 test_rgb = level_info->palette[i];
        if (rgb.r == test_rgb.r && rgb.g == test_rgb.g && rgb.b == test_rgb.b) {
            return i;
        }
    }

    // Match not found - expand the game palette
    level_info->palette_size++;
    level_info->palette = Memory_Realloc(
        level_info->palette, level_info->palette_size * sizeof(RGB_888));
    level_info->palette[level_info->palette_size - 1] = rgb;
    return level_info->palette_size - 1;
}

static void M_MeshEdits(INJECTION *injection, uint16_t *palette_map)
{
    INJECTION_INFO *inj_info = injection->info;
    VFILE *const fp = injection->fp;

    if (!inj_info->mesh_edit_count) {
        return;
    }

    BENCHMARK *const benchmark = Benchmark_Start();

    MESH_EDIT *mesh_edits =
        Memory_Alloc(sizeof(MESH_EDIT) * inj_info->mesh_edit_count);

    for (int32_t i = 0; i < inj_info->mesh_edit_count; i++) {
        MESH_EDIT *mesh_edit = &mesh_edits[i];
        mesh_edit->object_id = VFile_ReadS32(fp);
        mesh_edit->mesh_idx = VFile_ReadS16(fp);

        if (injection->version >= INJ_VERSION_6) {
            mesh_edit->centre_shift.x = VFile_ReadS16(fp);
            mesh_edit->centre_shift.y = VFile_ReadS16(fp);
            mesh_edit->centre_shift.z = VFile_ReadS16(fp);
            mesh_edit->radius_shift = VFile_ReadS32(fp);
        }

        mesh_edit->face_edit_count = VFile_ReadS32(fp);
        mesh_edit->face_edits =
            Memory_Alloc(sizeof(FACE_EDIT) * mesh_edit->face_edit_count);
        for (int32_t j = 0; j < mesh_edit->face_edit_count; j++) {
            FACE_EDIT *face_edit = &mesh_edit->face_edits[j];
            face_edit->object_id = VFile_ReadS32(fp);
            face_edit->source_identifier = VFile_ReadS16(fp);
            face_edit->face_type = VFile_ReadS32(fp);
            face_edit->face_index = VFile_ReadS16(fp);

            face_edit->target_count = VFile_ReadS32(fp);
            face_edit->targets =
                Memory_Alloc(sizeof(int16_t) * face_edit->target_count);
            VFile_Read(
                fp, face_edit->targets,
                sizeof(int16_t) * face_edit->target_count);
        }

        mesh_edit->vertex_edit_count = VFile_ReadS32(fp);
        mesh_edit->vertex_edits =
            Memory_Alloc(sizeof(VERTEX_EDIT) * mesh_edit->vertex_edit_count);
        for (int32_t i = 0; i < mesh_edit->vertex_edit_count; i++) {
            VERTEX_EDIT *vertex_edit = &mesh_edit->vertex_edits[i];
            vertex_edit->index = VFile_ReadS16(fp);
            vertex_edit->shift.x = VFile_ReadS16(fp);
            vertex_edit->shift.y = VFile_ReadS16(fp);
            vertex_edit->shift.z = VFile_ReadS16(fp);
        }

        M_ApplyMeshEdit(mesh_edit, palette_map);

        for (int32_t j = 0; j < mesh_edit->face_edit_count; j++) {
            FACE_EDIT *face_edit = &mesh_edit->face_edits[j];
            Memory_FreePointer(&face_edit->targets);
        }

        Memory_FreePointer(&mesh_edit->face_edits);
        Memory_FreePointer(&mesh_edit->vertex_edits);
    }

    Memory_FreePointer(&mesh_edits);
    Benchmark_End(benchmark, NULL);
}

static void M_ApplyMeshEdit(
    const MESH_EDIT *const mesh_edit, const uint16_t *const palette_map)
{
    OBJECT_MESH *mesh;
    if (mesh_edit->object_id < O_NUMBER_OF) {
        const OBJECT *const object = Object_GetObject(mesh_edit->object_id);
        if (!object->loaded) {
            return;
        }

        mesh = Object_GetMesh(object->mesh_idx + mesh_edit->mesh_idx);
    } else if (mesh_edit->object_id - O_NUMBER_OF < MAX_STATIC_OBJECTS) {
        const STATIC_OBJECT_3D *const info =
            Object_GetStaticObject3D(mesh_edit->object_id - O_NUMBER_OF);
        mesh = Object_GetMesh(info->mesh_idx);
    } else {
        LOG_WARNING("Invalid object ID %d", mesh_edit->object_id);
        return;
    }

    mesh->center.x += mesh_edit->centre_shift.x;
    mesh->center.y += mesh_edit->centre_shift.y;
    mesh->center.z += mesh_edit->centre_shift.z;
    mesh->radius += mesh_edit->radius_shift;

    for (int32_t i = 0; i < mesh_edit->vertex_edit_count; i++) {
        const VERTEX_EDIT *const edit = &mesh_edit->vertex_edits[i];
        XYZ_16 *const vertex = &mesh->vertices[edit->index];
        vertex->x += edit->shift.x;
        vertex->y += edit->shift.y;
        vertex->z += edit->shift.z;
    }

    // Find each face we are interested in and replace its texture
    // or palette reference with the one selected from each edit's
    // instructions.
    for (int32_t i = 0; i < mesh_edit->face_edit_count; i++) {
        const FACE_EDIT *const face_edit = &mesh_edit->face_edits[i];
        uint16_t texture;
        if (face_edit->source_identifier < 0) {
            texture = palette_map[-face_edit->source_identifier];
        } else {
            const uint16_t *const tex_ptr = M_GetMeshTexture(face_edit);
            if (tex_ptr == NULL) {
                continue;
            }
            texture = *tex_ptr;
        }

        switch (face_edit->face_type) {
        case FT_TEXTURED_QUAD:
            M_ApplyFace4Edit(face_edit, mesh->tex_face4s, texture);
            break;
        case FT_TEXTURED_TRIANGLE:
            M_ApplyFace3Edit(face_edit, mesh->tex_face3s, texture);
            break;
        case FT_COLOURED_QUAD:
            M_ApplyFace4Edit(face_edit, mesh->flat_face4s, texture);
            break;
        case FT_COLOURED_TRIANGLE:
            M_ApplyFace3Edit(face_edit, mesh->flat_face3s, texture);
            break;
        }
    }
}

static void M_ApplyFace4Edit(
    const FACE_EDIT *const edit, FACE4 *const faces, const uint16_t texture)
{
    for (int32_t i = 0; i < edit->target_count; i++) {
        FACE4 *const face = &faces[edit->targets[i]];
        face->texture_idx = texture;
    }
}

static void M_ApplyFace3Edit(
    const FACE_EDIT *const edit, FACE3 *const faces, const uint16_t texture)
{
    for (int32_t i = 0; i < edit->target_count; i++) {
        FACE3 *const face = &faces[edit->targets[i]];
        face->texture_idx = texture;
    }
}

static uint16_t *M_GetMeshTexture(const FACE_EDIT *const face_edit)
{
    const OBJECT *const object = Object_GetObject(face_edit->object_id);
    if (!object->loaded) {
        return NULL;
    }

    const OBJECT_MESH *const mesh =
        Object_GetMesh(object->mesh_idx + face_edit->source_identifier);

    if (face_edit->face_type == FT_TEXTURED_QUAD) {
        FACE4 *const face = &mesh->tex_face4s[face_edit->face_index];
        return &face->texture_idx;
    }

    if (face_edit->face_type == FT_TEXTURED_TRIANGLE) {
        FACE3 *const face = &mesh->tex_face3s[face_edit->face_index];
        return &face->texture_idx;
    }

    if (face_edit->face_type == FT_COLOURED_QUAD) {
        FACE4 *const face = &mesh->flat_face4s[face_edit->face_index];
        return &face->palette_idx;
    }

    if (face_edit->face_type == FT_COLOURED_TRIANGLE) {
        FACE3 *const face = &mesh->flat_face3s[face_edit->face_index];
        return &face->palette_idx;
    }

    return NULL;
}

static void M_TextureOverwrites(
    INJECTION *injection, LEVEL_INFO *level_info, uint16_t *palette_map)
{
    BENCHMARK *const benchmark = Benchmark_Start();

    INJECTION_INFO *inj_info = injection->info;
    VFILE *const fp = injection->fp;

    for (int32_t i = 0; i < inj_info->texture_overwrite_count; i++) {
        const uint16_t target_page = VFile_ReadU16(fp);
        const uint8_t target_x = VFile_ReadU8(fp);
        const uint8_t target_y = VFile_ReadU8(fp);
        const uint16_t source_width = VFile_ReadU16(fp);
        const uint16_t source_height = VFile_ReadU16(fp);

        uint8_t *source_img = Memory_Alloc(source_width * source_height);
        VFile_Read(fp, source_img, source_width * source_height);

        // Copy the source image pixels directly into the target page.
        RGBA_8888 *page =
            level_info->texture_rgb_page_ptrs + target_page * TEXTURE_PAGE_SIZE;
        for (int32_t y = 0; y < source_height; y++) {
            for (int32_t x = 0; x < source_width; x++) {
                const uint8_t pal_idx = source_img[y * source_width + x];
                const int32_t target_pixel =
                    (y + target_y) * TEXTURE_PAGE_WIDTH + x + target_x;
                if (pal_idx == 0) {
                    (*(page + target_pixel)).a = 0;
                } else {
                    const RGB_888 pix =
                        level_info->palette[palette_map[pal_idx]];
                    (*(page + target_pixel)).r = pix.r;
                    (*(page + target_pixel)).g = pix.g;
                    (*(page + target_pixel)).b = pix.b;
                    (*(page + target_pixel)).a = 255;
                }
            }
        }

        Memory_FreePointer(&source_img);
    }

    Benchmark_End(benchmark, NULL);
}

static void M_FloorDataEdits(INJECTION *injection, LEVEL_INFO *level_info)
{
    BENCHMARK *const benchmark = Benchmark_Start();

    INJECTION_INFO *inj_info = injection->info;
    VFILE *const fp = injection->fp;

    for (int32_t i = 0; i < inj_info->floor_edit_count; i++) {
        const int16_t room = VFile_ReadS16(fp);
        const uint16_t x = VFile_ReadU16(fp);
        const uint16_t z = VFile_ReadU16(fp);
        const int32_t fd_edit_count = VFile_ReadS32(fp);

        // Verify that the given room and coordinates are accurate.
        // Individual FD functions must check that sector is actually set.
        ROOM *r = NULL;
        SECTOR *sector = NULL;
        if (room < 0 || room >= g_RoomCount) {
            LOG_WARNING("Room index %d is invalid", room);
        } else {
            r = &g_RoomInfo[room];
            if (x >= r->size.x || z >= r->size.z) {
                LOG_WARNING(
                    "Sector [%d,%d] is invalid for room %d", x, z, room);
            } else {
                sector = &r->sectors[r->size.z * x + z];
            }
        }

        for (int32_t j = 0; j < fd_edit_count; j++) {
            const FLOOR_EDIT_TYPE edit_type = VFile_ReadS32(fp);
            switch (edit_type) {
            case FET_TRIGGER_PARAM:
                M_TriggerParameterChange(injection, sector);
                break;
            case FET_MUSIC_ONESHOT:
                M_SetMusicOneShot(sector);
                break;
            case FET_FD_INSERT:
                M_InsertFloorData(injection, sector, level_info);
                break;
            case FET_ROOM_SHIFT:
                M_RoomShift(injection, room);
                break;
            case FET_TRIGGER_ITEM:
                M_TriggeredItem(injection, level_info);
                break;
            default:
                LOG_WARNING("Unknown floor data edit type: %d", edit_type);
                break;
            }
        }
    }

    Benchmark_End(benchmark, NULL);
}

static void M_TriggerParameterChange(INJECTION *injection, SECTOR *sector)
{
    VFILE *const fp = injection->fp;

    const uint8_t cmd_type = VFile_ReadU8(fp);
    const int16_t old_param = VFile_ReadS16(fp);
    const int16_t new_param = VFile_ReadS16(fp);

    if (sector == NULL || sector->trigger == NULL) {
        return;
    }

    // If we can find an action item for the given sector that matches
    // the command type and old (current) parameter, change it to the
    // new parameter.
    TRIGGER_CMD *cmd = sector->trigger->command;
    for (; cmd != NULL; cmd = cmd->next_cmd) {
        if (cmd->type != cmd_type) {
            continue;
        }

        if (cmd->type == TO_CAMERA) {
            TRIGGER_CAMERA_DATA *const cam_data =
                (TRIGGER_CAMERA_DATA *)cmd->parameter;
            if (cam_data->camera_num == old_param) {
                cam_data->camera_num = new_param;
                break;
            }
        } else {
            if ((int16_t)(intptr_t)cmd->parameter == old_param) {
                cmd->parameter = (void *)(intptr_t)new_param;
                break;
            }
        }
    }
}

static void M_SetMusicOneShot(SECTOR *sector)
{
    if (sector == NULL || sector->trigger == NULL) {
        return;
    }

    TRIGGER_CMD *cmd = sector->trigger->command;
    for (; cmd != NULL; cmd = cmd->next_cmd) {
        if (cmd->type == TO_CD) {
            sector->trigger->one_shot = true;
        }
    }
}

static void M_InsertFloorData(
    INJECTION *injection, SECTOR *sector, LEVEL_INFO *level_info)
{
    VFILE *const fp = injection->fp;

    const int32_t data_length = VFile_ReadS32(fp);

    int16_t data[data_length];
    VFile_Read(fp, data, sizeof(int16_t) * data_length);

    if (sector == NULL) {
        return;
    }

    // This will reset all FD properties in the sector based on the raw data
    // imported. We pass a dummy null index to allow it to read from the
    // beginning of the array.
    Room_PopulateSectorData(sector, data, 0, NULL_FD_INDEX);
}

static void M_RoomShift(
    const INJECTION *const injection, const int16_t room_num)
{
    VFILE *const fp = injection->fp;

    const uint32_t x_shift = ROUND_TO_SECTOR(VFile_ReadU32(fp));
    const uint32_t z_shift = ROUND_TO_SECTOR(VFile_ReadU32(fp));
    const int32_t y_shift = ROUND_TO_CLICK(VFile_ReadS32(fp));

    ROOM *const room = Room_Get(room_num);
    room->pos.x += x_shift;
    room->pos.z += z_shift;
    room->min_floor += y_shift;
    room->max_ceiling += y_shift;

    // Move any items in the room to match.
    for (int32_t i = 0; i < Item_GetTotalCount(); i++) {
        ITEM *const item = Item_Get(i);
        if (item->room_num != room_num) {
            continue;
        }

        item->pos.x += x_shift;
        item->pos.y += y_shift;
        item->pos.z += z_shift;
    }

    if (y_shift == 0) {
        return;
    }

    // Update the sector floor and ceiling heights to match.
    for (int32_t i = 0; i < room->size.z * room->size.x; i++) {
        SECTOR *const sector = &room->sectors[i];
        if (sector->floor.height == NO_HEIGHT
            || sector->ceiling.height == NO_HEIGHT) {
            continue;
        }

        sector->floor.height += y_shift;
        sector->ceiling.height += y_shift;
    }

    // Update vertex Y values to match; x and z are room-relative.
    for (int32_t i = 0; i < room->mesh.num_vertices; i++) {
        (&room->mesh.vertices[i])->pos.y += y_shift;
    }
}

static void M_TriggeredItem(INJECTION *injection, LEVEL_INFO *level_info)
{
    VFILE *const fp = injection->fp;

    if (g_LevelItemCount == MAX_ITEMS) {
        VFile_Skip(
            fp, sizeof(int16_t) * 4 + sizeof(int32_t) * 3 + sizeof(uint16_t));
        LOG_WARNING("Cannot add more than %d items", MAX_ITEMS);
        return;
    }

    int16_t item_num = Item_Create();
    ITEM *item = &g_Items[item_num];

    item->object_id = VFile_ReadS16(fp);
    item->room_num = VFile_ReadS16(fp);
    item->pos.x = VFile_ReadS32(fp);
    item->pos.y = VFile_ReadS32(fp);
    item->pos.z = VFile_ReadS32(fp);
    item->rot.y = VFile_ReadS16(fp);
    item->shade.value_1 = VFile_ReadS16(fp);
    item->flags = VFile_ReadU16(fp);

    level_info->item_count++;
    g_LevelItemCount++;
}

static void M_RoomMeshEdits(const INJECTION *const injection)
{
    if (injection->version < INJ_VERSION_2) {
        return;
    }

    BENCHMARK *const benchmark = Benchmark_Start();

    const INJECTION_INFO *const inj_info = injection->info;
    VFILE *const fp = injection->fp;

    ROOM_MESH_EDIT_TYPE edit_type;
    for (int32_t i = 0; i < inj_info->room_mesh_edit_count; i++) {
        edit_type = VFile_ReadS32(fp);

        switch (edit_type) {
        case RMET_TEXTURE_FACE:
            M_TextureRoomFace(injection);
            break;
        case RMET_MOVE_FACE:
            M_MoveRoomFace(injection);
            break;
        case RMET_ALTER_VERTEX:
            M_AlterRoomVertex(injection);
            break;
        case RMET_ROTATE_FACE:
            M_RotateRoomFace(injection);
            break;
        case RMET_ADD_FACE:
            M_AddRoomFace(injection);
            break;
        case RMET_ADD_VERTEX:
            M_AddRoomVertex(injection);
            break;
        case RMET_ADD_SPRITE:
            M_AddRoomSprite(injection);
            break;
        default:
            LOG_WARNING("Unknown room mesh edit type: %d", edit_type);
            break;
        }
    }

    Benchmark_End(benchmark, NULL);
}

static void M_TextureRoomFace(const INJECTION *const injection)
{
    VFILE *const fp = injection->fp;

    const int16_t target_room = VFile_ReadS16(fp);
    const FACE_TYPE target_face_type = VFile_ReadS32(fp);
    const int16_t target_face = VFile_ReadS16(fp);
    const int16_t source_room = VFile_ReadS16(fp);
    const FACE_TYPE source_face_type = VFile_ReadS32(fp);
    const int16_t source_face = VFile_ReadS16(fp);

    const uint16_t *const source_texture =
        M_GetRoomTexture(source_room, source_face_type, source_face);
    uint16_t *const target_texture =
        M_GetRoomTexture(target_room, target_face_type, target_face);
    if (source_texture != NULL && target_texture != NULL) {
        *target_texture = *source_texture;
    }
}

static void M_MoveRoomFace(const INJECTION *const injection)
{
    VFILE *const fp = injection->fp;

    const int16_t target_room = VFile_ReadS16(fp);
    const FACE_TYPE face_type = VFile_ReadS32(fp);
    const int16_t target_face = VFile_ReadS16(fp);
    const int32_t vertex_count = VFile_ReadS32(fp);

    for (int32_t j = 0; j < vertex_count; j++) {
        const int16_t vertex_index = VFile_ReadS16(fp);
        const int16_t new_vertex = VFile_ReadS16(fp);

        uint16_t *const vertices =
            M_GetRoomFaceVertices(target_room, face_type, target_face);
        if (vertices != NULL) {
            vertices[vertex_index] = new_vertex;
        }
    }
}

static void M_AlterRoomVertex(const INJECTION *const injection)
{
    VFILE *const fp = injection->fp;

    const int16_t target_room = VFile_ReadS16(fp);
    VFile_Skip(fp, sizeof(int32_t));
    const int16_t target_vertex = VFile_ReadS16(fp);
    const int16_t x_change = VFile_ReadS16(fp);
    const int16_t y_change = VFile_ReadS16(fp);
    const int16_t z_change = VFile_ReadS16(fp);
    int16_t shade_change = 0;
    if (injection->version >= INJ_VERSION_7) {
        shade_change = VFile_ReadS16(fp);
    }

    if (target_room < 0 || target_room >= g_RoomCount) {
        LOG_WARNING("Room index %d is invalid", target_room);
        return;
    }

    const ROOM *const room = Room_Get(target_room);
    if (target_vertex < 0 || target_vertex >= room->mesh.num_vertices) {
        LOG_WARNING(
            "Vertex index %d, room %d is invalid", target_vertex, target_room);
        return;
    }

    ROOM_VERTEX *const vertex = &room->mesh.vertices[target_vertex];
    vertex->pos.x += x_change;
    vertex->pos.y += y_change;
    vertex->pos.z += z_change;
    vertex->light_base += shade_change;
    CLAMPG(vertex->light_base, MAX_LIGHTING);
    vertex->light_adder = vertex->light_base;
}

static void M_RotateRoomFace(const INJECTION *const injection)
{
    VFILE *const fp = injection->fp;

    const int16_t target_room = VFile_ReadS16(fp);
    const FACE_TYPE face_type = VFile_ReadS32(fp);
    const int16_t target_face = VFile_ReadS16(fp);
    const uint8_t num_rotations = VFile_ReadU8(fp);

    uint16_t *const face_vertices =
        M_GetRoomFaceVertices(target_room, face_type, target_face);
    if (face_vertices == NULL) {
        return;
    }

    const int32_t num_vertices = face_type == FT_TEXTURED_QUAD ? 4 : 3;
    uint16_t *vertices[num_vertices];
    for (int32_t i = 0; i < num_vertices; i++) {
        vertices[i] = face_vertices + i;
    }

    for (int32_t i = 0; i < num_rotations; i++) {
        const uint16_t first = *vertices[0];
        for (int32_t j = 0; j < num_vertices - 1; j++) {
            *vertices[j] = *vertices[j + 1];
        }
        *vertices[num_vertices - 1] = first;
    }
}

static void M_AddRoomFace(const INJECTION *const injection)
{
    VFILE *const fp = injection->fp;

    const int16_t target_room = VFile_ReadS16(fp);
    const FACE_TYPE face_type = VFile_ReadS32(fp);
    const int16_t source_room = VFile_ReadS16(fp);
    const int16_t source_face = VFile_ReadS16(fp);

    const int32_t num_vertices = face_type == FT_TEXTURED_QUAD ? 4 : 3;
    uint16_t vertices[num_vertices];
    for (int32_t i = 0; i < num_vertices; i++) {
        vertices[i] = VFile_ReadU16(fp);
    }

    if (injection->version < INJ_VERSION_9) {
        LOG_WARNING("Legacy room face injection is not supported");
        return;
    }

    if (target_room < 0 || target_room >= g_RoomCount) {
        LOG_WARNING("Room index %d is invalid", target_room);
        return;
    }

    const uint16_t *const source_texture =
        M_GetRoomTexture(source_room, face_type, source_face);
    if (source_texture == NULL) {
        return;
    }

    ROOM *const room = Room_Get(target_room);
    uint16_t *face_vertices;
    if (face_type == FT_TEXTURED_QUAD) {
        FACE4 *const face = &room->mesh.face4s[room->mesh.num_face4s];
        face->texture_idx = *source_texture;
        face_vertices = face->vertices;
        room->mesh.num_face4s++;

    } else {
        FACE3 *const face = &room->mesh.face3s[room->mesh.num_face3s];
        face->texture_idx = *source_texture;
        face_vertices = face->vertices;
        room->mesh.num_face3s++;
    }

    for (int32_t i = 0; i < num_vertices; i++) {
        face_vertices[i] = vertices[i];
    }
}

static void M_AddRoomVertex(const INJECTION *const injection)
{
    VFILE *const fp = injection->fp;

    const int16_t target_room = VFile_ReadS16(fp);
    VFile_Skip(fp, sizeof(int32_t));
    const XYZ_16 pos = {
        .x = VFile_ReadS16(fp),
        .y = VFile_ReadS16(fp),
        .z = VFile_ReadS16(fp),
    };
    const int16_t shade = VFile_ReadS16(fp);

    if (injection->version < INJ_VERSION_9) {
        LOG_WARNING("Legacy room vertex injection is not supported");
        return;
    }

    ROOM *const room = Room_Get(target_room);
    ROOM_VERTEX *const vertex = &room->mesh.vertices[room->mesh.num_vertices];
    vertex->pos = pos;
    vertex->light_base = shade;
    vertex->light_adder = shade;
    room->mesh.num_vertices++;
}

static void M_AddRoomSprite(const INJECTION *const injection)
{
    VFILE *const fp = injection->fp;

    const int16_t target_room = VFile_ReadS16(fp);
    VFile_Skip(fp, sizeof(int32_t));
    const uint16_t vertex = VFile_ReadU16(fp);
    const uint16_t texture = VFile_ReadU16(fp);

    ROOM *const room = Room_Get(target_room);
    ROOM_SPRITE *const sprite = &room->mesh.sprites[room->mesh.num_sprites];
    sprite->vertex = vertex;
    sprite->texture = texture;

    room->mesh.num_sprites++;
}

static uint16_t *M_GetRoomTexture(
    const int16_t room_num, const FACE_TYPE face_type, const int16_t face_index)
{
    const ROOM *const room = Room_Get(room_num);
    if (face_type == FT_TEXTURED_QUAD && face_index < room->mesh.num_face4s) {
        FACE4 *const face = &room->mesh.face4s[face_index];
        return &face->texture_idx;
    } else if (face_index < room->mesh.num_face3s) {
        FACE3 *const face = &room->mesh.face3s[face_index];
        return &face->texture_idx;
    }

    LOG_WARNING(
        "Invalid room face lookup: %d, %d, %d", room_num, face_type,
        face_index);
    return NULL;
}

static uint16_t *M_GetRoomFaceVertices(
    const int16_t room_num, const FACE_TYPE face_type, const int16_t face_index)
{
    if (room_num < 0 || room_num >= Room_GetTotalCount()) {
        LOG_WARNING("Room index %d is invalid", room_num);
        return NULL;
    }

    const ROOM *const room = Room_Get(room_num);
    if (face_type == FT_TEXTURED_QUAD) {
        if (face_index < 0 || face_index >= room->mesh.num_face4s) {
            LOG_WARNING(
                "Face4 index %d, room %d is invalid", face_index, room_num);
            return NULL;
        }

        FACE4 *const face = &room->mesh.face4s[face_index];
        return (uint16_t *)(void *)&face->vertices;
    }

    if (face_type == FT_TEXTURED_TRIANGLE) {
        if (face_index < 0 || face_index >= room->mesh.num_face3s) {
            LOG_WARNING(
                "Face3 index %d, room %d is invalid", face_index, room_num);
            return NULL;
        }

        FACE3 *const face = &room->mesh.face3s[face_index];
        return (uint16_t *)(void *)&face->vertices;
    }

    return NULL;
}

static void M_RoomDoorEdits(INJECTION *injection)
{
    if (injection->version < INJ_VERSION_2) {
        return;
    }

    BENCHMARK *const benchmark = Benchmark_Start();

    INJECTION_INFO *inj_info = injection->info;
    VFILE *const fp = injection->fp;

    for (int32_t i = 0; i < inj_info->room_door_edit_count; i++) {
        const int16_t base_room = VFile_ReadS16(fp);
        const int16_t link_room = VFile_ReadS16(fp);
        int16_t door_index = -1;
        if (injection->version >= INJ_VERSION_4) {
            door_index = VFile_ReadS16(fp);
        }

        if (base_room < 0 || base_room >= g_RoomCount) {
            VFile_Skip(fp, sizeof(int16_t) * 12);
            LOG_WARNING("Room index %d is invalid", base_room);
            continue;
        }

        ROOM *r = &g_RoomInfo[base_room];
        PORTAL *portal = NULL;
        for (int32_t j = 0; j < r->portals->count; j++) {
            PORTAL d = r->portals->portal[j];
            if (d.room_num == link_room
                && (j == door_index || door_index == -1)) {
                portal = &r->portals->portal[j];
                break;
            }
        }

        if (portal == NULL) {
            VFile_Skip(fp, sizeof(int16_t) * 12);
            LOG_WARNING(
                "Room index %d has no matching portal to %d", base_room,
                link_room);
            continue;
        }

        for (int32_t j = 0; j < 4; j++) {
            const int16_t x_change = VFile_ReadS16(fp);
            const int16_t y_change = VFile_ReadS16(fp);
            const int16_t z_change = VFile_ReadS16(fp);

            portal->vertex[j].x += x_change;
            portal->vertex[j].y += y_change;
            portal->vertex[j].z += z_change;
        }
    }

    Benchmark_End(benchmark, NULL);
}

static void M_ItemPositions(INJECTION *injection)
{
    if (injection->version < INJ_VERSION_4) {
        return;
    }

    BENCHMARK *const benchmark = Benchmark_Start();

    INJECTION_INFO *inj_info = injection->info;
    VFILE *const fp = injection->fp;

    for (int32_t i = 0; i < inj_info->item_position_count; i++) {
        const int16_t item_num = VFile_ReadS16(fp);
        const int16_t y_rot = VFile_ReadS16(fp);

        GAME_VECTOR pos = {};
        if (injection->version > INJ_VERSION_4) {
            pos.x = VFile_ReadS32(fp);
            pos.y = VFile_ReadS32(fp);
            pos.z = VFile_ReadS32(fp);
            pos.room_num = VFile_ReadS16(fp);
        }

        if (item_num < 0 || item_num >= g_LevelItemCount) {
            LOG_WARNING("Item number %d is out of level item range", item_num);
            continue;
        }

        ITEM *item = &g_Items[item_num];
        item->rot.y = y_rot;
        if (injection->version > INJ_VERSION_4) {
            item->pos.x = pos.x;
            item->pos.y = pos.y;
            item->pos.z = pos.z;
            item->room_num = pos.room_num;
        }
    }

    Benchmark_End(benchmark, NULL);
}

static void M_FrameEdits(
    const INJECTION *const injection, const LEVEL_INFO *const level_info)
{
    if (injection->version < INJ_VERSION_10) {
        return;
    }

    BENCHMARK *const benchmark = Benchmark_Start();

    VFILE *const fp = injection->fp;
    for (int32_t i = 0; i < injection->info->frame_edit_count; i++) {
        const GAME_OBJECT_ID object_id = VFile_ReadS32(fp);
        const int32_t anim_idx = VFile_ReadS32(fp);
        const int32_t packed_rot = VFile_ReadS32(fp);

        const OBJECT *const obj = Object_GetObject(object_id);
        if (!obj->loaded) {
            continue;
        }

        const ANIM *const anim = Object_GetAnim(obj, anim_idx);
        int16_t *data_ptr =
            &level_info->anim_frame_data[anim->frame_ofs / sizeof(int16_t)];
        data_ptr += 10;
        memcpy(data_ptr, &packed_rot, sizeof(int32_t));
    }

    Benchmark_End(benchmark, NULL);
}

static void M_CameraEdits(const INJECTION *const injection)
{
    BENCHMARK *const benchmark = Benchmark_Start();

    VFILE *const fp = injection->fp;
    for (int32_t i = 0; i < injection->info->camera_edit_count; i++) {
        const int16_t camera_num = VFile_ReadS16(fp);
        const XYZ_32 pos = {
            .x = VFile_ReadS32(fp),
            .y = VFile_ReadS32(fp),
            .z = VFile_ReadS32(fp),
        };
        const int16_t room_num = VFile_ReadS16(fp);
        const int16_t flags = VFile_ReadS16(fp);

        if (camera_num < 0 || camera_num >= g_NumberCameras) {
            LOG_WARNING(
                "Camera number %d is out of level camera range", camera_num);
            continue;
        }

        OBJECT_VECTOR *const camera = &g_Camera.fixed[camera_num];
        camera->pos = pos;
        camera->data = room_num;
        camera->flags = flags;
    }

    Benchmark_End(benchmark, NULL);
}

void Inject_Init(
    int32_t num_injections, char *filenames[], INJECTION_INFO *aggregate)
{
    m_NumInjections = num_injections;
    if (!num_injections) {
        return;
    }

    BENCHMARK *const benchmark = Benchmark_Start();

    m_Injections = Memory_Alloc(sizeof(INJECTION) * num_injections);
    m_Aggregate = aggregate;

    for (int32_t i = 0; i < num_injections; i++) {
        M_LoadFromFile(&m_Injections[i], filenames[i]);
    }

    Benchmark_End(benchmark, NULL);
}

void Inject_AllInjections(LEVEL_INFO *level_info)
{
    if (!m_Injections) {
        return;
    }

    BENCHMARK *const benchmark = Benchmark_Start();

    uint16_t palette_map[256];
    RGBA_8888 *source_pages = Memory_Alloc(
        m_Aggregate->texture_page_count * TEXTURE_PAGE_SIZE
        * sizeof(RGBA_8888));
    int32_t source_page_count = 0;
    int32_t tpage_base = level_info->texture_page_count;

    for (int32_t i = 0; i < m_NumInjections; i++) {
        INJECTION *injection = &m_Injections[i];
        if (!injection->relevant) {
            continue;
        }

        M_LoadTexturePages(
            injection, level_info, palette_map,
            source_pages + (source_page_count * TEXTURE_PAGE_SIZE));

        M_TextureData(injection, level_info, tpage_base);
        M_MeshData(injection, level_info);
        M_AnimData(injection, level_info);
        M_ObjectData(injection, level_info, palette_map);
        M_SFXData(injection, level_info);

        M_MeshEdits(injection, palette_map);
        M_TextureOverwrites(injection, level_info, palette_map);
        M_FloorDataEdits(injection, level_info);
        M_RoomMeshEdits(injection);
        M_RoomDoorEdits(injection);
        M_AnimRangeEdits(injection);

        M_ItemPositions(injection);
        M_FrameEdits(injection, level_info);
        M_CameraEdits(injection);

        // Realign base indices for the next injection.
        INJECTION_INFO *inj_info = injection->info;
        level_info->anim_command_count += inj_info->anim_cmd_count;
        level_info->anim_bone_count += inj_info->anim_bone_count;
        level_info->anim_frame_data_count += inj_info->anim_frame_data_count;
        level_info->anim_count += inj_info->anim_count;
        level_info->mesh_ptr_count += inj_info->mesh_ptr_count;
        level_info->texture_count += inj_info->texture_count;
        source_page_count += inj_info->texture_page_count;
        tpage_base += inj_info->texture_page_count;
    }

    if (source_page_count != 0) {
        PACKER_DATA data = {
            .level.page_count = level_info->texture_page_count,
            .level.pages_32 = level_info->texture_rgb_page_ptrs,
            .level.pages_24 = NULL,
            .level.palette_24 = NULL,
            .source.page_count = source_page_count,
            .source.pages_32 = source_pages,
            .source.pages_24 = NULL,
            .source.palette_24 = NULL,
            .object_count = level_info->texture_count,
            .sprite_count = level_info->sprite_info_count,
        };

        if (Packer_Pack(&data)) {
            level_info->texture_page_count += Packer_GetAddedPageCount();
            level_info->texture_rgb_page_ptrs = data.level.pages_32;
        }

        Memory_FreePointer(&source_pages);
    }

    Benchmark_End(benchmark, NULL);
}

void Inject_Cleanup(void)
{
    if (!m_NumInjections) {
        return;
    }

    BENCHMARK *const benchmark = Benchmark_Start();

    for (int32_t i = 0; i < m_NumInjections; i++) {
        INJECTION *injection = &m_Injections[i];
        if (injection->fp) {
            VFile_Close(injection->fp);
        }
        if (injection->info) {
            Memory_FreePointer(&injection->info->room_mesh_meta);
            Memory_FreePointer(&injection->info);
        }
    }

    Memory_FreePointer(&m_Injections);
    Benchmark_End(benchmark, NULL);
}

INJECTION_MESH_META Inject_GetRoomMeshMeta(const int32_t room_index)
{
    INJECTION_MESH_META summed_meta = {};
    if (m_Injections == NULL) {
        return summed_meta;
    }

    for (int32_t i = 0; i < m_NumInjections; i++) {
        const INJECTION *const injection = &m_Injections[i];
        if (!injection->relevant || injection->version < INJ_VERSION_9) {
            continue;
        }

        const INJECTION_INFO *const inj_info = injection->info;
        for (int32_t j = 0; j < inj_info->room_mesh_meta_count; j++) {
            const INJECTION_MESH_META *const meta =
                &inj_info->room_mesh_meta[j];
            if (meta->room_index != room_index) {
                continue;
            }

            summed_meta.num_vertices += meta->num_vertices;
            summed_meta.num_quads += meta->num_quads;
            summed_meta.num_triangles += meta->num_triangles;
            summed_meta.num_sprites += meta->num_sprites;
        }
    }

    return summed_meta;
}
