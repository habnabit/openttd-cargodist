/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file economy_sl.cpp Code handling saving and loading of economy data */

#include "../stdafx.h"
#include "../economy_func.h"
#include "../economy_base.h"

#include "saveload.h"

/** Prices */
static void SaveLoad_PRIC()
{
	int vt = CheckSavegameVersion(65) ? (SLE_FILE_I32 | SLE_VAR_I64) : SLE_INT64;
	SlArray(&_price,      NUM_PRICES, vt);
	SlArray(&_price_frac, NUM_PRICES, SLE_UINT16);
}

/** Cargo payment rates */
static void SaveLoad_CAPR()
{
	uint num_cargo = CheckSavegameVersion(55) ? 12 : NUM_CARGO;
	int vt = CheckSavegameVersion(65) ? (SLE_FILE_I32 | SLE_VAR_I64) : SLE_INT64;
	SlArray(&_cargo_payment_rates,      num_cargo, vt);
	SlArray(&_cargo_payment_rates_frac, num_cargo, SLE_UINT16);
}

static const SaveLoad _economy_desc[] = {
	SLE_CONDVAR(Economy, max_loan,                      SLE_FILE_I32 | SLE_VAR_I64,  0, 64),
	SLE_CONDVAR(Economy, max_loan,                      SLE_INT64,                  65, SL_MAX_VERSION),
	SLE_CONDVAR(Economy, max_loan_unround,              SLE_FILE_I32 | SLE_VAR_I64,  0, 64),
	SLE_CONDVAR(Economy, max_loan_unround,              SLE_INT64,                  65, SL_MAX_VERSION),
	SLE_CONDVAR(Economy, max_loan_unround_fract,        SLE_UINT16,                 70, SL_MAX_VERSION),
	    SLE_VAR(Economy, fluct,                         SLE_INT16),
	    SLE_VAR(Economy, interest_rate,                 SLE_UINT8),
	    SLE_VAR(Economy, infl_amount,                   SLE_UINT8),
	    SLE_VAR(Economy, infl_amount_pr,                SLE_UINT8),
	SLE_CONDVAR(Economy, industry_daily_change_counter, SLE_UINT32,                102, SL_MAX_VERSION),
	    SLE_END()
};

/** Economy variables */
static void Save_ECMY()
{
	SlObject(&_economy, _economy_desc);
}

/** Economy variables */
static void Load_ECMY()
{
	SlObject(&_economy, _economy_desc);
	StartupIndustryDailyChanges(CheckSavegameVersion(102));  // old savegames will need to be initialized
}

static const SaveLoad _cargopayment_desc[] = {
	SLE_REF(CargoPayment, front,         REF_VEHICLE),
	SLE_VAR(CargoPayment, route_profit,  SLE_INT64),
	SLE_VAR(CargoPayment, visual_profit, SLE_INT64),
//	SLEG_CONDVAR(CargoPayment, visual_transfer,  SLE_INT64, <some version>, SL_MAX_VERSION),
	SLE_END()
};

static void Save_CAPY()
{
	CargoPayment *cp;
	FOR_ALL_CARGO_PAYMENTS(cp) {
		SlSetArrayIndex(cp->index);
		SlObject(cp, _cargopayment_desc);
	}
}

static void Load_CAPY()
{
	int index;

	while ((index = SlIterateArray()) != -1) {
		CargoPayment *cp = new (index) CargoPayment();
		SlObject(cp, _cargopayment_desc);
	}
}

static void Ptrs_CAPY()
{
	CargoPayment *cp;
	FOR_ALL_CARGO_PAYMENTS(cp) {
		SlObject(cp, _cargopayment_desc);
	}
}


extern const ChunkHandler _economy_chunk_handlers[] = {
	{ 'CAPY', Save_CAPY,     Load_CAPY,     Ptrs_CAPY, CH_ARRAY},
	{ 'PRIC', SaveLoad_PRIC, SaveLoad_PRIC, NULL,      CH_RIFF | CH_AUTO_LENGTH},
	{ 'CAPR', SaveLoad_CAPR, SaveLoad_CAPR, NULL,      CH_RIFF | CH_AUTO_LENGTH},
	{ 'ECMY', Save_ECMY,     Load_ECMY,     NULL,      CH_RIFF | CH_LAST},
};
