/*
 * Copyright 2025 Electronic Arts Inc.
 *
 * Derived from Electronic Arts Inc.'s public source release for
 * Command & Conquer: Red Alert.
 * Licensed under GPLv3-or-later with additional Electronic Arts terms.
 * Modified for the macOS port, 2026.
 * See LICENSE.md.
 */

#ifndef TRIGTYPE_H
#define TRIGTYPE_H

#include	"tevent.h"
#include	"taction.h"

class TriggerClass;

/*
**	There can be multiple trigger events and trigger actions. This enumeration is used to
**	indicate if there are multiple events/actions and what their relationship is.
*/
typedef enum MultiStyleType {
	MULTI_ONLY,					// "Only" main trigger action/event?
	MULTI_AND,					// "And" secondary trigger action/event?
	MULTI_OR,					// "Or" secondary event?
	MULTI_LINKED				// Cause and effect pairs are linked?
} MultiStyleType;


class TriggerTypeClass : public AbstractTypeClass
{
	public:
		unsigned IsActive:1;

		typedef enum PersistantType {
			VOLATILE = 0,
			SEMIPERSISTANT = 1,
			PERSISTANT = 2
		} PersistantType;

		/*
		**	This flag controls whether the trigger destroys itself after it goes
		**	off.
		**	0 = trigger destroys itself immediately after going off, and removes
		**	    itself from all objects it's attached to
		**	1 = trigger is "Semi-Persistent"; it maintains a count of all objects
		**	    it's attached to, and only actually "springs" after its been
		**		 triggered from all the objects; then, it removes itself.
		**	2 = trigger is Fully Persistent; it just won't go away.
		*/
		PersistantType IsPersistant;

		/*
		**	For house-specific events, this is the house for that event.
		*/
		HousesType House;

		/*
		**	Each trigger must have an event which activates it. This is the event that is
		**	used to activate this trigger.
		*/
		TEventClass Event1;
		TEventClass Event2;
		MultiStyleType EventControl;

		/*
		**	This is the action to perform when the trigger event occurs.
		*/
		TActionClass Action1;
		TActionClass Action2;
		MultiStyleType ActionControl;


		TriggerTypeClass(void);
		TriggerTypeClass(NoInitClass const & x) : AbstractTypeClass(x), Event1(x), Event2(x), Action1(x), Action2(x) {};
		virtual ~TriggerTypeClass(void);

		static void * operator new(size_t ) throw();
		static void * operator new(size_t, void * ptr) {return(ptr);};
		static void operator delete(void *ptr);

		/*
		**	Initialization: clears all trigger types in preparation for new scenario
		*/
		static void Init(void);

		/*
		**	File I/O routines
		*/
		static void Read_INI(CCINIClass & ini);
		static void Write_INI(CCINIClass & ini);
		void Fill_In(char * name, char * entry);
		void Build_INI_Entry(char * buf) const;

		static char * INI_Name(void) {return "Trigs";};
		bool Load(Straw & file);
		bool Save(Pipe & file) const;
		void Code_Pointers(void);
		void Decode_Pointers(void);

		/*
		**	Processing routines
		*/
		TriggerClass * Create_One_Of(void) const;
		void Destroy_All_Of(void) const;

		/*
		**	Utility routines
		*/
		void Detach(TARGET target, bool all=true);
		AttachType Attaches_To(void) const;
		TARGET As_Target(void) const;
		static TriggerTypeClass * From_Name(char const * name);
		bool Edit(void);
#if defined(CHEAT_KEYS) || defined(SCENARIO_EDITOR)
		char const * Description(void) const;
		operator const char * (void) const {return(Description());};
#endif
		void Draw_It(int index, int x, int y, int width, int height, bool selected, TextPrintType flags) const;
};


#endif
