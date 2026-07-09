/*
 * Copyright 2025 Electronic Arts Inc.
 *
 * Derived from Electronic Arts Inc.'s public source release for
 * Command & Conquer: Red Alert.
 * Licensed under GPLv3-or-later with additional Electronic Arts terms.
 * Modified for the macOS port, 2026.
 * See LICENSE.md.
 */

#ifndef WARHEAD_H
#define WARHEAD_H


/**********************************************************************
**	Each of the warhead types has specific characteristics. This structure
**	holds these characteristics.
*/
class WarheadTypeClass
{
	public:
		WarheadTypeClass(char const * name);
		WarheadTypeClass(NoInitClass const &) {}

		void * operator new(size_t) throw();
		static void * operator new(size_t , void * ptr) {return(ptr);};
		void operator delete(void * pointer);

		void Code_Pointers(void) {}
		void Decode_Pointers(void) {}
		char const * Name(void) const {return(IniName);}
		bool Read_INI(CCINIClass & ini);
		static WarheadTypeClass * As_Pointer(WarheadType weapon);

		/*
		**	This is the ID number of the weapon type. It is the weapon
		**	type ID enum as well as the index number into the weapon
		**	heap.
		*/
		int ID;

		/*
		**	This is the identifying name of this warhead type.
		*/
		char const * IniName;

		/*
		**	This value control how damage from this warhead type will reduce
		**	over distance. The larger the number, the less the damage is reduced
		**	the further the distance from the source of the damage.
		*/
		int SpreadFactor;

		/*
		**	If this warhead type can destroy walls, then this flag will be true.
		*/
		bool IsWallDestroyer:1;

		/*
		**	If this warhead can destroy wooden walls, then this flag will be true.
		*/
		bool IsWoodDestroyer:1;

		/*
		**	Does this warhead damage tiberium?
		*/
		bool IsTiberiumDestroyer:1;

		/*
		**	Only effective against infantry?
		*/
		bool IsOrganic:1;

		/*
		**	The warhead damage is reduced depending on the the type of armor the
		**	defender has. This table is what gives weapons their "character".
		*/
		fixed Modifier[ARMOR_COUNT];

		/*
		**	Which explosion set to use for warhead impact.
		*/
		int ExplosionSet;

		/*
		**	This specifies the infantry death animation to use if the infantry dies as
		**	a result of a warhead of this type.
		*/
		int InfantryDeath;
};


#endif
