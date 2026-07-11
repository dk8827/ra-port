/*
 * Copyright 2025 Electronic Arts Inc.
 *
 * Derived from Electronic Arts Inc.'s public source release for
 * Command & Conquer: Red Alert.
 * Licensed under GPLv3-or-later with additional Electronic Arts terms.
 * Modified for the macOS port, 2026.
 * See LICENSE.md.
 */

#include	"function.h"
#include	"triggerwidth.h"

/*
**	This is the text name for all of the trigger events. These are used by the scenario editor
*/
const char * EventText[TEVENT_COUNT] = {
	"-No Event-",
	"Entered by...",
	"Spied by...",
	"Thieved by...",
	"Discovered by player",
	"House Discovered...",
	"Attacked by anybody",
	"Destroyed by anybody",
	"Any Event",
	"Destroyed, Units, All...",
	"Destroyed, Buildings, All...",
	"Destroyed, All...",
	"Credits exceed (x100)...",
	"Elapsed Time (1/10th min)...",
	"Mission Timer Expired",
	"Destroyed, Buildings, #...",
	"Destroyed, Units, #...",
	"No Factories left",
	"Civilians Evacuated",
	"Build Building Type...",
	"Build Unit Type...",
	"Build Infantry Type...",
	"Build Aircraft Type...",
	"Leaves map (team)...",
	"Zone Entry by...",
	"Crosses Horizontal Line...",
	"Crosses Vertical Line...",
	"Global is set...",
	"Global is clear...",
	"Destroyed, Fakes, All...",
	"Low Power...",
	"All bridges destroyed",
	"Building exists..."
};


/*
**	This is an ordinal list of trigger events. This list
**	is used when generating the trigger dialog box.
*/
EventChoiceClass EventChoices[TEVENT_COUNT] = {
	EventChoiceClass(TEVENT_NONE),
	EventChoiceClass(TEVENT_PLAYER_ENTERED),
	EventChoiceClass(TEVENT_SPIED),
	EventChoiceClass(TEVENT_THIEVED),
	EventChoiceClass(TEVENT_DISCOVERED),
	EventChoiceClass(TEVENT_HOUSE_DISCOVERED),
	EventChoiceClass(TEVENT_ATTACKED),
	EventChoiceClass(TEVENT_DESTROYED),
	EventChoiceClass(TEVENT_ANY),
	EventChoiceClass(TEVENT_UNITS_DESTROYED),
	EventChoiceClass(TEVENT_BUILDINGS_DESTROYED),
	EventChoiceClass(TEVENT_ALL_DESTROYED),
	EventChoiceClass(TEVENT_CREDITS),
	EventChoiceClass(TEVENT_TIME),
	EventChoiceClass(TEVENT_MISSION_TIMER_EXPIRED),
	EventChoiceClass(TEVENT_NBUILDINGS_DESTROYED),
	EventChoiceClass(TEVENT_NUNITS_DESTROYED),
	EventChoiceClass(TEVENT_NOFACTORIES),
	EventChoiceClass(TEVENT_EVAC_CIVILIAN),
	EventChoiceClass(TEVENT_BUILD),
	EventChoiceClass(TEVENT_BUILD_UNIT),
	EventChoiceClass(TEVENT_BUILD_INFANTRY),
	EventChoiceClass(TEVENT_BUILD_AIRCRAFT),
	EventChoiceClass(TEVENT_LEAVES_MAP),
	EventChoiceClass(TEVENT_ENTERS_ZONE),
	EventChoiceClass(TEVENT_CROSS_HORIZONTAL),
	EventChoiceClass(TEVENT_CROSS_VERTICAL),
	EventChoiceClass(TEVENT_GLOBAL_SET),
	EventChoiceClass(TEVENT_GLOBAL_CLEAR),
	EventChoiceClass(TEVENT_FAKES_DESTROYED),
	EventChoiceClass(TEVENT_LOW_POWER),
	EventChoiceClass(TEVENT_ALL_BRIDGES_DESTROYED),
	EventChoiceClass(TEVENT_BUILDING_EXISTS),
};


