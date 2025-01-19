#include <libtrx/game/const.h>

#define FRAMES_PER_SECOND LOGIC_FPS
#define TICKS_PER_FRAME 2

#define PHD_ONE 0x10000
#define PHD_HALF 0x100

#define NO_BAD_POS (-NO_HEIGHT) // = 32512
#define NO_BAD_NEG (NO_HEIGHT) // = -32512
#define BAD_JUMP_CEILING ((STEP_L * 3) / 4) // = 192
#define STEPUP_HEIGHT ((STEP_L * 3) / 2) // = 384
#define SLOPE_DIF 60
#define WIBBLE_SIZE 32
#define MAX_WIBBLE 2
#define MAX_SHADE 0x300
#define LIGHT_MAP_SIZE 32
#define MAX_ROOM_LIGHT_UNIT (0x2000 / (WIBBLE_SIZE / 2))

#define MIN_SQUARE SQUARE(WALL_L / 3)
#define NO_BOX (-1)
#define NO_ITEM (-1)
#define NO_CAMERA (-1)

#define MAX_FRAMES 10
#define SUNSET_TIMEOUT (40 * 60 * (FRAMES_PER_SECOND)) // = 72000

#define MAX_AUDIO_SAMPLE_BUFFERS 256
#define MAX_AUDIO_SAMPLE_TRACKS 32
#define MAX_CD_TRACKS 64
#define MAX_TEXTURE_PAGES 32
#define MAX_TEXTURES 2048
#define MAX_PALETTES 16
#define MAX_ROOMS 1024
#define MAX_ROOMS_TO_DRAW 100
#define MAX_FLIP_MAPS 10
#define MAX_VERTICES 0x2000
#define MAX_BOUND_ROOMS 128
#define MAX_ITEMS 256
#define MAX_EFFECTS 100
#define MAX_DYNAMIC_LIGHTS 10
#define MAX_LEVELS 24
#define MAX_LEVEL_NAME_SIZE 50 // TODO: get rid of this limit
#define MAX_DEMO_FILES MAX_LEVELS
#define MAX_DEMO_SIZE 9000
#define MAX_REQUESTER_ITEMS 24
#define MAX_SAVE_SLOTS 16
#define MAX_ASSAULT_TIMES 10

#define TEXTURE_PAGE_WIDTH 256
#define TEXTURE_PAGE_HEIGHT 256
#define DEATH_WAIT (5 * 2 * FRAMES_PER_SECOND) // = 300
#define DEATH_WAIT_INPUT (2 * FRAMES_PER_SECOND) // = 60
#define DAMAGE_START 140
#define DAMAGE_LENGTH 14

