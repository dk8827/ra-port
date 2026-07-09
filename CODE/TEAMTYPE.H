/*
 * Copyright 2025 Electronic Arts Inc.
 *
 * Derived from Electronic Arts Inc.'s public source release for
 * Command & Conquer: Red Alert.
 * Licensed under GPLv3-or-later with additional Electronic Arts terms.
 * Modified for the macOS port, 2026.
 * See LICENSE.md.
 */

#ifndef TEAMTYPE_H
#define TEAMTYPE_H

/*
**	TeamMissionType: the various missions that a team can have.
*/
typedef enum TeamMissionType {
	TMISSION_NONE=-1,
	TMISSION_ATTACK,					// Attack specified quarry type.
	TMISSION_ATT_WAYPT,				// Attack specified waypoint
	TMISSION_FORMATION,				// Change formation of team.
	TMISSION_MOVE,						// moves to waypoint specified.
	TMISSION_MOVECELL,				// moves to cell # specified.
	TMISSION_GUARD,					// works like an infantry's guard mission
	TMISSION_LOOP,						// loop back to start of mission list
	TMISSION_ATTACKTARCOM,			// attack tarcom
	TMISSION_UNLOAD,					// Unload at current location.
	TMISSION_DEPLOY,					// Deploy mobile building type.
	TMISSION_HOUND_DOG,				// Follow nearest friendly unit.
	TMISSION_DO,						// Do guard, sticky, area guard (mission sticks on this).
	TMISSION_SET_GLOBAL,				// Set global variable.
	TMISSION_INVULNERABLE,			// Magical invulnerability.
	TMISSION_LOAD,						// Load onto transport member of team.
	TMISSION_SPY,						// Spy enter the building at specified waypoint
	TMISSION_PATROL,					// Move but look for enemies as well.

	TMISSION_COUNT,
	TMISSION_FIRST=0
} TeamMissionType;


/*
** Forward declarations.
*/
class TechnoTypeClass;


/*
**	This structure contains one team mission value & its argument.
*/
class TeamMissionClass
{
	public:
#if defined(CHEAT_KEYS) || defined(SCENARIO_EDITOR)
		char const * Description(int index) const;
		operator const char * () const {return(Description(0));};
#endif
		void Draw_It(int index, int x, int y, int width, int height, bool selected, TextPrintType flags);

		TeamMissionType Mission;		// Mission type.
		union {
			FormationType Formation;	// Formation to use.
			QuarryType Quarry;			// Combat quarry type.
			MissionType Mission;			// General mission orders.
			int Value;						// Usually a waypoint number.
		} Data;
};


/*
**	This class specifies the quantity and type of members desired for the
**	team.
*/
class TeamMemberClass
{
	public:
		int Quantity;							// Number of objects desired for this type.
		TechnoTypeClass const * Class;	// The type of object desired.
};


/*
**	TeamTypeClass declaration
*/
class TeamTypeClass : public AbstractTypeClass
{
	public:
		enum TeamTypeClassEnums {
			MAX_TEAM_CLASSCOUNT=5,
			MAX_TEAM_MISSIONS=20
		};

		/*
		**	Constructor/Destructor
		*/
		TeamTypeClass(void);
		TeamTypeClass(NoInitClass const & x) : AbstractTypeClass(x), Trigger(x) {};
		virtual ~TeamTypeClass(void) {};

		static void * operator new(size_t ) throw();
		static void * operator new(size_t, void * ptr) {return(ptr);};
		static void operator delete(void *ptr);

		/*
		**	Initialization: clears all team types in preparation for new scenario
		*/
		static void Init(void);

		/*
		**	File I/O routines
		*/
		void Build_INI_Entry(char * buffer);
		static void Read_INI(CCINIClass & ini);
		void Fill_In(char *name, char *entry);
		static void Write_INI(CCINIClass & ini);
		static char * INI_Name(void) {return "TeamTypes";};
		bool Load(Straw & file);
		bool Save(Pipe & file) const;
		void Code_Pointers(void);
		void Decode_Pointers(void);

		/*
		**	As_Pointer gets a pointer to the trigger object give its name
		*/
		static TeamTypeClass *As_Pointer(char const * name);

		/*
		**	Processing routines
		*/
		TeamClass * Create_One_Of(void) const;
		void Destroy_All_Of(void) const;
		void Detach(TARGET target, bool all=true);

		/*
		**	Utility routines
		*/
		void Draw_It(int index, int x, int y, int width, int height, bool selected, TextPrintType flags) const;
		static char const * Name_From_Mission(TeamMissionType order);
		static TeamMissionType Mission_From_Name(char const *name);
		static TeamTypeClass const * Suggested_New_Team(HouseClass * house, long atypes, long utypes, long itypes, long vtypes, bool alerted);
		static TeamTypeClass * From_Name(char const * name);
		bool Edit(void);
#if defined(CHEAT_KEYS) || defined(SCENARIO_EDITOR)
		char const * Member_Description(void) const;
		char const * Description(void) const;
		operator const char * (void) const {return(Description());};
#endif

		/*
		**	If this teamtype object is active, then this flag will be true.
		**	TeamType objects that are not active are either not yet created or have
		**	been deleted after fulfilling their action.
		*/
		unsigned IsActive:1;

		/*
		**	If RoundAbout, the team avoids high-threat areas
		*/
		unsigned IsRoundAbout:1;

		/*
		**	If Suicide, the team won't stop until it achieves its mission or it's
		**	dead
		*/
		unsigned IsSuicide:1;

		/*
		**	Is this team type allowed to be created automatically by the computer
		**	when the appropriate trigger indicates?
		*/
		unsigned IsAutocreate:1;

		/*
		**	This flag tells the computer that it should build members to fill
		**	a team of this type regardless of whether there actually is a team
		**	of this type active.
		*/
		unsigned IsPrebuilt:1;

		/*
		**	If this team should allow recruitment of new members, then this flag
		**	will be true. A false value results in a team that fights until it
		**	is dead. This is similar to IsSuicide, but they will defend themselves.
		*/
		unsigned IsReinforcable:1;

		/*
		**	A transient team type was created exclusively to bring on reinforcements
		**	as a result of some special event. As soon as there are no teams
		**	existing of this type, then this team type should be deleted.
		*/
		unsigned IsTransient:1;

		/*
		**	Priority given the team for recruiting purposes; higher priority means
		**	it can steal members from other teams (scale: 0 - 15)
		*/
		int RecruitPriority;

		/*
		**	Initial # of this type of team
		*/
		unsigned char InitNum;

		/*
		**	Max # of this type of team allowed at one time
		*/
		unsigned char MaxAllowed;

		/*
		**	Fear level of this team
		*/
		unsigned char Fear;

		/*
		**	House the team belongs to
		*/
		HousesType House;

		/*
		**	Trigger to assign to each object as it joins this team.
		*/
		CCPtr<TriggerTypeClass> Trigger;

		/*
		**	This is the waypoint origin to use when creating this team or
		**	when bringing the team on as a reinforcement.
		*/
		WAYPOINT Origin;

		/*
		**	This records the number of teams of this type that are currently
		**	active.
		*/
		int Number;

		/*
		**	Number and list of missions that this team will follow.
		*/
		int MissionCount;
		TeamMissionClass MissionList[MAX_TEAM_MISSIONS];

		/*
		**	Number and type of members desired for this team.
		*/
		int ClassCount;
		TeamMemberClass Members[MAX_TEAM_CLASSCOUNT];

		static char const * TMissions[TMISSION_COUNT];
};


#endif
