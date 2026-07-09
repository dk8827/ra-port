/*
 * Copyright 2025 Electronic Arts Inc.
 *
 * Derived from Electronic Arts Inc.'s public source release for
 * Command & Conquer: Red Alert.
 * Licensed under GPLv3-or-later with additional Electronic Arts terms.
 * Modified for the macOS port, 2026.
 * See LICENSE.md.
 */

#ifndef FACTORY_H
#define FACTORY_H

#include	"stage.h"

class FactoryClass : private StageClass
{
	public:
		RTTIType RTTI;
		int ID;


		FactoryClass(void);
		FactoryClass(NoInitClass const & x) : StageClass(x) {};
		~FactoryClass(void);
		static void * operator new(size_t size) throw();
		static void * operator new(size_t , void * ptr) {return(ptr);};
		static void operator delete(void *ptr);

		static void Init(void);

		/*
		**	File I/O.
		*/
		bool Load(Straw & file);
		bool Save(Pipe & file) const;
		void Code_Pointers(void);
		void Decode_Pointers(void);

		bool Abandon(void);
		bool Completed(void);
		bool Has_Changed(void);
		bool Has_Completed(void);
		bool Is_Building(void) const {return(Fetch_Rate() != 0);};
		bool Set(TechnoTypeClass const & object, HouseClass & house);
		bool Set(int const & type, HouseClass & house);
		bool Start(void);
		bool Suspend(void);
		int Completion(void);
		TechnoClass * Get_Object(void) const;
		int Get_Special_Item(void) const;
		void AI(void);
		void Set(TechnoClass & object);
		HouseClass * Get_House(void) {return(House);};
		char const * Name(void) {return("Factory");}

		/*
		**	This flag is used to maintain the pool of factory class objects. If the object has
		**	been allocated, then this flag is true. Otherwise, the object is free to be
		**	allocated.
		*/
		unsigned IsActive:1;

		enum StepCountEnum {
			STEP_COUNT=54			// Number of steps to break production down into.
		};
	protected:

		int Cost_Per_Tick(void);

	private:

		/*
		**	If production is temporarily suspended, then this flag will be true. A factory
		**	is suspended when it is first created, when production has completed, and when
		**	explicitly instructed to Suspend() production. Suspended production is not
		**	abandoned. It may be resumed with a call to Start().
		*/
		unsigned IsSuspended:1;

		/*
		**	If the AI process detected that the production process has advanced far enough
		**	that a change in the building animation would occur, this flag will be true.
		**	Examination of this flag (through the Has_Changed function) allows intelligent
		**	updating of any production graphic.
		*/
		unsigned IsDifferent:1;

		/*
		**	This records the balance due on the current production item. This value will
		**	be reduced as production proceeds. It will reach zero the moment production has
		**	finished. Using this method ensures that the total production cost will be EXACT
		**	regardless of the number of installment payments that are made.
		*/
		int Balance;
		int OriginalBalance;

		/*
		**	This is the object that is being produced. It is held in a state of limbo while
		**	undergoing production. Since the object is created at the time production is
		**	started, it is always available when production completes.
		*/
		TechnoClass * Object;

		/*
		**	If the factory is not producing an object and is instead producing
		** a special item, then special item will be set.
		*/
		int SpecialItem;

		/*
		** The factory has to be doing production for one house or another.
		** The house pointer will point to whichever house it is being done
		** for.
		*/
		HouseClass  * House;
};

#endif
