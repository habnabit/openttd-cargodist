/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file depot_gui.cpp The GUI for depots. */

#include "train.h"
#include "ship.h"
#include "aircraft.h"
#include "roadveh.h"
#include "gui.h"
#include "textbuf_gui.h"
#include "viewport_func.h"
#include "gfx_func.h"
#include "command_func.h"
#include "depot_base.h"
#include "vehicle_gui.h"
#include "newgrf_engine.h"
#include "spritecache.h"
#include "strings_func.h"
#include "window_func.h"
#include "vehicle_func.h"
#include "company_func.h"
#include "tilehighlight_func.h"
#include "window_gui.h"
#include "vehiclelist.h"

#include "table/strings.h"
#include "table/sprites.h"

/*
 * Since all depot window sizes aren't the same, we need to modify sizes a little.
 * It's done with the following arrays of widget indexes. Each of them tells if a widget side should be moved and in what direction.
 * How long they should be moved and for what window types are controlled in ShowDepotWindow()
 */

/* Names of the widgets. Keep them in the same order as in the widget array */
enum DepotWindowWidgets {
	DEPOT_WIDGET_CLOSEBOX = 0,
	DEPOT_WIDGET_CAPTION,
	DEPOT_WIDGET_STICKY,
	DEPOT_WIDGET_SELL,
	DEPOT_WIDGET_SELL_CHAIN,
	DEPOT_WIDGET_SELL_ALL,
	DEPOT_WIDGET_AUTOREPLACE,
	DEPOT_WIDGET_MATRIX,
	DEPOT_WIDGET_V_SCROLL, ///< Vertical scrollbar
	DEPOT_WIDGET_H_SCROLL, ///< Horizontal scrollbar
	DEPOT_WIDGET_BUILD,
	DEPOT_WIDGET_CLONE,
	DEPOT_WIDGET_LOCATION,
	DEPOT_WIDGET_VEHICLE_LIST,
	DEPOT_WIDGET_STOP_ALL,
	DEPOT_WIDGET_START_ALL,
	DEPOT_WIDGET_RESIZE,
};

/* Widget array for all depot windows.
 * If a widget is needed in some windows only (like train specific), add it for all windows
 * and use HideWindowWidget in ShowDepotWindow() to remove it in the windows where it should not be
 * Keep the widget numbers in sync with the enum or really bad stuff will happen!!! */

/* When adding widgets, place them as you would place them for the ship depot and define how you want it to move in widget_moves[]
 * If you want a widget for one window only, set it to be hidden in ShowDepotWindow() for the windows where you don't want it
 * NOTE: the train only widgets are moved/resized in ShowDepotWindow() so they follow certain other widgets if they are moved to ensure that they stick together.
 *    Changing the size of those here will not have an effect at all. It should be done in ShowDepotWindow()
 */

/*
 * Some of the widgets are placed outside the window (negative coordinates).
 * The reason is that they are placed relatively to the matrix and the matrix is just one pixel (in 0, 14).
 * The matrix and the rest of the window will be resized when the size of the boxes is set and then all the widgets will be inside the window.
 */
static const Widget _depot_widgets[] = {
	{   WWT_CLOSEBOX,   RESIZE_NONE,  COLOUR_GREY,     0,    10,     0,    13, STR_BLACK_CROSS,     STR_TOOLTIP_CLOSE_WINDOW},           // DEPOT_WIDGET_CLOSEBOX
	{    WWT_CAPTION,  RESIZE_RIGHT,  COLOUR_GREY,    11,    23,     0,    13, 0x0,                 STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS}, // DEPOT_WIDGET_CAPTION
	{  WWT_STICKYBOX,     RESIZE_LR,  COLOUR_GREY,    24,    35,     0,    13, 0x0,                 STR_TOOLTIP_STICKY},                  // DEPOT_WIDGET_STICKY

	/* Widgets are set up run-time */
	{     WWT_IMGBTN,    RESIZE_LRB,  COLOUR_GREY,     1,    23,    14,   -32, 0x0,                 STR_NULL},                         // DEPOT_WIDGET_SELL
	{     WWT_IMGBTN,   RESIZE_LRTB,  COLOUR_GREY,     1,    23,   -55,   -32, SPR_SELL_CHAIN_TRAIN,STR_DEPOT_DRAG_WHOLE_TRAIN_TO_SELL_TOOLTIP}, // DEPOT_WIDGET_SELL_CHAIN, trains only
	{ WWT_PUSHIMGBTN,   RESIZE_LRTB,  COLOUR_GREY,     1,    23,   -31,    -9, 0x0,                 STR_NULL},                         // DEPOT_WIDGET_SELL_ALL
	{ WWT_PUSHIMGBTN,   RESIZE_LRTB,  COLOUR_GREY,     1,    23,    -8,    14, 0x0,                 STR_NULL},                         // DEPOT_WIDGET_AUTOREPLACE

	{     WWT_MATRIX,     RESIZE_RB,  COLOUR_GREY,     0,     0,    14,    14, 0x0,                 STR_NULL},                         // DEPOT_WIDGET_MATRIX
	{  WWT_SCROLLBAR,    RESIZE_LRB,  COLOUR_GREY,    24,    35,    14,    14, 0x0,                 STR_TOOLTIP_VSCROLL_BAR_SCROLLS_LIST}, // DEPOT_WIDGET_V_SCROLL

	{ WWT_HSCROLLBAR,    RESIZE_RTB,  COLOUR_GREY,     0,     0,     3,    14, 0x0,                 STR_TOOLTIP_HSCROLL_BAR_SCROLLS_LIST}, // DEPOT_WIDGET_H_SCROLL, trains only

	/* The buttons in the bottom of the window. left and right is not important as they are later resized to be equal in size
	 * This calculation is based on right in DEPOT_WIDGET_LOCATION and it presumes left of DEPOT_WIDGET_BUILD is 0            */
	{ WWT_PUSHTXTBTN,     RESIZE_TB,  COLOUR_GREY,     0,     0,    15,    26, 0x0,                 STR_NULL},                         // DEPOT_WIDGET_BUILD
	{    WWT_TEXTBTN,     RESIZE_TB,  COLOUR_GREY,     0,     0,    15,    26, 0x0,                 STR_NULL},                         // DEPOT_WIDGET_CLONE
	{ WWT_PUSHTXTBTN,    RESIZE_RTB,  COLOUR_GREY,     0,   -12,    15,    26, STR_BUTTON_LOCATION, STR_NULL},                         // DEPOT_WIDGET_LOCATION
	{ WWT_PUSHTXTBTN,   RESIZE_LRTB,  COLOUR_GREY,   -11,     0,    15,    26, 0x0,                 STR_NULL},                         // DEPOT_WIDGET_VEHICLE_LIST
	{ WWT_PUSHIMGBTN,   RESIZE_LRTB,  COLOUR_GREY,     1,    11,    15,    26, SPR_FLAG_VEH_STOPPED,STR_NULL},                         // DEPOT_WIDGET_STOP_ALL
	{ WWT_PUSHIMGBTN,   RESIZE_LRTB,  COLOUR_GREY,    12,    23,    15,    26, SPR_FLAG_VEH_RUNNING,STR_NULL},                         // DEPOT_WIDGET_START_ALL
	{  WWT_RESIZEBOX,   RESIZE_LRTB,  COLOUR_GREY,    24,    35,    15,    26, 0x0,                 STR_TOOLTIP_RESIZE},                // DEPOT_WIDGET_RESIZE
	{   WIDGETS_END},
};


