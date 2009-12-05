/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file cargotype.h Types/functions related to cargos. */

#ifndef CARGOTYPE_H
#define CARGOTYPE_H

#include "economy_type.h"
#include "cargo_type.h"
#include "gfx_type.h"
#include "strings_type.h"
#include "landscape_type.h"

typedef uint32 CargoLabel;

enum TownEffect {
	TE_NONE,
	TE_PASSENGERS,
	TE_MAIL,
	TE_GOODS,
	TE_WATER,
	TE_FOOD,
};

enum CargoClass {
	CC_NOAVAILABLE  = 0,       ///< No cargo class has been specified
	CC_PASSENGERS   = 1 <<  0, ///< Passengers
	CC_MAIL         = 1 <<  1, ///< Mail
	CC_EXPRESS      = 1 <<  2, ///< Express cargo (Goods, Food, Candy, but also possible for passengers)
	CC_ARMOURED     = 1 <<  3, ///< Armoured cargo (Valuables, Gold, Diamonds)
	CC_BULK         = 1 <<  4, ///< Bulk cargo (Coal, Grain etc., Ores, Fruit)
	CC_PIECE_GOODS  = 1 <<  5, ///< Piece goods (Livestock, Wood, Steel, Paper)
	CC_LIQUID       = 1 <<  6, ///< Liquids (Oil, Water, Rubber)
	CC_REFRIGERATED = 1 <<  7, ///< Refrigerated cargo (Food, Fruit)
	CC_HAZARDOUS    = 1 <<  8, ///< Hazardous cargo (Nuclear Fuel, Explosives, etc.)
	CC_COVERED      = 1 <<  9, ///< Covered/Sheltered Freight (Transporation in Box Vans, Silo Wagons, etc.)
	CC_SPECIAL      = 1 << 15  ///< Special bit used for livery refit tricks instead of normal cargoes.
};

static const byte INVALID_CARGO = 0xFF;

struct CargoSpec {
	uint8 bitnum;
	CargoLabel label;
	uint8 legend_colour;
	uint8 rating_colour;
	uint8 weight;
	uint16 initial_payment;
	uint8 transit_days[2];

	bool is_freight;
	TownEffect town_effect; ///< The effect this cargo type has on towns
	uint16 multipliertowngrowth;
	uint8 callback_mask;         ///< Bitmask of cargo callbacks that have to be called

	StringID name;
	StringID name_single;
	StringID units_volume;
	StringID quantifier;
	StringID abbrev;

	SpriteID sprite;

	uint16 classes;
	const struct GRFFile *grffile;   ///< NewGRF where 'group' belongs to
	const struct SpriteGroup *group;

	Money current_payment;

	/**
	 * Determines index of this cargospec
	 * @return index (in the CargoSpec::array array)
	 */
	FORCEINLINE CargoID Index() const
	{
		return this - CargoSpec::array;
	}

	/**
	 * Tests for validity of this cargospec
	 * @return is this cargospec valid?
	 * @note assert(cs->IsValid()) can be triggered when GRF config is modified
	 */
	FORCEINLINE bool IsValid() const
	{
		return this->bitnum != INVALID_CARGO;
	}

	/**
	 * Total number of cargospecs, both valid and invalid
	 * @return length of CargoSpec::array
	 */
	static FORCEINLINE size_t GetArraySize()
	{
		return lengthof(CargoSpec::array);
	}

	/**
	 * Retrieve cargo details for the given cargo ID
	 * @param index ID of cargo
	 * @pre index is a valid cargo ID
	 */
	static FORCEINLINE CargoSpec *Get(size_t index)
	{
		assert(index < lengthof(CargoSpec::array));
		return &CargoSpec::array[index];
	}

	SpriteID GetCargoIcon() const;

private:
	static CargoSpec array[NUM_CARGO]; ///< Array holding all CargoSpecs

	friend void SetupCargoForClimate(LandscapeID l);
};

extern uint32 _cargo_mask;

/* Set up the default cargo types for the given landscape type */
void SetupCargoForClimate(LandscapeID l);
/* Get the cargo ID with the cargo label */
CargoID GetCargoIDByLabel(CargoLabel cl);
CargoID GetCargoIDByBitnum(uint8 bitnum);

/* set up the cargos to be displayed in the smallmap's route legend */
void BuildLinkStatsLegend();

static inline bool IsCargoInClass(CargoID c, CargoClass cc)
{
	return (CargoSpec::Get(c)->classes & cc) != 0;
}

#define FOR_ALL_CARGOSPECS_FROM(var, start) for (size_t cargospec_index = start; var = NULL, cargospec_index < CargoSpec::GetArraySize(); cargospec_index++) \
		if ((var = CargoSpec::Get(cargospec_index))->IsValid())
#define FOR_ALL_CARGOSPECS(var) FOR_ALL_CARGOSPECS_FROM(var, 0)

#endif /* CARGOTYPE_H */