#define HEAD_TURN (2 * DEG_1) // = 364
#define MAX_HEAD_TILT (22 * DEG_1) // = 4004
#define MIN_HEAD_TILT (-42 * DEG_1) // = -7644
#define MAX_HEAD_ROTATION (44 * DEG_1) // = 8008
#define MIN_HEAD_ROTATION (-MAX_HEAD_ROTATION) // = -8008
#define HEAD_TURN_CAM (4 * DEG_1) // = 728
#define MAX_HEAD_TILT_CAM (85 * DEG_1) // = 15470
#define MIN_HEAD_TILT_CAM (-MAX_HEAD_TILT_CAM) // = -15470
#define MAX_HEAD_ROTATION_CAM (50 * DEG_1) // = 9100
#define MIN_HEAD_ROTATION_CAM (-MAX_HEAD_ROTATION_CAM) // = -9100
#define LARA_TURN_UNDO (2 * DEG_1) // = 364
#define LARA_TURN_RATE ((DEG_1 / 4) + LARA_TURN_UNDO) // = 409
#define LARA_TURN_RATE_UW (2 * DEG_1) // = 364
#define LARA_MED_TURN ((DEG_1 * 4) + LARA_TURN_UNDO) // = 1092
#define LARA_SLOW_TURN ((DEG_1 * 2) + LARA_TURN_UNDO) // = 728
#define LARA_SURF_TURN (LARA_SLOW_TURN / 2) // = 364
#define LARA_FAST_TURN ((DEG_1 * 6) + LARA_TURN_UNDO) // = 1456
#define LARA_LEAN_UNDO DEG_1 // = 182
#define LARA_LEAN_UNDO_SURF (LARA_LEAN_UNDO * 2) // = 364
#define LARA_LEAN_UNDO_UW LARA_LEAN_UNDO_SURF // = 364
#define LARA_LEAN_RATE 273
#define LARA_LEAN_RATE_SWIM (LARA_LEAN_RATE * 2) // = 546
#define LARA_LEAN_MAX ((10 * DEG_1) + LARA_LEAN_UNDO) // = 2002
#define LARA_LEAN_MAX_UW (LARA_LEAN_MAX * 2) // = 4004
#define LARA_JUMP_TURN ((DEG_1 * 1) + LARA_TURN_UNDO) // = 546
#define LARA_FAST_FALL_SPEED (FAST_FALL_SPEED + 3) // = 131
#define LARA_MAX_SURF_SPEED 60
#define LARA_MAX_SWIM_SPEED 200
#define LARA_UW_FRICTION 6
#define LARA_UW_WALL_DEFLECT (2 * DEG_1) // = 364
#define LARA_CLIMB_WIDTH_LEFT 80
#define LARA_CLIMB_WIDTH_RIGHT 120
#define LARA_CLIMB_HEIGHT (WALL_L / 2) // = 512
#define LARA_WADE_DEPTH 384
#define LARA_SWIM_DEPTH 730
#define LARA_RADIUS 100
#define LARA_RADIUS_UW 300
#define LARA_HEIGHT 762
#define LARA_HEIGHT_SURF 700
#define LARA_HEIGHT_UW 400
#define LARA_DEFLECT_ANGLE (5 * DEG_1) // = 910
#define LARA_HANG_ANGLE (35 * DEG_1) // = 6370
#define LARA_VAULT_ANGLE (30 * DEG_1) // = 5460
#define LARA_MAX_AIR 1800
#define LARA_MAX_HITPOINTS 1000

#define CAM_HANG_ANGLE 0
#define CAM_HANG_ELEVATION (-60 * DEG_1) // = -10920
#define CAM_REACH_ANGLE (85 * DEG_1) // = 15470
#define CAM_SLIDE_ELEVATION (-45 * DEG_1) // = -8190
#define CAM_BACK_JUMP_ANGLE (135 * DEG_1) // = 24570
#define CAM_PUSH_BLOCK_ANGLE (35 * DEG_1) // = 6370
#define CAM_PUSH_BLOCK_ELEVATION (-25 * DEG_1) // = -4550
#define CAM_PP_READY_ANGLE (75 * DEG_1) // = 13650
#define CAM_PICKUP_ANGLE (-130 * DEG_1) // = -23660
#define CAM_PICKUP_ELEVATION (-15 * DEG_1) // = -2730
#define CAM_PICKUP_DISTANCE WALL_L // = 1024
#define CAM_SWITCH_ON_ANGLE (80 * DEG_1) // = 14560
#define CAM_SWITCH_ON_ELEVATION (-25 * DEG_1) // = -4550
#define CAM_SWITCH_ON_DISTANCE WALL_L // = 1024
#define CAM_SWITCH_ON_SPEED 6
#define CAM_USE_KEY_ANGLE (-CAM_SWITCH_ON_ANGLE) // = -14560
#define CAM_USE_KEY_ELEVATION CAM_SWITCH_ON_ELEVATION // = -4550
#define CAM_USE_KEY_DISTANCE WALL_L // = 1024
#define CAM_SPECIAL_ANGLE (170 * DEG_1) // = 30940
#define CAM_SPECIAL_ELEVATION (-25 * DEG_1) // = -4550
#define CAM_WADE_ELEVATION (-22 * DEG_1) // = -4004
#define CAM_ZIPLINE_ANGLE (70 * DEG_1) // = 12740
#define CAM_YETI_KILL_ANGLE (160 * DEG_1) // = 29120
#define CAM_YETI_KILL_DISTANCE (3 * WALL_L) // = 3072
#define CAM_SHARK_KILL_ANGLE (160 * DEG_1) // = 29120
#define CAM_SHARK_KILL_DISTANCE (3 * WALL_L) // = 3072
#define CAM_AIRLOCK_ANGLE (80 * DEG_1) // = 14560
#define CAM_AIRLOCK_ELEVATION (-25 * DEG_1) // = -4550
#define CAM_GONG_BONG_ANGLE (-25 * DEG_1) // = -4550
#define CAM_GONG_BONG_ELEVATION (-20 * DEG_1) // = -3640
#define CAM_GONG_BONG_DISTANCE (3 * WALL_L) // = 3072
#define CAM_TREX_KILL_ANGLE (170 * DEG_1) // = 30940
#define CAM_TREX_KILL_ELEVATION (-25 * DEG_1) // = -4550
#define CAM_CLIMB_LEFT_ANGLE (-30 * DEG_1) // = -5460
#define CAM_CLIMB_LEFT_ELEVATION (-15 * DEG_1) // = -2730
#define CAM_CLIMB_RIGHT_ANGLE (-CAM_CLIMB_LEFT_ANGLE) // = 5460
#define CAM_CLIMB_RIGHT_ELEVATION CAM_CLIMB_LEFT_ELEVATION // = -2730
#define CAM_CLIMB_STANCE_ELEVATION (-20 * DEG_1) // = -3640
#define CAM_CLIMBING_ELEVATION (30 * DEG_1) // = 5460
#define CAM_CLIMB_END_ELEVATION (-45 * DEG_1) // = -8190
#define CAM_CLIMB_DOWN_ELEVATION CAM_CLIMB_END_ELEVATION // = -8190