/***********************************************************************************************
 * EventChoiceClass::Draw_It -- Displays the event choice class as a text string.              *
 *                                                                                             *
 *    This utility routine is used by the list box display code to print the text line that    *
 *    describes the event choice action.                                                       *
 *                                                                                             *
 * INPUT:   see below...                                                                       *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/29/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
void EventChoiceClass::Draw_It(int , int x, int y, int width, int height, bool selected, TextPrintType flags) const
{
	RemapControlType * scheme = GadgetClass::Get_Color_Scheme();
	static int _tabs[] = {13,40};
	if ((flags & 0x0F) == TPF_6PT_GRAD || (flags & 0x0F) == TPF_EFNT) {

		if (selected) {
			flags = flags | TPF_BRIGHT_COLOR;
			LogicPage->Fill_Rect(x, y, x + width - 1, y + height - 1, scheme->Shadow);
		} else {
			if (!(flags & TPF_USE_GRAD_PAL)) {
				flags = flags | TPF_MEDIUM_COLOR;
			}
		}

		Conquer_Clip_Text_Print(Description(), x, y, scheme, TBLACK, flags, width, _tabs);
	} else {
		Conquer_Clip_Text_Print(Description(), x, y, (selected ? &ColorRemaps[PCOLOR_DIALOG_BLUE] : &ColorRemaps[PCOLOR_GREY]), TBLACK, flags, width, _tabs);
	}
}


/***********************************************************************************************
 * TEventClass::Reset -- Reset the trigger for a subsequent "spring".                          *
 *                                                                                             *
 *    This will reset the trigger event so that it may be "sprung" again. Typically, this      *
 *    occurs for persistent triggers.                                                          *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   11/28/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
void TEventClass::Reset(TDEventClass & td) const
{
	td.IsTripped = false;
	if (Event == TEVENT_TIME) {
		td.Timer = Data.Value * (TICKS_PER_MINUTE/10);
	}
}


/***********************************************************************************************
 * TEventClass::operator () -- Action operator to see if event is satisfied.                   *
 *                                                                                             *
 *    This routine is called to see if the event has been satisfied. Typically, when the       *
 *    necessary trigger events have been satisfied, then the trigger is sprung. For some       *
 *    events, the act of calling this routine is tacit proof enough that the event has         *
 *    occurred. For most other events, the success condition must be explicitly checked.       *
 *                                                                                             *
 * INPUT:   event -- The event that has occurred according to the context from which this      *
 *                   routine was called. In the case of no specific event having occurred,     *
 *                   then TEVENT_ANY will be passed in.                                        *
 *                                                                                             *
 *          house -- The house that this event is tied to.                                     *
 *                                                                                             *
 *          object-- The object that this event might apply to. For object triggering          *
 *                   events, this will point to the perpetrator object.                        *
 *                                                                                             *
 *          forced-- If this event is forced by some outside action, this flag will be true.   *
 *                   Forcing only occurs as an explicit action from another trigger.           *
 *                                                                                             *
 * OUTPUT:  Was this event satisfied? A satisfied event will probably spring the trigger       *
 *          it is attached to.                                                                 *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   11/28/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
bool TEventClass::operator () (TDEventClass & td, TEventType event, HousesType house, ObjectClass const * object, bool forced)
{
	/*
	**	Mission INI stores event types in a one-byte field; narrow so comparisons
	**	match original Watcom enum width (see Normalize_Trigger_Data).
	*/
	TEventType const Type = (TEventType)Legacy_Trigger_Byte(Event);

	/*
	**	If this trigger event has been forced, then no further checks are required.
	**	Force the trigger to be tripped.
	*/
	if (forced) {
		td.IsTripped = true;
	}

	if (td.IsTripped) return(true);

	/*
	**	Triggers based on the game's global environment such as time or
	**	global flags are triggered only when the appropriate condition
	**	is true.
	*/
	switch (Type) {
		case TEVENT_GLOBAL_SET:
			if (!Scen.GlobalFlags[Data.Value]) return(false);
			return(true);

		case TEVENT_GLOBAL_CLEAR:
			if (Scen.GlobalFlags[Data.Value]) return(false);
			return(true);

		case TEVENT_MISSION_TIMER_EXPIRED:
//			if (MissionSuspend == -1 || MissionTimer != 0) return(false);
			if (!Scen.MissionTimer.Is_Active() || Scen.MissionTimer != 0) return(false);
			return(true);

		case TEVENT_TIME:
			if (td.Timer != 0) return(false);
			return(true);
	}

	/*
	**	Don't trigger this event if the parameters mean nothing. Typical of
	**	this would be for events related to time or other outside influences.
	*/
	if (Type == TEVENT_NONE) {
		return(false);
	}

	/*
	**	If this is not the event for this trigger, just return. This is only
	**	necessary to check for those trigger events that are presumed to be
	**	true just by the fact that this routine is called with the appropriate
	**	event identifier.
	*/
	if (Type == TEVENT_ATTACKED ||
		Type == TEVENT_DESTROYED ||
		Type == TEVENT_DISCOVERED ||
		Type == TEVENT_SPIED ||
		Type == TEVENT_NONE ||
		Type == TEVENT_CROSS_HORIZONTAL ||
		Type == TEVENT_CROSS_VERTICAL ||
		Type == TEVENT_ENTERS_ZONE ||
		Type == TEVENT_PLAYER_ENTERED) {

		if (event != Type && event != TEVENT_ANY) {
			return(false);
		}
	}

	/*
	**	The cell entry trigger event is only tripped when an object of the
	**	matching ownership has entered the cell in question. All other
	**	conditions will not trigger the event.
	*/
	if (Type == TEVENT_PLAYER_ENTERED || Type == TEVENT_CROSS_HORIZONTAL || Type == TEVENT_CROSS_VERTICAL || Type == TEVENT_ENTERS_ZONE) {
		if (!object || object->Owner() != Data.House) return(false);
		td.IsTripped = true;
		return(true);
	}

	/*
	**	Check for all bridges destroyed condition.
	*/
	if (Type == TEVENT_ALL_BRIDGES_DESTROYED) {
		if (Scen.BridgeCount) return(false);
		td.IsTripped = true;
		return(true);
	}

	/*
	**	The following trigger events are not considered to have sprung
	**	merely by fact that this routine has been called. These trigger
	**	events must be verified manually by examining the house that
	**	they are assigned to.
	*/
	HouseClass * hptr = HouseClass::As_Pointer(house);
	int index;
	if (hptr != NULL) {
		switch (Type) {
			/*
			**	Check to see if a team of the appropriate type has left the map.
			*/
			case TEVENT_LEAVES_MAP:
				for (index = 0; index < Teams.Count(); index++) {
					TeamClass * ptr = Teams.Ptr(index);
					if (ptr->Class == Team && ptr->Is_Empty() && ptr->IsLeaveMap) {
//					if (ptr->Class == Team && ptr->House == hptr && ptr->Is_Empty() && ptr->IsLeaveMap) {
						td.IsTripped = true;
						break;
					}
				}
				if (index == Teams.Count()) return(false);
				break;

			/*
			**	Credits must be equal or greater to the value specified.
			*/
			case TEVENT_CREDITS:
				if (hptr->Available_Money() < Data.Value) return(false);
				break;

			/*
			**	Ensure that there are no more factories left.
			*/
			case TEVENT_NOFACTORIES:
				if (hptr->BScan & (STRUCTF_AIRSTRIP|STRUCTF_TENT|STRUCTF_WEAP|STRUCTF_BARRACKS|STRUCTF_CONST)) return(false);
				break;

			/*
			**	Ensure that there are no fake structures left.
			*/
			case TEVENT_FAKES_DESTROYED:
				if (hptr->BScan & (STRUCTF_FAKECONST|STRUCTF_FAKEWEAP)) return(false);
				break;

			/*
			**	A civilian must have been evacuated.
			*/
			case TEVENT_EVAC_CIVILIAN:
				if (!hptr->IsCivEvacuated) return(false);
				break;

			/*
			**	Verify that the structure has been built.
			*/
			case TEVENT_BUILDING_EXISTS:
				if ((hptr->ActiveBScan & (1 << Data.Structure)) == 0) return(false);
//				if (hptr->Get_Quantity(Data.Structure) == 0) return(false);
				break;

			/*
			**	Verify that the structure has been built.
			*/
			case TEVENT_BUILD:
				if (hptr->JustBuiltStructure != Data.Structure) return(false);
				td.IsTripped = true;
				break;

			/*
			**	Verify that the unit has been built.
			*/
			case TEVENT_BUILD_UNIT:
				if (hptr->JustBuiltUnit != Data.Unit) return(false);
				td.IsTripped = true;
				break;

			/*
			**	Verify that the infantry has been built.
			*/
			case TEVENT_BUILD_INFANTRY:
				if (hptr->JustBuiltInfantry != Data.Infantry) return(false);
				td.IsTripped = true;
				break;

			/*
			**	Verify that the aircraft has been built.
			*/
			case TEVENT_BUILD_AIRCRAFT:
				if (hptr->JustBuiltAircraft != Data.Aircraft) return(false);
				td.IsTripped = true;
				break;

			/*
			**	Verify that the specified number of buildings have been destroyed.
			*/
			case TEVENT_NBUILDINGS_DESTROYED:
				if (hptr->BuildingsLost < Data.Value) return(false);
				break;

			/*
			**	Verify that the specified number of units have been destroyed.
			*/
			case TEVENT_NUNITS_DESTROYED:
				if (hptr->UnitsLost < Data.Value) return(false);
				break;

			default:
				break;
		}
	}

	hptr = HouseClass::As_Pointer(Data.House);
	if (hptr) {
		switch (Type) {
			case TEVENT_LOW_POWER:
				if (hptr->Power_Fraction() >= 1) return(false);
				break;

//			case TEVENT_SPIED:
//				if (!hptr->IsSpied) return(false);
//				break;

			case TEVENT_THIEVED:
				if (!hptr->IsThieved) return(false);
				break;

			/*
			**	Verify that the house has been discovered.
			*/
			case TEVENT_HOUSE_DISCOVERED:
				if (!hptr->IsDiscovered) return(false);
				break;

			/*
			**	Verify that all buildings have been destroyed.
			*/
			case TEVENT_BUILDINGS_DESTROYED:
				if (hptr->ActiveBScan) return(false);
				break;

			/*
			**	Verify that all units have been destroyed -- with some
			**	exceptions.
			*/
			case TEVENT_UNITS_DESTROYED:
				if (hptr->ActiveUScan | hptr->ActiveIScan) return(false);
				break;

			/*
			**	Verify that all buildings and units have been destroyed.
			*/
			case TEVENT_ALL_DESTROYED:
				if (hptr->ActiveBScan | hptr->ActiveUScan | hptr->ActiveIScan | hptr->ActiveVScan) return(false);
				break;

			default:
				break;
		}
	} else {
		switch (Type) {
			case TEVENT_LOW_POWER:
			case TEVENT_THIEVED:
			case TEVENT_HOUSE_DISCOVERED:
			case TEVENT_BUILDINGS_DESTROYED:
			case TEVENT_UNITS_DESTROYED:
			case TEVENT_ALL_DESTROYED:
				return(false);

			default:
				break;
		}
	}

	return(true);
}


