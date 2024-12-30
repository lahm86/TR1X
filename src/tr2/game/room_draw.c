#include "game/room_draw.h"

#include "decomp/decomp.h"
#include "game/effects.h"
#include "game/lara/draw.h"
#include "game/matrix.h"
#include "game/output.h"
#include "global/vars.h"

#include <libtrx/utils.h>

static int32_t m_Outside;
static int32_t m_OutsideRight;
static int32_t m_OutsideLeft;
static int32_t m_OutsideTop;
static int32_t m_OutsideBottom;

static int32_t m_BoundStart;
static int32_t m_BoundEnd;
static int32_t m_BoundRooms[MAX_BOUND_ROOMS] = {};

static int32_t m_BoxLines[12][2] = {
    { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 }, { 4, 5 }, { 5, 6 },
    { 6, 7 }, { 7, 4 }, { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 },
};

void Room_MarkToBeDrawn(const int16_t room_num)
{
    for (int32_t i = 0; i < g_RoomsToDrawCount; i++) {
        if (g_RoomsToDraw[i] == room_num) {
            return;
        }
    }

    if (g_RoomsToDrawCount + 1 < MAX_ROOMS_TO_DRAW) {
        g_RoomsToDraw[g_RoomsToDrawCount++] = room_num;
    }
}

void Room_GetBounds(void)
{
    while (m_BoundStart != m_BoundEnd) {
        const int16_t room_num = m_BoundRooms[m_BoundStart++ % MAX_BOUND_ROOMS];
        ROOM *const r = &g_Rooms[room_num];
        r->bound_active &= ~2;
        g_MidSort = (r->bound_active >> 8) + 1;

        if (r->test_left < r->bound_left) {
            r->bound_left = r->test_left;
        }
        if (r->test_top < r->bound_top) {
            r->bound_top = r->test_top;
        }
        if (r->test_right > r->bound_right) {
            r->bound_right = r->test_right;
        }
        if (r->test_bottom > r->bound_bottom) {
            r->bound_bottom = r->test_bottom;
        }

        if (!(r->bound_active & 1)) {
            Room_MarkToBeDrawn(room_num);
            r->bound_active |= 1;
            if (r->flags & RF_OUTSIDE) {
                m_Outside = RF_OUTSIDE;
            }
        }

        if (!(r->flags & RF_INSIDE) || (r->flags & RF_OUTSIDE)) {
            if (r->bound_left < m_OutsideLeft) {
                m_OutsideLeft = r->bound_left;
            }
            if (r->bound_right > m_OutsideRight) {
                m_OutsideRight = r->bound_right;
            }
            if (r->bound_top < m_OutsideTop) {
                m_OutsideTop = r->bound_top;
            }
            if (r->bound_bottom > m_OutsideBottom) {
                m_OutsideBottom = r->bound_bottom;
            }
        }

        if (r->portals == NULL) {
            continue;
        }

        Matrix_Push();
        Matrix_TranslateAbs(r->pos.x, r->pos.y, r->pos.z);
        for (int32_t i = 0; i < r->portals->count; i++) {
            const PORTAL *const portal = &r->portals->portal[i];

            // clang-format off
            const XYZ_32 offset = {
                .x = portal->normal.x * (r->pos.x + portal->vertex[0].x - g_W2VMatrix._03),
                .y = portal->normal.y * (r->pos.y + portal->vertex[0].y - g_W2VMatrix._13),
                .z = portal->normal.z * (r->pos.z + portal->vertex[0].z - g_W2VMatrix._23),
            };
            // clang-format on

            if (offset.x + offset.y + offset.z >= 0) {
                continue;
            }

            Room_SetBounds(&portal->normal.x, portal->room_num, r);
        }
        Matrix_Pop();
    }
}