static const WindowDesc _train_depot_desc(
	WDP_AUTO, WDP_AUTO, 36, 27, 362, 123,
	WC_VEHICLE_DEPOT, WC_NONE,
	WDF_STD_TOOLTIPS | WDF_STD_BTN | WDF_DEF_WIDGET | WDF_UNCLICK_BUTTONS | WDF_STICKY_BUTTON | WDF_RESIZABLE,
	_depot_widgets
);

static const WindowDesc _road_depot_desc(
	WDP_AUTO, WDP_AUTO, 36, 27, 316, 97,
	WC_VEHICLE_DEPOT, WC_NONE,
	WDF_STD_TOOLTIPS | WDF_STD_BTN | WDF_DEF_WIDGET | WDF_UNCLICK_BUTTONS | WDF_STICKY_BUTTON | WDF_RESIZABLE,
	_depot_widgets
);

static const WindowDesc _ship_depot_desc(
	WDP_AUTO, WDP_AUTO, 36, 27, 306, 99,
	WC_VEHICLE_DEPOT, WC_NONE,
	WDF_STD_TOOLTIPS | WDF_STD_BTN | WDF_DEF_WIDGET | WDF_UNCLICK_BUTTONS | WDF_STICKY_BUTTON | WDF_RESIZABLE,
	_depot_widgets
);

static const WindowDesc _aircraft_depot_desc(
	WDP_AUTO, WDP_AUTO, 36, 27, 332, 99,
	WC_VEHICLE_DEPOT, WC_NONE,
	WDF_STD_TOOLTIPS | WDF_STD_BTN | WDF_DEF_WIDGET | WDF_UNCLICK_BUTTONS | WDF_STICKY_BUTTON | WDF_RESIZABLE,
	_depot_widgets
);

extern void DepotSortList(VehicleList *list);

/**
 * This is the Callback method after the cloning attempt of a vehicle
 * @param success indicates completion (or not) of the operation
 * @param tile unused
 * @param p1 unused
 * @param p2 unused
 */
void CcCloneVehicle(bool success, TileIndex tile, uint32 p1, uint32 p2)
{
	if (!success) return;

	const Vehicle *v = Vehicle::Get(_new_vehicle_id);

	ShowVehicleViewWindow(v);
}

static void TrainDepotMoveVehicle(const Vehicle *wagon, VehicleID sel, const Vehicle *head)
{
	const Vehicle *v = Vehicle::Get(sel);

	if (v == wagon) return;

	if (wagon == NULL) {
		if (head != NULL) wagon = head->Last();
	} else {
		wagon = wagon->Previous();
		if (wagon == NULL) return;
	}

	if (wagon == v) return;

	DoCommandP(v->tile, v->index + ((wagon == NULL ? INVALID_VEHICLE : wagon->index) << 16), _ctrl_pressed ? 1 : 0, CMD_MOVE_RAIL_VEHICLE | CMD_MSG(STR_ERROR_CAN_T_MOVE_VEHICLE));
}

/* Array to hold the block sizes
 * First part is the vehicle type, while the last is 0 = x, 1 = y */
uint _block_sizes[4][2];

/* Array to hold the default resize capacities
 * First part is the vehicle type, while the last is 0 = x, 1 = y */
const uint _resize_cap[][2] = {
/* VEH_TRAIN */    {6, 10 * 29},
/* VEH_ROAD */     {5, 5},
/* VEH_SHIP */     {3, 3},
/* VEH_AIRCRAFT */ {3, 4},
};

static void ResizeDefaultWindowSizeForTrains()
{
	_block_sizes[VEH_TRAIN][0] = 1;
	_block_sizes[VEH_TRAIN][1] = GetVehicleListHeight(VEH_TRAIN);
}

static void ResizeDefaultWindowSizeForRoadVehicles()
{
	_block_sizes[VEH_ROAD][0] = 56;
	_block_sizes[VEH_ROAD][1] = GetVehicleListHeight(VEH_ROAD);
}

