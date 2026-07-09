/*
 * Copyright 2025 Electronic Arts Inc.
 *
 * Derived from Electronic Arts Inc.'s public source release for
 * Command & Conquer: Red Alert.
 * Licensed under GPLv3-or-later with additional Electronic Arts terms.
 * Modified for the macOS port, 2026.
 * See LICENSE.md.
 */

#ifndef SMUDGE_H
#define SMUDGE_H

#include	"object.h"
#include	"type.h"


/******************************************************************************
**	This is the transitory form for smudges. They exist as independent objects
**	only in the transition stage from creation to placement upon the map. Once
**	they are placed on the map, they merely become 'smudges' in the cell data. This
**	object is then destroyed.
*/
class SmudgeClass : public ObjectClass
{
	public:
		/*
		**	This is a pointer to the template object's class.
		*/
		CCPtr<SmudgeTypeClass> Class;

		/*-------------------------------------------------------------------
		**	Constructors and destructors.
		*/
		static void * operator new(size_t size) throw();
		static void * operator new(size_t , void * ptr) {return(ptr);};
		static void operator delete(void *ptr);
		SmudgeClass(SmudgeType type, COORDINATE pos=0xFFFFFFFFUL, HousesType house = HOUSE_NONE);
		SmudgeClass(NoInitClass const & x) : ObjectClass(x), Class(x) {};
		operator SmudgeType(void) const {return Class->Type;};
		virtual ~SmudgeClass(void) {if (GameActive) SmudgeClass::Limbo();Class=0;};

		static void Init(void);

		/*
		**	File I/O.
		*/
		static void Read_INI(CCINIClass & ini);
		static void Write_INI(CCINIClass & ini);
		static char *INI_Name(void) {return "SMUDGE";};
		bool Load(Straw & file);
		bool Save(Pipe & file) const;

		virtual ObjectTypeClass const & Class_Of(void) const {return *Class;};
		virtual bool Mark(MarkType);
		virtual void Draw_It(int , int , WindowNumberType ) const {};

		void Disown(CELL cell);

	private:

		static HousesType ToOwn;
};

#endif
