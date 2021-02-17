#ifndef TR1MAIN_GAME_LARA_H
#define TR1MAIN_GAME_LARA_H

#include "game/types.h"
#include "util.h"

// clang-format off
#define InitialiseLaraInventory ((void          __cdecl(*)(int level_id))0x00428170)
#define LaraControl             ((void          __cdecl(*)(int16_t item_num))0x00427850)
#define UpdateLaraRoom          ((void          __cdecl(*)(ITEM_INFO* item, int height))0x004126A0)
#define ShiftItem               ((void          __cdecl(*)(ITEM_INFO* item, COLL_INFO *coll))0x00412660)
#define AnimateLara             ((void          __cdecl(*)(ITEM_INFO* item))0x00427C00)
#define LaraGun                 ((void          __cdecl(*)())0x00426BD0)
#define LaraWaterCurrent        ((void          __cdecl(*)(COLL_INFO* coll))0x00429440)
#define LaraDeflectEdgeJump     ((void          __cdecl(*)(ITEM_INFO *item, COLL_INFO* coll))0x004255A0)
#define LaraSlideSlope          ((void          __cdecl(*)(ITEM_INFO* item, COLL_INFO* coll))0x004251D0)
#define LaraLandedBad           ((int32_t       __cdecl(*)(ITEM_INFO *item, COLL_INFO* coll))0x00425D70)
#define LaraHangTest            ((void          __cdecl(*)(ITEM_INFO *item, COLL_INFO *coll))0x00425350)
#define LaraTestHangJump        ((int32_t       __cdecl(*)(ITEM_INFO *item, COLL_INFO *coll))0x00425890)
#define TestLaraVault           ((int32_t       __cdecl(*)(ITEM_INFO *item, COLL_INFO *coll))0x004256C0)
#define TestLaraSlide           ((int32_t       __cdecl(*)(ITEM_INFO *item, COLL_INFO *coll))0x00425C50)
// clang-format on

void __cdecl InitialiseLara();
void __cdecl LaraAboveWater(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraUnderWater(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsWalk(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsRun(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsStop(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsForwardJump(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsPose(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsFastBack(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsTurnR(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsTurnL(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsDeath(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsFastFall(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsHang(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsReach(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsSplat(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsLand(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsCompress(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsBack(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsFastTurn(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsStepRight(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsStepLeft(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsSlide(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsBackJump(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsRightJump(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsLeftJump(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsUpJump(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsFallBack(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsHangLeft(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsHangRight(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsSlideBack(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsPushBlock(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsPullBlock(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsPPReady(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsPickup(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsSwitchOn(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsSwitchOff(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsUseKey(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsUsePuzzle(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsRoll(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsRoll2(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsSpecial(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsUseMidas(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsDieMidas(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsSwanDive(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsFastDive(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsGymnast(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraAsWaterOut(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColWalk(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColRun(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColStop(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColForwardJump(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColFastBack(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColTurnR(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColDeath(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColFastFall(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColHang(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColReach(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColSplat(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColLand(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColCompress(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColBack(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColStepRight(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColStepLeft(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColSlide(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColBackJump(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColRightJump(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColLeftJump(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraColJumper(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl GetLaraCollisionInfo(ITEM_INFO* item, COLL_INFO* coll);
int32_t __cdecl LaraHitCeiling(ITEM_INFO* item, COLL_INFO* coll);
int32_t __cdecl LaraDeflectEdge(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl LaraSlideEdgeJump(ITEM_INFO* item, COLL_INFO* coll);
int16_t __cdecl LaraFloorFront(ITEM_INFO* item, PHD_ANGLE ang, int32_t dist);
void __cdecl UseItem(__int16 object_num);

void TR1MLookLeftRight();
void TR1MResetLook();

void TR1MInjectLara();
void TR1MInjectLaraMisc();
void TR1MInjectLaraSwim();

#endif