static void ResizeDefaultWindowSize(VehicleType type)
{
	uint max_width  = 0;
	uint max_height = 0;

	const Engine *e;
	FOR_ALL_ENGINES_OF_TYPE(e, type) {
		EngineID eid = e->index;
		uint x, y;

		switch (type) {
			default: NOT_REACHED();
			case VEH_SHIP:     GetShipSpriteSize(    eid, x, y); break;
			case VEH_AIRCRAFT: GetAircraftSpriteSize(eid, x, y); break;
		}
		if (x > max_width)  max_width  = x;
		if (y > max_height) max_height = y;
	}

	switch (type) {
		default: NOT_REACHED();
		case VEH_SHIP:
			_block_sizes[VEH_SHIP][0] = max(90U, max_width + 20); // we need 20 pixels from the right edge to the sprite
			break;
		case VEH_AIRCRAFT:
			_block_sizes[VEH_AIRCRAFT][0] = max(74U, max_width);
			break;
	}
	_block_sizes[type][1] = max(GetVehicleListHeight(type), max_height);
}

/* Set the size of the blocks in the window so we can be sure that they are big enough for the vehicle sprites in the current game
 * We will only need to call this once for each game */
void InitDepotWindowBlockSizes()
{
	ResizeDefaultWindowSizeForTrains();
	ResizeDefaultWindowSizeForRoadVehicles();
	ResizeDefaultWindowSize(VEH_SHIP);
	ResizeDefaultWindowSize(VEH_AIRCRAFT);
}

static void DepotSellAllConfirmationCallback(Window *w, bool confirmed);
const Sprite *GetAircraftSprite(EngineID engine);

struct DepotWindow : Window {
	VehicleID sel;
	VehicleType type;
	bool generate_list;
	VehicleList vehicle_list;
	VehicleList wagon_list;

	DepotWindow(const WindowDesc *desc, TileIndex tile, VehicleType type) : Window(desc, tile)
	{
		this->sel = INVALID_VEHICLE;
		this->generate_list = true;

		this->owner = GetTileOwner(tile);
		this->CreateDepotListWindow(type);

		this->FindWindowPlacementAndResize(desc);
	}

	~DepotWindow()
	{
		DeleteWindowById(WC_BUILD_VEHICLE, this->window_number);
	}

	/** Draw a vehicle in the depot window in the box with the top left corner at x,y
	 * @param *w Window to draw in
	 * @param *v Vehicle to draw
	 * @param x Left side of the box to draw in
	 * @param y Top of the box to draw in
	 */
	void DrawVehicleInDepot(Window *w, const Vehicle *v, int x, int y)
	{
		bool free_wagon = false;
		int sprite_y = y + this->resize.step_height - GetVehicleListHeight(v->type);

		switch (v->type) {
			case VEH_TRAIN: {
				const Train *u = Train::From(v);
				free_wagon = u->IsFreeWagon();

				uint x_space = free_wagon ? TRAININFO_DEFAULT_VEHICLE_WIDTH : 0;
				DrawTrainImage(u, x + 24 + x_space, sprite_y - 1, this->sel, this->hscroll.cap - x_space, this->hscroll.pos);

				/* Number of wagons relative to a standard length wagon (rounded up) */
				SetDParam(0, (u->tcache.cached_total_length + 7) / 8);
				DrawString(this->widget[DEPOT_WIDGET_MATRIX].left, this->widget[DEPOT_WIDGET_MATRIX].right - 1, y + 4, STR_TINY_BLACK_COMA, TC_FROMSTRING, SA_RIGHT); // Draw the counter
				break;
			}

			case VEH_ROAD:     DrawRoadVehImage( v, x + 24, sprite_y, this->sel, ROADVEHINFO_DEFAULT_VEHICLE_WIDTH); break;
			case VEH_SHIP:     DrawShipImage(    v, x + 19, sprite_y - 1, this->sel); break;
			case VEH_AIRCRAFT: {
				const Sprite *spr = GetSprite(v->GetImage(DIR_W), ST_NORMAL);
				DrawAircraftImage(v, x + 12,
									y + max(spr->height + spr->y_offs - 14, 0), // tall sprites needs an y offset
									this->sel);
			} break;
			default: NOT_REACHED();
		}

		if (free_wagon) {
			DrawString(x, this->widget[DEPOT_WIDGET_MATRIX].right - 1, y + 2, STR_DEPOT_NO_ENGINE);
		} else {
			byte diff_x = 0, diff_y = 0;

			if (v->type == VEH_TRAIN || v->type == VEH_ROAD) {
				/* Arrange unitnumber and flag horizontally */
				diff_x = 15;
			} else {
				/* Arrange unitnumber and flag vertically */
				diff_y = 12;
			}

			DrawSprite((v->vehstatus & VS_STOPPED) ? SPR_FLAG_VEH_STOPPED : SPR_FLAG_VEH_RUNNING, PAL_NONE, x + diff_x, y + diff_y);

			SetDParam(0, v->unitnumber);
			DrawString(x, this->widget[DEPOT_WIDGET_MATRIX].right - 1, y + 2, (uint16)(v->max_age - DAYS_IN_LEAP_YEAR) >= v->age ? STR_BLACK_COMMA : STR_RED_COMMA);
		}
	}

