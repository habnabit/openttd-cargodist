/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file autoreplace_gui.cpp GUI for autoreplace handling. */

#include "stdafx.h"
#include "command_func.h"
#include "vehicle_gui.h"
#include "newgrf_engine.h"
#include "group.h"
#include "rail.h"
#include "strings_func.h"
#include "window_func.h"
#include "autoreplace_func.h"
#include "gfx_func.h"
#include "company_func.h"
#include "widgets/dropdown_type.h"
#include "engine_base.h"
#include "window_gui.h"
#include "engine_gui.h"
#include "settings_func.h"

#include "table/strings.h"

void DrawEngineList(VehicleType type, int x, int r, int y, const GUIEngineList *eng_list, uint16 min, uint16 max, EngineID selected_id, int count_location, GroupID selected_group);

/** Widget numbers of the autoreplace GUI. */
enum ReplaceVehicleWindowWidgets {
	RVW_WIDGET_CLOSEBOX,
	RVW_WIDGET_CAPTION,
	RVW_WIDGET_STICKY,

	/* Left and right matrix + details. */
	RVW_WIDGET_LEFT_MATRIX,
	RVW_WIDGET_LEFT_SCROLLBAR,
	RVW_WIDGET_RIGHT_MATRIX,
	RVW_WIDGET_RIGHT_SCROLLBAR,
	RVW_WIDGET_LEFT_DETAILS,
	RVW_WIDGET_RIGHT_DETAILS,

	/* Button row. */
	RVW_WIDGET_START_REPLACE,
	RVW_WIDGET_INFO_TAB,
	RVW_WIDGET_STOP_REPLACE,
	RVW_WIDGET_RESIZE,

	/* Train only widgets. */
	RVW_WIDGET_TRAIN_ENGINEWAGON_TOGGLE,
	RVW_WIDGET_TRAIN_FLUFF_LEFT,
	RVW_WIDGET_TRAIN_RAILTYPE_DROPDOWN,
	RVW_WIDGET_TRAIN_FLUFF_RIGHT,
	RVW_WIDGET_TRAIN_WAGONREMOVE_TOGGLE,
};

static int CDECL EngineNumberSorter(const EngineID *a, const EngineID *b)
{
	int r = ListPositionOfEngine(*a) - ListPositionOfEngine(*b);

	return r;
}

/** Rebuild the left autoreplace list if an engine is removed or added
 * @param e Engine to check if it is removed or added
 * @param id_g The group the engine belongs to
 *  Note: this function only works if it is called either
 *   - when a new vehicle is build, but before it's counted in num_engines
 *   - when a vehicle is deleted and after it's substracted from num_engines
 *   - when not changing the count (used when changing replace orders)
 */
void InvalidateAutoreplaceWindow(EngineID e, GroupID id_g)
{
	Company *c = Company::Get(_local_company);
	uint num_engines = GetGroupNumEngines(_local_company, id_g, e);

	if (num_engines == 0 || c->num_engines[e] == 0) {
		/* We don't have any of this engine type.
		 * Either we just sold the last one, we build a new one or we stopped replacing it.
		 * In all cases, we need to update the left list */
		InvalidateWindowData(WC_REPLACE_VEHICLE, Engine::Get(e)->type, true);
	}
}

/** When an engine is made buildable or is removed from being buildable, add/remove it from the build/autoreplace lists
 * @param type The type of engine
 */
void AddRemoveEngineFromAutoreplaceAndBuildWindows(VehicleType type)
{
	InvalidateWindowData(WC_REPLACE_VEHICLE, type, false); // Update the autoreplace window
	InvalidateWindowClassesData(WC_BUILD_VEHICLE); // The build windows needs updating as well
}

/**
 * Window for the autoreplacing of vehicles.
 */
class ReplaceVehicleWindow : public Window {
	byte sel_index[2];
	EngineID sel_engine[2];
	uint16 count[2];
	bool wagon_btnstate; ///< true means engine is selected
	GUIEngineList list[2];
	bool update_left;
	bool update_right;
	bool init_lists;
	GroupID sel_group;
	static RailType sel_railtype;

