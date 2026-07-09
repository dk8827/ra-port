/*
 * Copyright 2025 Electronic Arts Inc.
 *
 * Derived from Electronic Arts Inc.'s public source release for
 * Command & Conquer: Red Alert.
 * Licensed under GPLv3-or-later with additional Electronic Arts terms.
 * Modified for the macOS port, 2026.
 * See LICENSE.md.
 */

#ifndef TRIGGER_H
#define TRIGGER_H

class TriggerClass {
	public:
		RTTIType RTTI;
		int ID;

		CCPtr<TriggerTypeClass> Class;

		/*
		**	Record of the "already sprung" flag for the events.
		*/
		TDEventClass Event1;
		TDEventClass Event2;

		/*
		**	Constructor/Destructor
		*/
		TriggerClass(TriggerTypeClass * trigtype=NULL);
		TriggerClass(NoInitClass const & x) : Class(x), Event1(x), Event2(x) {};
		~TriggerClass(void);

		/*
		**	Initialization: clears all triggers in preparation for new scenario
		*/
		static void Init(void);

		/*
		**	Processing routines
		*/
		bool  Spring(TEventType event=TEVENT_ANY, ObjectClass * object=0, CELL cell=0, bool forced=false);
		void Detach(TARGET target, bool all=true);

		/*
		**	File I/O routines
		*/
		bool Load(Straw & file);
		bool Save(Pipe & file) const;
		void Code_Pointers(void) {};
		void Decode_Pointers(void) {};

		/*
		**	Utility routines
		*/
		TARGET As_Target(void) const;
		char const * Description(void) const;
		void Draw_It(int , int x, int y, int width, int height, bool selected, TextPrintType flags) const;
		char const * Name(void) const {return(Class->Name());}

		/*
		**	Overloaded operators
		*/
		static void * operator new(size_t size) throw();
		static void * operator new(size_t, void * ptr) {return(ptr);};
		static void operator delete(void * ptr);

		/*
		**	If this trigger object is active, then this flag will be true. Trigger
		**	objects that are not active are either not yet created or have been
		**	deleted after fulfilling their action.
		*/
		unsigned IsActive:1;

		/*
		**	This value tells how many objects or cells this trigger is attached
		**	to.  The Read_INI routine for all classes that point to a trigger must
		**	increment this value!
		*/
		int AttachCount;

		/*
		**	This value is used for triggers that can only exist in one cell. It is
		**	needed for such triggers that the exact location of the trigger is needed
		**	during processing but its location cannot be inferred from other data.
		**	For all other triggers, this value is ignored.
		*/
		CELL Cell;
};




#endif