	void DrawDepotWindow(Window *w)
	{
		TileIndex tile = this->window_number;
		int x, y, maxval;

		/* Set the row and number of boxes in each row based on the number of boxes drawn in the matrix */
		uint16 rows_in_display   = GB(this->widget[DEPOT_WIDGET_MATRIX].data, MAT_ROW_START, MAT_ROW_BITS);
		uint16 boxes_in_each_row = GB(this->widget[DEPOT_WIDGET_MATRIX].data, MAT_COL_START, MAT_COL_BITS);

		/* setup disabled buttons */
		this->SetWidgetsDisabledState(!IsTileOwner(tile, _local_company),
			DEPOT_WIDGET_STOP_ALL,
			DEPOT_WIDGET_START_ALL,
			DEPOT_WIDGET_SELL,
			DEPOT_WIDGET_SELL_CHAIN,
			DEPOT_WIDGET_SELL_ALL,
			DEPOT_WIDGET_BUILD,
			DEPOT_WIDGET_CLONE,
			DEPOT_WIDGET_AUTOREPLACE,
			WIDGET_LIST_END);

		/* determine amount of items for scroller */
		if (this->type == VEH_TRAIN) {
			uint max_width = VEHICLEINFO_FULL_VEHICLE_WIDTH;
			for (uint num = 0; num < this->vehicle_list.Length(); num++) {
				uint width = 0;
				for (const Train *v = Train::From(this->vehicle_list[num]); v != NULL; v = v->Next()) {
					width += v->GetDisplayImageWidth();
				}
				max_width = max(max_width, width);
			}
			/* Always have 1 empty row, so people can change the setting of the train */
			SetVScrollCount(w, this->vehicle_list.Length() + this->wagon_list.Length() + 1);
			SetHScrollCount(w, max_width);
		} else {
			SetVScrollCount(w, (this->vehicle_list.Length() + this->hscroll.cap - 1) / this->hscroll.cap);
		}

		/* locate the depot struct */
		if (this->type == VEH_AIRCRAFT) {
			SetDParam(0, GetStationIndex(tile)); // Airport name
		} else {
			Depot *depot = Depot::GetByTile(tile);
			assert(depot != NULL);

			SetDParam(0, depot->town_index);
		}

		w->DrawWidgets();

		uint16 num = this->vscroll.pos * boxes_in_each_row;
		maxval = min(this->vehicle_list.Length(), num + (rows_in_display * boxes_in_each_row));

		for (x = 2, y = 15; num < maxval; y += this->resize.step_height, x = 2) { // Draw the rows
			byte i;

			for (i = 0; i < boxes_in_each_row && num < maxval; i++, num++, x += this->resize.step_width) {
				/* Draw all vehicles in the current row */
				const Vehicle *v = this->vehicle_list[num];
				DrawVehicleInDepot(w, v, x, y);
			}
		}

		maxval = min(this->vehicle_list.Length() + this->wagon_list.Length(), (this->vscroll.pos * boxes_in_each_row) + (rows_in_display * boxes_in_each_row));

		/* draw the train wagons, that do not have an engine in front */
		for (; num < maxval; num++, y += 14) {
			const Vehicle *v = this->wagon_list[num - this->vehicle_list.Length()];
			DrawVehicleInDepot(w, v, x, y);
		}
	}

	struct GetDepotVehiclePtData {
		const Vehicle *head;
		const Vehicle *wagon;
	};

	enum DepotGUIAction {
		MODE_ERROR,
		MODE_DRAG_VEHICLE,
		MODE_SHOW_VEHICLE,
		MODE_START_STOP,
	};

	DepotGUIAction GetVehicleFromDepotWndPt(int x, int y, const Vehicle **veh, GetDepotVehiclePtData *d) const
	{
		uint xt, row, xm = 0, ym = 0;
		int pos, skip = 0;
		uint16 boxes_in_each_row = GB(this->widget[DEPOT_WIDGET_MATRIX].data, MAT_COL_START, MAT_COL_BITS);

		if (this->type == VEH_TRAIN) {
			xt = 0;
			x -= 23;
		} else {
			xt = x / this->resize.step_width;
			xm = x % this->resize.step_width;
			if (xt >= this->hscroll.cap) return MODE_ERROR;

			ym = (y - 14) % this->resize.step_height;
		}

		row = (y - 14) / this->resize.step_height;
		if (row >= this->vscroll.cap) return MODE_ERROR;

		pos = ((row + this->vscroll.pos) * boxes_in_each_row) + xt;

		if ((int)(this->vehicle_list.Length() + this->wagon_list.Length()) <= pos) {
			if (this->type == VEH_TRAIN) {
				d->head  = NULL;
				d->wagon = NULL;
				return MODE_DRAG_VEHICLE;
			} else {
				return MODE_ERROR; // empty block, so no vehicle is selected
			}
		}

		if ((int)this->vehicle_list.Length() > pos) {
			*veh = this->vehicle_list[pos];
			skip = this->hscroll.pos;
		} else {
			pos -= this->vehicle_list.Length();
			*veh = this->wagon_list[pos];
			/* free wagons don't have an initial loco. */
			x -= VEHICLEINFO_FULL_VEHICLE_WIDTH;
		}

		switch (this->type) {
			case VEH_TRAIN: {
				const Train *v = Train::From(*veh);
				d->head = d->wagon = v;

				/* either pressed the flag or the number, but only when it's a loco */
				if (x < 0 && v->IsFrontEngine()) return (x >= -10) ? MODE_START_STOP : MODE_SHOW_VEHICLE;

				/* Skip vehicles that are scrolled off the list */
				x += skip;

				/* find the vehicle in this row that was clicked */
				for (; v != NULL; v = v->Next()) {
					x -= v->GetDisplayImageWidth();
					if (x < 0) break;
				}

				d->wagon = (v != NULL ? v->GetFirstEnginePart() : NULL);

				return MODE_DRAG_VEHICLE;
			}

			case VEH_ROAD:
				if (xm >= 24) return MODE_DRAG_VEHICLE;
				if (xm <= 16) return MODE_SHOW_VEHICLE;
				break;

			case VEH_SHIP:
				if (xm >= 19) return MODE_DRAG_VEHICLE;
				if (ym <= 10) return MODE_SHOW_VEHICLE;
				break;

			case VEH_AIRCRAFT:
				if (xm >= 12) return MODE_DRAG_VEHICLE;
				if (ym <= 12) return MODE_SHOW_VEHICLE;
				break;

			default: NOT_REACHED();
		}
		return MODE_START_STOP;
	}

