#include "game/lara/common.h"

#include "config.h"
#include "game/camera.h"
#include "game/collide.h"
#include "game/gameflow.h"
#include "game/gun.h"
#include "game/input.h"
#include "game/inventory.h"
#include "game/item_actions.h"
#include "game/items.h"
#include "game/lara/cheat.h"
#include "game/lara/control.h"
#include "game/lara/misc.h"
#include "game/lot.h"
#include "game/music.h"
#include "game/objects/common.h"
#include "game/objects/vars.h"
#include "game/room.h"
#include "game/savegame.h"
#include "game/sound.h"
#include "game/spawn.h"
#include "global/const.h"
#include "global/vars.h"
#include "math/math.h"
#include "math/matrix.h"

#include <libtrx/log.h>
#include <libtrx/utils.h>

#include <stddef.h>

#define LARA_MOVE_TIMEOUT 90
#define LARA_PUSH_TIMEOUT 15
#define LARA_MOVE_ANIM_VELOCITY 12
#define LARA_MOVE_SPEED 16
#define LARA_UW_DAMAGE 5

static int16_t m_DeathCameraTarget = NO_ITEM;

static void M_CacheMeshWaterStatus(LARA_MESH mesh);
static void M_CalculateWaterStatus_I(
    const FRAME_INFO *frame_1, const FRAME_INFO *frame_2, int32_t frac,
    int32_t rate);
static void M_CalculateWaterStatus(void);

static void M_CacheMeshWaterStatus(const LARA_MESH mesh)
{
    const XYZ_32 pos = {
        .x = g_LaraItem->pos.x + (g_MatrixPtr->_03 >> W2V_SHIFT),
        .y = g_LaraItem->pos.y + (g_MatrixPtr->_13 >> W2V_SHIFT),
        .z = g_LaraItem->pos.z + (g_MatrixPtr->_23 >> W2V_SHIFT),
    };
    int16_t room_num = g_LaraItem->room_num;
    Room_GetSector(pos.x, pos.y, pos.z, &room_num);
    g_Lara.mesh_underwater[mesh] =
        (Room_Get(room_num)->flags & RF_UNDERWATER) != 0;
}

