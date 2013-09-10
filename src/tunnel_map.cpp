/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file tunnel_map.cpp Map accessors for tunnels. */

#include "stdafx.h"
#include "tunnelbridge_map.h"
#include "station_map.h"


/**
 * Gets the other end of the tunnel. Where a vehicle would reappear when it
 * enters at the given tile.
 * @param tile the tile to search from.
 * @return the tile of the other end of the tunnel.
 */
TileIndex GetOtherTunnelEnd(TileIndex tile)
{
	DiagDirection dir = GetTunnelBridgeDirection(tile);
	TileIndexDiff delta = TileOffsByDiagDir(dir);
	int z = GetTileZ(tile);

	dir = ReverseDiagDir(dir);
	do {
		tile += delta;
	} while (
		!IsTunnelTile(tile) ||
		GetTunnelBridgeDirection(tile) != dir ||
		GetTileZ(tile) != z
	);

	return tile;
}

/**
 * Helper function for under_water tunnel finding.
 * @param tile the tile to search from.
 * @param dir the direction to start searching to.
 * @return true if and only if there is a tunnel.
 */
bool IsTunnelInWayDirLevelZero(TileIndex tile, DiagDirection dir)
{
	TileIndexDiff delta = TileOffsByDiagDir(dir);
	int height;
	do {
		tile -= delta;
		if (!IsValidTile(tile)) return false;
		height = GetTileZ(tile);
	} while (height > 0);

	return IsTunnelTile(tile) && GetTunnelBridgeDirection(tile) == dir;
}

/**
 * Is there a tunnel in the way in the given direction?
 * @param tile the tile to search from.
 * @param z    the 'z' to search on.
 * @param dir  the direction to start searching to.
 * @return true if and only if there is a tunnel.
 */
bool IsTunnelInWayDir(TileIndex tile, int z, DiagDirection dir)
{
	TileIndexDiff delta = TileOffsByDiagDir(dir);
	int height;

	do {
		tile -= delta;
		if (!IsValidTile(tile)) return false;
		height = GetTileZ(tile);
	} while (z < height ||
			(height == 0 &&
			GetTileMaxZ(tile) == 0 &&	// Flat tile.
			(IsTileType(tile, MP_WATER) || IsBuoyTile(tile))));

	if (z == height) {
		if (IsTunnelTile(tile)) return GetTunnelBridgeDirection(tile) == dir;

		if (height == 0) {
			switch (GetTileSlope(tile, NULL)) {
				case SLOPE_NE: if (dir == DIAGDIR_NE || dir == DIAGDIR_SW) return IsTunnelInWayDirLevelZero(tile, DIAGDIR_SW); break;
				case SLOPE_SE: if (dir == DIAGDIR_SE || dir == DIAGDIR_NW) return IsTunnelInWayDirLevelZero(tile, DIAGDIR_NW); break;
				case SLOPE_SW: if (dir == DIAGDIR_SW || dir == DIAGDIR_NE) return IsTunnelInWayDirLevelZero(tile, DIAGDIR_NE); break;
				case SLOPE_NW: if (dir == DIAGDIR_NW || dir == DIAGDIR_SE) return IsTunnelInWayDirLevelZero(tile, DIAGDIR_SE); break;
				default: return false;
			}
		}
	}
	return false;
}

/**
 * Is there a tunnel in the way in any direction?
 * @param tile the tile to search from.
 * @param z the 'z' to search on.
 * @return true if and only if there is a tunnel.
 */
bool IsTunnelInWay(TileIndex tile, int z)
{
	return IsTunnelInWayDir(tile, z, (TileX(tile) > (MapMaxX() / 2)) ? DIAGDIR_NE : DIAGDIR_SW) ||
			IsTunnelInWayDir(tile, z, (TileY(tile) > (MapMaxY() / 2)) ? DIAGDIR_NW : DIAGDIR_SE);
}