	/** Figure out if an engine should be added to a list.
	 * @param e            The EngineID.
	 * @param draw_left    If \c true, the left list is drawn (the engines specific to the railtype you selected).
	 * @param show_engines If \c true, the locomotives are drawn, else the wagons are drawn (never both).
	 * @return \c true if the engine should be in the list (based on this check), else \c false.
	 */
	bool GenerateReplaceRailList(EngineID e, bool draw_left, bool show_engines)
	{
		const RailVehicleInfo *rvi = RailVehInfo(e);

		/* Ensure that the wagon/engine selection fits the engine. */
		if ((rvi->railveh_type == RAILVEH_WAGON) == show_engines) return false;

		if (draw_left && show_engines) {
			/* Ensure that the railtype is specific to the selected one */
			if (rvi->railtype != this->sel_railtype) return false;
		}
		return true;
	}


	/** Generate a list
	 * @param w Window, that contains the list
	 * @param draw_left true if generating the left list, otherwise false
	 */
	void GenerateReplaceVehList(Window *w, bool draw_left)
	{
		EngineID selected_engine = INVALID_ENGINE;
		VehicleType type = (VehicleType)this->window_number;
		byte i = draw_left ? 0 : 1;

		GUIEngineList *list = &this->list[i];
		list->Clear();

		const Engine *e;
		FOR_ALL_ENGINES_OF_TYPE(e, type) {
			EngineID eid = e->index;
			if (type == VEH_TRAIN && !GenerateReplaceRailList(eid, draw_left, this->wagon_btnstate)) continue; // special rules for trains

			if (draw_left) {
				const GroupID selected_group = this->sel_group;
				const uint num_engines = GetGroupNumEngines(_local_company, selected_group, eid);

				/* Skip drawing the engines we don't have any of and haven't set for replacement */
				if (num_engines == 0 && EngineReplacementForCompany(Company::Get(_local_company), eid, selected_group) == INVALID_ENGINE) continue;
			} else {
				if (!CheckAutoreplaceValidity(this->sel_engine[0], eid, _local_company)) continue;
			}

			*list->Append() = eid;
			if (eid == this->sel_engine[i]) selected_engine = eid; // The selected engine is still in the list
		}
		this->sel_engine[i] = selected_engine; // update which engine we selected (the same or none, if it's not in the list anymore)
		EngList_Sort(list, &EngineNumberSorter);
	}

	/** Generate the lists */
	void GenerateLists()
	{
		EngineID e = this->sel_engine[0];

		if (this->update_left == true) {
			/* We need to rebuild the left list */
			GenerateReplaceVehList(this, true);
			this->vscroll.SetCount(this->list[0].Length());
			if (this->init_lists && this->sel_engine[0] == INVALID_ENGINE && this->list[0].Length() != 0) {
				this->sel_engine[0] = this->list[0][0];
			}
		}

		if (this->update_right || e != this->sel_engine[0]) {
			/* Either we got a request to rebuild the right list or the left list selected a different engine */
			if (this->sel_engine[0] == INVALID_ENGINE) {
				/* Always empty the right list when nothing is selected in the left list */
				this->list[1].Clear();
				this->sel_engine[1] = INVALID_ENGINE;
			} else {
				GenerateReplaceVehList(this, false);
				this->vscroll2.SetCount(this->list[1].Length());
				if (this->init_lists && this->sel_engine[1] == INVALID_ENGINE && this->list[1].Length() != 0) {
					this->sel_engine[1] = this->list[1][0];
				}
			}
		}
		/* Reset the flags about needed updates */
		this->update_left  = false;
		this->update_right = false;
		this->init_lists   = false;
	}

public:
	ReplaceVehicleWindow(const WindowDesc *desc, VehicleType vehicletype, GroupID id_g) : Window(desc, vehicletype)
	{
		this->wagon_btnstate = true; // start with locomotives (all other vehicles will not read this bool)
		this->update_left   = true;
		this->update_right  = true;
		this->init_lists    = true;
		this->sel_engine[0] = INVALID_ENGINE;
		this->sel_engine[1] = INVALID_ENGINE;

		this->resize.step_height = GetVehicleListHeight(vehicletype);
		this->vscroll.SetCapacity(this->resize.step_height == 14 ? 8 : 4);

		Widget *widget = this->widget;
		widget[RVW_WIDGET_LEFT_MATRIX].data = widget[RVW_WIDGET_RIGHT_MATRIX].data = (this->vscroll.GetCapacity() << MAT_ROW_START) + (1 << MAT_COL_START);

		if (vehicletype != VEH_TRAIN) {
			/* Since it's not a train we will hide the train only widgets. */
			this->SetWidgetsHiddenState(true,
									RVW_WIDGET_TRAIN_ENGINEWAGON_TOGGLE,
									RVW_WIDGET_TRAIN_FLUFF_LEFT,
									RVW_WIDGET_TRAIN_RAILTYPE_DROPDOWN,
									RVW_WIDGET_TRAIN_FLUFF_RIGHT,
									RVW_WIDGET_TRAIN_WAGONREMOVE_TOGGLE,
									WIDGET_LIST_END);
		}

		ResizeWindow(this, 0, this->resize.step_height * this->vscroll.GetCapacity());

		/* Set the minimum window size to the current window size */
		this->resize.width  = this->width;
		this->resize.height = this->height;

		this->owner = _local_company;
		this->sel_group = id_g;
		this->vscroll2.SetCapacity(this->vscroll.GetCapacity());   // these two are always the same

		this->FindWindowPlacementAndResize(desc);
	}