static void M_CalculateWaterStatus_I(
    const FRAME_INFO *const frame_1, const FRAME_INFO *const frame_2,
    const int32_t frac, const int32_t rate)
{
    Matrix_PushUnit();
    Matrix_TranslateSet(0, 0, 0);
    Matrix_RotYXZ(g_LaraItem->rot.y, g_LaraItem->rot.x, g_LaraItem->rot.z);

    const OBJECT *object = Object_GetObject(g_LaraItem->object_id);
    const int32_t *bone = &g_AnimBones[object->bone_idx];
    const int32_t *packed_rotation_1 = frame_1->mesh_rots;
    const int32_t *packed_rotation_2 = frame_2->mesh_rots;
    Matrix_InitInterpolate(frac, rate);

    Matrix_TranslateRel_ID(
        frame_1->offset.x, frame_1->offset.y, frame_1->offset.z,
        frame_2->offset.x, frame_2->offset.y, frame_2->offset.z);

    Matrix_RotYXZpack_I(packed_rotation_1[LM_HIPS], packed_rotation_2[LM_HIPS]);
    M_CacheMeshWaterStatus(LM_HIPS);

    Matrix_Push_I();

    Matrix_TranslateRel_I(bone[1], bone[2], bone[3]);
    Matrix_RotYXZpack_I(
        packed_rotation_1[LM_THIGH_L], packed_rotation_2[LM_THIGH_L]);
    M_CacheMeshWaterStatus(LM_THIGH_L);

    Matrix_TranslateRel_I(bone[5], bone[6], bone[7]);
    Matrix_RotYXZpack_I(
        packed_rotation_1[LM_CALF_L], packed_rotation_1[LM_CALF_L]);
    M_CacheMeshWaterStatus(LM_CALF_L);

    Matrix_TranslateRel_I(bone[9], bone[10], bone[11]);
    Matrix_RotYXZpack_I(
        packed_rotation_1[LM_FOOT_L], packed_rotation_2[LM_FOOT_L]);
    M_CacheMeshWaterStatus(LM_FOOT_L);

    Matrix_Pop_I();

    Matrix_Push_I();

    Matrix_TranslateRel_I(bone[13], bone[14], bone[15]);
    Matrix_RotYXZpack_I(
        packed_rotation_1[LM_THIGH_R], packed_rotation_2[LM_THIGH_R]);
    M_CacheMeshWaterStatus(LM_THIGH_R);

    Matrix_TranslateRel_I(bone[17], bone[18], bone[19]);
    Matrix_RotYXZpack_I(
        packed_rotation_1[LM_CALF_R], packed_rotation_2[LM_CALF_R]);
    M_CacheMeshWaterStatus(LM_CALF_R);

    Matrix_TranslateRel_I(bone[21], bone[22], bone[23]);
    Matrix_RotYXZpack_I(
        packed_rotation_1[LM_FOOT_R], packed_rotation_2[LM_FOOT_R]);
    M_CacheMeshWaterStatus(LM_FOOT_R);

    Matrix_Pop_I();

    Matrix_TranslateRel_I(bone[25], bone[26], bone[27]);
    Matrix_RotYXZpack_I(
        packed_rotation_1[LM_TORSO], packed_rotation_2[LM_TORSO]);
    Matrix_RotYXZ_I(
        g_Lara.interp.result.torso_rot.y, g_Lara.interp.result.torso_rot.x,
        g_Lara.interp.result.torso_rot.z);
    M_CacheMeshWaterStatus(LM_TORSO);

    Matrix_Push_I();

    Matrix_TranslateRel_I(bone[53], bone[54], bone[55]);
    Matrix_RotYXZpack_I(packed_rotation_1[LM_HEAD], packed_rotation_2[LM_HEAD]);
    Matrix_RotYXZ_I(
        g_Lara.interp.result.head_rot.y, g_Lara.interp.result.head_rot.x,
        g_Lara.interp.result.head_rot.z);
    M_CacheMeshWaterStatus(LM_HEAD);

    Matrix_Pop_I();

    LARA_GUN_TYPE gun_type = LGT_UNARMED;
    if (g_Lara.gun_status == LGS_READY || g_Lara.gun_status == LGS_DRAW
        || g_Lara.gun_status == LGS_UNDRAW) {
        gun_type = g_Lara.gun_type;
    }

    switch (gun_type) {
    case LGT_UNARMED:
    default:
        Matrix_Push_I();

        Matrix_TranslateRel_I(bone[29], bone[30], bone[31]);
        Matrix_RotYXZpack_I(
            packed_rotation_1[LM_UARM_R], packed_rotation_2[LM_UARM_R]);
        M_CacheMeshWaterStatus(LM_UARM_R);

        Matrix_TranslateRel_I(bone[33], bone[34], bone[35]);
        Matrix_RotYXZpack_I(
            packed_rotation_1[LM_LARM_R], packed_rotation_2[LM_LARM_R]);
        M_CacheMeshWaterStatus(LM_LARM_R);

        Matrix_TranslateRel_I(bone[37], bone[38], bone[39]);
        Matrix_RotYXZpack_I(
            packed_rotation_1[LM_HAND_R], packed_rotation_2[LM_HAND_R]);
        M_CacheMeshWaterStatus(LM_HAND_R);

        Matrix_Pop_I();

        Matrix_Push_I();

        Matrix_TranslateRel_I(bone[41], bone[42], bone[43]);
        Matrix_RotYXZpack_I(
            packed_rotation_1[LM_UARM_L], packed_rotation_2[LM_UARM_L]);
        M_CacheMeshWaterStatus(LM_UARM_L);

        Matrix_TranslateRel_I(bone[45], bone[46], bone[47]);
        Matrix_RotYXZpack_I(
            packed_rotation_1[LM_LARM_L], packed_rotation_2[LM_LARM_L]);
        M_CacheMeshWaterStatus(LM_LARM_L);

        Matrix_TranslateRel_I(bone[49], bone[50], bone[51]);
        Matrix_RotYXZpack_I(
            packed_rotation_1[LM_HAND_L], packed_rotation_2[LM_HAND_L]);
        M_CacheMeshWaterStatus(LM_HAND_L);

        Matrix_Pop_I();
        break;

    case LGT_PISTOLS:
    case LGT_MAGNUMS:
    case LGT_UZIS:
        Matrix_Push_I();

        Matrix_TranslateRel_I(bone[29], bone[30], bone[31]);
        Matrix_InterpolateArm();

        packed_rotation_1 =
            g_Lara.right_arm.frame_base[g_Lara.right_arm.frame_num].mesh_rots;
        Matrix_RotYXZ(
            g_Lara.right_arm.interp.result.rot.y,
            g_Lara.right_arm.interp.result.rot.x,
            g_Lara.right_arm.interp.result.rot.z);
        Matrix_RotYXZpack(packed_rotation_1[LM_UARM_R]);
        M_CacheMeshWaterStatus(LM_UARM_R);

        Matrix_TranslateRel(bone[33], bone[34], bone[35]);
        Matrix_RotYXZpack(packed_rotation_1[LM_LARM_R]);
        M_CacheMeshWaterStatus(LM_LARM_R);

        Matrix_TranslateRel(bone[37], bone[38], bone[39]);
        Matrix_RotYXZpack(packed_rotation_1[LM_HAND_R]);
        M_CacheMeshWaterStatus(LM_HAND_R);

        Matrix_Pop_I();

        Matrix_Push_I();

        Matrix_TranslateRel_I(bone[41], bone[42], bone[43]);
        Matrix_InterpolateArm();

        packed_rotation_1 =
            g_Lara.left_arm.frame_base[g_Lara.left_arm.frame_num].mesh_rots;
        Matrix_RotYXZ(
            g_Lara.left_arm.interp.result.rot.y,
            g_Lara.left_arm.interp.result.rot.x,
            g_Lara.left_arm.interp.result.rot.z);
        Matrix_RotYXZpack(packed_rotation_1[LM_UARM_L]);
        M_CacheMeshWaterStatus(LM_UARM_L);

        Matrix_TranslateRel(bone[45], bone[46], bone[47]);
        Matrix_RotYXZpack(packed_rotation_1[LM_LARM_L]);
        M_CacheMeshWaterStatus(LM_LARM_L);

        Matrix_TranslateRel(bone[49], bone[50], bone[51]);
        Matrix_RotYXZpack(packed_rotation_1[LM_HAND_L]);
        M_CacheMeshWaterStatus(LM_HAND_L);

        Matrix_Pop_I();
        break;

    case LGT_SHOTGUN:
        Matrix_Push_I();

        packed_rotation_1 =
            g_Lara.right_arm.frame_base[g_Lara.right_arm.frame_num].mesh_rots;
        packed_rotation_2 = packed_rotation_1;
        Matrix_TranslateRel_I(bone[29], bone[30], bone[31]);
        Matrix_RotYXZpack_I(
            packed_rotation_1[LM_UARM_R], packed_rotation_2[LM_UARM_R]);
        M_CacheMeshWaterStatus(LM_UARM_R);

        Matrix_TranslateRel_I(bone[33], bone[34], bone[35]);
        Matrix_RotYXZpack_I(
            packed_rotation_1[LM_LARM_R], packed_rotation_2[LM_LARM_R]);
        M_CacheMeshWaterStatus(LM_LARM_R);

        Matrix_TranslateRel_I(bone[37], bone[38], bone[39]);
        Matrix_RotYXZpack_I(
            packed_rotation_1[LM_HAND_R], packed_rotation_2[LM_HAND_R]);
        M_CacheMeshWaterStatus(LM_HAND_R);

        Matrix_Pop_I();

        Matrix_Push_I();

        packed_rotation_1 =
            g_Lara.left_arm.frame_base[g_Lara.left_arm.frame_num].mesh_rots;
        packed_rotation_2 = packed_rotation_1;
        Matrix_TranslateRel_I(bone[41], bone[42], bone[43]);
        Matrix_RotYXZpack_I(
            packed_rotation_1[LM_UARM_L], packed_rotation_2[LM_UARM_L]);
        M_CacheMeshWaterStatus(LM_UARM_L);

        Matrix_TranslateRel_I(bone[45], bone[46], bone[47]);
        Matrix_RotYXZpack_I(
            packed_rotation_1[LM_LARM_L], packed_rotation_2[LM_LARM_L]);
        M_CacheMeshWaterStatus(LM_LARM_L);

        Matrix_TranslateRel_I(bone[49], bone[50], bone[51]);
        Matrix_RotYXZpack_I(
            packed_rotation_1[LM_HAND_L], packed_rotation_2[LM_HAND_L]);
        M_CacheMeshWaterStatus(LM_HAND_L);

        Matrix_Pop_I();
        break;
    }

    Matrix_Pop();
}

