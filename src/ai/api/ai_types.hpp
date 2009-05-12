/* $Id$ */

/** @file ai_types.hpp Defines all the types of the game, like IDs of various objects.
 *
 * IDs are used to identify certain objects. They are only unique within the object type, so for example a vehicle may have VehicleID 2009,
 * while a station has StationID 2009 at the same time. Also IDs are assigned arbitrary, you cannot assume them to be consecutive.
 * Also note, that some IDs are static and never change, while others are allocated dynamically and might be
 * reused for other objects once they are released. So be careful, which IDs you store for which purpose and whether they stay valid all the time.
 *
 * <table>
 * <tr><th>type        </th><th> object                                            </th>
 *                          <th> acquired                                          </th>
 *                          <th> released                                          </th>
 *                          <th> reused                                            </th></tr>
 * <tr><td>#BridgeID   </td><td> bridge type                                       </td>
 *                          <td> introduction \ref newgrf_changes "(1)"            </td>
 *                          <td> never \ref newgrf_changes "(1)"                   </td>
 *                          <td> no \ref newgrf_changes "(1)"                      </td></tr>
 * <tr><td>#CargoID    </td><td> cargo type                                        </td>
 *                          <td> game start \ref newgrf_changes "(1)"              </td>
 *                          <td> never \ref newgrf_changes "(1)"                   </td>
 *                          <td> no \ref newgrf_changes "(1)"                      </td></tr>
 * <tr><td>#EngineID   </td><td> engine type                                       </td>
 *                          <td> introduction, preview \ref dynamic_engines "(2)"  </td>
 *                          <td> engines retires \ref dynamic_engines "(2)"        </td>
 *                          <td> no \ref dynamic_engines "(2)"                     </td></tr>
 * <tr><td>#GroupID    </td><td> vehicle group                                     </td>
 *                          <td> creation                                          </td>
 *                          <td> deletion                                          </td>
 *                          <td> yes                                               </td></tr>
 * <tr><td>#IndustyID  </td><td> industry                                          </td>
 *                          <td> construction                                      </td>
 *                          <td> closure                                           </td>
 *                          <td> yes                                               </td></tr>
 * <tr><td>#IndustyType</td><td> industry type                                     </td>
 *                          <td> game start \ref newgrf_changes "(1)"              </td>
 *                          <td> never \ref newgrf_changes "(1)"                   </td>
 *                          <td> no                                                </td></tr>
 * <tr><td>#SignID     </td><td> sign                                              </td>
 *                          <td> construction                                      </td>
 *                          <td> deletion                                          </td>
 *                          <td> yes                                               </td></tr>
 * <tr><td>#StationID  </td><td> station                                           </td>
 *                          <td> construction                                      </td>
 *                          <td> expiration of 'grey' station sign after deletion  </td>
 *                          <td> yes                                               </td></tr>
 * <tr><td>#SubsidyID  </td><td> subsidy                                           </td>
 *                          <td> offer announcement                                </td>
 *                          <td> (offer) expiration                                </td>
 *                          <td> yes                                               </td></tr>
 * <tr><td>#TileIndex  </td><td> tile on map                                       </td>
 *                          <td> game start                                        </td>
 *                          <td> never                                             </td>
 *                          <td> no                                                </td></tr>
 * <tr><td>#TownID     </td><td> town                                              </td>
 *                          <td> game start                                        </td>
 *                          <td> never                                             </td>
 *                          <td> no                                                </td></tr>
 * <tr><td>#VehicleID  </td><td> vehicle                                           </td>
 *                          <td> construction, autorenew, autoreplace              </td>
 *                          <td> destruction, autorenew, autoreplace               </td>
 *                          <td> yes                                               </td></tr>
 * <tr><td>#WaypointID </td><td> waypoint                                          </td>
 *                          <td> construction                                      </td>
 *                          <td> destruction                                       </td>
 *                          <td> yes                                               </td></tr>
 * </table>
 *
 * @remarks
 *  \li \anchor newgrf_changes  (1) in-game changes of newgrfs may reassign/invalidate IDs (will also cause other trouble though).
 *  \li \anchor dynamic_engines (2) engine IDs are reassigned/invalidated on changing 'allow multiple newgrf engine sets' (only allowed as long as no vehicles are built).
 */

#ifndef AI_TYPES_HPP
#define AI_TYPES_HPP

#include "../../core/overflowsafe_type.hpp"
#include "../../company_type.h"
#include "../../script/fake_squirrel_types.hpp"

/* Define all types here, so we don't have to include the whole _type.h maze */
typedef uint BridgeType;     //!< Internal name, not of any use for you.
typedef byte CargoID;        //!< The ID of a cargo.
class CommandCost;           //!< The cost of a command.
typedef uint16 EngineID;     //!< The ID of an engine.
typedef uint16 GroupID;      //!< The ID of a group.
typedef uint16 IndustryID;   //!< The ID of an industry.
typedef uint8 IndustryType;  //!< The ID of an industry-type.
typedef OverflowSafeInt64 Money; //!< Money, stored in a 32bit/64bit safe way. For AIs money is always in pounds.
typedef uint16 SignID;       //!< The ID of a sign.
typedef uint16 StationID;    //!< The ID of a station.
typedef uint16 StringID;     //!< The ID of a string.
typedef uint32 TileIndex;    //!< The ID of a tile (just named differently).
typedef uint16 TownID;       //!< The ID of a town.
typedef uint16 VehicleID;    //!< The ID of a vehicle.
typedef uint16 WaypointID;   //!< The ID of a waypoint.

/* Types we defined ourself, as the OpenTTD core doesn't have them (yet) */
typedef uint AIErrorType;    //!< The types of errors inside the NoAI framework.
typedef BridgeType BridgeID; //!< The ID of a bridge.
typedef uint16 SubsidyID;    //!< The ID of a subsidy.

#endif /* AI_TYPES_HPP */