	virtual void OnPaint()
	{
		if (this->update_left || this->update_right) this->GenerateLists();

		Company *c = Company::Get(_local_company);
		EngineID selected_id[2];
		const GroupID selected_group = this->sel_group;

		selected_id[0] = this->sel_engine[0];
		selected_id[1] = this->sel_engine[1];

		/* Disable the "Start Replacing" button if:
		 *    Either list is empty
		 * or The selected replacement engine has a replacement (to prevent loops)
		 * or The right list (new replacement) has the existing replacement vehicle selected */
		this->SetWidgetDisabledState(RVW_WIDGET_START_REPLACE,
										selected_id[0] == INVALID_ENGINE ||
										selected_id[1] == INVALID_ENGINE ||
										EngineReplacementForCompany(c, selected_id[1], selected_group) != INVALID_ENGINE ||
										EngineReplacementForCompany(c, selected_id[0], selected_group) == selected_id[1]);

		/* Disable the "Stop Replacing" button if:
		 *   The left list (existing vehicle) is empty
		 *   or The selected vehicle has no replacement set up */
		this->SetWidgetDisabledState(RVW_WIDGET_STOP_REPLACE,
										selected_id[0] == INVALID_ENGINE ||
										!EngineHasReplacementForCompany(c, selected_id[0], selected_group));

		/* now the actual drawing of the window itself takes place */
		SetDParam(0, STR_REPLACE_VEHICLE_TRAIN + this->window_number);

		if (this->window_number == VEH_TRAIN) {
			/* set on/off for renew_keep_length */
			SetDParam(1, c->settings.renew_keep_length ? STR_CONFIG_SETTING_ON : STR_CONFIG_SETTING_OFF);

			/* set wagon/engine button */
			SetDParam(2, this->wagon_btnstate ? STR_REPLACE_ENGINES : STR_REPLACE_WAGONS);

			/* sets the colour of that art thing */
			this->widget[RVW_WIDGET_TRAIN_FLUFF_LEFT].colour  = _company_colours[_local_company];
			this->widget[RVW_WIDGET_TRAIN_FLUFF_RIGHT].colour = _company_colours[_local_company];
		}

		if (this->window_number == VEH_TRAIN) {
			/* Show the selected railtype in the pulldown menu */
			const RailtypeInfo *rti = GetRailTypeInfo(sel_railtype);
			this->widget[RVW_WIDGET_TRAIN_RAILTYPE_DROPDOWN].data = rti->strings.replace_text;
		}

		this->DrawWidgets();

		/* sets up the string for the vehicle that is being replaced to */
		if (selected_id[0] != INVALID_ENGINE) {
			if (!EngineHasReplacementForCompany(c, selected_id[0], selected_group)) {
				SetDParam(0, STR_REPLACE_NOT_REPLACING);
			} else {
				SetDParam(0, STR_ENGINE_NAME);
				SetDParam(1, EngineReplacementForCompany(c, selected_id[0], selected_group));
			}
		} else {
			SetDParam(0, STR_REPLACE_NOT_REPLACING_VEHICLE_SELECTED);
		}

		DrawString(this->widget[RVW_WIDGET_INFO_TAB].left + 6, this->widget[RVW_WIDGET_INFO_TAB].right - 6, this->widget[RVW_WIDGET_INFO_TAB].top + 1, STR_BLACK_STRING);

		/* Draw the lists */
		for (byte i = 0; i < 2; i++) {
			uint widget     = (i == 0) ? RVW_WIDGET_LEFT_MATRIX : RVW_WIDGET_RIGHT_MATRIX;
			GUIEngineList *list = &this->list[i]; // which list to draw
			EngineID start  = i == 0 ? this->vscroll.GetPosition() : this->vscroll2.GetPosition(); // what is the offset for the start (scrolling)
			EngineID end    = min((i == 0 ? this->vscroll.GetCapacity() : this->vscroll2.GetCapacity()) + start, list->Length());

			/* Do the actual drawing */
			DrawEngineList((VehicleType)this->window_number, this->widget[widget].left + 2, this->widget[widget].right, this->widget[widget].top + 1, list, start, end, this->sel_engine[i], i == 0 ? this->widget[RVW_WIDGET_LEFT_MATRIX].right - 2 : 0, selected_group);

			/* Also draw the details if an engine is selected */
			if (this->sel_engine[i] != INVALID_ENGINE) {
				const Widget *wi = &this->widget[i == 0 ? RVW_WIDGET_LEFT_DETAILS : RVW_WIDGET_RIGHT_DETAILS];
				int text_end = DrawVehiclePurchaseInfo(wi->left + 2, wi->right - 2, wi->top + 1, this->sel_engine[i]);

				if (text_end > wi->bottom) {
					this->SetDirty();
					ResizeWindowForWidget(this, i == 0 ? RVW_WIDGET_LEFT_DETAILS : RVW_WIDGET_RIGHT_DETAILS, 0, text_end - wi->bottom);
					this->SetDirty();
				}
			}
		}
	}

