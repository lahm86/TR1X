#pragma once

#include "./types.h"

void Box_InitialiseBoxes(int32_t num_boxes);
int16_t *Box_InitialiseOverlaps(int32_t num_overlaps);
int32_t Box_GetCount(void);
BOX_INFO *Box_GetBox(int32_t box_idx);
int16_t Box_GetOverlap(int32_t overlap_idx);