static void M_CalculateWaterStatus(void)
{
    if (!g_Config.visuals.enable_mesh_tint) {
        const ROOM *const room = Room_Get(g_LaraItem->room_num);
        const bool underwater = (room->flags & RF_UNDERWATER) != 0;
        for (int32_t i = 0; i < LM_NUMBER_OF; i++) {
            g_Lara.mesh_underwater[i] = underwater;
        }

        return;
    }

    const OBJECT *const object = Object_GetObject(g_LaraItem->object_id);
    const FRAME_INFO *frame;
    if (g_Lara.hit_direction < 0) {
        FRAME_INFO *frmptr[2];
        int32_t rate;
        const int32_t frac = Item_GetFrames(g_LaraItem, frmptr, &rate);
        if (frac) {
            M_CalculateWaterStatus_I(frmptr[0], frmptr[1], frac, rate);
            goto finish;
        } else {
            frame = frmptr[0];
        }
    } else {
        switch (g_Lara.hit_direction) {
        default:
        case DIR_NORTH:
            frame = g_Anims[object->anim_idx + LA_SPAZ_FORWARD].frame_ptr;
            break;
        case DIR_EAST:
            frame = g_Anims[object->anim_idx + LA_SPAZ_RIGHT].frame_ptr;
            break;
        case DIR_SOUTH:
            frame = g_Anims[object->anim_idx + LA_SPAZ_BACK].frame_ptr;
            break;
        case DIR_WEST:
            frame = g_Anims[object->anim_idx + LA_SPAZ_LEFT].frame_ptr;
            break;
        }
    }

    Matrix_PushUnit();
    Matrix_TranslateSet(0, 0, 0);
    Matrix_RotYXZ(g_LaraItem->rot.y, g_LaraItem->rot.x, g_LaraItem->rot.z);

    const int32_t *packed_rotation = frame->mesh_rots;
    const int32_t *bone = &g_AnimBones[object->bone_idx];

    Matrix_TranslateRel(frame->offset.x, frame->offset.y, frame->offset.z);
    Matrix_RotYXZpack(packed_rotation[LM_HIPS]);
    M_CacheMeshWaterStatus(LM_HIPS);

    Matrix_Push();

    Matrix_TranslateRel(bone[1], bone[2], bone[3]);
    Matrix_RotYXZpack(packed_rotation[LM_THIGH_L]);
    M_CacheMeshWaterStatus(LM_THIGH_L);

    Matrix_TranslateRel(bone[5], bone[6], bone[7]);
    Matrix_RotYXZpack(packed_rotation[LM_CALF_L]);
    M_CacheMeshWaterStatus(LM_CALF_L);

    Matrix_TranslateRel(bone[9], bone[10], bone[11]);
    Matrix_RotYXZpack(packed_rotation[LM_FOOT_L]);
    M_CacheMeshWaterStatus(LM_FOOT_L);

    Matrix_Pop();

    Matrix_Push();

    Matrix_TranslateRel(bone[13], bone[14], bone[15]);
    Matrix_RotYXZpack(packed_rotation[LM_THIGH_R]);
    M_CacheMeshWaterStatus(LM_THIGH_R);

    Matrix_TranslateRel(bone[17], bone[18], bone[19]);
    Matrix_RotYXZpack(packed_rotation[LM_CALF_R]);
    M_CacheMeshWaterStatus(LM_CALF_R);

    Matrix_TranslateRel(bone[21], bone[22], bone[23]);
    Matrix_RotYXZpack(packed_rotation[LM_FOOT_R]);
    M_CacheMeshWaterStatus(LM_FOOT_R);

    Matrix_Pop();

    Matrix_TranslateRel(bone[25], bone[26], bone[27]);
    Matrix_RotYXZpack(packed_rotation[LM_TORSO]);
    Matrix_RotYXZ(
        g_Lara.interp.result.torso_rot.y, g_Lara.interp.result.torso_rot.x,
        g_Lara.interp.result.torso_rot.z);
    M_CacheMeshWaterStatus(LM_TORSO);
    Matrix_TranslateRel(0, bone[26], 0);

    Matrix_Push();

    Matrix_TranslateRel(bone[53], bone[54], bone[55]);
    Matrix_RotYXZpack(packed_rotation[LM_HEAD]);
    Matrix_RotYXZ(
        g_Lara.interp.result.head_rot.y, g_Lara.interp.result.head_rot.x,
        g_Lara.interp.result.head_rot.z);
    M_CacheMeshWaterStatus(LM_HEAD);

    Matrix_Pop();

    LARA_GUN_TYPE gun_type = LGT_UNARMED;
    if (g_Lara.gun_status == LGS_READY || g_Lara.gun_status == LGS_DRAW
        || g_Lara.gun_status == LGS_UNDRAW) {
        gun_type = g_Lara.gun_type;
    }

    switch (gun_type) {
    case LGT_UNARMED:
    default:
        Matrix_Push();

        Matrix_TranslateRel(bone[29], bone[30], bone[31]);
        Matrix_RotYXZpack(packed_rotation[LM_UARM_R]);
        M_CacheMeshWaterStatus(LM_UARM_R);

        Matrix_TranslateRel(bone[33], bone[34], bone[35]);
        Matrix_RotYXZpack(packed_rotation[LM_LARM_R]);
        M_CacheMeshWaterStatus(LM_LARM_R);

        Matrix_TranslateRel(bone[37], bone[38], bone[39]);
        Matrix_RotYXZpack(packed_rotation[LM_HAND_R]);
        M_CacheMeshWaterStatus(LM_HAND_R);

        Matrix_Pop();

        Matrix_Push();

        Matrix_TranslateRel(bone[41], bone[42], bone[43]);
        Matrix_RotYXZpack(packed_rotation[LM_UARM_L]);
        M_CacheMeshWaterStatus(LM_UARM_L);

        Matrix_TranslateRel(bone[45], bone[46], bone[47]);
        Matrix_RotYXZpack(packed_rotation[LM_LARM_L]);
        M_CacheMeshWaterStatus(LM_LARM_L);

        Matrix_TranslateRel(bone[49], bone[50], bone[51]);
        Matrix_RotYXZpack(packed_rotation[LM_HAND_L]);
        M_CacheMeshWaterStatus(LM_HAND_L);

        Matrix_Pop();
        break;

    case LGT_PISTOLS:
    case LGT_MAGNUMS:
    case LGT_UZIS:
        Matrix_Push();

        Matrix_TranslateRel(bone[29], bone[30], bone[31]);

        g_MatrixPtr->_00 = g_MatrixPtr[-2]._00;
        g_MatrixPtr->_01 = g_MatrixPtr[-2]._01;
        g_MatrixPtr->_02 = g_MatrixPtr[-2]._02;
        g_MatrixPtr->_10 = g_MatrixPtr[-2]._10;
        g_MatrixPtr->_11 = g_MatrixPtr[-2]._11;
        g_MatrixPtr->_12 = g_MatrixPtr[-2]._12;
        g_MatrixPtr->_20 = g_MatrixPtr[-2]._20;
        g_MatrixPtr->_21 = g_MatrixPtr[-2]._21;
        g_MatrixPtr->_22 = g_MatrixPtr[-2]._22;

        packed_rotation =
            g_Lara.right_arm.frame_base[g_Lara.right_arm.frame_num].mesh_rots;
        Matrix_RotYXZ(
            g_Lara.right_arm.interp.result.rot.y,
            g_Lara.right_arm.interp.result.rot.x,
            g_Lara.right_arm.interp.result.rot.z);
        Matrix_RotYXZpack(packed_rotation[LM_UARM_R]);
        M_CacheMeshWaterStatus(LM_UARM_R);

        Matrix_TranslateRel(bone[33], bone[34], bone[35]);
        Matrix_RotYXZpack(packed_rotation[LM_LARM_R]);
        M_CacheMeshWaterStatus(LM_LARM_R);

        Matrix_TranslateRel(bone[37], bone[38], bone[39]);
        Matrix_RotYXZpack(packed_rotation[LM_HAND_R]);
        M_CacheMeshWaterStatus(LM_HAND_R);

        Matrix_Pop();

        Matrix_Push();

        Matrix_TranslateRel(bone[41], bone[42], bone[43]);

        g_MatrixPtr->_00 = g_MatrixPtr[-2]._00;
        g_MatrixPtr->_01 = g_MatrixPtr[-2]._01;
        g_MatrixPtr->_02 = g_MatrixPtr[-2]._02;
        g_MatrixPtr->_10 = g_MatrixPtr[-2]._10;
        g_MatrixPtr->_11 = g_MatrixPtr[-2]._11;
        g_MatrixPtr->_12 = g_MatrixPtr[-2]._12;
        g_MatrixPtr->_20 = g_MatrixPtr[-2]._20;
        g_MatrixPtr->_21 = g_MatrixPtr[-2]._21;
        g_MatrixPtr->_22 = g_MatrixPtr[-2]._22;

        packed_rotation =
            g_Lara.left_arm.frame_base[g_Lara.left_arm.frame_num].mesh_rots;
        Matrix_RotYXZ(
            g_Lara.left_arm.interp.result.rot.y,
            g_Lara.left_arm.interp.result.rot.x,
            g_Lara.left_arm.interp.result.rot.z);
        Matrix_RotYXZpack(packed_rotation[LM_UARM_L]);
        M_CacheMeshWaterStatus(LM_UARM_L);

        Matrix_TranslateRel(bone[45], bone[46], bone[47]);
        Matrix_RotYXZpack(packed_rotation[LM_LARM_L]);
        M_CacheMeshWaterStatus(LM_LARM_L);

        Matrix_TranslateRel(bone[49], bone[50], bone[51]);
        Matrix_RotYXZpack(packed_rotation[LM_HAND_L]);
        M_CacheMeshWaterStatus(LM_HAND_L);

        Matrix_Pop();
        break;

    case LGT_SHOTGUN:
        Matrix_Push();

        packed_rotation =
            g_Lara.right_arm.frame_base[g_Lara.right_arm.frame_num].mesh_rots;
        Matrix_TranslateRel(bone[29], bone[30], bone[31]);
        Matrix_RotYXZpack(packed_rotation[LM_UARM_R]);
        M_CacheMeshWaterStatus(LM_UARM_R);

        Matrix_TranslateRel(bone[33], bone[34], bone[35]);
        Matrix_RotYXZpack(packed_rotation[LM_LARM_R]);
        M_CacheMeshWaterStatus(LM_LARM_R);

        Matrix_TranslateRel(bone[37], bone[38], bone[39]);
        Matrix_RotYXZpack(packed_rotation[LM_HAND_R]);
        M_CacheMeshWaterStatus(LM_HAND_R);

        Matrix_Pop();

        Matrix_Push();

        packed_rotation =
            g_Lara.left_arm.frame_base[g_Lara.left_arm.frame_num].mesh_rots;
        Matrix_TranslateRel(bone[41], bone[42], bone[43]);
        Matrix_RotYXZpack(packed_rotation[LM_UARM_L]);
        M_CacheMeshWaterStatus(LM_UARM_L);

        Matrix_TranslateRel(bone[45], bone[46], bone[47]);
        Matrix_RotYXZpack(packed_rotation[LM_LARM_L]);
        M_CacheMeshWaterStatus(LM_LARM_L);

        Matrix_TranslateRel(bone[49], bone[50], bone[51]);
        Matrix_RotYXZpack(packed_rotation[LM_HAND_L]);
        M_CacheMeshWaterStatus(LM_HAND_L);

        Matrix_Pop();
        break;
    }

    Matrix_Pop();

finish:
    // Torso looks strange if tinted and upper arms are not...
    g_Lara.mesh_underwater[LM_TORSO] =
        g_Lara.mesh_underwater[LM_UARM_L] || g_Lara.mesh_underwater[LM_UARM_R];
}