	virtual void OnClick(Point pt, int widget)
	{
		switch (widget) {
			case RVW_WIDGET_TRAIN_ENGINEWAGON_TOGGLE:
				this->wagon_btnstate = !(this->wagon_btnstate);
				this->update_left = true;
				this->init_lists  = true;
				this->SetDirty();
				break;

			case RVW_WIDGET_TRAIN_RAILTYPE_DROPDOWN: { // Railtype selection dropdown menu
				const Company *c = Company::Get(_local_company);
				DropDownList *list = new DropDownList();
				for (RailType rt = RAILTYPE_BEGIN; rt != RAILTYPE_END; rt++) {
					const RailtypeInfo *rti = GetRailTypeInfo(rt);
					/* Skip rail type if it has no label */
					if (rti->label == 0) continue;
					list->push_back(new DropDownListStringItem(rti->strings.replace_text, rt, !HasBit(c->avail_railtypes, rt)));
				}
				ShowDropDownList(this, list, sel_railtype, RVW_WIDGET_TRAIN_RAILTYPE_DROPDOWN);
				break;
			}

			case RVW_WIDGET_TRAIN_WAGONREMOVE_TOGGLE: // toggle renew_keep_length
				DoCommandP(0, GetCompanySettingIndex("company.renew_keep_length"), Company::Get(_local_company)->settings.renew_keep_length ? 0 : 1, CMD_CHANGE_COMPANY_SETTING);
				break;

			case RVW_WIDGET_START_REPLACE: { // Start replacing
				EngineID veh_from = this->sel_engine[0];
				EngineID veh_to = this->sel_engine[1];
				DoCommandP(0, this->sel_group << 16, veh_from + (veh_to << 16), CMD_SET_AUTOREPLACE);
				this->SetDirty();
			} break;

			case RVW_WIDGET_STOP_REPLACE: { // Stop replacing
				EngineID veh_from = this->sel_engine[0];
				DoCommandP(0, this->sel_group << 16, veh_from + (INVALID_ENGINE << 16), CMD_SET_AUTOREPLACE);
				this->SetDirty();
			} break;

			case RVW_WIDGET_LEFT_MATRIX:
			case RVW_WIDGET_RIGHT_MATRIX: {
				uint i = (pt.y - 14) / this->resize.step_height;
				uint16 click_scroll_pos = widget == RVW_WIDGET_LEFT_MATRIX ? this->vscroll.GetPosition() : this->vscroll2.GetPosition();
				uint16 click_scroll_cap = widget == RVW_WIDGET_LEFT_MATRIX ? this->vscroll.GetCapacity() : this->vscroll2.GetCapacity();
				byte click_side         = widget == RVW_WIDGET_LEFT_MATRIX ? 0 : 1;
				size_t engine_count     = this->list[click_side].Length();

				if (i < click_scroll_cap) {
					i += click_scroll_pos;
					EngineID e = engine_count > i ? this->list[click_side][i] : INVALID_ENGINE;
					if (e == this->sel_engine[click_side]) break; // we clicked the one we already selected
					this->sel_engine[click_side] = e;
					if (click_side == 0) {
						this->update_right = true;
						this->init_lists   = true;
					}
					this->SetDirty();
					}
				break;
				}
		}
	}

