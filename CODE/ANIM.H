/*
 * Copyright 2025 Electronic Arts Inc.
 *
 * Derived from Electronic Arts Inc.'s public source release for
 * Command & Conquer: Red Alert.
 * Licensed under GPLv3-or-later with additional Electronic Arts terms.
 * Modified for the macOS port, 2026.
 * See LICENSE.md.
 */

#ifndef ANIM_H
#define ANIM_H

#include	"type.h"


/**********************************************************************************************
**	This is the class that controls the shape animation objects. Shape animation objects are
**	displayed over the top of the game map. Typically, they are used for explosion and fire
**	effects.
*/
class AnimClass : public ObjectClass, public StageClass {
		/*
		**	This points to the type of animation object this is.
		*/
		CCPtr<AnimTypeClass> Class;

	public:

		AnimClass(AnimType animnum, COORDINATE coord, unsigned char timedelay=0, unsigned char loop=1);
		AnimClass(NoInitClass const & x) : ObjectClass(x), Class(x), StageClass(x) {};
		virtual ~AnimClass(void);

		operator AnimType(void) const {return Class->Type;};

		static void * operator new(size_t size) throw();
		static void * operator new(size_t , void * ptr) {return(ptr);};
		static void operator delete(void *ptr);

		/*---------------------------------------------------------------------
		**	Member function prototypes.
		*/
		static void Init(void);

		void Attach_To(ObjectClass *obj);
		void Make_Invisible(void) {IsInvisible = true;};
		static void Do_Atom_Damage(HousesType ownerhouse, CELL cell);

		virtual bool Can_Place_Here(COORDINATE ) const {return true;}
		virtual bool Mark(MarkType mark=MARK_CHANGE);
		virtual bool Render(bool forced) const;
		virtual COORDINATE Center_Coord(void) const;
		virtual COORDINATE Sort_Y(void) const;
		virtual LayerType In_Which_Layer(void) const;
		virtual ObjectTypeClass const & Class_Of(void) const {return *Class;};
		virtual short const * Occupy_List(bool = false) const;
		virtual short const * Overlap_List(void) const;
		virtual void Draw_It(int x, int y, WindowNumberType window) const;
		virtual void AI(void);
		virtual void Detach(TARGET target, bool all);

		/*
		**	File I/O.
		*/
		bool Load(Straw & file);
		bool Save(FileClass & file);

		/*
		**	If this animation is attached to an object, then this points to that object. An
		**	animation that is attached will follow that object as it moves. This is important
		**	for animations such as flames and smoke.
		*/
		TARGET xObject;

		/*
		**	If this animation has an owner, then it will be recorded here. An owner
		**	is used when damage is caused by this animation during the middle of its
		**	animation.
		*/
		HousesType OwnerHouse;

		/*
		**	This counter tells how many more times the animation should loop before it
		**	terminates.
		*/
		unsigned char Loops;

	protected:
		void Middle(void);
		void Start(void);

	private:
		/*
		**	Delete this animation at the next opportunity. This is flagged when the
		**	animation is to be prematurely ended as a result of some outside event.
		*/
		unsigned IsToDelete:1;

		/*
		**	If the animation has just been created, then don't do any animation
		**	processing until it has been through the render loop at least once.
		*/
		unsigned IsBrandNew:1;

		/*
		**	If this animation is invisible, then this flag will be true. An invisible
		**	animation is one that is created for the sole purpose of keeping all
		**	machines synchronized. It will not be displayed.
		*/
		unsigned IsInvisible:1;

		/*
		**	Is this animation in a temporary suspended state?  If so, then it won't
		**	be rendered until this value is zero. The flag will be set to false
		**	after the first countdown timer reaches 0.
		*/
		int Delay;

		/*
		**	If this is an animation that damages whatever it is attached to, then this
		**	value holds the accumulation of fractional damage points. When the accumulated
		**	fractions reach 256, then one damage point is applied to the attached object.
		*/
		fixed Accum;
};



#endif
