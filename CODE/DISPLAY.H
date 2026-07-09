/*
 * Copyright 2025 Electronic Arts Inc.
 *
 * Derived from Electronic Arts Inc.'s public source release for
 * Command & Conquer: Red Alert.
 * Licensed under GPLv3-or-later with additional Electronic Arts terms.
 * Modified for the macOS port, 2026.
 * See LICENSE.md.
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include	"map.h"
#include	"layer.h"


#define	ICON_PIXEL_W	 		24
#define	ICON_PIXEL_H			24
#define	ICON_LEPTON_W			256
#define	ICON_LEPTON_H			256
#define	CELL_PIXEL_W	 		ICON_PIXEL_W
#define	CELL_PIXEL_H			ICON_PIXEL_H
#define	CELL_LEPTON_W			ICON_LEPTON_W
#define	CELL_LEPTON_H			ICON_LEPTON_H

//	-----------------------------------------------------------
#define	PIXEL_LEPTON_W			(ICON_LEPTON_W/ICON_PIXEL_W)
#define	PIXEL_LEPTON_H			(ICON_LEPTON_H/ICON_PIXEL_H)

#define	SIDE_BAR_TAC_WIDTH	10
#define  SIDE_BAR_TAC_HEIGHT	8

extern COORDINATE Coord_Add(COORDINATE coord1, COORDINATE coord2);

class DisplayClass: public MapClass
{
	public:

		/*
		** The tactical map display position is indicated by the cell of the
		**	upper left hand corner. These should not be altered directly. Use
		**	the Set_Tactical_Position function instead.
		*/
		COORDINATE TacticalCoord;

		/*
		**	The dimensions (in cells) of the visible window onto the game map. This tactical
		**	map is how the player interacts and views the game world.
		*/
		LEPTON TacLeptonWidth;
		LEPTON TacLeptonHeight;

		/*
		**	These layer control elements are used to group the displayable objects
		**	so that proper overlap can be obtained.
		*/
		static LayerClass Layer[LAYER_COUNT];

		/*
		**	This records the position and shape of a placement cursor to display
		**	over the map. This cursor is used when placing buildings and also used
		**	extensively by the scenario editor.
		*/
		CELL ZoneCell;
		short ZoneOffset;
		short const *CursorSize;
		bool ProximityCheck;		// Is proximity check ok?

		/*
		** This holds the building type that is about to be placed upon the map.
		**	It is only valid during the building placement state. The PendingLegal
		**	flag is updated as the cursor moves and it reflects the legality of
		**	placing the building at the desired location.
		*/
		ObjectClass * PendingObjectPtr;
		ObjectTypeClass const * PendingObject;
		HousesType PendingHouse;

		static unsigned char FadingBrighten[256];
		static unsigned char FadingShade[256];
		static unsigned char FadingWayDark[256];
		static unsigned char FadingLight[256];
		static unsigned char FadingGreen[256];
		static unsigned char FadingYellow[256];
		static unsigned char FadingRed[256];
		static unsigned char TranslucentTable[(MAGIC_COL_COUNT+1)*256];
		static unsigned char WhiteTranslucentTable[(1+1)*256];
		static unsigned char MouseTranslucentTable[(4+1)*256];
		static void const *TransIconset;
		static unsigned char UnitShadow[(USHADOW_COL_COUNT+1)*256];
		static unsigned char UnitShadowAir[(USHADOW_COL_COUNT+1)*256];
		static unsigned char SpecialGhost[2*256];

		//-------------------------------------------------------------------------
		DisplayClass(void);
		DisplayClass(NoInitClass const & x) : MapClass(x) {};

		virtual void Read_INI(CCINIClass & ini);
		void Write_INI(CCINIClass & ini);

		/*
		** Initialization
		*/
		virtual void One_Time(void);							// One-time inits
		virtual void Init_Clear(void);						// Clears all to known state
		virtual void Init_IO(void);							// Inits button list
		virtual void Init_Theater(TheaterType theater);	// Theater-specific inits

		/*
		**	General display/map/interface support functionality.
		*/
		virtual void AI(KeyNumType &input, int x, int y);
		virtual void Draw_It(bool complete=false);

		/*
		**	Added functionality.
		*/
		void All_To_Look(bool units_only=false);
		void Constrained_Look(COORDINATE coord, LEPTON distance);
		void Shroud_Cell(CELL cell/*KO, bool shadeit = false*/);
		void Encroach_Shadow(void);
		void Center_Map(COORDINATE center=0L);
		virtual bool Map_Cell(CELL cell, HouseClass *house);
		virtual CELL Click_Cell_Calc(int x, int y) const;
		virtual void Help_Text(int , int =-1, int =-1, int =YELLOW, bool =false) {};
		virtual MouseType Get_Mouse_Shape(void) const = 0;
		virtual bool Scroll_Map(DirType facing, int & distance, bool really);
		virtual void Refresh_Cells(CELL cell, short const *list);
		virtual void Set_View_Dimensions(int x, int y, int width=-1, int height=-1);

		/*
		**	Pending object placement control.
		*/
		virtual void Put_Place_Back(TechnoClass * ) {}; // Affects 'pending' system.
		void Cursor_Mark(CELL pos, bool on);
		void Set_Cursor_Shape(short const * list);
		CELL Set_Cursor_Pos(CELL pos = -1);
		void Get_Occupy_Dimensions(int & w, int & h, short const *list) const;

		/*
		**	Tactical map only functionality.
		*/
		virtual void Set_Tactical_Position(COORDINATE coord);
		void Refresh_Band(void);
		void Select_These(COORDINATE coord1, COORDINATE coord2);
		COORDINATE Pixel_To_Coord(int x, int y) const;
		bool Coord_To_Pixel(COORDINATE coord, int & x, int & y) const;
		bool Push_Onto_TacMap(COORDINATE &source, COORDINATE &dest);
		void Remove(ObjectClass const * object, LayerType layer);
		void Submit(ObjectClass const * object, LayerType layer);
		CELL Calculated_Cell(SourceType dir, WAYPOINT waypoint=-1, CELL cell=-1, SpeedType loco=SPEED_FOOT, bool zonecheck=true, MZoneType mzone=MZONE_NORMAL) const;
		bool In_View(register CELL cell) const;
		bool Passes_Proximity_Check(ObjectTypeClass const * object, HousesType house, short const * list, CELL trycell) const;
		ObjectClass * Cell_Object(CELL cell, int x=0, int y=0) const;
		ObjectClass * Next_Object(ObjectClass * object) const;
		ObjectClass * Prev_Object(ObjectClass * object) const;
		int Cell_Shadow(CELL cell) const;
		short const * Text_Overlap_List(char const * text, int x, int y) const;
		bool Is_Spot_Free(COORDINATE coord) const;
		COORDINATE Closest_Free_Spot(COORDINATE coord, bool any=false) const;
		void Sell_Mode_Control(int control);
		void Repair_Mode_Control(int control);

		virtual void Flag_Cell(CELL cell);
		bool Is_Cell_Flagged(CELL cell) const {return CellRedraw.Is_True(cell);};

		/*
		** Computes starting position based on player's units' Coords.
		*/
		void Compute_Start_Pos(void);

		/*
		**	File I/O.
		*/
		virtual void Code_Pointers(void);
		virtual void Decode_Pointers(void);

	protected:
		virtual void Mouse_Right_Press(void);
		virtual void Mouse_Left_Press(int x, int y);
		virtual void Mouse_Left_Up(CELL cell, bool shadow, ObjectClass * object, ActionType action, bool wsmall = false);
		virtual void Mouse_Left_Held(int x, int y);
		virtual void Mouse_Left_Release(CELL cell, int x, int y, ObjectClass * object, ActionType action, bool wsmall = false);

	public:
		/*
		**	This is the pixel offset for the upper left corner of the tactical map.
		*/
		int TacPixelX;
		int TacPixelY;

		/*
		**	This is the coordinate that the tactical map should be in at next available opportunity.
		*/
		COORDINATE DesiredTacticalCoord;

		/*
		**	If something in the tactical map is to be redrawn, this flag is set to true.
		*/
		unsigned IsToRedraw:1;

		/*
		**	If the player is currently wielding a wrench (to select buildings for repair),
		**	then this flag is true. In such a state, normal movement and combat orders
		**	are preempted.
		*/
		unsigned IsRepairMode:1;

		/*
		**	If the player is currently in "sell back" mode, then this flag will be
		**	true. While in this mode, anything clicked on will be sold back to the
		**	"factory".
		*/
		unsigned IsSellMode:1;

		/*
		**	If the player is currently in ion cannon targeting mode, then this
		** flag will be true.  While in this mode, anything clicked on will be
		** be destroyed by the ION cannon.
		*/
		SpecialWeaponType IsTargettingMode;

	protected:

		/*
		**	If it is currently in rubber band mode (multi unit selection), then this
		**	flag will be true. While in such a mode, normal input is preempted while
		**	the extended selection is in progress.
		*/
		unsigned IsRubberBand:1;

		/*
		**	The moment the mouse is held down, this flag gets set. If the mouse is dragged
		**	a sufficient distance while held down, then true rubber band mode selection
		**	can begin. Using a minimum distance prevents accidental rubber band selection
		**	mode from being initiated.
		*/
		unsigned IsTentative:1;