	virtual void OnDropdownSelect(int widget, int index)
	{
		RailType temp = (RailType)index;
		if (temp == sel_railtype) return; // we didn't select a new one. No need to change anything
		sel_railtype = temp;
		/* Reset scrollbar positions */
		this->vscroll.SetPosition(0);
		this->vscroll2.SetPosition(0);
		/* Rebuild the lists */
		this->update_left  = true;
		this->update_right = true;
		this->init_lists   = true;
		this->SetDirty();
	}

	virtual void OnResize(Point delta)
	{
		this->vscroll.UpdateCapacity(delta.y / (int)this->resize.step_height);
		this->vscroll2.UpdateCapacity(delta.y / (int)this->resize.step_height);

		Widget *widget = this->widget;

		widget[RVW_WIDGET_LEFT_MATRIX].data = widget[RVW_WIDGET_RIGHT_MATRIX].data = (this->vscroll.GetCapacity() << MAT_ROW_START) + (1 << MAT_COL_START);

		if (delta.x != 0) {
			/* We changed the width of the window so we have to resize the lists.
			 * Because ResizeButtons() makes each widget the same size it can't be used on the lists
			 * because then the lists would have the same size as the scrollbars.
			 * Instead we use it on the detail panels.
			 * Afterwards we use the new location of the detail panels (the middle of the window)
			 * to place the lists.
			 * This way the lists will have equal size while keeping the width of the scrollbars unchanged. */
			ResizeButtons(this, RVW_WIDGET_LEFT_DETAILS, RVW_WIDGET_RIGHT_DETAILS);
			widget[RVW_WIDGET_RIGHT_MATRIX].left    = widget[RVW_WIDGET_RIGHT_DETAILS].left;
			widget[RVW_WIDGET_LEFT_SCROLLBAR].right = widget[RVW_WIDGET_LEFT_DETAILS].right;
			widget[RVW_WIDGET_LEFT_SCROLLBAR].left  = widget[RVW_WIDGET_LEFT_SCROLLBAR].right - 11;
			widget[RVW_WIDGET_LEFT_MATRIX].right    = widget[RVW_WIDGET_LEFT_SCROLLBAR].left - 1;
		}
	}

	virtual void OnInvalidateData(int data)
	{
		if (data != 0) {
			this->update_left = true;
		} else {
			this->update_right = true;
		}
	}
};