void Room_SetBounds(
    const int16_t *obj_ptr, int32_t room_num, const ROOM *parent)
{
    ROOM *const r = &g_Rooms[room_num];
    const PORTAL *const portal = (const PORTAL *)(obj_ptr - 1);

    // clang-format off
    if (r->bound_left <= parent->test_left &&
        r->bound_right >= parent->test_right &&
        r->bound_top <= parent->test_top &&
        r->bound_bottom >= parent->test_bottom
   ) {
        return;
    }
    // clang-format on

    const MATRIX *const m = g_MatrixPtr;
    int32_t left = parent->test_right;
    int32_t right = parent->test_left;
    int32_t bottom = parent->test_top;
    int32_t top = parent->test_bottom;

    PORTAL_VBUF portal_vbuf[4];
    int32_t z_behind = 0;

    for (int32_t i = 0; i < 4; i++) {
        PORTAL_VBUF *const dvbuf = &portal_vbuf[i];
        const XYZ_16 *const dvtx = &portal->vertex[i];
        const int32_t xv =
            dvtx->x * m->_00 + dvtx->y * m->_01 + dvtx->z * m->_02 + m->_03;
        const int32_t yv =
            dvtx->x * m->_10 + dvtx->y * m->_11 + dvtx->z * m->_12 + m->_13;
        const int32_t zv =
            dvtx->x * m->_20 + dvtx->y * m->_21 + dvtx->z * m->_22 + m->_23;
        dvbuf->xv = xv;
        dvbuf->yv = yv;
        dvbuf->zv = zv;

        if (zv <= 0) {
            z_behind++;
            continue;
        }

        int32_t xs;
        int32_t ys;
        const int32_t zp = zv / g_PhdPersp;
        if (zp) {
            xs = xv / zp + g_PhdWinCenterX;
            ys = yv / zp + g_PhdWinCenterY;
        } else {
            xs = xv < 0 ? g_PhdWinLeft : g_PhdWinRight;
            ys = yv < 0 ? g_PhdWinTop : g_PhdWinBottom;
        }

        if (xs - 1 < left) {
            left = xs - 1;
        }
        if (xs + 1 > right) {
            right = xs + 1;
        }
        if (ys - 1 < top) {
            top = ys - 1;
        }
        if (ys + 1 > bottom) {
            bottom = ys + 1;
        }
    }

    if (z_behind == 4) {
        return;
    }

    if (z_behind > 0) {
        const PORTAL_VBUF *dest = &portal_vbuf[0];
        const PORTAL_VBUF *last = &portal_vbuf[3];

        for (int32_t i = 0; i < 4; i++, last = dest++) {
            if ((dest->zv < 0) == (last->zv < 0)) {
                continue;
            }

            if (dest->xv < 0 && last->xv < 0) {
                left = 0;
            } else if (dest->xv > 0 && last->xv > 0) {
                right = g_PhdWinMaxX;
            } else {
                left = 0;
                right = g_PhdWinMaxX;
            }

            if (dest->yv < 0 && last->yv < 0) {
                top = 0;
            } else if (dest->yv > 0 && last->yv > 0) {
                bottom = g_PhdWinMaxY;
            } else {
                top = 0;
                bottom = g_PhdWinMaxY;
            }
        }
    }

    if (left < parent->test_left) {
        left = parent->test_left;
    }
    if (right > parent->test_right) {
        right = parent->test_right;
    }
    if (top < parent->test_top) {
        top = parent->test_top;
    }
    if (bottom > parent->test_bottom) {
        bottom = parent->test_bottom;
    }

    if (left >= right || top >= bottom) {
        return;
    }

    if (r->bound_active & 2) {
        if (left < r->test_left) {
            r->test_left = left;
        }
        if (top < r->test_top) {
            r->test_top = top;
        }
        if (right > r->test_right) {
            r->test_right = right;
        }
        if (bottom > r->test_bottom) {
            r->test_bottom = bottom;
        }
    } else {
        m_BoundRooms[m_BoundEnd++ % MAX_BOUND_ROOMS] = room_num;
        r->bound_active |= 2;
        r->bound_active += (int16_t)(g_MidSort << 8);
        r->test_left = left;
        r->test_right = right;
        r->test_top = top;
        r->test_bottom = bottom;
    }
}

