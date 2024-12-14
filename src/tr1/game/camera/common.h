#pragma once

#include "global/types.h"

#include <stdint.h>

#define CAMERA_DEFAULT_DISTANCE (WALL_L * 3 / 2)

void Camera_Initialise(void);
bool Camera_SetType(CAMERA_TYPE type);
void Camera_Reset(void);
void Camera_ResetPosition(void);
void Camera_Update(void);
void Camera_UpdateCutscene(void);
void Camera_RefreshFromTrigger(const TRIGGER *trigger);
void Camera_MoveManual(void);
void Camera_Apply(void);