	void DepotClick(int x, int y)
	{
		GetDepotVehiclePtData gdvp = { NULL, NULL };
		const Vehicle *v = NULL;
		DepotGUIAction mode = this->GetVehicleFromDepotWndPt(x, y, &v, &gdvp);

		/* share / copy orders */
		if (_thd.place_mode != HT_NONE && mode != MODE_ERROR) {
			_place_clicked_vehicle = (this->type == VEH_TRAIN ? gdvp.head : v);
			return;
		}

		if (this->type == VEH_TRAIN) v = gdvp.wagon;

		switch (mode) {
			case MODE_ERROR: // invalid
				return;

			case MODE_DRAG_VEHICLE: { // start dragging of vehicle
				VehicleID sel = this->sel;

				if (this->type == VEH_TRAIN && sel != INVALID_VEHICLE) {
					this->sel = INVALID_VEHICLE;
					TrainDepotMoveVehicle(v, sel, gdvp.head);
				} else if (v != NULL) {
					int image = v->GetImage(DIR_W);

					this->sel = v->index;
					this->SetDirty();
					SetObjectToPlaceWnd(image, GetVehiclePalette(v), HT_DRAG, this);

					switch (v->type) {
						case VEH_TRAIN:
							_cursor.short_vehicle_offset = 16 - Train::From(v)->tcache.cached_veh_length * 2;
							break;

						case VEH_ROAD:
							_cursor.short_vehicle_offset = 16 - RoadVehicle::From(v)->rcache.cached_veh_length * 2;
							break;

						default:
							_cursor.short_vehicle_offset = 0;
							break;
					}
					_cursor.vehchain = _ctrl_pressed;
				}
			} break;

			case MODE_SHOW_VEHICLE: // show info window
				ShowVehicleViewWindow(v);
				break;

			case MODE_START_STOP: { // click start/stop flag
				uint command;

				switch (this->type) {
					case VEH_TRAIN:    command = CMD_START_STOP_VEHICLE | CMD_MSG(STR_ERROR_CAN_T_STOP_START_TRAIN);        break;
					case VEH_ROAD:     command = CMD_START_STOP_VEHICLE | CMD_MSG(STR_ERROR_CAN_T_STOP_START_ROAD_VEHICLE); break;
					case VEH_SHIP:     command = CMD_START_STOP_VEHICLE | CMD_MSG(STR_ERROR_CAN_T_STOP_START_SHIP);         break;
					case VEH_AIRCRAFT: command = CMD_START_STOP_VEHICLE | CMD_MSG(STR_ERROR_CAN_T_STOP_START_AIRCRAFT);     break;
					default: NOT_REACHED();
				}
				DoCommandP(v->tile, v->index, 0, command);
			} break;

			default: NOT_REACHED();
		}
	}

	/**
	 * Clones a vehicle
	 * @param *v is the original vehicle to clone
	 */
	void HandleCloneVehClick(const Vehicle *v)
	{
		if (v == NULL || !IsCompanyBuildableVehicleType(v)) return;

		if (!v->IsPrimaryVehicle()) {
			v = v->First();
			/* Do nothing when clicking on a train in depot with no loc attached */
			if (v->type == VEH_TRAIN && !Train::From(v)->IsFrontEngine()) return;
		}

		DoCommandP(this->window_number, v->index, _ctrl_pressed ? 1 : 0, CMD_CLONE_VEHICLE | CMD_MSG(STR_ERROR_CAN_T_BUILD_TRAIN + v->type), CcCloneVehicle);

		ResetObjectToPlace();
	}

	void ResizeDepotButtons(Window *w)
	{
		ResizeButtons(w, DEPOT_WIDGET_BUILD, DEPOT_WIDGET_LOCATION);

		if (this->type == VEH_TRAIN) {
			/* Divide the size of DEPOT_WIDGET_SELL into two equally big buttons so DEPOT_WIDGET_SELL and DEPOT_WIDGET_SELL_CHAIN will get the same size.
			 * This way it will stay the same even if DEPOT_WIDGET_SELL_CHAIN is resized for some reason                                                  */
			this->widget[DEPOT_WIDGET_SELL_CHAIN].top  = ((this->widget[DEPOT_WIDGET_SELL_CHAIN].bottom - this->widget[DEPOT_WIDGET_SELL].top) / 2) + this->widget[DEPOT_WIDGET_SELL].top;
			this->widget[DEPOT_WIDGET_SELL].bottom     = this->widget[DEPOT_WIDGET_SELL_CHAIN].top - 1;
		}
	}

