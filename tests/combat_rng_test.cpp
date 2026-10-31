// Real combat side effects, without artwork. Run with the desktop engine objects.
#include "function.h"
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
static int seen[ANIM_COUNT]={0};
static int craters=0;
static unsigned long digest = 2166136261UL;
static void mix(unsigned long n) { digest = ((digest ^ n) * 16777619UL) & 0xffffffffUL; }
static void snapshot()
{
 mix(Scen.RandomNumber.Seed & 0xffffffffUL);
 mix(Anims.Count());
 for (int i=0; i<Anims.Count(); ++i) {
  AnimClass *a=Anims.Ptr(i);
  ++seen[a->Class->Type];
  mix(a->Class->Type); mix(a->Coord); mix(a->Delay); mix(a->Loops);
 }
 for (int c=0; c<MAP_CELL_TOTAL; ++c) {
  if (Map[(CELL)c].Smudge != SMUDGE_NONE) {
   if (Map[(CELL)c].Smudge>=SMUDGE_CRATER1 && Map[(CELL)c].Smudge<=SMUDGE_CRATER6) ++craters;
   mix(c); mix(Map[(CELL)c].Smudge); mix(Map[(CELL)c].SmudgeData);
  }
 }
 while (Anims.Count()) delete Anims.Ptr(0);
}
int main()
{
 HouseTypes.Set_Heap(HOUSE_COUNT); HouseTypeClass::Init_Heap();
 BuildingTypes.Set_Heap(STRUCT_COUNT); BuildingTypeClass::Init_Heap();
 AnimTypes.Set_Heap(ANIM_COUNT); AnimTypeClass::Init_Heap();
 SmudgeTypes.Set_Heap(SMUDGE_COUNT); SmudgeTypeClass::Init_Heap();
 Houses.Set_Heap(HOUSE_MAX); Buildings.Set_Heap(4); Anims.Set_Heap(200); Smudges.Set_Heap(100);
 HouseClass::One_Time(); Map.Size=MAP_CELL_TOTAL; Map.Alloc_Cells(); Map.Init_Cells();
 Map.CellRedraw.Resize(MAP_CELL_TOTAL);
 Map.MapCellX=Map.MapCellY=1; Map.MapCellWidth=Map.MapCellHeight=126;
 Ground[LAND_CLEAR].Cost[SPEED_TRACK]=1;
 ScenarioInit=1; Session.Type=GAME_NORMAL;
 static char strings[4000]={0}; SystemStrings=DebugStrings=strings;
 HouseClass *house=new HouseClass(HOUSE_GREECE); house->IsHuman=true; house->IsToDie=true; PlayerPtr=house;
 for (int i=0; i<ANIM_COUNT; ++i) { AnimTypes.Ptr(i)->Stages=10; AnimTypes.Ptr(i)->LoopEnd=8; }
 BuildingClass *b=new BuildingClass(STRUCT_POWER,HOUSE_GREECE);
 b->Coord=Cell_Coord(XY_Cell(64,64)); b->IsInLimbo=false;
 const_cast<BuildingTypeClass *>(b->Class.operator->())->MaxStrength=1000;
 ScenarioInit=0; GameActive=true;
 // Clang/GCC must agree on object state as well as the shared RNG state.
 unsigned long const expected[4]={0xdb093242UL,0x36e5458aUL,0xca772854UL,0xdc1ce682UL};
 int mismatches=0;
 for (int path=0; path<4; ++path) {
  digest=2166136261UL;
  for (int seed=1; seed<=64; ++seed) {
   Scen.RandomNumber.Seed=seed*0x10203UL;
   Map.Init_Cells();
   if (path==0) {
    AnimClass *napalm=new AnimClass((AnimType)(ANIM_NAPALM1+seed%3), b->Coord);
    napalm->Middle();
   } else if (path==1) {
    b->Drop_Debris(TARGET_NONE);
   } else {
    b->Strength=1000;
    int damage=path==2 ? 600 : 1001;
    ResultType result=b->Take_Damage(damage,0,seed%2 ? WARHEAD_FIRE : WARHEAD_HE,NULL,true);
    CHECK(result==(path==2 ? RESULT_HALF : RESULT_DESTROYED));
   }
   CHECK(Anims.Count()>0 || path==1 || path==2);
   snapshot();
  }
  printf("combat path=%d digest=%08lx\n",path,digest);
  if (digest!=expected[path]) ++mismatches;
 }
 CHECK(seen[ANIM_FIRE_SMALL] && seen[ANIM_FIRE_MED] && seen[ANIM_ON_FIRE_SMALL] && seen[ANIM_ON_FIRE_MED]);
 CHECK(seen[ANIM_SMOKE_M] && seen[ANIM_FBALL1] && craters);
 CHECK(mismatches==0);
 return 0;
}
