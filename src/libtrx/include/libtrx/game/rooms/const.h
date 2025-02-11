#pragma once

#define NO_ROOM_NEG (-1)
#define NO_ROOM 255

#define NEG_TILT(T, H) ((T * (H & (WALL_L - 1))) >> 2)
#define POS_TILT(T, H) ((T * ((WALL_L - 1 - H) & (WALL_L - 1))) >> 2)

#define ROUND_TO_CLICK(V) ((V) & ~(STEP_L - 1))
#define ROUND_TO_SECTOR(V) ((V) & ~(WALL_L - 1))

#define MAX_LIGHTING 0x1FFF