	/* Function to set up vehicle specific sprites and strings
	 * Only use this if it's the same widget, that's used for more than one vehicle type and it needs different text/sprites
	 * Vehicle specific text/sprites, that's in a widget, that's only shown for one vehicle type (like sell whole train) is set in the widget array
	 */
	void SetupStringsForDepotWindow(VehicleType type)
	{
		this->widget[DEPOT_WIDGET_CAPTION].data          = STR_DEPOT_TRAIN_CAPTION + type;
		this->widget[DEPOT_WIDGET_STOP_ALL].tooltips     = STR_DEPOT_MASS_STOP_DEPOT_TRAIN_TOOLTIP + type;
		this->widget[DEPOT_WIDGET_START_ALL].tooltips    = STR_DEPOT_MASS_START_DEPOT_TRAIN_TOOLTIP + type;
		this->widget[DEPOT_WIDGET_SELL].tooltips         = STR_DEPOT_TRAIN_SELL_TOOLTIP + type;
		this->widget[DEPOT_WIDGET_SELL_ALL].tooltips     = STR_DEPOT_SELL_ALL_BUTTON_TRAIN_TOOLTIP + type;

		this->widget[DEPOT_WIDGET_BUILD].data            = STR_DEPOT_TRAIN_NEW_VEHICLES_BUTTON + type;
		this->widget[DEPOT_WIDGET_BUILD].tooltips        = STR_DEPOT_TRAIN_NEW_VEHICLES_TOOLTIP + type;
		this->widget[DEPOT_WIDGET_CLONE].data            = STR_DEPOT_CLONE_TRAIN + type;
		this->widget[DEPOT_WIDGET_CLONE].tooltips        = STR_DEPOT_CLONE_TRAIN_DEPOT_INFO + type;

		this->widget[DEPOT_WIDGET_LOCATION].tooltips     = STR_DEPOT_TRAIN_LOCATION_TOOLTIP + type;
		this->widget[DEPOT_WIDGET_VEHICLE_LIST].tooltips = STR_DEPOT_VEHICLE_ORDER_LIST_TRAIN_TOOLTIP + type;
		this->widget[DEPOT_WIDGET_AUTOREPLACE].tooltips  = STR_DEPOT_AUTOREPLACE_TRAIN_TOOLTIP + type;

		switch (type) {
			default: NOT_REACHED();

			case VEH_TRAIN:
				this->widget[DEPOT_WIDGET_VEHICLE_LIST].data = STR_TRAIN;

				/* Sprites */
				this->widget[DEPOT_WIDGET_SELL].data        = SPR_SELL_TRAIN;
				this->widget[DEPOT_WIDGET_SELL_ALL].data    = SPR_SELL_ALL_TRAIN;
				this->widget[DEPOT_WIDGET_AUTOREPLACE].data = SPR_REPLACE_TRAIN;
				break;

			case VEH_ROAD:
				this->widget[DEPOT_WIDGET_VEHICLE_LIST].data = STR_LORRY;

				/* Sprites */
				this->widget[DEPOT_WIDGET_SELL].data        = SPR_SELL_ROADVEH;
				this->widget[DEPOT_WIDGET_SELL_ALL].data    = SPR_SELL_ALL_ROADVEH;
				this->widget[DEPOT_WIDGET_AUTOREPLACE].data = SPR_REPLACE_ROADVEH;
				break;

			case VEH_SHIP:
				this->widget[DEPOT_WIDGET_VEHICLE_LIST].data = STR_SHIP;

				/* Sprites */
				this->widget[DEPOT_WIDGET_SELL].data        = SPR_SELL_SHIP;
				this->widget[DEPOT_WIDGET_SELL_ALL].data    = SPR_SELL_ALL_SHIP;
				this->widget[DEPOT_WIDGET_AUTOREPLACE].data = SPR_REPLACE_SHIP;
				break;

			case VEH_AIRCRAFT:
				this->widget[DEPOT_WIDGET_VEHICLE_LIST].data = STR_PLANE;

				/* Sprites */
				this->widget[DEPOT_WIDGET_SELL].data        = SPR_SELL_AIRCRAFT;
				this->widget[DEPOT_WIDGET_SELL_ALL].data    = SPR_SELL_ALL_AIRCRAFT;
				this->widget[DEPOT_WIDGET_AUTOREPLACE].data = SPR_REPLACE_AIRCRAFT;
				break;
		}
	}

	void CreateDepotListWindow(VehicleType type)
	{
		this->type = type;
		_backup_orders_tile = 0;

		assert(IsCompanyBuildableVehicleType(type)); // ensure that we make the call with a valid type

		/* Resize the window according to the vehicle type */

		/* Set the number of blocks in each direction */
		this->vscroll.cap = _resize_cap[type][0];
		this->hscroll.cap = _resize_cap[type][1];

		/* Set the block size */
		this->resize.step_width  = _block_sizes[type][0];
		this->resize.step_height = _block_sizes[type][1];

		/* Enlarge the window to fit with the selected number of blocks of the selected size */
		ResizeWindow(this,
					_block_sizes[type][0] * this->hscroll.cap,
					_block_sizes[type][1] * this->vscroll.cap);

		if (type == VEH_TRAIN) {
			/* Make space for the horizontal scrollbar vertically, and the unit
			 * number, flag, and length counter horizontally. */
			ResizeWindow(this, 36, 12);
			/* substract the newly added space from the matrix since it was meant for the scrollbar */
			this->widget[DEPOT_WIDGET_MATRIX].bottom -= 12;
		}

		/* Set the minimum window size to the current window size */
		this->resize.width  = this->width;
		this->resize.height = this->height;

		this->SetupStringsForDepotWindow(type);

		this->widget[DEPOT_WIDGET_MATRIX].data =
			(this->vscroll.cap << MAT_ROW_START) // number of rows to draw on the background
			+ ((type == VEH_TRAIN ? 1 : this->hscroll.cap) << MAT_COL_START); // number of boxes in each row. Trains always have just one


		this->SetWidgetsHiddenState(type != VEH_TRAIN,
			DEPOT_WIDGET_H_SCROLL,
			DEPOT_WIDGET_SELL_CHAIN,
			WIDGET_LIST_END);

		ResizeDepotButtons(this);
	}

	virtual void OnInvalidateData(int data)
	{
		this->generate_list = true;
	}

	virtual void OnPaint()
	{
		if (this->generate_list) {
			/* Generate the vehicle list
			 * It's ok to use the wagon pointers for non-trains as they will be ignored */
			BuildDepotVehicleList(this->type, this->window_number, &this->vehicle_list, &this->wagon_list);
			this->generate_list = false;
			DepotSortList(&this->vehicle_list);
		}
		DrawDepotWindow(this);
	}