void Room_Clip(const ROOM *const r)
{
    int32_t xv[8];
    int32_t yv[8];
    int32_t zv[8];

    xv[0] = WALL_L;
    yv[0] = r->max_ceiling - r->pos.y;
    zv[0] = WALL_L;

    xv[1] = (r->size.x - 1) * WALL_L;
    yv[1] = r->max_ceiling - r->pos.y;
    zv[1] = WALL_L;

    xv[2] = (r->size.x - 1) * WALL_L;
    yv[2] = r->max_ceiling - r->pos.y;
    zv[2] = (r->size.z - 1) * WALL_L;

    xv[3] = WALL_L;
    yv[3] = r->max_ceiling - r->pos.y;
    zv[3] = (r->size.z - 1) * WALL_L;

    xv[4] = WALL_L;
    yv[4] = r->min_floor - r->pos.y;
    zv[4] = WALL_L;

    xv[5] = (r->size.x - 1) * WALL_L;
    yv[5] = r->min_floor - r->pos.y;
    zv[5] = WALL_L;

    xv[6] = (r->size.x - 1) * WALL_L;
    yv[6] = r->min_floor - r->pos.y;
    zv[6] = (r->size.z - 1) * WALL_L;

    xv[7] = WALL_L;
    yv[7] = r->min_floor - r->pos.y;
    zv[7] = (r->size.z - 1) * WALL_L;

    bool clip_room = false;
    bool clip[8];

    const MATRIX *const m = g_MatrixPtr;
    for (int32_t i = 0; i < 8; i++) {
        const int32_t x = xv[i];
        const int32_t y = yv[i];
        const int32_t z = zv[i];
        xv[i] = x * m->_00 + y * m->_01 + z * m->_02 + m->_03;
        yv[i] = x * m->_10 + y * m->_11 + z * m->_12 + m->_13;
        zv[i] = x * m->_20 + y * m->_21 + z * m->_22 + m->_23;
        if (zv[i] > g_PhdFarZ) {
            clip_room = true;
            clip[i] = true;
        } else {
            clip[i] = false;
        }
    }

    if (!clip_room) {
        return;
    }

    int32_t min_x = 0x10000000;
    int32_t min_y = 0x10000000;
    int32_t max_x = -0x10000000;
    int32_t max_y = -0x10000000;
    for (int32_t i = 0; i < 12; i++) {
        const int32_t p1 = m_BoxLines[i][0];
        const int32_t p2 = m_BoxLines[i][1];

        if (clip[p1] == clip[p2]) {
            continue;
        }

        const int32_t zdiv = (zv[p2] - zv[p1]) >> W2V_SHIFT;
        if (zdiv) {
            const int32_t znom = (g_PhdFarZ - zv[p1]) >> W2V_SHIFT;
            const int32_t x = xv[p1]
                + ((((xv[p2] - xv[p1]) >> W2V_SHIFT) * znom / zdiv)
                   << W2V_SHIFT);
            const int32_t y = yv[p1]
                + ((((yv[p2] - yv[p1]) >> W2V_SHIFT) * znom / zdiv)
                   << W2V_SHIFT);

            CLAMPG(min_x, x);
            CLAMPL(max_x, x);
            CLAMPG(min_y, y);
            CLAMPL(max_y, y);
        } else {
            CLAMPG(min_x, xv[p1]);
            CLAMPG(min_x, xv[p2]);
            CLAMPL(max_x, xv[p1]);
            CLAMPL(max_x, xv[p2]);
            CLAMPG(min_y, yv[p1]);
            CLAMPG(min_y, yv[p2]);
            CLAMPL(max_y, yv[p1]);
            CLAMPL(max_y, yv[p2]);
        }
    }

    const int32_t zp = g_PhdFarZ / g_PhdPersp;
    min_x = g_PhdWinCenterX + min_x / zp;
    min_y = g_PhdWinCenterY + min_y / zp;
    max_x = g_PhdWinCenterX + max_x / zp;
    max_y = g_PhdWinCenterY + max_y / zp;

    // clang-format off
    if (min_x > g_PhdWinRight ||
        min_y > g_PhdWinBottom ||
        max_x < g_PhdWinLeft ||
        max_y < g_PhdWinTop
    ) {
        return;
    }
    // clang-format on

    CLAMPL(min_x, g_PhdWinLeft);
    CLAMPL(min_y, g_PhdWinTop);
    CLAMPG(max_x, g_PhdWinRight);
    CLAMPG(max_y, g_PhdWinBottom);
    Output_InsertBackPolygon(min_x, min_y, max_x, max_y);
}

void Room_DrawSingleRoomGeometry(const int16_t room_num)
{
    ROOM *const r = &g_Rooms[room_num];

    if (r->flags & RF_UNDERWATER) {
        Output_SetupBelowWater(g_CameraUnderwater);
    } else {
        Output_SetupAboveWater(g_CameraUnderwater);
    }

    Matrix_TranslateAbs(r->pos.x, r->pos.y, r->pos.z);
    g_PhdWinLeft = r->bound_left;
    g_PhdWinRight = r->bound_right;
    g_PhdWinTop = r->bound_top;
    g_PhdWinBottom = r->bound_bottom;

    Output_LightRoom(r);
    if (m_Outside > 0 && !(r->flags & RF_INSIDE)) {
        Output_InsertRoom(r->data, true);
    } else {
        if (m_Outside >= 0) {
            Room_Clip(r);
        }
        Output_InsertRoom(r->data, false);
    }
}