LARA_INFO *Lara_GetLaraInfo(void)
{
    return &g_Lara;
}

ITEM *Lara_GetItem(void)
{
    return g_LaraItem;
}

ITEM *Lara_GetDeathCameraTarget(void)
{
    return m_DeathCameraTarget == -1 ? NULL : Item_Get(m_DeathCameraTarget);
}

void Lara_SetDeathCameraTarget(const int16_t item_num)
{
    m_DeathCameraTarget = item_num;
}

void Lara_Control(void)
{
    COLL_INFO coll = { 0 };

    ITEM *item = g_LaraItem;
    const ROOM *const room = Room_Get(item->room_num);
    const bool room_submerged = (room->flags & RF_UNDERWATER) != 0;
    const int32_t water_depth = Lara_GetWaterDepth(
        item->pos.x, item->pos.y, item->pos.z, item->room_num);
    const int32_t water_height = Room_GetWaterHeight(
        item->pos.x, item->pos.y, item->pos.z, item->room_num);
    const int32_t water_height_diff =
        water_height == NO_HEIGHT ? NO_HEIGHT : item->pos.y - water_height;
    g_Lara.water_surface_dist = -water_height_diff;

    if (g_Lara.interact_target.is_moving
        && g_Lara.interact_target.move_count++ > LARA_MOVE_TIMEOUT) {
        g_Lara.interact_target.is_moving = false;
        g_Lara.gun_status = LGS_ARMLESS;
    }

    if (g_InputDB.level_skip_cheat) {
        Lara_Cheat_EndLevel();
    }

    if (g_InputDB.item_cheat) {
        Lara_Cheat_GiveAllItems();
    }

    if (g_Lara.water_status != LWS_CHEAT && g_InputDB.fly_cheat) {
        Lara_Cheat_EnterFlyMode();
    }

    switch (g_Lara.water_status) {
    case LWS_ABOVE_WATER: {
        if (g_Config.enable_wading
            && (water_height_diff == NO_HEIGHT
                || water_height_diff < LARA_WADE_DEPTH)) {
            break;
        }

        if (g_Config.enable_wading && water_depth <= LARA_SWIM_DEPTH - STEP_L) {
            if (water_height_diff > LARA_WADE_DEPTH) {
                g_Lara.water_status = LWS_WADE;
                if (!item->gravity) {
                    item->goal_anim_state = LS_STOP;
                }
            }
        } else if (room_submerged) {
            g_Lara.water_status = LWS_UNDERWATER;
            g_Lara.air = LARA_MAX_AIR;
            item->pos.y += 100;
            item->gravity = 0;
            Item_UpdateRoom(item, 0);
            Sound_StopEffect(SFX_LARA_FALL, NULL);
            if (item->current_anim_state == LS_SWAN_DIVE) {
                item->goal_anim_state = LS_DIVE;
                item->rot.x = -45 * PHD_DEGREE;
                Lara_Animate(item);
                item->fall_speed *= 2;
            } else if (item->current_anim_state == LS_FAST_DIVE) {
                item->goal_anim_state = LS_DIVE;
                item->rot.x = -85 * PHD_DEGREE;
                Lara_Animate(item);
                item->fall_speed *= 2;
            } else {
                item->current_anim_state = LS_DIVE;
                item->goal_anim_state = LS_SWIM;
                Item_SwitchToAnim(item, LA_JUMP_IN, 0);
                item->rot.x = -45 * PHD_DEGREE;
                item->fall_speed = (item->fall_speed * 3) / 2;
            }
            g_Lara.head_rot.x = 0;
            g_Lara.head_rot.y = 0;
            g_Lara.torso_rot.x = 0;
            g_Lara.torso_rot.y = 0;
            Spawn_Splash(item);
        }

        break;
    }

    case LWS_UNDERWATER: {
        if (room_submerged) {
            break;
        }

        if (water_depth == NO_HEIGHT || ABS(water_height_diff) >= STEP_L) {
            g_Lara.water_status = LWS_ABOVE_WATER;
            g_Lara.gun_status = LGS_ARMLESS;
            item->current_anim_state = LS_JUMP_FORWARD;
            item->goal_anim_state = LS_JUMP_FORWARD;
            Item_SwitchToAnim(item, LA_FALL_DOWN, 0);
            item->speed = item->fall_speed / 4;
            item->fall_speed = 0;
            item->gravity = 1;
            item->rot.x = 0;
            item->rot.z = 0;
            g_Lara.head_rot.x = 0;
            g_Lara.head_rot.y = 0;
            g_Lara.torso_rot.x = 0;
            g_Lara.torso_rot.y = 0;
        } else {
            g_Lara.water_status = LWS_SURFACE;
            g_Lara.dive_timer = DIVE_WAIT + 1;
            item->current_anim_state = LS_SURF_TREAD;
            item->goal_anim_state = LS_SURF_TREAD;
            Item_SwitchToAnim(item, LA_SURF_TREAD, 0);
            item->fall_speed = 0;
            item->pos.y += 1 - water_height_diff;
            item->rot.x = 0;
            item->rot.z = 0;
            g_Lara.head_rot.x = 0;
            g_Lara.head_rot.y = 0;
            g_Lara.torso_rot.x = 0;
            g_Lara.torso_rot.y = 0;
            Item_UpdateRoom(item, -LARA_HEIGHT / 2);
            Sound_Effect(SFX_LARA_BREATH, &item->pos, SPM_ALWAYS);
        }
        break;
    }

    case LWS_SURFACE: {
        if (room_submerged) {
            break;
        }

        if (g_Config.enable_wading && water_height_diff > LARA_WADE_DEPTH) {
            g_Lara.water_status = LWS_WADE;
            Item_SwitchToAnim(item, LA_STAND_IDLE, 0);
            item->current_anim_state = LS_STOP;
            item->goal_anim_state = LS_WADE;
            Item_Animate(item);
            item->fall_speed = 0;
        } else {
            g_Lara.water_status = LWS_ABOVE_WATER;
            g_Lara.gun_status = LGS_ARMLESS;
            item->current_anim_state = LS_JUMP_FORWARD;
            item->goal_anim_state = LS_JUMP_FORWARD;
            Item_SwitchToAnim(item, LA_FALL_DOWN, 0);
            item->speed = item->fall_speed / 4;
            item->fall_speed = 0;
            item->gravity = 1;
            item->rot.x = 0;
            item->rot.z = 0;
            g_Lara.head_rot.x = 0;
            g_Lara.head_rot.y = 0;
            g_Lara.torso_rot.x = 0;
            g_Lara.torso_rot.y = 0;
        }
        break;
    }

    case LWS_WADE: {
        g_Camera.target_elevation = CAM_WADE_ELEVATION;

        if (water_height_diff < LARA_WADE_DEPTH) {
            g_Lara.water_status = LWS_ABOVE_WATER;
            if (item->current_anim_state == LS_WADE) {
                item->goal_anim_state = LS_RUN;
            }
        } else if (water_height_diff > LARA_SWIM_DEPTH) {
            g_Lara.water_status = LWS_SURFACE;
            item->pos.y += 1 - water_height_diff;

            LARA_ANIMATION anim;
            switch (item->current_anim_state) {
            case LS_BACK:
                item->goal_anim_state = LS_SURF_BACK;
                anim = LA_SURF_SWIM_BACK;
                break;

            case LS_STEP_RIGHT:
                item->goal_anim_state = LS_SURF_RIGHT;
                anim = LA_SURF_SWIM_RIGHT;
                break;

            case LS_STEP_LEFT:
                item->goal_anim_state = LS_SURF_LEFT;
                anim = LA_SURF_SWIM_LEFT;
                break;

            default:
                item->goal_anim_state = LS_SURF_SWIM;
                anim = LA_SURF_SWIM_FORWARD;
                break;
            }

            item->current_anim_state = item->goal_anim_state;
            Item_SwitchToAnim(item, anim, 0);

            item->rot.z = 0;
            item->rot.x = 0;
            item->gravity = 0;
            item->fall_speed = 0;
            g_Lara.dive_timer = 0;
            g_Lara.torso_rot.y = 0;
            g_Lara.torso_rot.x = 0;
            g_Lara.head_rot.y = 0;
            g_Lara.head_rot.x = 0;
            Item_UpdateRoom(item, -LARA_HEIGHT / 2);
        }
        break;
    }

    default:
        break;
    }

    if (item->hit_points <= 0) {
        item->hit_points = -1;
        if (!g_Lara.death_timer) {
            Music_Stop();
            g_GameInfo.current[g_CurrentLevel].stats.death_count++;
            if (g_GameInfo.current_save_slot != -1) {
                Savegame_UpdateDeathCounters(
                    g_GameInfo.current_save_slot, &g_GameInfo);
            }
        }
        g_Lara.death_timer++;
        // make sure the enemy healthbar is no longer rendered. If g_Lara later
        // is resurrected with DOZY, she should no longer aim at the target.
        g_Lara.target = NULL;

        if (g_LaraItem->flags & IF_INVISIBLE) {
            return;
        }
    }

    Camera_MoveManual();

    switch (g_Lara.water_status) {
    case LWS_ABOVE_WATER:
    case LWS_WADE:
        g_Lara.air = LARA_MAX_AIR;
        Lara_HandleAboveWater(item, &coll);
        break;

    case LWS_UNDERWATER:
        if (item->hit_points >= 0) {
            g_Lara.air--;
            if (g_Lara.air < 0) {
                g_Lara.air = -1;
                Lara_TakeDamage(LARA_UW_DAMAGE, false);
            }
        }
        Lara_HandleUnderwater(item, &coll);
        break;

    case LWS_SURFACE:
        if (item->hit_points >= 0) {
            g_Lara.air += 10;
            if (g_Lara.air > LARA_MAX_AIR) {
                g_Lara.air = LARA_MAX_AIR;
            }
        }
        Lara_HandleSurface(item, &coll);
        break;

    case LWS_CHEAT:
        item->hit_points = LARA_MAX_HITPOINTS;
        g_Lara.death_timer = 0;
        Lara_HandleUnderwater(item, &coll);
        if (g_Input.slow && !g_Input.look && !g_Input.fly_cheat) {
            Lara_Cheat_ExitFlyMode();
        }
        break;

    default:
        break;
    }

    M_CalculateWaterStatus();
}