static const Widget _replace_vehicle_widgets[] = {
{   WWT_CLOSEBOX,   RESIZE_NONE,  COLOUR_GREY,     0,    10,     0,    13, STR_BLACK_CROSS,                 STR_TOOLTIP_CLOSE_WINDOW},             // RVW_WIDGET_CLOSEBOX
{    WWT_CAPTION,  RESIZE_RIGHT,  COLOUR_GREY,    11,   443,     0,    13, STR_REPLACE_VEHICLES_WHITE,      STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS},   // RVW_WIDGET_CAPTION
{  WWT_STICKYBOX,     RESIZE_LR,  COLOUR_GREY,   444,   455,     0,    13, STR_NULL,                        STR_TOOLTIP_STICKY},                    // RVW_WIDGET_STICKY

{     WWT_MATRIX, RESIZE_BOTTOM,  COLOUR_GREY,     0,   215,    14,    13, 0x1,                             STR_REPLACE_HELP_LEFT_ARRAY},          // RVW_WIDGET_LEFT_MATRIX
{  WWT_SCROLLBAR, RESIZE_BOTTOM,  COLOUR_GREY,   216,   227,    14,    13, STR_NULL,                        STR_TOOLTIP_VSCROLL_BAR_SCROLLS_LIST}, // RVW_WIDGET_LEFT_SCROLLBAR
{     WWT_MATRIX,    RESIZE_LRB,  COLOUR_GREY,   228,   443,    14,    13, 0x1,                             STR_REPLACE_HELP_RIGHT_ARRAY},         // RVW_WIDGET_RIGHT_MATRIX
{ WWT_SCROLL2BAR,    RESIZE_LRB,  COLOUR_GREY,   444,   455,    14,    13, STR_NULL,                        STR_TOOLTIP_VSCROLL_BAR_SCROLLS_LIST}, // RVW_WIDGET_RIGHT_SCROLLBAR
{      WWT_PANEL,     RESIZE_TB,  COLOUR_GREY,     0,   227,    14,   105, 0x0,                             STR_NULL},                             // RVW_WIDGET_LEFT_DETAILS
{      WWT_PANEL,    RESIZE_RTB,  COLOUR_GREY,   228,   455,    14,   105, 0x0,                             STR_NULL},                             // RVW_WIDGET_RIGHT_DETAILS

{ WWT_PUSHTXTBTN,     RESIZE_TB,  COLOUR_GREY,     0,   138,   106,   117, STR_REPLACE_VEHICLES_START,      STR_REPLACE_HELP_START_BUTTON},        // RVW_WIDGET_START_REPLACE
{      WWT_PANEL,    RESIZE_RTB,  COLOUR_GREY,   139,   305,   106,   117, 0x0,                             STR_REPLACE_HELP_REPLACE_INFO_TAB},    // RVW_WIDGET_INFO_TAB
{ WWT_PUSHTXTBTN,   RESIZE_LRTB,  COLOUR_GREY,   306,   443,   106,   117, STR_REPLACE_VEHICLES_STOP,       STR_REPLACE_HELP_STOP_BUTTON},         // RVW_WIDGET_STOP_REPLACE
{  WWT_RESIZEBOX,   RESIZE_LRTB,  COLOUR_GREY,   444,   455,   106,   117, STR_NULL,                        STR_TOOLTIP_RESIZE},                    // RVW_WIDGET_RESIZE

{ WWT_PUSHTXTBTN,     RESIZE_TB,  COLOUR_GREY,     0,   138,   128,   139, STR_REPLACE_ENGINE_WAGON_SELECT, STR_REPLACE_ENGINE_WAGON_SELECT_HELP}, // RVW_WIDGET_TRAIN_ENGINEWAGON_TOGGLE
{      WWT_PANEL,     RESIZE_TB,  COLOUR_GREY,   139,   153,   128,   139, 0x0,                             STR_NULL},                             // RVW_WIDGET_TRAIN_FLUFF_LEFT
{   WWT_DROPDOWN,    RESIZE_RTB,  COLOUR_GREY,   154,   289,   128,   139, 0x0,                             STR_REPLACE_HELP_RAILTYPE},            // RVW_WIDGET_TRAIN_RAILTYPE_DROPDOWN
{      WWT_PANEL,   RESIZE_LRTB,  COLOUR_GREY,   290,   305,   128,   139, 0x0,                             STR_NULL},                             // RVW_WIDGET_TRAIN_FLUFF_RIGHT
{ WWT_PUSHTXTBTN,   RESIZE_LRTB,  COLOUR_GREY,   306,   443,   128,   139, STR_REPLACE_REMOVE_WAGON,        STR_REPLACE_REMOVE_WAGON_HELP},        // RVW_WIDGET_TRAIN_WAGONREMOVE_TOGGLE
{   WIDGETS_END},
};

