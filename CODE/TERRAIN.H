/*
 * Copyright 2025 Electronic Arts Inc.
 *
 * Derived from Electronic Arts Inc.'s public source release for
 * Command & Conquer: Red Alert.
 * Licensed under GPLv3-or-later with additional Electronic Arts terms.
 * Modified for the macOS port, 2026.
 * See LICENSE.md.
 */

#ifndef TERRAIN_H
#define TERRAIN_H

#include	"object.h"
#include	"type.h"


/****************************************************************************
**	Each type of terrain has certain pieces of static information associated
**	with it. This class elaborates this data.
*/
class TerrainClass : public ObjectClass, public StageClass
{
	public:
		/*
		**	This points to the constant terrain data (for this type) that gives this
		**	terrain object its character.
		*/
		CCPtr<TerrainTypeClass> Class;

  		/*
		**	Constructor for terrain object class.
		*/
		static void * operator new(size_t size) throw();
		static void * operator new(size_t , void * ptr) {return(ptr);};
		static void operator delete(void *ptr);
		TerrainClass(TerrainType id, CELL cell);
		TerrainClass(NoInitClass const & x) : ObjectClass(x), Class(x), StageClass(x) {};
		virtual ~TerrainClass(void);
		operator TerrainType(void) const {return Class->Type;};

		static void Init(void);

		/*
		**	Terrain specific support functions.
		*/
		void Start_To_Crumble(void);

		/*
		**	Query functions.
		*/
		virtual ObjectTypeClass const & Class_Of(void) const {return *Class;};

		/*
		**	Coordinate inquiry functions. These are used for both display and
		**	combat purposes.
		*/
		virtual COORDINATE Center_Coord(void) const;
		virtual COORDINATE Render_Coord(void) const {return Coord;};
		virtual COORDINATE Sort_Y(void) const {return Coord_Add(Coord, Class->CenterBase);};
		virtual COORDINATE Target_Coord(void) const;

		/*
		**	Object entry and exit from the game system.
		*/
		virtual bool Unlimbo(COORDINATE coord, DirType dir=DIR_N);
		virtual bool Limbo(void);
		virtual MoveType Can_Enter_Cell(CELL cell, FacingType facing = FACING_NONE) const;

		/*
		**	Display and rendering support functionality. Supports imagery and how
		**	object interacts with the map and thus indirectly controls rendering.
		*/
		virtual void Draw_It(int x, int y, WindowNumberType window) const;
		virtual bool Mark(MarkType mark=MARK_CHANGE);
		unsigned char *Radar_Icon(CELL cell);

		/*
		**	User I/O.
		*/
		virtual void Clicked_As_Target(int) {};

		/*
		**	Combat related.
		*/
		virtual void Fire_Out(void);
		virtual bool Catch_Fire(void);
		virtual ResultType Take_Damage(int & damage, int distance, WarheadType warhead, TechnoClass * source, bool forced=false);

		/*
		**	AI.
		*/
		virtual void AI(void);

		/*
		**	Scenario and debug support.
		*/
		#ifdef CHEAT_KEYS
		virtual void Debug_Dump(MonoClass *mono) const;
		#endif

		/*
		**	File I/O.
		*/
		static void Read_INI(CCINIClass & ini);
		static void Write_INI(CCINIClass & ini);
		static char *INI_Name(void) {return "TERRAIN";};
		bool Load(Straw & file);
		bool Save(Pipe & file) const;

	private:

		/*
		**	If this terrain object is on fire, then this flag will be true.
		*/
		unsigned IsOnFire:1;

		/*
		**	Is this a terrain object that undergoes crumbling animation and it is
		**	in fact crumbling at this time?
		*/
		unsigned IsCrumbling:1;
};

#endif