	virtual void OnClick(Point pt, int widget)
	{
		switch (widget) {
			case DEPOT_WIDGET_MATRIX: // List
				this->DepotClick(pt.x, pt.y);
				break;

			case DEPOT_WIDGET_BUILD: // Build vehicle
				ResetObjectToPlace();
				ShowBuildVehicleWindow(this->window_number, this->type);
				break;

			case DEPOT_WIDGET_CLONE: // Clone button
				this->InvalidateWidget(DEPOT_WIDGET_CLONE);
				this->ToggleWidgetLoweredState(DEPOT_WIDGET_CLONE);

				if (this->IsWidgetLowered(DEPOT_WIDGET_CLONE)) {
					static const CursorID clone_icons[] = {
						SPR_CURSOR_CLONE_TRAIN, SPR_CURSOR_CLONE_ROADVEH,
						SPR_CURSOR_CLONE_SHIP, SPR_CURSOR_CLONE_AIRPLANE
					};

					_place_clicked_vehicle = NULL;
					SetObjectToPlaceWnd(clone_icons[this->type], PAL_NONE, HT_RECT, this);
				} else {
					ResetObjectToPlace();
				}
					break;

			case DEPOT_WIDGET_LOCATION:
				if (_ctrl_pressed) {
					ShowExtraViewPortWindow(this->window_number);
				} else {
					ScrollMainWindowToTile(this->window_number);
				}
				break;

			case DEPOT_WIDGET_STOP_ALL:
			case DEPOT_WIDGET_START_ALL:
				DoCommandP(this->window_number, 0, this->type | (widget == DEPOT_WIDGET_START_ALL ? (1 << 5) : 0), CMD_MASS_START_STOP);
				break;

			case DEPOT_WIDGET_SELL_ALL:
				/* Only open the confimation window if there are anything to sell */
				if (this->vehicle_list.Length() != 0 || this->wagon_list.Length() != 0) {
					TileIndex tile = this->window_number;
					byte vehtype = this->type;

					SetDParam(0, (vehtype == VEH_AIRCRAFT) ? GetStationIndex(tile) : Depot::GetByTile(tile)->town_index);
					ShowQuery(
						STR_DEPOT_TRAIN_CAPTION + vehtype,
						STR_DEPOT_SELL_CONFIRMATION_TEXT,
						this,
						DepotSellAllConfirmationCallback
					);
				}
				break;

			case DEPOT_WIDGET_VEHICLE_LIST:
				ShowVehicleListWindow(GetTileOwner(this->window_number), this->type, (TileIndex)this->window_number);
				break;

			case DEPOT_WIDGET_AUTOREPLACE:
				DoCommandP(this->window_number, this->type, 0, CMD_DEPOT_MASS_AUTOREPLACE);
				break;

		}
	}

	virtual void OnRightClick(Point pt, int widget)
	{
		if (widget != DEPOT_WIDGET_MATRIX) return;

		GetDepotVehiclePtData gdvp = { NULL, NULL };
		const Vehicle *v = NULL;
		DepotGUIAction mode = this->GetVehicleFromDepotWndPt(pt.x, pt.y, &v, &gdvp);

		if (this->type == VEH_TRAIN) v = gdvp.wagon;

		if (v != NULL && mode == MODE_DRAG_VEHICLE) {
			CargoArray capacity, loaded;

			/* Display info for single (articulated) vehicle, or for whole chain starting with selected vehicle */
			bool whole_chain = (this->type == VEH_TRAIN && _ctrl_pressed);

			/* loop through vehicle chain and collect cargos */
			uint num = 0;
			for (const Vehicle *w = v; w != NULL; w = w->Next()) {
				if (w->cargo_cap > 0 && w->cargo_type < NUM_CARGO) {
					capacity[w->cargo_type] += w->cargo_cap;
					loaded  [w->cargo_type] += w->cargo.Count();
				}

				if (w->type == VEH_TRAIN && !Train::From(w)->HasArticulatedPart()) {
					num++;
					if (!whole_chain) break;
				}
			}

			/* Build tooltipstring */
			static char details[1024];
			details[0] = '\0';
			char *pos = details;

			for (CargoID cargo_type = 0; cargo_type < NUM_CARGO; cargo_type++) {
				if (capacity[cargo_type] == 0) continue;

				SetDParam(0, cargo_type);           // {CARGO} #1
				SetDParam(1, loaded[cargo_type]);   // {CARGO} #2
				SetDParam(2, cargo_type);           // {SHORTCARGO} #1
				SetDParam(3, capacity[cargo_type]); // {SHORTCARGO} #2
				pos = GetString(pos, STR_DEPOT_VEHICLE_TOOLTIP_CARGO, lastof(details));
			}

			/* Show tooltip window */
			uint64 args[2];
			args[0] = (whole_chain ? num : v->engine_type);
			args[1] = (uint64)(size_t)details;
			GuiShowTooltips(whole_chain ? STR_DEPOT_VEHICLE_TOOLTIP_CHAIN : STR_DEPOT_VEHICLE_TOOLTIP, 2, args);
		} else {
			/* Show tooltip help */
			GuiShowTooltips(STR_DEPOT_TRAIN_LIST_TOOLTIP + this->type);
		}
	}


	virtual void OnPlaceObject(Point pt, TileIndex tile)
	{
		const Vehicle *v = CheckMouseOverVehicle();

		if (v != NULL) this->HandleCloneVehClick(v);
	}

	virtual void OnPlaceObjectAbort()
	{
		/* abort clone */
		this->RaiseWidget(DEPOT_WIDGET_CLONE);
		this->InvalidateWidget(DEPOT_WIDGET_CLONE);

		/* abort drag & drop */
		this->sel = INVALID_VEHICLE;
		this->InvalidateWidget(DEPOT_WIDGET_MATRIX);
	};

	/* check if a vehicle in a depot was clicked.. */
	virtual void OnMouseLoop()
	{
		const Vehicle *v = _place_clicked_vehicle;

		/* since OTTD checks all open depot windows, we will make sure that it triggers the one with a clicked clone button */
		if (v != NULL && this->IsWidgetLowered(DEPOT_WIDGET_CLONE)) {
			_place_clicked_vehicle = NULL;
			this->HandleCloneVehClick(v);
		}
	}