void Lara_SwapMeshExtra(void)
{
    if (!g_Objects[O_LARA_EXTRA].loaded) {
        return;
    }
    for (LARA_MESH mesh = LM_FIRST; mesh < LM_NUMBER_OF; mesh++) {
        Lara_SwapSingleMesh(mesh, O_LARA_EXTRA);
    }
}

void Lara_SwapSingleMesh(const LARA_MESH mesh, const GAME_OBJECT_ID object_id)
{
    const OBJECT *const object = Object_GetObject(object_id);
    g_Lara.mesh_ptrs[mesh] = Object_GetMesh(object->mesh_idx + mesh);
}

OBJECT_MESH *Lara_GetMesh(const LARA_MESH mesh)
{
    return g_Lara.mesh_ptrs[mesh];
}

void Lara_SetMesh(const LARA_MESH mesh, OBJECT_MESH *const mesh_ptr)
{
    g_Lara.mesh_ptrs[mesh] = mesh_ptr;
}

void Lara_Animate(ITEM *item)
{
    int16_t *command;
    ANIM *anim;

    item->frame_num++;
    anim = &g_Anims[item->anim_num];
    if (anim->num_changes > 0 && Item_GetAnimChange(item, anim)) {
        anim = &g_Anims[item->anim_num];
        item->current_anim_state = anim->current_anim_state;
    }

    if (item->frame_num > anim->frame_end) {
        if (anim->num_commands > 0) {
            command = &g_AnimCommands[anim->command_idx];
            for (int i = 0; i < anim->num_commands; i++) {
                switch (*command++) {
                case AC_MOVE_ORIGIN:
                    Item_Translate(item, command[0], command[1], command[2]);
                    command += 3;
                    break;

                case AC_JUMP_VELOCITY:
                    item->fall_speed = command[0];
                    item->speed = command[1];
                    command += 2;
                    item->gravity = 1;
                    if (g_Lara.calc_fall_speed) {
                        item->fall_speed = g_Lara.calc_fall_speed;
                        g_Lara.calc_fall_speed = 0;
                    }
                    break;

                case AC_ATTACK_READY:
                    g_Lara.gun_status = LGS_ARMLESS;
                    break;

                case AC_SOUND_FX:
                case AC_EFFECT:
                    command += 2;
                    break;
                }
            }
        }

        item->anim_num = anim->jump_anim_num;
        item->frame_num = anim->jump_frame_num;

        anim = &g_Anims[anim->jump_anim_num];
        item->current_anim_state = anim->current_anim_state;
    }

    if (anim->num_commands > 0) {
        command = &g_AnimCommands[anim->command_idx];
        for (int i = 0; i < anim->num_commands; i++) {
            switch (*command++) {
            case AC_MOVE_ORIGIN:
                command += 3;
                break;

            case AC_JUMP_VELOCITY:
                command += 2;
                break;

            case AC_SOUND_FX:
                Item_PlayAnimSFX(item, command, SPM_ALWAYS);
                command += 2;
                break;

            case AC_EFFECT:
                if (item->frame_num == command[0]) {
                    ItemAction_Run(command[1], item);
                }
                command += 2;
                break;
            }
        }
    }

    if (item->gravity) {
        int32_t speed = anim->velocity
            + anim->acceleration * (item->frame_num - anim->frame_base - 1);
        item->speed -= (int16_t)(speed >> 16);
        speed += anim->acceleration;
        item->speed += (int16_t)(speed >> 16);

        item->fall_speed += (item->fall_speed < FASTFALL_SPEED) ? GRAVITY : 1;
        item->pos.y += item->fall_speed;
    } else {
        int32_t speed = anim->velocity;
        if (anim->acceleration) {
            speed += anim->acceleration * (item->frame_num - anim->frame_base);
        }
        item->speed = (int16_t)(speed >> 16);
    }

    item->pos.x += (Math_Sin(g_Lara.move_angle) * item->speed) >> W2V_SHIFT;
    item->pos.z += (Math_Cos(g_Lara.move_angle) * item->speed) >> W2V_SHIFT;
}