#if defined(RA_MOBILE_TOUCH)
		/*
		**	Mobile touch can clear IsTentative through cursor hover updates before
		**	the release event is processed. Keep the press anchor eligible for
		**	drag selection until the matching mobile release arrives.
		*/
		unsigned IsMobileRubberBandCandidate:1;
#endif

		/*
		**	This gadget class is used for capturing input to the tactical map. All mouse input
		**	will be routed through this gadget.
		*/
		class TacticalClass : public GadgetClass {
			public:
				TacticalClass(void) : GadgetClass(0,0,0,0,LEFTPRESS|LEFTRELEASE|LEFTHELD|LEFTUP|RIGHTPRESS,true) {};

			protected:
				virtual int Action(unsigned flags, KeyNumType & key);
		};
		friend class TacticalClass;

		/*
		**	This is the "button" that tracks all input to the tactical map.
		** It must be available to derived classes, for Save/Load purposes.
		*/
		static TacticalClass TacButton;

	private:

		/*
		**	This is a utility flag that is set during the icon draw process only if there
		**	was at least one shadow icon detected that should be redrawn. When the shadow
		**	drawing logic is to take place, but this flag is false, then the shadow drawing
		**	will be skipped since it would perform no function.
		*/
		unsigned IsShadowPresent:1;

		/*
		**	Rubber band mode consists of stretching a box from the anchor point (specified
		**	here) to the current cursor position.
		*/
		int BandX,BandY;
		int NewX,NewY;

		static void const *ShadowShapes;
		static unsigned char ShadowTrans[(SHADOW_COL_COUNT+1)*256];

		void Redraw_Icons(void);
		void Redraw_OIcons(void);
		void Redraw_Shadow(void);

		/*
		**	This bit array is used to flag cells to be redrawn. If the icon needs to
		**	be redrawn for a cell, then the corresponding flag will be true.
		*/
		static BooleanVectorClass CellRedraw;

		bool Good_Reinforcement_Cell(CELL outcell, CELL incell, SpeedType loco, int zone, MZoneType mzone) const;
};


#endif
