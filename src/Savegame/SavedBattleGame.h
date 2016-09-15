/*
 * Copyright 2010-2015 OpenXcom Developers.
 *
 * This file is part of OpenXcom.
 *
 * OpenXcom is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenXcom is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef OPENXCOM_SAVEDBATTLEGAME_H
#define OPENXCOM_SAVEDBATTLEGAME_H

#include <vector>
#include <string>
#include <yaml-cpp/yaml.h>
#include "Tile.h"
#include "BattleUnit.h"

namespace OpenXcom
{

class Tile;
class SavedGame;
class MapDataSet;
class Node;
class BattlescapeState;
class Position;
class Pathfinding;
class TileEngine;
class BattleItem;
class Mod;
class State;
class ItemContainer;

/**
 * The battlescape data that gets written to disk when the game is saved.
 * A saved game holds all the variable info in a game like mapdata,
 * soldiers, items, etc.
 */
class SavedBattleGame
{
private:
	BattlescapeState *_battleState;
	Mod *_rule;
	int _mapsize_x, _mapsize_y, _mapsize_z;
	std::vector<MapDataSet*> _mapDataSets;
	std::vector<Tile> _tiles;
	BattleUnit *_selectedUnit, *_lastSelectedUnit;
	std::vector<Node*> _nodes;
	std::vector<BattleUnit*> _units;
	std::vector<BattleItem*> _items, _deleted;
	Pathfinding *_pathfinding;
	TileEngine *_tileEngine;
	std::string _missionType, _alienCustomDeploy, _alienCustomMission, _startingConditionType;
	int _globalShade;
	UnitFaction _side;
	int _turn;
	bool _debugMode;
	bool _aborted;
	int _itemId;
	int _objectiveType, _objectivesDestroyed, _objectivesNeeded;
	std::vector<BattleUnit*> _exposedUnits;
	std::list<BattleUnit*> _fallingUnits;
	bool _unitsFalling, _cheating;
	std::vector<Position> _tileSearch, _storageSpace;
	BattleActionType _tuReserved;
	bool _kneelReserved;
	std::vector< std::vector<std::pair<int, int> > > _baseModules;
	ItemContainer *_baseItems;
	int _depth, _ambience;
	double _ambientVolume;
	std::vector<BattleItem*> _recoverGuaranteed, _recoverConditional;
	std::string _music;
	/// Selects a soldier.
	BattleUnit *selectPlayerUnit(int dir, bool checkReselect = false, bool setReselect = false, bool checkInventory = false);
public:
    /// FIXME: hit log
	std::wostringstream hitLog;
	/// Creates a new battle save, based on the current generic save.
	SavedBattleGame(Mod *rule);
	/// Cleans up the saved game.
	~SavedBattleGame();
	/// Loads a saved battle game from YAML.
	void load(const YAML::Node& node, Mod *mod, SavedGame* savedGame);
	/// Saves a saved battle game to YAML.
	YAML::Node save() const;
	/// Sets the dimensions of the map and initializes it.
	void initMap(int mapsize_x, int mapsize_y, int mapsize_z);
	/// Initialises the pathfinding and tileengine.
	void initUtilities(Mod *mod);
	/// Gets the game's mapdatafiles.
	std::vector<MapDataSet*> *getMapDataSets();
	/// Sets the mission type.
	void setMissionType(const std::string &missionType);
	/// Gets the mission type.
	const std::string &getMissionType() const;
	/// Gets the base's items BEFORE the mission.
	ItemContainer *getBaseStorageItems();
	/// Sets the starting condition type.
	void setStartingConditionType(const std::string &startingConditionType);
	/// Gets the starting condition type.
	const std::string &getStartingConditionType() const;
	/// Sets the custom alien data.
	void setAlienCustom(const std::string &deploy, const std::string &mission);
	/// Gets the custom alien deploy.
	const std::string &getAlienCustomDeploy() const;
	/// Gets the custom mission definition.
	const std::string &getAlienCustomMission() const;
	/// Sets the global shade.
	void setGlobalShade(int shade);
	/// Gets the global shade.
	int getGlobalShade() const;
	/// Gets a pointer to the list of nodes.
	std::vector<Node*> *getNodes();
	/// Gets a pointer to the list of items.
	std::vector<BattleItem*> *getItems();
	/// Gets a pointer to the list of units.
	std::vector<BattleUnit*> *getUnits();
	/// Gets terrain size x.
	int getMapSizeX() const;
	/// Gets terrain size y.
	int getMapSizeY() const;
	/// Gets terrain size z.
	int getMapSizeZ() const;
	/// Gets terrain x*y*z
	int getMapSizeXYZ() const;