#define SW_DETAIL_LOW (0 * WALL_L << W2V_SHIFT) // = 0
#define SW_DETAIL_MEDIUM (3 * WALL_L << W2V_SHIFT) // = 50331648
#define SW_DETAIL_HIGH (6 * WALL_L << W2V_SHIFT) // = 100663296
#define SW_DETAIL_ULTRA (20 * WALL_L << W2V_SHIFT) // = 335544320

#define SPRITE_REL 0x00000000
#define SPRITE_ABS 0x01000000
#define SPRITE_SEMI_TRANS 0x02000000
#define SPRITE_SCALE 0x04000000
#define SPRITE_SHADE 0x08000000
#define SPRITE_TINT 0x10000000
#define SPRITE_TRANS_HALF 0x00000000
#define SPRITE_TRANS_ADD 0x20000000
#define SPRITE_TRANS_SUB 0x40000000
#define SPRITE_TRANS_QUARTER 0x60000000
#define SPRITE_COLOUR(r, g, b) ((r) | ((g) << 8) | ((b) << 16))

#define VIEW_NEAR (20 * 1) // = 20
#define VIEW_FAR (20 * WALL_L) // = 20480

#define FOG_START (12 * WALL_L) // = 12288
#define FOG_END (20 * WALL_L) // = 20480

#define NUM_SLOTS 5
#define PITCH_SHIFT 4
#define TARGET_DIST (WALL_L * 4) // = 4096

#define IDS_DX5_REQUIRED 1

#define MONK_FRIENDLY_FIRE_THRESHOLD 10

#define GUN_AMMO_CLIP 16
#define GUN_AMMO_QTY (GUN_AMMO_CLIP * 2) // = 32

#define MAGNUM_AMMO_CLIP 20
#define MAGNUM_AMMO_QTY (MAGNUM_AMMO_CLIP * 2) // = 40

#define UZI_AMMO_CLIP 40
#define UZI_AMMO_QTY (UZI_AMMO_CLIP * 2) // =  80

#define M16_AMMO_CLIP 40
#define M16_AMMO_QTY M16_AMMO_CLIP // = 40

#define SHOTGUN_SHELL_COUNT 2
#define SHOTGUN_AMMO_CLIP 6
#define SHOTGUN_AMMO_QTY (SHOTGUN_AMMO_CLIP * SHOTGUN_SHELL_COUNT) // = 12
#define SHOTGUN_PELLET_SCATTER (DEG_1 * 20) // = 3640

#define HARPOON_BOLT_SPEED 150
#define HARPOON_RECOIL 4
#define HARPOON_AMMO_CLIP 3
#define HARPOON_AMMO_QTY HARPOON_AMMO_CLIP // = 3

#define GRENADE_SPEED 200
#define GRENADE_AMMO_CLIP 1
#define GRENADE_AMMO_QTY (GRENADE_AMMO_CLIP * 2) // = 2

#if defined(PSX_VERSION) && defined(JAPAN)
    #define FLARE_AMMO_BOX 8
#else
    #define FLARE_AMMO_BOX 6
#endif
#define FLARE_AMMO_QTY FLARE_AMMO_BOX

#define LOW_LIGHT 5120
#define HIGH_LIGHT 4096

#define UNIT_SHADOW 256