/***********************************************************************************************
 * TEventClass::Build_INI_Entry -- Builds the ini text for this event.                         *
 *                                                                                             *
 *    This routine will build the ini text for this trigger event. The ini text is appended    *
 *    to the string buffer specified. This routine is used to build the complete trigger       *
 *    ini text for writing out to the INI scenario file.                                       *
 *                                                                                             *
 * INPUT:   ptr   -- Pointer to the string buffer that will hold the event INI data.           *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   11/28/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
void TEventClass::Build_INI_Entry(char * ptr) const
{
	sprintf(ptr, "%d,%d,%d", Event, TeamTypes.Logical_ID(Team), Data.Value);
}


/***********************************************************************************************
 * TEventClass::Read_INI -- Parses the INI text for this event's data.                         *
 *                                                                                             *
 *    This routine is used to parse the INI data line to fetch the event's data from it.       *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   11/28/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
void TEventClass::Read_INI(void)
{
	char const * token;
	switch (NewINIFormat) {
		default:
			Event = (TEventType)Legacy_Trigger_Byte(atoi(strtok(NULL, ",")));
			Team.Set_Raw(atoi(strtok(NULL, ",")));
			Data.Value = atoi(strtok(NULL, ","));
			break;

		case 1:
			token = strtok(NULL, ",");
			Event = TEVENT_NONE;
			if (token) Event = (TEventType)Legacy_Trigger_Byte(atoi(token));

			token = strtok(NULL, ",");
			Team = NULL;
			Data.Value = -1;
			if (token) {
				if (Event_Needs(Event) == NEED_TEAM) {
					Team = TeamTypes.Raw_Ptr(atoi(token));
				} else {
					Data.Value = atoi(token);
				}
			}
			break;

		case 0:
			Event = (TEventType)Legacy_Trigger_Byte(atoi(strtok(NULL, ",")));

			strtok(NULL, ",");
			strtok(NULL, ",");

			Team = TeamTypeClass::From_Name(strtok(NULL, ","));
			Data.Value = atoi(strtok(NULL, ","));
			strtok(NULL, ",");
			break;
	}
	Data.Value = Normalize_Trigger_Data(Event_Needs(Event), Data.Value);
}


/***********************************************************************************************
 * Event_Needs -- Returns with what this event type needs for data.                            *
 *                                                                                             *
 *    This routine will examine the specified event type and return a code that indicates      *
 *    the type of data that must be supplied to this event. Some events require no data and    *
 *    these will return NEED_NONE.                                                             *
 *                                                                                             *
 * INPUT:   event -- The event type to examine.                                                *
 *                                                                                             *
 * OUTPUT:  Returns with the type of additional data that is needed by this event.             *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   11/28/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
NeedType Event_Needs(TEventType event)
{
	switch (event) {
		case TEVENT_THIEVED:
		case TEVENT_PLAYER_ENTERED:
		case TEVENT_CROSS_HORIZONTAL:
		case TEVENT_CROSS_VERTICAL:
		case TEVENT_ENTERS_ZONE:
		case TEVENT_HOUSE_DISCOVERED:
		case TEVENT_BUILDINGS_DESTROYED:
		case TEVENT_UNITS_DESTROYED:
		case TEVENT_ALL_DESTROYED:
		case TEVENT_LOW_POWER:
			return(NEED_HOUSE);

		case TEVENT_NUNITS_DESTROYED:
		case TEVENT_NBUILDINGS_DESTROYED:
		case TEVENT_CREDITS:
		case TEVENT_TIME:
		case TEVENT_GLOBAL_SET:
		case TEVENT_GLOBAL_CLEAR:
			return(NEED_NUMBER);

		case TEVENT_BUILDING_EXISTS:
		case TEVENT_BUILD:
			return(NEED_STRUCTURE);

		case TEVENT_BUILD_UNIT:
			return(NEED_UNIT);

		case TEVENT_BUILD_INFANTRY:
			return(NEED_INFANTRY);

		case TEVENT_BUILD_AIRCRAFT:
			return(NEED_AIRCRAFT);

		case TEVENT_LEAVES_MAP:
			return(NEED_TEAM);

		default:
			break;
	}
	return(NEED_NONE);
}


/***********************************************************************************************
 * Event_From_Name -- retrieves EventType for given name                                       *
 *                                                                                             *
 * INPUT:                                                                                      *
 *      name      name to get event for                                                        *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *      EventType for given name                                                               *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *      none.                                                                                  *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   11/29/1994 BR : Created.                                                                  *
 *=============================================================================================*/
