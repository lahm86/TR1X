#pragma once

#include "./types.h"

int32_t Math_Cos(int32_t angle);
int32_t Math_Sin(int32_t angle);
int32_t Math_Atan(int32_t x, int32_t y);
uint32_t Math_Sqrt(uint32_t n);

void Math_GetVectorAngles(int32_t x, int32_t y, int32_t z, int16_t *dest);
int32_t Math_AngleInCone(int32_t angle1, int32_t angle2, int32_t cone);
DIRECTION Math_GetDirection(int16_t angle);
DIRECTION Math_GetDirectionCone(int16_t angle, int16_t cone);
int16_t Math_DirectionToAngle(DIRECTION dir);
int32_t Math_AngleMean(int32_t angle1, int32_t angle2, double ratio);
int32_t XYZ_32_GetDistance(const XYZ_32 *pos1, const XYZ_32 *pos2);
int32_t XYZ_32_GetDistance0(const XYZ_32 *pos);
bool XYZ_32_AreEquivalent(const XYZ_32 *pos1, const XYZ_32 *pos2);