void Room_DrawSingleRoomObjects(const int16_t room_num)
{
    ROOM *const r = &g_Rooms[room_num];

    if (r->flags & RF_UNDERWATER) {
        Output_SetupBelowWater(g_CameraUnderwater);
    } else {
        Output_SetupAboveWater(g_CameraUnderwater);
    }

    r->bound_active = 0;

    Matrix_Push();
    Matrix_TranslateAbs(r->pos.x, r->pos.y, r->pos.z);

    g_PhdWinLeft = r->bound_left;
    g_PhdWinTop = r->bound_top;
    g_PhdWinRight = r->bound_right;
    g_PhdWinBottom = r->bound_bottom;

    for (int32_t i = 0; i < r->num_static_meshes; i++) {
        const STATIC_MESH *const mesh = &r->static_meshes[i];
        const STATIC_INFO *const static_obj =
            &g_StaticObjects[mesh->static_num];
        if (!(static_obj->flags & SMF_VISIBLE)) {
            continue;
        }

        Matrix_Push();
        Matrix_TranslateAbs(mesh->pos.x, mesh->pos.y, mesh->pos.z);
        Matrix_RotY(mesh->rot.y);
        const int16_t bounds = Output_GetObjectBounds(&static_obj->draw_bounds);
        if (bounds) {
            Output_CalculateStaticMeshLight(
                mesh->pos.x, mesh->pos.y, mesh->pos.z, mesh->shade_1,
                mesh->shade_2, r);
            Output_InsertPolygons(g_Meshes[static_obj->mesh_idx], bounds);
        }
        Matrix_Pop();
    }

    g_PhdWinLeft = 0;
    g_PhdWinTop = 0;
    g_PhdWinRight = g_PhdWinMaxX + 1;
    g_PhdWinBottom = g_PhdWinMaxY + 1;

    int16_t item_num = r->item_num;
    while (item_num != NO_ITEM) {
        ITEM *const item = &g_Items[item_num];
        if (item->status != IS_INVISIBLE) {
            const OBJECT *const object = &g_Objects[item->object_id];
            object->draw_routine(item);
        }
        item_num = item->next_item;
    }

    int16_t effect_num = r->effect_num;
    while (effect_num != NO_EFFECT) {
        const EFFECT *const effect = Effect_Get(effect_num);
        Effect_Draw(effect_num);
        effect_num = effect->next_free;
    }

    Matrix_Pop();

    r->bound_left = g_PhdWinMaxX;
    r->bound_top = g_PhdWinMaxY;
    r->bound_right = 0;
    r->bound_bottom = 0;
}

void Room_DrawAllRooms(const int16_t current_room)
{
    ROOM *const r = &g_Rooms[current_room];
    r->test_left = 0;
    r->test_top = 0;
    r->test_right = g_PhdWinMaxX;
    r->test_bottom = g_PhdWinMaxY;
    r->bound_active = 2;

    g_PhdWinLeft = r->test_left;
    g_PhdWinTop = r->test_top;
    g_PhdWinRight = r->test_right;
    g_PhdWinBottom = r->test_bottom;

    m_BoundRooms[0] = current_room;
    m_BoundStart = 0;
    m_BoundEnd = 1;

    g_RoomsToDrawCount = 0;
    m_Outside = r->flags & RF_OUTSIDE;

    if (m_Outside) {
        m_OutsideTop = 0;
        m_OutsideLeft = 0;
        m_OutsideRight = g_PhdWinMaxX;
        m_OutsideBottom = g_PhdWinMaxY;
    } else {
        m_OutsideLeft = g_PhdWinMaxX;
        m_OutsideTop = g_PhdWinMaxY;
        m_OutsideBottom = 0;
        m_OutsideRight = 0;
    }

    g_CameraUnderwater = r->flags & RF_UNDERWATER;
    Room_GetBounds();

    g_MidSort = 0;
    if (m_Outside) {
        g_PhdWinLeft = m_OutsideLeft;
        g_PhdWinRight = m_OutsideRight;
        g_PhdWinBottom = m_OutsideBottom;
        g_PhdWinTop = m_OutsideTop;
        if (g_Objects[O_SKYBOX].loaded) {
            Output_SetupAboveWater(g_CameraUnderwater);
            Matrix_Push();
            g_MatrixPtr->_03 = 0;
            g_MatrixPtr->_13 = 0;
            g_MatrixPtr->_23 = 0;
            const int16_t *frame =
                g_Anims[g_Objects[O_SKYBOX].anim_idx].frame_ptr + FBBOX_ROT;
            Matrix_RotYXZsuperpack(&frame, 0);
            Output_InsertSkybox(g_Meshes[g_Objects[O_SKYBOX].mesh_idx]);
            Matrix_Pop();
        } else {
            m_Outside = -1;
        }
    }

    if (g_Objects[O_LARA].loaded && !(g_LaraItem->flags & IF_ONE_SHOT)) {
        g_MidSort = g_Rooms[g_LaraItem->room_num].bound_active >> 8;
        if (g_MidSort) {
            g_MidSort--;
        }
        Lara_Draw(g_LaraItem);
    }

    for (int32_t i = 0; i < g_RoomsToDrawCount; i++) {
        const int16_t room_num = g_RoomsToDraw[i];
        Room_DrawSingleRoomGeometry(room_num);
    }

    for (int32_t i = 0; i < g_RoomsToDrawCount; i++) {
        const int16_t room_num = g_RoomsToDraw[i];
        Room_DrawSingleRoomObjects(room_num);
    }
}