TEventType Event_From_Name (char const * name)
{
	if (name) {
		for (TEventType i = TEVENT_NONE; i < TEVENT_COUNT; i++) {
			if (!stricmp(name, EventText[i])) {
				return(i);
			}
		}
	}

	return(TEVENT_NONE);
}


/***********************************************************************************************
 * Name_From_Event -- retrieves name for EventType                                             *
 *                                                                                             *
 * INPUT:                                                                                      *
 *      event      EventType to get name for                                                   *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *      name for EventType                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *      none.                                                                                  *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   11/29/1994 BR : Created.                                                                  *
 *=============================================================================================*/
char const * Name_From_Event(TEventType event)
{
	return(EventText[event]);
}


/***********************************************************************************************
 * Attaches_To -- Determines what event can be attached to.                                    *
 *                                                                                             *
 *    This routine is used to determine what this event type can be attached to in the game.   *
 *    Some events are specifically tied to cells or game object. This routine will indicate    *
 *    this requirement.                                                                        *
 *                                                                                             *
 * INPUT:   event -- The event type to examine.                                                *
 *                                                                                             *
 * OUTPUT:  Returns with the attachable characteristics for this event type. These             *
 *          characteristics are represented by a composite bit field. Some events can be       *
 *          attached to multiple objects.                                                      *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   11/28/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
AttachType Attaches_To(TEventType event)
{
	AttachType attach = ATTACH_NONE;

	switch (event) {
		case TEVENT_CROSS_HORIZONTAL:
		case TEVENT_CROSS_VERTICAL:
		case TEVENT_ENTERS_ZONE:
		case TEVENT_PLAYER_ENTERED:
		case TEVENT_ANY:
		case TEVENT_DISCOVERED:
		case TEVENT_NONE:
			attach = attach | ATTACH_CELL;
			break;

		default:
			break;
	}

	switch (event) {
		case TEVENT_SPIED:
		case TEVENT_PLAYER_ENTERED:
		case TEVENT_DISCOVERED:
		case TEVENT_DESTROYED:
		case TEVENT_ATTACKED:
		case TEVENT_ANY:
		case TEVENT_NONE:
			attach = attach | ATTACH_OBJECT;
			break;

		default:
			break;
	}

	switch (event) {
//		case TEVENT_CROSS_HORIZONTAL:
//		case TEVENT_CROSS_VERTICAL:
		case TEVENT_ENTERS_ZONE:
		case TEVENT_ANY:
			attach = attach | ATTACH_MAP;
			break;

		default:
			break;
	}

	switch (event) {
		case TEVENT_LOW_POWER:
		case TEVENT_EVAC_CIVILIAN:
		case TEVENT_BUILDING_EXISTS:
		case TEVENT_BUILD:
		case TEVENT_BUILD_UNIT:
		case TEVENT_BUILD_INFANTRY:
		case TEVENT_BUILD_AIRCRAFT:
		case TEVENT_NOFACTORIES:
		case TEVENT_BUILDINGS_DESTROYED:
		case TEVENT_NBUILDINGS_DESTROYED:
		case TEVENT_UNITS_DESTROYED:
		case TEVENT_NUNITS_DESTROYED:
		case TEVENT_ALL_DESTROYED:
		case TEVENT_HOUSE_DISCOVERED:
		case TEVENT_CREDITS:
//		case TEVENT_ATTACKED:
		case TEVENT_THIEVED:
		case TEVENT_ANY:
		case TEVENT_FAKES_DESTROYED:
			attach = attach | ATTACH_HOUSE;
			break;

		default:
			break;
	}

	switch (event) {
		case TEVENT_TIME:
		case TEVENT_GLOBAL_SET:
		case TEVENT_GLOBAL_CLEAR:
		case TEVENT_MISSION_TIMER_EXPIRED:
		case TEVENT_ANY:
		case TEVENT_ALL_BRIDGES_DESTROYED:
		case TEVENT_LEAVES_MAP:
			attach = attach | ATTACH_GENERAL;
			break;

		default:
			break;
	}

	return(attach);
}