void Lara_AnimateUntil(ITEM *lara_item, int32_t goal)
{
    lara_item->goal_anim_state = goal;
    do {
        Lara_Animate(lara_item);
    } while (lara_item->current_anim_state != goal);
}

void Lara_UseItem(GAME_OBJECT_ID object_id)
{
    LOG_INFO("%d", object_id);
    switch (object_id) {
    case O_PISTOL_ITEM:
    case O_PISTOL_OPTION:
        g_Lara.request_gun_type = LGT_PISTOLS;
        if (g_Lara.gun_status == LGS_ARMLESS
            && g_Lara.gun_type == LGT_PISTOLS) {
            g_Lara.gun_type = LGT_UNARMED;
        }
        break;

    case O_SHOTGUN_ITEM:
    case O_SHOTGUN_OPTION:
        g_Lara.request_gun_type = LGT_SHOTGUN;
        if (g_Lara.gun_status == LGS_ARMLESS
            && g_Lara.gun_type == LGT_SHOTGUN) {
            g_Lara.gun_type = LGT_UNARMED;
        }
        break;

    case O_MAGNUM_ITEM:
    case O_MAGNUM_OPTION:
        g_Lara.request_gun_type = LGT_MAGNUMS;
        if (g_Lara.gun_status == LGS_ARMLESS
            && g_Lara.gun_type == LGT_MAGNUMS) {
            g_Lara.gun_type = LGT_UNARMED;
        }
        break;

    case O_UZI_ITEM:
    case O_UZI_OPTION:
        g_Lara.request_gun_type = LGT_UZIS;
        if (g_Lara.gun_status == LGS_ARMLESS && g_Lara.gun_type == LGT_UZIS) {
            g_Lara.gun_type = LGT_UNARMED;
        }
        break;

    case O_MEDI_ITEM:
    case O_MEDI_OPTION:
        if (g_LaraItem->hit_points <= 0
            || g_LaraItem->hit_points >= LARA_MAX_HITPOINTS) {
            return;
        }
        g_LaraItem->hit_points += LARA_MAX_HITPOINTS / 2;
        CLAMPG(g_LaraItem->hit_points, LARA_MAX_HITPOINTS);
        Inv_RemoveItem(O_MEDI_ITEM);
        Sound_Effect(SFX_MENU_MEDI, NULL, SPM_ALWAYS);
        break;

    case O_BIGMEDI_ITEM:
    case O_BIGMEDI_OPTION:
        if (g_LaraItem->hit_points <= 0
            || g_LaraItem->hit_points >= LARA_MAX_HITPOINTS) {
            return;
        }
        g_LaraItem->hit_points = g_LaraItem->hit_points + LARA_MAX_HITPOINTS;
        CLAMPG(g_LaraItem->hit_points, LARA_MAX_HITPOINTS);
        Inv_RemoveItem(O_BIGMEDI_ITEM);
        Sound_Effect(SFX_MENU_MEDI, NULL, SPM_ALWAYS);
        break;

    case O_KEY_ITEM_1:
    case O_KEY_OPTION_1:
    case O_KEY_ITEM_2:
    case O_KEY_OPTION_2:
    case O_KEY_ITEM_3:
    case O_KEY_OPTION_3:
    case O_KEY_ITEM_4:
    case O_KEY_OPTION_4:
    case O_PUZZLE_ITEM_1:
    case O_PUZZLE_OPTION_1:
    case O_PUZZLE_ITEM_2:
    case O_PUZZLE_OPTION_2:
    case O_PUZZLE_ITEM_3:
    case O_PUZZLE_OPTION_3:
    case O_PUZZLE_ITEM_4:
    case O_PUZZLE_OPTION_4:
    case O_LEADBAR_ITEM:
    case O_LEADBAR_OPTION:
    case O_SCION_ITEM_1:
    case O_SCION_ITEM_2:
    case O_SCION_ITEM_3:
    case O_SCION_ITEM_4:
    case O_SCION_OPTION: {
        int16_t receptacle_item_num = Object_FindReceptacle(object_id);
        if (receptacle_item_num == NO_OBJECT) {
            Sound_Effect(SFX_LARA_NO, NULL, SPM_NORMAL);
            return;
        }
        g_Lara.interact_target.item_num = receptacle_item_num;
        g_Lara.interact_target.is_moving = true;
        g_Lara.interact_target.move_count = 0;
        Inv_RemoveItem(object_id);
        break;
    }

    default:
        break;
    }
}

