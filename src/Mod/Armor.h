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
#ifndef OPENXCOM_ARMOR_H
#define OPENXCOM_ARMOR_H

#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>
#include "MapData.h"
#include "Unit.h"
#include "RuleStatBonus.h"
#include "RuleDamageType.h"
#include "../Engine/Script.h"

namespace OpenXcom
{

enum ForcedTorso { TORSO_USE_GENDER, TORSO_ALWAYS_MALE, TORSO_ALWAYS_FEMALE };
enum UnitSide { SIDE_FRONT, SIDE_LEFT, SIDE_RIGHT, SIDE_REAR, SIDE_UNDER, SIDE_MAX };

class BattleUnit;

/**
 * Represents a specific type of armor.
 * Not only soldier armor, but also alien armor - some alien races wear Soldier Armor, Leader Armor or Commander Armor
 * depending on their rank.
 */
class Armor
{
private:
	std::string _type, _spriteSheet, _spriteInv, _corpseGeo, _storeItem, _specWeapon;
	std::vector<std::string> _corpseBattle;
	std::vector<std::string> _builtInWeapons;
	int _frontArmor, _sideArmor, _rearArmor, _underArmor, _drawingRoutine;
	MovementType _movementType;
	int _size, _weight, _visibilityAtDark, _regeneration;
	float _damageModifier[DAMAGE_TYPES];
	std::vector<int> _loftempsSet;
	UnitStats _stats;
	int _deathFrames;
	bool _constantAnimation, _canHoldWeapon, _hasInventory;
	ForcedTorso _forcedTorso;
	int _faceColorGroup, _hairColorGroup, _utileColorGroup, _rankColorGroup;
	std::vector<int> _faceColor, _hairColor, _utileColor, _rankColor;
	int _fearImmune, _bleedImmune, _painImmune, _zombiImmune;
	float _overKill, _meleeDodgeBackPenalty;
	RuleStatBonus _psiDefence, _meleeDodge;
	RuleStatBonus _timeRecovery, _energyRecovery, _moraleRecovery, _healthRecovery, _stunRecovery;
	ScriptContainer<BattleUnit> *_recolorScript;
public:
	/// Creates a blank armor ruleset.
	Armor(const std::string &type);
	/// Cleans up the armor ruleset.
	~Armor();
	/// Loads the armor data from YAML.
	void load(const YAML::Node& node, const ScriptParser<BattleUnit>& parser);
	/// Gets the armor's type.
	std::string getType() const;
	/// Gets the unit's sprite sheet.
	std::string getSpriteSheet() const;
	/// Gets the unit's inventory sprite.
	std::string getSpriteInventory() const;
	/// Gets the front armor level.
	int getFrontArmor() const;
	/// Gets the side armor level.
	int getSideArmor() const;
	/// Gets the rear armor level.
	int getRearArmor() const;
	/// Gets the under armor level.
	int getUnderArmor() const;
	/// Gets the armor level of armor side.
	int getArmor(UnitSide side) const;
	/// Gets the Geoscape corpse item.
	std::string getCorpseGeoscape() const;
	/// Gets the Battlescape corpse item.
	const std::vector<std::string> &getCorpseBattlescape() const;
	/// Gets the stores item.
	std::string getStoreItem() const;
	/// Gets the special weapon type.
	std::string getSpecialWeapon() const;
	/// Gets the battlescape drawing routine ID.
	int getDrawingRoutine() const;
	/// DO NOT USE THIS FUNCTION OUTSIDE THE BATTLEUNIT CONSTRUCTOR OR I WILL HUNT YOU DOWN.
	MovementType getMovementType() const;
	/// Gets whether this is a normal or big unit.
	int getSize() const;
	/// Gets damage modifier.
	float getDamageModifier(ItemDamageType dt) const;
	/// Gets loftempSet
	const std::vector<int> &getLoftempsSet() const;
	/// Gets the armor's stats.
	const UnitStats *getStats() const;
	/// Gets unit psi defense.
	int getPsiDefence(const BattleUnit* unit) const;
	/// Gets unit melee dodge chance.
	int getMeleeDodge(const BattleUnit* unit) const;
	/// Gets unit dodge penalty if hit from behind.
	float getMeleeDodgeBackPenalty() const;

	/// Gets unit TU recovery.
	int getTimeRecovery(const BattleUnit* unit) const;
	/// Gets unit Energy recovery.
	int getEnergyRecovery(const BattleUnit* unit) const;
	/// Gets unit Morale recovery.
	int getMoraleRecovery(const BattleUnit* unit) const;
	/// Gets unit Health recovery.
	int getHealthRecovery(const BattleUnit* unit) const;
	/// Gets unit Stun recovery.
	int getStunRegeneration(const BattleUnit* unit) const;

	/// Gets the armor's weight.
	int getWeight() const;
	/// Gets number of death frames.
	int getDeathFrames() const;
	/// Gets if armor uses constant animation.
	bool getConstantAnimation() const;
	/// Gets if armor can hold weapon.
	bool getCanHoldWeapon() const;
	/// Checks if this armor ignores gender (power suit/flying suit).
	ForcedTorso getForcedTorso() const;
	/// Gets buildin weapons of armor.
	const std::vector<std::string> &getBuiltInWeapons() const;
	/// Gets max view distance at dark in BattleScape.
	int getVisibilityAtDark() const;
	/// Gets how armor react to fear.
	bool getFearImmune(bool def = false) const;
	/// Gets how armor react to bleeding.
	bool getBleedImmune(bool def = false) const;
	/// Gets how armor react to inflicted pain.
	bool getPainImmune(bool def = false) const;
	/// Gets how armor react to zombification.
	bool getZombiImmune(bool def = false) const;
	/// Gets how much negative hp is require to gib unit.
	float getOverKill() const;
	/// Get face base color
	int getFaceColorGroup() const;
	/// Get hair base color
	int getHairColorGroup() const;
	/// Get utile base color
	int getUtileColorGroup() const;
	/// Get rank base color
	int getRankColorGroup() const;
	/// Get face base color
	int getFaceColor(int i) const;
	/// Get hair base color
	int getHairColor(int i) const;
	/// Get utile base color
	int getUtileColor(int i) const;
	/// Get rank base color
	int getRankColor(int i) const;
	/// can we access this unit's inventory?
	bool hasInventory() const;
	/// Gets script used to recolor unit.
	ScriptContainer<BattleUnit> *getRecolorScript();
};

}

#endif