static const NWidgetPart _nested_replace_rail_vehicle_widgets[] = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_GREY, RVW_WIDGET_CLOSEBOX),
		NWidget(WWT_CAPTION, COLOUR_GREY, RVW_WIDGET_CAPTION), SetMinimalSize(433, 14), SetDataTip(STR_REPLACE_VEHICLES_WHITE, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS),
		NWidget(WWT_STICKYBOX, COLOUR_GREY, RVW_WIDGET_STICKY),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_MATRIX, COLOUR_GREY, RVW_WIDGET_LEFT_MATRIX), SetMinimalSize(216, 0), SetDataTip(0x1, STR_REPLACE_HELP_LEFT_ARRAY), SetResize(0, 1),
		NWidget(WWT_SCROLLBAR, COLOUR_GREY, RVW_WIDGET_LEFT_SCROLLBAR),
		NWidget(WWT_MATRIX, COLOUR_GREY, RVW_WIDGET_RIGHT_MATRIX), SetMinimalSize(216, 0), SetDataTip(0x1, STR_REPLACE_HELP_RIGHT_ARRAY), SetResize(1, 1),
		NWidget(WWT_SCROLL2BAR, COLOUR_GREY, RVW_WIDGET_RIGHT_SCROLLBAR),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_PANEL, COLOUR_GREY, RVW_WIDGET_LEFT_DETAILS), SetMinimalSize(228, 102),
		EndContainer(),
		NWidget(WWT_PANEL, COLOUR_GREY, RVW_WIDGET_RIGHT_DETAILS), SetMinimalSize(228, 102), SetResize(1, 0),
		EndContainer(),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, RVW_WIDGET_START_REPLACE), SetMinimalSize(139, 12), SetDataTip(STR_REPLACE_VEHICLES_START, STR_REPLACE_HELP_START_BUTTON),
		NWidget(WWT_PANEL, COLOUR_GREY, RVW_WIDGET_INFO_TAB), SetMinimalSize(167, 12), SetDataTip(0x0, STR_REPLACE_HELP_REPLACE_INFO_TAB), SetResize(1, 0),
		EndContainer(),
		NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, RVW_WIDGET_STOP_REPLACE), SetMinimalSize(150, 12), SetDataTip(STR_REPLACE_VEHICLES_STOP, STR_REPLACE_HELP_STOP_BUTTON),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, RVW_WIDGET_TRAIN_ENGINEWAGON_TOGGLE), SetMinimalSize(139, 12), SetDataTip(STR_REPLACE_ENGINE_WAGON_SELECT, STR_REPLACE_ENGINE_WAGON_SELECT_HELP),
		NWidget(WWT_PANEL, COLOUR_GREY, RVW_WIDGET_TRAIN_FLUFF_LEFT), SetMinimalSize(15, 12),
		EndContainer(),
		NWidget(WWT_DROPDOWN, COLOUR_GREY, RVW_WIDGET_TRAIN_RAILTYPE_DROPDOWN), SetMinimalSize(136, 12), SetDataTip(0x0, STR_REPLACE_HELP_RAILTYPE), SetResize(1, 0),
		NWidget(WWT_PANEL, COLOUR_GREY, RVW_WIDGET_TRAIN_FLUFF_RIGHT), SetMinimalSize(16, 12),
		EndContainer(),
		NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, RVW_WIDGET_TRAIN_WAGONREMOVE_TOGGLE), SetMinimalSize(138, 12), SetDataTip(STR_REPLACE_REMOVE_WAGON, STR_REPLACE_REMOVE_WAGON_HELP),
		NWidget(WWT_RESIZEBOX, COLOUR_GREY, RVW_WIDGET_RESIZE),
	EndContainer(),
};

static const WindowDesc _replace_rail_vehicle_desc(
	WDP_AUTO, WDP_AUTO, 456, 140, 456, 140,
	WC_REPLACE_VEHICLE, WC_NONE,
	WDF_STD_TOOLTIPS | WDF_STD_BTN | WDF_DEF_WIDGET | WDF_UNCLICK_BUTTONS | WDF_STICKY_BUTTON | WDF_RESIZABLE | WDF_CONSTRUCTION,
	_replace_vehicle_widgets, _nested_replace_rail_vehicle_widgets, lengthof(_nested_replace_rail_vehicle_widgets)
);

