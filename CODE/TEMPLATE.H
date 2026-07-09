/*
 * Copyright 2025 Electronic Arts Inc.
 *
 * Derived from Electronic Arts Inc.'s public source release for
 * Command & Conquer: Red Alert.
 * Licensed under GPLv3-or-later with additional Electronic Arts terms.
 * Modified for the macOS port, 2026.
 * See LICENSE.md.
 */

#ifndef TEMPLATE_H
#define TEMPLATE_H

#include	"object.h"
#include	"type.h"


/******************************************************************************
**	This class controls the template object. Template objects function congruously
**	to carpet on a floor. They have no depth, but merely control the icon to be rendered
**	as the cell's bottom most layer.
*/
class TemplateClass : public ObjectClass
{
	public:
		/*
		**	This is a pointer to the template object's class.
		*/
		CCPtr<TemplateTypeClass> Class;

		/*-------------------------------------------------------------------
		**	Constructors and destructors.
		*/
		static void * operator new(size_t size) throw();
		static void * operator new(size_t , void * ptr) {return(ptr);};
		static void operator delete(void *ptr);
		TemplateClass(TemplateType type, CELL pos=-1);
		TemplateClass(NoInitClass const & x) : ObjectClass(x), Class(x) {};
		virtual ~TemplateClass(void) {if (GameActive) TemplateClass::Limbo();Class=0;};
		operator TemplateType(void) const {return Class->Type;};

		static void Init(void);

		/*
		**	Query functions.
		*/
		virtual ObjectTypeClass const & Class_Of(void) const {return *Class;};
		int Icon_Number(CELL cell);

		/*
		**	Display and rendering support functionality. Supports imagery and how
		**	object interacts with the map and thus indirectly controls rendering.
		*/
		virtual void Draw_It(int , int , WindowNumberType ) const {};
		virtual bool Mark(MarkType mark);

		/*
		**	File I/O.
		*/
		bool Load(Straw & file);
		bool Save(Pipe & file) const;
};

#endif
