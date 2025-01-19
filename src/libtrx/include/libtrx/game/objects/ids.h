#pragma once

typedef enum {
    NO_OBJECT = -1,
#define OBJ_ID_DEFINE(object_id, enum_value) object_id = enum_value,
#include "./ids.def"
    O_NUMBER_OF,
} GAME_OBJECT_ID;

typedef enum {
    // clang-format off
    STATIC_PLANT_0        = 0,
    STATIC_PLANT_1        = 1,
    STATIC_PLANT_2        = 2,
    STATIC_PLANT_3        = 3,
    STATIC_PLANT_4        = 4,
    STATIC_PLANT_5        = 5,
    STATIC_PLANT_6        = 6,
    STATIC_PLANT_7        = 7,
    STATIC_PLANT_8        = 8,
    STATIC_PLANT_9        = 9,
    STATIC_FURNITURE_0    = 10,
    STATIC_FURNITURE_1    = 11,
    STATIC_FURNITURE_2    = 12,
    STATIC_FURNITURE_3    = 13,
    STATIC_FURNITURE_4    = 14,
    STATIC_FURNITURE_5    = 15,
    STATIC_FURNITURE_6    = 16,
    STATIC_FURNITURE_7    = 17,
    STATIC_FURNITURE_8    = 18,
    STATIC_FURNITURE_9    = 19,
    STATIC_ROCK_0         = 20,
    STATIC_ROCK_1         = 21,
    STATIC_ROCK_2         = 22,
    STATIC_ROCK_3         = 23,
    STATIC_ROCK_4         = 24,
    STATIC_ROCK_5         = 25,
    STATIC_ROCK_6         = 26,
    STATIC_ROCK_7         = 27,
    STATIC_ROCK_8         = 28,
    STATIC_ROCK_9         = 29,
    STATIC_ARCHITECTURE_0 = 30,
    STATIC_ARCHITECTURE_1 = 31,
    STATIC_ARCHITECTURE_2 = 32,
    STATIC_ARCHITECTURE_3 = 33,
    STATIC_ARCHITECTURE_4 = 34,
    STATIC_ARCHITECTURE_5 = 35,
    STATIC_ARCHITECTURE_6 = 36,
    STATIC_ARCHITECTURE_7 = 37,
    STATIC_ARCHITECTURE_8 = 38,
    STATIC_ARCHITECTURE_9 = 39,
    STATIC_DEBRIS_0       = 40,
    STATIC_DEBRIS_1       = 41,
    STATIC_DEBRIS_2       = 42,
    STATIC_DEBRIS_3       = 43,
    STATIC_DEBRIS_4       = 44,
    STATIC_DEBRIS_5       = 45,
    STATIC_DEBRIS_6       = 46,
    STATIC_DEBRIS_7       = 47,
    STATIC_DEBRIS_8       = 48,
    STATIC_DEBRIS_9       = 49,
    STATIC_NUMBER_OF      = 50,
    // clang-format on
} GAME_STATIC_ID;