	/**
	 * Converts coordinates into a unique index.
	 * getTile() calls this every time, so should be inlined along with it.
	 * @param pos The position to convert.
	 * @return A unique index.
	 */
	inline int getTileIndex(const Position& pos) const
	{
		return pos.z * _mapsize_y * _mapsize_x + pos.y * _mapsize_x + pos.x;
	}

	/// Converts a tile index to its coordinates.
	void getTileCoords(int index, int *x, int *y, int *z) const;

	/**
	 * Gets the Tile at a given position on the map.
	 * This method is called over 50mil+ times per turn so it seems useful
	 * to inline it.
	 * @param pos Map position.
	 * @return Pointer to the tile at that position.
	 */
	inline Tile *getTile(const Position& pos)
	{
		if (pos.x < 0 || pos.y < 0 || pos.z < 0
			|| pos.x >= _mapsize_x || pos.y >= _mapsize_y || pos.z >= _mapsize_z)
			return 0;

		return &_tiles[getTileIndex(pos)];
	}

	/*
	 * Gets a pointer to the tiles, a tile is the smallest component of battlescape.
	 * @param pos Index position, less than `getMapSizeXYZ()`.
	 * @return Pointer to the tile at that index.
	 */
	Tile* getTile(int i)
	{
		return &_tiles[i];
	}
	/// Gets the currently selected unit.
	BattleUnit *getSelectedUnit() const;
	/// Sets the currently selected unit.
	void setSelectedUnit(BattleUnit *unit);
	/// Selects the previous soldier.
	BattleUnit *selectPreviousPlayerUnit(bool checkReselect = false, bool setReselect = false, bool checkInventory = false);
	/// Selects the next soldier.
	BattleUnit *selectNextPlayerUnit(bool checkReselect = false, bool setReselect = false, bool checkInventory = false);
	/// Selects the unit with position on map.
	BattleUnit *selectUnit(const Position& pos);
	/// Gets the pathfinding object.
	Pathfinding *getPathfinding() const;
	/// Gets a pointer to the tileengine.
	TileEngine *getTileEngine() const;
	/// Gets the playing side.
	UnitFaction getSide() const;
	/// Can unit use that weapon?
	bool canUseWeapon(const BattleItem *weapon, const BattleUnit *unit, bool isBerserking) const;
	/// Gets the turn number.
	int getTurn() const;
	/// Ends the turn.
	void endTurn();
	/// Sets debug mode.
	void setDebugMode();
	/// Gets debug mode.
	bool getDebugMode() const;
	/// Load map resources.
	void loadMapResources(Mod *mod);
	/// Resets tiles units are standing on
	void resetUnitTiles();
	/// Add item to delete list.
	void deleteList(BattleItem *item);
	/// Removes an item from the game.
	void removeItem(BattleItem *item);
	/// Adds an item to a unit and the game.
	bool addItem(BattleItem *item, BattleUnit *unit, bool allowSecondClip = false, bool allowAutoLoadout = false, bool allowUnloadedWeapons = false);
	/// Add buildIn weapon form list to unit.
	void addFixedItems(BattleUnit *unit, const std::vector<std::string> &fixed);
	/// Create all fixed items for new created unit.
	void initFixedItems(BattleUnit *unit);
	/// Sets whether the mission was aborted.
	void setAborted(bool flag);
	/// Checks if the mission was aborted.
	bool isAborted() const;
	/// Sets how many objectives need to be destroyed.
	void setObjectiveCount(int counter);
	/// increments the objective counter.
	void addDestroyedObjective();
	/// Checks if all the objectives are destroyed.
	bool allObjectivesDestroyed();
	/// Gets the current item ID.
	int *getCurrentItemId();
	/// Gets a spawn node.
	Node *getSpawnNode(int nodeRank, BattleUnit *unit);
	/// Gets a patrol node.
	Node *getPatrolNode(bool scout, BattleUnit *unit, Node *fromNode);
	/// Carries out new turn preparations.
	void prepareNewTurn();
	/// Revives unconscious units (healthcheck).
	void reviveUnconsciousUnits(bool noTU = false);
	/// Removes the body item that corresponds to the unit.
	void removeUnconsciousBodyItem(BattleUnit *bu);
	/// Sets or tries to set a unit of a certain size on a certain position of the map.
	bool setUnitPosition(BattleUnit *bu, const Position &position, bool testOnly = false);
	/// Adds this unit to the vector of falling units.
	bool addFallingUnit(BattleUnit* unit);
	/// Gets the vector of falling units.
	std::list<BattleUnit*> *getFallingUnits();
	/// Toggles the switch that says "there are units falling, start the fall state".
	void setUnitsFalling(bool fall);
	/// Checks the status of the switch that says "there are units falling".
	bool getUnitsFalling() const;
	/// Gets a pointer to the BattlescapeState.
	BattlescapeState *getBattleState();
	/// Gets a pointer to the BattlescapeGame.
	BattlescapeGame *getBattleGame();
	/// Sets the pointer to the BattlescapeState.
	void setBattleState(BattlescapeState *bs);
	/// Gets the highest ranked, living XCom unit.
	BattleUnit* getHighestRankedXCom();
	/// Gets the morale modifier for the unit passed to this function.
	int getUnitMoraleModifier(BattleUnit* unit);
	/// Gets the morale modifier for Aliens based on they number or XCom based on the highest ranked soldier.
	int getFactionMoraleModifier(bool player);
	/// Checks whether a particular faction has eyes on *unit (whether any unit on that faction sees *unit).
	bool eyesOnTarget(UnitFaction faction, BattleUnit* unit);
	/// Attempts to place a unit on or near entryPoint.
	bool placeUnitNearPosition(BattleUnit *unit, Position entryPoint, bool largeFriend);
	/// Resets the turn counter.
	void resetTurnCounter();
	/// Resets the visibility of all tiles on the map.
	void resetTiles();
	/// get an 11x11 grid of positions (-10 to +10) to check.
	const std::vector<Position> &getTileSearch();
	/// check if the AI has engaged cheat mode.
	bool isCheating();
	/// get the reserved fire mode.
	BattleActionType getTUReserved() const;
	/// set the reserved fire mode.
	void setTUReserved(BattleActionType reserved);
	/// get whether we are reserving TUs to kneel.
	bool getKneelReserved() const;
	/// set whether we are reserving TUs to kneel.
	void setKneelReserved(bool reserved);
	/// give me access to the storage tiles vector.
	std::vector<Position> &getStorageSpace();
	/// move all the leftover items to random locations in the storage tiles vector.
	void randomizeItemLocations(Tile *t);
	/// get a reference to the baseModules map.
	std::vector< std::vector<std::pair<int, int> > > &getModuleMap();
	/// calculate the number of map modules remaining
	void calculateModuleMap();
	/// a shortcut to the geoscape save.
	SavedGame *getGeoscapeSave();
	/// get the depth of the battlescape game.
	int getDepth() const;
	/// set the depth of the battlescape game.
	void setDepth(int depth);
	/// uses the depth variable to set a palette.
	void setPaletteByDepth(State *state);
	/// sets the ambient sound effect;
	void setAmbientSound(int sound);
	/// gets the ambient sound effect;
	int getAmbientSound() const;
	// gets ruleset.
	const Mod *getMod() const;
	/// gets the list of items we're guaranteed.
	std::vector<BattleItem*> *getGuaranteedRecoveredItems();
	/// gets the list of items we MIGHT get.
	std::vector<BattleItem*> *getConditionalRecoveredItems();
	/// Get the name of the music track.
	const std::string &getMusic() const;
	/// Set the name of the music track.
	void setMusic(const std::string &track);
	/// Sets the objective type for this mission.
	void setObjectiveType(int type);
	/// Gets the objective type of this mission.
	SpecialTileType getObjectiveType();
	/// sets the ambient sound effect;
	void setAmbientVolume(double volume);
	/// gets the ambient sound effect;
	double getAmbientVolume() const;
};

}

#endif