	virtual void OnDragDrop(Point pt, int widget)
	{
		switch (widget) {
			case DEPOT_WIDGET_MATRIX: {
				const Vehicle *v = NULL;
				VehicleID sel = this->sel;

				this->sel = INVALID_VEHICLE;
				this->SetDirty();

				if (this->type == VEH_TRAIN) {
					GetDepotVehiclePtData gdvp = { NULL, NULL };

					if (this->GetVehicleFromDepotWndPt(pt.x, pt.y, &v, &gdvp) == MODE_DRAG_VEHICLE &&
						sel != INVALID_VEHICLE) {
						if (gdvp.wagon != NULL && gdvp.wagon->index == sel && _ctrl_pressed) {
							DoCommandP(Vehicle::Get(sel)->tile, Vehicle::Get(sel)->index, true, CMD_REVERSE_TRAIN_DIRECTION | CMD_MSG(STR_ERROR_CAN_T_REVERSE_DIRECTION_RAIL_VEHICLE));
						} else if (gdvp.wagon == NULL || gdvp.wagon->index != sel) {
							TrainDepotMoveVehicle(gdvp.wagon, sel, gdvp.head);
						} else if (gdvp.head != NULL && Train::From(gdvp.head)->IsFrontEngine()) {
							ShowVehicleViewWindow(gdvp.head);
						}
					}
				} else if (this->GetVehicleFromDepotWndPt(pt.x, pt.y, &v, NULL) == MODE_DRAG_VEHICLE &&
					v != NULL &&
					sel == v->index) {
					ShowVehicleViewWindow(v);
				}
			} break;

			case DEPOT_WIDGET_SELL: case DEPOT_WIDGET_SELL_CHAIN:
				if (!this->IsWidgetDisabled(DEPOT_WIDGET_SELL) &&
					this->sel != INVALID_VEHICLE) {

					if (this->IsWidgetDisabled(widget)) return;
					if (this->sel == INVALID_VEHICLE) return;

					this->HandleButtonClick(widget);

					const Vehicle *v = Vehicle::Get(this->sel);
					this->sel = INVALID_VEHICLE;
					this->SetDirty();

					int sell_cmd = (v->type == VEH_TRAIN && (widget == DEPOT_WIDGET_SELL_CHAIN || _ctrl_pressed)) ? 1 : 0;

					bool is_engine = (v->type != VEH_TRAIN || Train::From(v)->IsFrontEngine());

					if (is_engine) {
						_backup_orders_tile = v->tile;
						BackupVehicleOrders(v);
					}

					if (!DoCommandP(v->tile, v->index, sell_cmd, GetCmdSellVeh(v->type)) && is_engine) _backup_orders_tile = 0;
				}
				break;
			default:
				this->sel = INVALID_VEHICLE;
				this->SetDirty();
		}
		_cursor.vehchain = false;
	}

	virtual void OnTimeout()
	{
		if (!this->IsWidgetDisabled(DEPOT_WIDGET_SELL)) {
			this->RaiseWidget(DEPOT_WIDGET_SELL);
			this->InvalidateWidget(DEPOT_WIDGET_SELL);
		}
		if (!this->IsWidgetDisabled(DEPOT_WIDGET_SELL_CHAIN)) {
			this->RaiseWidget(DEPOT_WIDGET_SELL_CHAIN);
			this->InvalidateWidget(DEPOT_WIDGET_SELL_CHAIN);
		}
	}

	virtual void OnResize(Point delta)
	{
		this->vscroll.cap += delta.y / (int)this->resize.step_height;
		this->hscroll.cap += delta.x / (int)this->resize.step_width;
		this->widget[DEPOT_WIDGET_MATRIX].data = (this->vscroll.cap << MAT_ROW_START) + ((this->type == VEH_TRAIN ? 1 : this->hscroll.cap) << MAT_COL_START);
		ResizeDepotButtons(this);
	}

	virtual EventState OnCTRLStateChange()
	{
		if (this->sel != INVALID_VEHICLE) {
			_cursor.vehchain = _ctrl_pressed;
			this->InvalidateWidget(DEPOT_WIDGET_MATRIX);
			return ES_HANDLED;
		}

		return ES_NOT_HANDLED;
	}
};

static void DepotSellAllConfirmationCallback(Window *win, bool confirmed)
{
	if (confirmed) {
		DepotWindow *w = (DepotWindow*)win;
		TileIndex tile = w->window_number;
		byte vehtype = w->type;
		DoCommandP(tile, vehtype, 0, CMD_DEPOT_SELL_ALL_VEHICLES);
	}
}

/** Opens a depot window
 * @param tile The tile where the depot/hangar is located
 * @param type The type of vehicles in the depot
 */
void ShowDepotWindow(TileIndex tile, VehicleType type)
{
	if (BringWindowToFrontById(WC_VEHICLE_DEPOT, tile) != NULL) return;

	const WindowDesc *desc;
	switch (type) {
		default: NOT_REACHED();
		case VEH_TRAIN:    desc = &_train_depot_desc;    break;
		case VEH_ROAD:     desc = &_road_depot_desc;     break;
		case VEH_SHIP:     desc = &_ship_depot_desc;     break;
		case VEH_AIRCRAFT: desc = &_aircraft_depot_desc; break;
	}

	new DepotWindow(desc, tile, type);
}

/** Removes the highlight of a vehicle in a depot window
 * @param *v Vehicle to remove all highlights from
 */
void DeleteDepotHighlightOfVehicle(const Vehicle *v)
{
	DepotWindow *w;

	/* If we haven't got any vehicles on the mouse pointer, we haven't got any highlighted in any depots either
	 * If that is the case, we can skip looping though the windows and save time
	 */
	if (_special_mouse_mode != WSM_DRAGDROP) return;

	w = dynamic_cast<DepotWindow*>(FindWindowById(WC_VEHICLE_DEPOT, v->tile));
	if (w != NULL) {
		if (w->sel == v->index) ResetObjectToPlace();
	}
}