static const NWidgetPart _nested_replace_vehicle_widgets[] = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_GREY, RVW_WIDGET_CLOSEBOX),
		NWidget(WWT_CAPTION, COLOUR_GREY, RVW_WIDGET_CAPTION), SetMinimalSize(433, 14), SetDataTip(STR_REPLACE_VEHICLES_WHITE, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS),
		NWidget(WWT_STICKYBOX, COLOUR_GREY, RVW_WIDGET_STICKY),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_MATRIX, COLOUR_GREY, RVW_WIDGET_LEFT_MATRIX), SetMinimalSize(216, 0), SetDataTip(0x1, STR_REPLACE_HELP_LEFT_ARRAY), SetResize(0, 1),
		NWidget(WWT_SCROLLBAR, COLOUR_GREY, RVW_WIDGET_LEFT_SCROLLBAR),
		NWidget(WWT_MATRIX, COLOUR_GREY, RVW_WIDGET_RIGHT_MATRIX), SetMinimalSize(216, 0), SetDataTip(0x1, STR_REPLACE_HELP_RIGHT_ARRAY), SetResize(1, 1),
		NWidget(WWT_SCROLL2BAR, COLOUR_GREY, RVW_WIDGET_RIGHT_SCROLLBAR),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_PANEL, COLOUR_GREY, RVW_WIDGET_LEFT_DETAILS), SetMinimalSize(228, 92),
		EndContainer(),
		NWidget(WWT_PANEL, COLOUR_GREY, RVW_WIDGET_RIGHT_DETAILS), SetMinimalSize(228, 92), SetResize(1, 0),
		EndContainer(),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, RVW_WIDGET_START_REPLACE), SetMinimalSize(139, 12), SetDataTip(STR_REPLACE_VEHICLES_START, STR_REPLACE_HELP_START_BUTTON),
		NWidget(WWT_PANEL, COLOUR_GREY, RVW_WIDGET_INFO_TAB), SetMinimalSize(167, 12), SetDataTip(0x0, STR_REPLACE_HELP_REPLACE_INFO_TAB), SetResize(1, 0),
		EndContainer(),
		NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, RVW_WIDGET_STOP_REPLACE), SetMinimalSize(138, 12), SetDataTip(STR_REPLACE_VEHICLES_STOP, STR_REPLACE_HELP_STOP_BUTTON),
		NWidget(WWT_RESIZEBOX, COLOUR_GREY, RVW_WIDGET_RESIZE),
	EndContainer(),
	NWidget(WWT_EMPTY, COLOUR_GREY, RVW_WIDGET_TRAIN_ENGINEWAGON_TOGGLE), SetResize(1, 0),
	NWidget(WWT_EMPTY, COLOUR_GREY, RVW_WIDGET_TRAIN_FLUFF_LEFT), SetResize(1, 0),
	NWidget(WWT_EMPTY, COLOUR_GREY, RVW_WIDGET_TRAIN_RAILTYPE_DROPDOWN), SetResize(1, 0),
	NWidget(WWT_EMPTY, COLOUR_GREY, RVW_WIDGET_TRAIN_FLUFF_RIGHT), SetResize(1, 0),
	NWidget(WWT_EMPTY, COLOUR_GREY, RVW_WIDGET_TRAIN_WAGONREMOVE_TOGGLE), SetResize(1, 0),
};

static const WindowDesc _replace_vehicle_desc(
	WDP_AUTO, WDP_AUTO, 456, 118, 456, 118,
	WC_REPLACE_VEHICLE, WC_NONE,
	WDF_STD_TOOLTIPS | WDF_STD_BTN | WDF_DEF_WIDGET | WDF_UNCLICK_BUTTONS | WDF_STICKY_BUTTON | WDF_RESIZABLE | WDF_CONSTRUCTION,
	_replace_vehicle_widgets, _nested_replace_vehicle_widgets, lengthof(_nested_replace_vehicle_widgets)
);

RailType ReplaceVehicleWindow::sel_railtype = RAILTYPE_RAIL;

void ShowReplaceGroupVehicleWindow(GroupID id_g, VehicleType vehicletype)
{
	DeleteWindowById(WC_REPLACE_VEHICLE, vehicletype);
	new ReplaceVehicleWindow(vehicletype == VEH_TRAIN ? &_replace_rail_vehicle_desc : &_replace_vehicle_desc, vehicletype, id_g);
}