void Lara_ControlExtra(int16_t item_num)
{
    Item_Animate(&g_Items[item_num]);
}

void Lara_InitialiseLoad(int16_t item_num)
{
    g_Lara.item_num = item_num;
    if (item_num == NO_ITEM) {
        g_LaraItem = NULL;
    } else {
        g_LaraItem = &g_Items[item_num];
    }
}

void Lara_Initialise(int32_t level_num)
{
    RESUME_INFO *resume = &g_GameInfo.current[level_num];

    g_LaraItem->collidable = 0;
    g_LaraItem->data = &g_Lara;
    if (g_Config.disable_healing_between_levels) {
        g_LaraItem->hit_points = resume->lara_hitpoints;
    } else {
        g_LaraItem->hit_points = g_Config.start_lara_hitpoints;
    }

    m_DeathCameraTarget = NO_ITEM;
    g_Lara.air = LARA_MAX_AIR;
    g_Lara.torso_rot.y = 0;
    g_Lara.torso_rot.x = 0;
    g_Lara.torso_rot.z = 0;
    g_Lara.head_rot.y = 0;
    g_Lara.head_rot.x = 0;
    g_Lara.head_rot.z = 0;
    g_Lara.calc_fall_speed = 0;
    g_Lara.mesh_effects = 0;
    g_Lara.hit_frame = 0;
    g_Lara.hit_direction = 0;
    g_Lara.death_timer = 0;
    g_Lara.target = NULL;
    g_Lara.spaz_effect = NULL;
    g_Lara.spaz_effect_count = 0;
    g_Lara.turn_rate = 0;
    g_Lara.move_angle = 0;
    g_Lara.right_arm.flash_gun = 0;
    g_Lara.left_arm.flash_gun = 0;
    g_Lara.right_arm.lock = 0;
    g_Lara.left_arm.lock = 0;
    g_Lara.interact_target.is_moving = false;
    g_Lara.interact_target.item_num = NO_OBJECT;
    g_Lara.interact_target.move_count = 0;

    if (g_RoomInfo[g_LaraItem->room_num].flags & RF_UNDERWATER) {
        g_Lara.water_status = LWS_UNDERWATER;
        g_LaraItem->fall_speed = 0;
        g_LaraItem->goal_anim_state = LS_TREAD;
        g_LaraItem->current_anim_state = LS_TREAD;
        Item_SwitchToAnim(g_LaraItem, LA_TREAD, 0);
    } else {
        g_Lara.water_status = LWS_ABOVE_WATER;
        g_LaraItem->goal_anim_state = LS_STOP;
        g_LaraItem->current_anim_state = LS_STOP;
        Item_SwitchToAnim(g_LaraItem, LA_STOP, 0);
    }

    g_Lara.current_active = 0;

    LOT_InitialiseLOT(&g_Lara.lot);
    g_Lara.lot.step = WALL_L * 20;
    g_Lara.lot.drop = -WALL_L * 20;
    g_Lara.lot.fly = STEP_L;

    Lara_InitialiseInventory(level_num);
}

void Lara_InitialiseInventory(int32_t level_num)
{
    Inv_RemoveAllItems();

    RESUME_INFO *resume = &g_GameInfo.current[level_num];

    g_Lara.pistols.ammo = 1000;

    if (g_GameInfo.remove_guns) {
        resume->flags.got_pistols = 0;
        resume->flags.got_shotgun = 0;
        resume->flags.got_magnums = 0;
        resume->flags.got_uzis = 0;
        resume->equipped_gun_type = LGT_UNARMED;
        resume->holsters_gun_type = LGT_UNARMED;
        resume->back_gun_type = LGT_UNARMED;
        resume->gun_status = LGS_ARMLESS;
    }

    if (g_GameInfo.remove_scions) {
        resume->num_scions = 0;
    }

    if (g_GameInfo.remove_ammo) {
        resume->shotgun_ammo = 0;
        resume->magnum_ammo = 0;
        resume->uzi_ammo = 0;
    }

    if (g_GameInfo.remove_medipacks) {
        resume->num_medis = 0;
        resume->num_big_medis = 0;
    }

    if (resume->flags.got_pistols) {
        Inv_AddItem(O_PISTOL_ITEM);
    }

    if (resume->flags.got_magnums) {
        Inv_AddItem(O_MAGNUM_ITEM);
        g_Lara.magnums.ammo = resume->magnum_ammo;
        Item_GlobalReplace(O_MAGNUM_ITEM, O_MAG_AMMO_ITEM);
    } else {
        int32_t ammo = resume->magnum_ammo / MAGNUM_AMMO_QTY;
        for (int i = 0; i < ammo; i++) {
            Inv_AddItem(O_MAG_AMMO_ITEM);
        }
        g_Lara.magnums.ammo = 0;
    }

    if (resume->flags.got_uzis) {
        Inv_AddItem(O_UZI_ITEM);
        g_Lara.uzis.ammo = resume->uzi_ammo;
        Item_GlobalReplace(O_UZI_ITEM, O_UZI_AMMO_ITEM);
    } else {
        int32_t ammo = resume->uzi_ammo / UZI_AMMO_QTY;
        for (int i = 0; i < ammo; i++) {
            Inv_AddItem(O_UZI_AMMO_ITEM);
        }
        g_Lara.uzis.ammo = 0;
    }

    if (resume->flags.got_shotgun) {
        Inv_AddItem(O_SHOTGUN_ITEM);
        g_Lara.shotgun.ammo = resume->shotgun_ammo;
        Item_GlobalReplace(O_SHOTGUN_ITEM, O_SG_AMMO_ITEM);
    } else {
        int32_t ammo = resume->shotgun_ammo / SHOTGUN_AMMO_QTY;
        for (int i = 0; i < ammo; i++) {
            Inv_AddItem(O_SG_AMMO_ITEM);
        }
        g_Lara.shotgun.ammo = 0;
    }

    for (int i = 0; i < resume->num_scions; i++) {
        Inv_AddItem(O_SCION_ITEM_1);
    }

    for (int i = 0; i < resume->num_medis; i++) {
        Inv_AddItem(O_MEDI_ITEM);
    }

    for (int i = 0; i < resume->num_big_medis; i++) {
        Inv_AddItem(O_BIGMEDI_ITEM);
    }

    g_Lara.gun_status = resume->gun_status;
    g_Lara.gun_type = resume->equipped_gun_type;
    g_Lara.request_gun_type = resume->equipped_gun_type;
    g_Lara.holsters_gun_type = resume->holsters_gun_type;
    g_Lara.back_gun_type = resume->back_gun_type;

    Lara_InitialiseMeshes(level_num);
    Gun_InitialiseNewWeapon();
}

