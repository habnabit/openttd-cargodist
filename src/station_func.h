/* $Id$ */

/** @file station_func.h Functions related to stations. */

#ifndef STATION_FUNC_H
#define STATION_FUNC_H

#include "station_type.h"
#include "sprite.h"
#include "rail_type.h"
#include "road_type.h"
#include "tile_type.h"
#include "cargo_type.h"
#include "vehicle_type.h"
#include "core/smallvec_type.hpp"

void ModifyStationRatingAround(TileIndex tile, Owner owner, int amount, uint radius);

typedef SmallVector<Station*, 1> StationList;
void FindStationsAroundTiles(TileIndex tile, int w_prod, int h_prod, StationList *stations);

void ShowStationViewWindow(StationID station);
void UpdateAllStationVirtCoord();

CargoArray GetProductionAroundTiles(TileIndex tile, int w, int h, int rad);
CargoArray GetAcceptanceAroundTiles(TileIndex tile, int w, int h, int rad);

const DrawTileSprites *GetStationTileLayout(StationType st, byte gfx);
void StationPickerDrawSprite(int x, int y, StationType st, RailType railtype, RoadType roadtype, int image);

bool HasStationInUse(StationID station, CompanyID company);

RoadStop *GetRoadStopByTile(TileIndex tile, RoadStopType type);
uint GetNumRoadStops(const Station *st, RoadStopType type);

void ClearSlot(struct RoadVehicle *v);

void DeleteOilRig(TileIndex t);

/* Check if a rail station tile is traversable. */
bool IsStationTileBlocked(TileIndex tile);

/* Check if a rail station tile is electrifiable. */
bool IsStationTileElectrifiable(TileIndex tile);

void UpdateAirportsNoise();

void DecreaseFrozen(Station *st, Vehicle *v, StationID next_station_id);

void IncreaseFrozen(Station *st, Vehicle *v, StationID next_station_id);

void RecalcFrozen(Station * st);

void IncreaseStats(Station *st, Vehicle *v, StationID next_station_id);

void UpdateFlows(Station * st, Vehicle *v, StationID next_station_id);

void DeleteStaleFlows(StationID at, CargoID c_id, StationID to);

#endif /* STATION_FUNC_H */
