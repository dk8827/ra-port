/*
 * Copyright 2025 Electronic Arts Inc.
 *
 * Derived from Electronic Arts Inc.'s public source release for
 * Command & Conquer: Red Alert.
 * Licensed under GPLv3-or-later with additional Electronic Arts terms.
 * Modified for the macOS port, 2026.
 * See LICENSE.md.
 */

#ifndef OVERLAY_H
#define OVERLAY_H

#include	"object.h"
#include	"type.h"

/******************************************************************************
**	This class controls the overlay object. Overlay objects function congruously
**	to carpet on a floor. They have no depth, but merely control the icon to be rendered
**	as the cell's bottom most layer.
*/
class OverlayClass : public ObjectClass
{
	public:
		/*
		**	This is a pointer to the overlay object's class.
		*/
		CCPtr<OverlayTypeClass> Class;

		/*-------------------------------------------------------------------
		**	Constructors and destructors.
		*/
		static void * operator new(size_t size) throw();
		static void * operator new(size_t , void * ptr) {return(ptr);};
		static void operator delete(void *ptr);
		OverlayClass(OverlayType type, CELL pos=-1, HousesType = HOUSE_NONE);
		OverlayClass(NoInitClass const & x) : ObjectClass(x), Class(x) {};
		virtual ~OverlayClass(void) {if (GameActive) OverlayClass::Limbo();Class=0;};
		operator OverlayType(void) const {return Class->Type;};

		static void Init(void);

		/*
		**	File I/O.
		*/
		static void Read_INI(CCINIClass & ini);
		static void Write_INI(CCINIClass & ini);
		static char *INI_Name(void) {return "OVERLAY";};
		bool Load(Straw & file);
		bool Save(Pipe & file) const;

		/*
		**	Virtual support functionality.
		*/
		virtual bool Mark(MarkType);
		virtual ObjectTypeClass const & Class_Of(void) const {return *Class;};
		virtual void Draw_It(int , int , WindowNumberType ) const {};

	private:
		/*
		**	This is used to control the marking process of the overlay. If this is
		**	set to a valid house number, then the cell that the overlay is marked down
		**	upon will be flagged as being owned by the specified house.
		*/
		static HousesType ToOwn;
};

#endif