void Lara_RevertToPistolsIfNeeded(void)
{
    if (!g_Config.revert_to_pistols || !Inv_RequestItem(O_PISTOL_ITEM)) {
        return;
    }

    g_Lara.gun_type = LGT_PISTOLS;

    if (g_Lara.gun_status != LGS_ARMLESS) {
        g_Lara.holsters_gun_type = LGT_UNARMED;
    }
    if (Inv_RequestItem(O_SHOTGUN_ITEM)) {
        g_Lara.back_gun_type = LGT_SHOTGUN;
    } else {
        g_Lara.back_gun_type = LGT_UNARMED;
    }
    Gun_InitialiseNewWeapon();
    Gun_SetLaraHolsterLMesh(g_Lara.holsters_gun_type);
    Gun_SetLaraHolsterRMesh(g_Lara.holsters_gun_type);
    Gun_SetLaraBackMesh(g_Lara.back_gun_type);
}

void Lara_InitialiseMeshes(int32_t level_num)
{
    const RESUME_INFO *const resume = &g_GameInfo.current[level_num];

    if (resume->flags.costume) {
        for (LARA_MESH mesh = LM_FIRST; mesh < LM_NUMBER_OF; mesh++) {
            Lara_SwapSingleMesh(mesh, mesh == LM_HEAD ? O_LARA : O_LARA_EXTRA);
        }
        return;
    }

    for (LARA_MESH mesh = LM_FIRST; mesh < LM_NUMBER_OF; mesh++) {
        Lara_SwapSingleMesh(mesh, O_LARA);
    }

    LARA_GUN_TYPE holsters_gun_type = resume->holsters_gun_type;
    LARA_GUN_TYPE back_gun_type = resume->back_gun_type;

    if (holsters_gun_type != LGT_UNKNOWN) {
        Gun_SetLaraHolsterLMesh(holsters_gun_type);
        Gun_SetLaraHolsterRMesh(holsters_gun_type);
    }

    if (back_gun_type != LGT_UNKNOWN) {
        Gun_SetLaraBackMesh(back_gun_type);
    }
}

bool Lara_IsNearItem(const XYZ_32 *pos, int32_t distance)
{
    return Item_IsNearItem(g_LaraItem, pos, distance);
}

int16_t Lara_GetNearestEnemy(void)
{
    if (g_LaraItem == NULL) {
        return NO_ITEM;
    }

    int32_t best_distance = -1;
    int16_t best_item_num = NO_ITEM;
    int16_t item_num = g_NextItemActive;
    while (item_num != NO_ITEM) {
        ITEM *item = &g_Items[item_num];

        if (Object_IsObjectType(item->object_id, g_EnemyObjects)) {
            const int32_t distance = Item_GetDistance(item, &g_LaraItem->pos);
            if (best_item_num == NO_ITEM || distance < best_distance) {
                best_item_num = item_num;
                best_distance = distance;
            }
        }

        item_num = item->next_active;
    }

    return best_item_num;
}

bool Lara_TestBoundsCollide(ITEM *item, int32_t radius)
{
    return Item_TestBoundsCollide(g_LaraItem, item, radius);
}

bool Lara_TestPosition(const ITEM *item, const OBJECT_BOUNDS *const bounds)
{
    return Item_TestPosition(g_LaraItem, item, bounds);
}

void Lara_AlignPosition(ITEM *item, XYZ_32 *vec)
{
    Item_AlignPosition(g_LaraItem, item, vec);
}

bool Lara_MovePosition(ITEM *item, XYZ_32 *vec)
{
    int32_t velocity =
        g_Config.walk_to_items && g_Lara.water_status != LWS_UNDERWATER
        ? LARA_MOVE_ANIM_VELOCITY
        : LARA_MOVE_SPEED;

    return Item_MovePosition(g_LaraItem, item, vec, velocity);
}

void Lara_Push(ITEM *item, COLL_INFO *coll, bool spaz_on, bool big_push)
{
    ITEM *const lara_item = g_LaraItem;
    int32_t x = lara_item->pos.x - item->pos.x;
    int32_t z = lara_item->pos.z - item->pos.z;
    const int32_t c = Math_Cos(item->rot.y);
    const int32_t s = Math_Sin(item->rot.y);
    int32_t rx = (c * x - s * z) >> W2V_SHIFT;
    int32_t rz = (c * z + s * x) >> W2V_SHIFT;

    const BOUNDS_16 *const bounds = &Item_GetBestFrame(item)->bounds;
    int32_t min_x = bounds->min.x;
    int32_t max_x = bounds->max.x;
    int32_t min_z = bounds->min.z;
    int32_t max_z = bounds->max.z;

    if (big_push) {
        min_x -= coll->radius;
        max_x += coll->radius;
        min_z -= coll->radius;
        max_z += coll->radius;
    }

    if (rx >= min_x && rx <= max_x && rz >= min_z && rz <= max_z) {
        int32_t l = rx - min_x;
        int32_t r = max_x - rx;
        int32_t t = max_z - rz;
        int32_t b = rz - min_z;

        if (l <= r && l <= t && l <= b) {
            rx -= l;
        } else if (r <= l && r <= t && r <= b) {
            rx += r;
        } else if (t <= l && t <= r && t <= b) {
            rz += t;
        } else {
            rz -= b;
        }

        int32_t ax = (c * rx + s * rz) >> W2V_SHIFT;
        int32_t az = (c * rz - s * rx) >> W2V_SHIFT;

        lara_item->pos.x = item->pos.x + ax;
        lara_item->pos.z = item->pos.z + az;

        rx = (bounds->min.x + bounds->max.x) / 2;
        rz = (bounds->min.z + bounds->max.z) / 2;
        x -= (c * rx + s * rz) >> W2V_SHIFT;
        z -= (c * rz - s * rx) >> W2V_SHIFT;

        if (spaz_on) {
            PHD_ANGLE hitang = lara_item->rot.y - (PHD_180 + Math_Atan(z, x));
            g_Lara.hit_direction = (hitang + PHD_45) / PHD_90;
            if (!g_Lara.hit_frame) {
                Sound_Effect(SFX_LARA_BODYSL, &lara_item->pos, SPM_NORMAL);
            }

            g_Lara.hit_frame++;
            if (g_Lara.hit_frame > 34) {
                g_Lara.hit_frame = 34;
            }
        }

        coll->bad_pos = NO_BAD_POS;
        coll->bad_neg = -STEPUP_HEIGHT;
        coll->bad_ceiling = 0;

        int16_t old_facing = coll->facing;
        coll->facing = Math_Atan(
            lara_item->pos.z - coll->old.z, lara_item->pos.x - coll->old.x);
        Collide_GetCollisionInfo(
            coll, lara_item->pos.x, lara_item->pos.y, lara_item->pos.z,
            lara_item->room_num, LARA_HEIGHT);
        coll->facing = old_facing;

        if (coll->coll_type != COLL_NONE) {
            lara_item->pos.x = coll->old.x;
            lara_item->pos.z = coll->old.z;
        } else {
            coll->old.x = lara_item->pos.x;
            coll->old.y = lara_item->pos.y;
            coll->old.z = lara_item->pos.z;
            Item_UpdateRoom(item, -10);
        }

        if (g_Lara.interact_target.is_moving
            && g_Lara.interact_target.move_count > LARA_PUSH_TIMEOUT) {
            g_Lara.interact_target.is_moving = false;
            g_Lara.gun_status = LGS_ARMLESS;
        }
    }
}

void Lara_TakeDamage(int16_t damage, bool hit_status)
{
    Item_TakeDamage(g_LaraItem, damage, hit_status);
}
