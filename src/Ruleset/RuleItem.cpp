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
#include "Unit.h"
#include "RuleItem.h"
#include "RuleInventory.h"
#include "../Savegame/BattleUnit.h"
#include "../Engine/SurfaceSet.h"
#include "../Engine/Surface.h"
#include "../fmath.h"

namespace OpenXcom
{

namespace
{

/**
 * Function returning static value.
 */
template<int I>
float stat(const BattleUnit *unit)
{
	return I;
}
/**
 * Getter for one basic stat of unit.
 */
template<int UnitStats::* field>
float stat(const BattleUnit *unit)
{
	const UnitStats *stat = unit->getBaseStats();
	return stat->*field;
}

/**
 * Getter for multiply of two basic stat of unit.
 */
template<int UnitStats::* fieldA, int UnitStats::* fieldB>
float stat(const BattleUnit *unit)
{
	const UnitStats *stat = unit->getBaseStats();
	return (stat->*fieldA) * (stat->*fieldB);
}

float curretTimeUnits(const BattleUnit *unit)
{
	return unit->getTimeUnits();
}

float curretHealth(const BattleUnit *unit)
{
	return unit->getHealth();
}

float curretEnergy(const BattleUnit *unit)
{
	return unit->getEnergy();
}

float curretMorale(const BattleUnit *unit)
{
	return unit->getMorale();
}


float normalizedTimeUnits(const BattleUnit *unit)
{
	return 1.0 * unit->getTimeUnits()/ unit->getBaseStats()->tu;
}

float normalizedHealth(const BattleUnit *unit)
{
	return 1.0 * unit->getHealth() / unit->getBaseStats()->health;
}

float normalizedEnergy(const BattleUnit *unit)
{
	return 1.0 * unit->getEnergy() / unit->getBaseStats()->stamina;
}

float normalizedMorale(const BattleUnit *unit)
{
	return 1.0 * unit->getMorale() / 100;
}

template<BonusStatFunc func, int p>
float power(const BattleUnit *unit)
{
	return std::pow(func(unit), p);
}

const size_t statDataFuncSize = 3;

/**
 * Data describing same functions but with different exponent.
 */
struct BonusStatDataFunc
{
	BonusStatFunc power[statDataFuncSize];
};
/**
 * Data describing basic stat getter.
 */
struct BonusStatData
{
	std::string name;
	BonusStatDataFunc func;
};

/**
 * Helper function creating BonusStatData with proper functions.
 */
template<int Val>
BonusStatDataFunc create()
{
	BonusStatDataFunc data =
	{
		{
			&power< &stat<Val>, 1>,
			&power< &stat<Val>, 2>,
			&power< &stat<Val>, 3>,
		}
	};
	return data;
}

/**
 * Helper function creating BonusStatData with proper functions.
 */
template<int UnitStats::* fieldA>
BonusStatDataFunc create()
{
	BonusStatDataFunc data =
	{
		{
			&power< &stat<fieldA>, 1>,
			&power< &stat<fieldA>, 2>,
			&power< &stat<fieldA>, 3>,
		}
	};
	return data;
}

/**
 * Helper function creating BonusStatData with proper functions.
 */
template<int UnitStats::* fieldA, int UnitStats::* fieldB>
BonusStatDataFunc create()
{
	BonusStatDataFunc data =
	{
		{
			&power< &stat<fieldA, fieldB>, 1>,
			&power< &stat<fieldA, fieldB>, 2>,
			&power< &stat<fieldA, fieldB>, 3>,
		}
	};
	return data;
}

/**
 * Helper function creating BonusStatData with proper functions.
 */
template<BonusStatFunc func>
BonusStatDataFunc create()
{
	BonusStatDataFunc data =
	{
		{
			&power< func, 1>,
			&power< func, 2>,
			&power< func, 3>,
		}
	};
	return data;
}

/**
 * List of all possible getters of basic stats.
 */
BonusStatData statDataMap[] =
{
	{ "flatOne", create<1>() },
	{ "flatHunderd", create<100>() },
	{ "strength", create<&UnitStats::strength>() },
	{ "psi", create<&UnitStats::psiSkill, &UnitStats::psiStrength>() },
	{ "psiSkill", create<&UnitStats::psiSkill>() },
	{ "psiStrength", create<&UnitStats::psiStrength>() },
	{ "throwing", create<&UnitStats::throwing>() },
	{ "bravery", create<&UnitStats::bravery>() },
	{ "firing", create<&UnitStats::firing>() },
	{ "health", create<&UnitStats::health>() },
	{ "tu", create<&UnitStats::tu>() },
	{ "reactions", create<&UnitStats::reactions>() },
	{ "stamina", create<&UnitStats::stamina>() },
	{ "melee", create<&UnitStats::melee>() },
	{ "strengthMelee", create<&UnitStats::strength, &UnitStats::melee>() },
	{ "strengthThrowing", create<&UnitStats::strength, &UnitStats::throwing>() },
	{ "firingReactions", create<&UnitStats::firing, &UnitStats::reactions>() },

	{ "healthCurrent", create<&curretHealth>() },
	{ "tuCurrent", create<&curretTimeUnits>() },
	{ "energyCurrent", create<&curretEnergy>() },
	{ "moraleCurrent", create<&curretMorale>() },

	{ "healthNormalized", create<&normalizedHealth>() },
	{ "tuNormalized", create<&normalizedTimeUnits>() },
	{ "energyNormalized", create<&normalizedEnergy>() },
	{ "moraleNormalized", create<&normalizedMorale>() },
};
const size_t statDataMapSize = sizeof(statDataMap) / sizeof(statDataMap[0]);

int statsModiferRead(const std::vector<std::pair<BonusStatFunc, float> >& mod, const BattleUnit *unit)
{
	float power = 0;
	for (size_t i = 0; i < mod.size(); ++i)
		power += mod[i].first(unit) * mod[i].second;
	return power;
}

void statsModiferWrite(std::vector<std::pair<BonusStatFunc, float> >& mod, const YAML::Node &node)
{
	if (node)
	{
		mod.clear();
		for (size_t i = 0; i < statDataMapSize; ++i)
		{
			if (const YAML::Node &dd = node[statDataMap[i].name])
			{
				if (dd.IsScalar())
				{
					mod.push_back(std::make_pair(statDataMap[i].func.power[0], dd.as<float>()));
				}
				else
				{
					for (size_t j = 0; j < statDataFuncSize && j < dd.size(); ++j)
					{
						float val = dd[j].as<float>();
						if (!AreSame(val, 0.0f))
						{
							mod.push_back(std::make_pair(statDataMap[i].func.power[j], val));
						}
					}
				}
			}
		}
	}
}

} //namespace

/**
 * Creates a blank ruleset for a certain type of item.
 * @param type String defining the type.
 */
RuleItem::RuleItem(const std::string &type) :
	_type(type), _name(type), _size(0.0), _costBuy(0), _costSell(0), _transferTime(24), _weight(3), _bigSprite(0), _bigSpriteAlt(0), _floorSprite(-1), _floorSpriteAlt(-1), _handSprite(120), _bulletSprite(-1),
	_fireSound(-1), _hitSound(-1), _hitAnimation(0), _power(0), _powerRangeReduction(0), _damageType(),
	_accuracyAimed(0), _accuracyAuto(0), _accuracySnap(0), _accuracyMelee(0), _accuracyUse(0), _accuracyMind(0), _accuracyPanic(20), _accuracyThrow(0),
	_tuAimed(0), _tuAuto(0), _tuSnap(0), _tuMelee(0), _tuUse(25), _tuMind(-1), _tuPanic(-1), _tuThrow(25),
	_energyAimed(0), _energyAuto(-1), _energySnap(-1), _energyMelee(0), _energyUse(0), _energyMind(-1), _energyPanic(-1), _energyThrow(0),
	_clipSize(0), _tuPrime(50), _tuLoad(15), _tuUnload(8),
	_battleType(BT_NONE), _twoHanded(false), _waypoint(false), _fixedWeapon(false), _invWidth(1), _invHeight(1),
	_painKiller(0), _heal(0), _stimulant(0), _woundRecovery(0), _healthRecovery(0), _stunRecovery(0), _energyRecovery(0), _recoveryPoints(0), _armor(20), _turretType(-1),
	_recover(true), _liveAlien(false), _attraction(0), _flatRate(false), _arcingShot(false), _listOrder(0),
	_maxRange(200), _aimRange(200), _snapRange(15), _autoRange(7), _minRange(0), _dropoff(2), _bulletSpeed(0), _explosionSpeed(0), _autoShots(3), _shotgunPellets(0),
	_LOSRequired(false), _underwaterOnly(false), _psiReqiured(false), _meleeSound(39), _meleePower(0), _meleeAnimation(0), _meleeHitSound(-1), _specialType(-1), _vaporColor(-1), _vaporDensity(0), _vaporProbability(15)
{
	_accuracyMulti.push_back(std::make_pair(&stat<&UnitStats::firing>, 1.0f));
	_meleeMulti.push_back(std::make_pair(&stat<&UnitStats::melee>, 1.0f));
	_throwMulti.push_back(std::make_pair(&stat<&UnitStats::throwing>, 1.0f));
}

/**
 *
 */
RuleItem::~RuleItem()
{
}

/**
 * Loads the item from a YAML file.
 * @param node YAML node.
 * @param modIndex Offsets the sounds and sprite values to avoid conflicts.
 * @param listOrder The list weight for this item.
 */
void RuleItem::load(const YAML::Node &node, int modIndex, int listOrder, const std::vector<RuleDamageType*> &damageTypes)
{
	_type = node["type"].as<std::string>(_type);
	_name = node["name"].as<std::string>(_name);
	_requires = node["requires"].as< std::vector<std::string> >(_requires);
	_requiresBuy = node["requiresBuy"].as< std::vector<std::string> >(_requiresBuy);
	_size = node["size"].as<double>(_size);
	_costBuy = node["costBuy"].as<int>(_costBuy);
	_costSell = node["costSell"].as<int>(_costSell);
	_transferTime = node["transferTime"].as<int>(_transferTime);
	_weight = node["weight"].as<int>(_weight);
	if (node["bigSprite"])
	{
		_bigSprite = node["bigSprite"].as<int>(_bigSprite);
		// BIGOBS.PCK: 57 entries
		if (_bigSprite > 56)
			_bigSprite += modIndex;
	}
	if (node["bigSpriteAlt"])
	{
		_bigSpriteAlt = node["bigSpriteAlt"].as<int>(_bigSpriteAlt);
		// BIGOBS.PCK: 57 entries
		if (_bigSpriteAlt > 56)
			_bigSpriteAlt += modIndex;
	}
	else
	{
		_bigSpriteAlt = _bigSprite;
	}
	if (node["floorSprite"])
	{
		_floorSprite = node["floorSprite"].as<int>(_floorSprite);
		// FLOOROB.PCK: 73 entries
		if (_floorSprite > 72)
			_floorSprite += modIndex;
	}
	if (node["floorSpriteAlt"])
	{
		_floorSpriteAlt = node["floorSpriteAlt"].as<int>(_floorSpriteAlt);
		// FLOOROB.PCK: 73 entries
		if (_floorSpriteAlt > 72)
			_floorSpriteAlt += modIndex;
	}
	else
	{
		_floorSpriteAlt = _floorSprite;
	}
	if (node["handSprite"])
	{
		_handSprite = node["handSprite"].as<int>(_handSprite);
		// HANDOBS.PCK: 128 entries
		if (_handSprite > 127)
			_handSprite += modIndex;
	}
	if (node["bulletSprite"])
	{
		// Projectiles: 385 entries ((105*33) / (3*3)) (35 sprites per projectile(0-34), 11 projectiles (0-10))
		_bulletSprite = node["bulletSprite"].as<int>(_bulletSprite) * 35;
		if (_bulletSprite >= 385)
			_bulletSprite += modIndex;
	}
	if (node["fireSound"])
	{
		_fireSound = node["fireSound"].as<int>(_fireSound);
		// BATTLE.CAT: 55 entries
		if (_fireSound > 54)
			_fireSound += modIndex;
	}
	if (node["hitSound"])
	{
		_hitSound = node["hitSound"].as<int>(_hitSound);
		// BATTLE.CAT: 55 entries
		if (_hitSound > 54)
			_hitSound += modIndex;
	}
	if (node["meleeSound"])
	{
		_meleeSound = node["meleeSound"].as<int>(_meleeSound);
		// BATTLE.CAT: 55 entries
		if (_meleeSound > 54)
			_meleeSound += modIndex;
	}
	if (node["hitAnimation"])
	{
		_hitAnimation = node["hitAnimation"].as<int>(_hitAnimation);
		// SMOKE.PCK: 56 entries
		if (_hitAnimation > 55)
			_hitAnimation += modIndex;
	}
	if (node["meleeAnimation"])
	{
		_meleeAnimation = node["meleeAnimation"].as<int>(_meleeAnimation);
		// HIT.PCK: 4 entries
		if (_meleeAnimation > 3)
			_meleeAnimation += modIndex;
	}
	if (node["meleeHitSound"])
	{
		_meleeHitSound = node["meleeHitSound"].as<int>(_meleeHitSound);
		// BATTLE.CAT: 55 entries
		if (_meleeHitSound > 54)
			_meleeHitSound += modIndex;
	}

	if (node["damageType"])
	{
		//compatibility hack for corpse explosion, that didn't have defined damage type
		ItemDamageType type = node["blastRadius"].as<int>(0) > 0 ? DT_HE : DT_NONE;
		//load predefined damage type
		_damageType = *damageTypes.at(node["damageType"].as<int>(type));
	}
	_damageType.FixRadius = node["blastRadius"].as<int>(_damageType.FixRadius);
	if (node["damageAlter"])
	{
		_damageType.load(node["damageAlter"]);
	}

	_power = node["power"].as<int>(_power);
	_psiAttackName = node["psiAttackName"].as<std::string>(_psiAttackName);
	_compatibleAmmo = node["compatibleAmmo"].as< std::vector<std::string> >(_compatibleAmmo);

	_accuracyAimed = node["accuracyAimed"].as<int>(_accuracyAimed);
	_accuracyAuto = node["accuracyAuto"].as<int>(_accuracyAuto);
	_accuracySnap = node["accuracySnap"].as<int>(_accuracySnap);
	_accuracyMelee = node["accuracyMelee"].as<int>(_accuracyMelee);
	_accuracyUse = node["accuracyUse"].as<int>(_accuracyUse);
	_accuracyMind = node["accuracyMindControl"].as<int>(_accuracyMind);
	_accuracyPanic = node["accuracyPanic"].as<int>(_accuracyPanic);
	_accuracyThrow = node["accuracyThrow"].as<int>(_accuracyThrow);

	_tuAimed = node["tuAimed"].as<int>(_tuAimed);
	_tuAuto = node["tuAuto"].as<int>(_tuAuto);
	_tuSnap = node["tuSnap"].as<int>(_tuSnap);
	_tuMelee = node["tuMelee"].as<int>(_tuMelee);
	_tuUse = node["tuUse"].as<int>(_tuUse);
	_tuMind = node["tuMindControl"].as<int>(_tuMind);
	_tuPanic = node["tuPanic"].as<int>(_tuPanic);
	_tuThrow = node["tuThrow"].as<int>(_tuThrow);

	_energyAimed = node["energyAimed"].as<int>(_energyAimed);
	_energyAuto = node["energyAuto"].as<int>(_energyAuto);
	_energySnap = node["energySnap"].as<int>(_energySnap);
	_energyMelee = node["energyMelee"].as<int>(_energyMelee);
	_energyUse = node["energyUse"].as<int>(_energyUse);
	_energyMind = node["energyMindControl"].as<int>(_energyMind);
	_energyPanic = node["energyPanic"].as<int>(_energyPanic);
	_energyThrow = node["energyThrow"].as<int>(_energyThrow);

	_clipSize = node["clipSize"].as<int>(_clipSize);
	_tuPrime = node["tuPrime"].as<int>(_tuPrime);
	_tuLoad = node["tuLoad"].as<int>(_tuLoad);
	_tuUnload = node["tuUnload"].as<int>(_tuUnload);
	_twoHanded = node["twoHanded"].as<bool>(_twoHanded);
	_waypoint = node["waypoint"].as<bool>(_waypoint);
	_fixedWeapon = node["fixedWeapon"].as<bool>(_fixedWeapon);
	_invWidth = node["invWidth"].as<int>(_invWidth);
	_invHeight = node["invHeight"].as<int>(_invHeight);
	_painKiller = node["painKiller"].as<int>(_painKiller);
	_heal = node["heal"].as<int>(_heal);
	_stimulant = node["stimulant"].as<int>(_stimulant);
	_woundRecovery = node["woundRecovery"].as<int>(_woundRecovery);
	_healthRecovery = node["healthRecovery"].as<int>(_healthRecovery);
	_stunRecovery = node["stunRecovery"].as<int>(_stunRecovery);
	_energyRecovery = node["energyRecovery"].as<int>(_energyRecovery);
	_recoveryPoints = node["recoveryPoints"].as<int>(_recoveryPoints);
	_armor = node["armor"].as<int>(_armor);
	_turretType = node["turretType"].as<int>(_turretType);
	_recover = node["recover"].as<bool>(_recover);
	_liveAlien = node["liveAlien"].as<bool>(_liveAlien);
	_attraction = node["attraction"].as<int>(_attraction);
	_flatRate = node["flatRate"].as<bool>(_flatRate);
	_arcingShot = node["arcingShot"].as<bool>(_arcingShot);
	_listOrder = node["listOrder"].as<int>(_listOrder);
	_maxRange = node["maxRange"].as<int>(_maxRange);
	_aimRange = node["aimRange"].as<int>(_aimRange);
	_snapRange = node["snapRange"].as<int>(_snapRange);
	_autoRange = node["autoRange"].as<int>(_autoRange);
	_minRange = node["minRange"].as<int>(_minRange);
	_dropoff = node["dropoff"].as<int>(_dropoff);
	_bulletSpeed = node["bulletSpeed"].as<int>(_bulletSpeed);
	_explosionSpeed = node["explosionSpeed"].as<int>(_explosionSpeed);
	_autoShots = node["autoShots"].as<int>(_autoShots);
	_shotgunPellets = node["shotgunPellets"].as<int>(_shotgunPellets);
	_zombieUnit = node["zombieUnit"].as<std::string>(_zombieUnit);
	_LOSRequired = node["LOSRequired"].as<bool>(_LOSRequired);
	_meleePower = node["meleePower"].as<int>(_meleePower);
	_underwaterOnly = node["underwaterOnly"].as<bool>(_underwaterOnly);
	_specialType = node["specialType"].as<int>(_specialType);
	_vaporColor = node["vaporColor"].as<int>(_vaporColor);
	_vaporDensity = node["vaporDensity"].as<int>(_vaporDensity);
	_vaporProbability = node["vaporProbability"].as<int>(_vaporProbability);

	if (node["battleType"])
	{
		_battleType = (BattleType)node["battleType"].as<int>(_battleType);
		_psiReqiured = _battleType == BT_PSIAMP;
		if (_psiReqiured)
		{
			_powerRangeReduction = 1;
			_accuracyMulti.clear();
			_accuracyMulti.push_back(std::make_pair(&stat<&UnitStats::psiSkill, &UnitStats::psiStrength>, 0.02f));
		}
	}
	if (node["skillApplied"])
	{
		_meleeMulti.clear();
		if (node["skillApplied"].as<int>(false))
		{
			_meleeMulti.push_back(std::make_pair(&stat<&UnitStats::melee>, 1.0f));
		}
		else
		{
			_meleeMulti.push_back(std::make_pair(&stat<100>, 1.0f));
		}
	}
	if (node["strengthApplied"].as<bool>(false))
	{
		_damageBonus.clear();
		_damageBonus.push_back(std::make_pair(&stat<&UnitStats::strength>, 1.0f));
	}

	statsModiferWrite(_damageBonus, node["damageBonus"]);
	statsModiferWrite(_accuracyMulti, node["accuracyMultiplier"]);
	statsModiferWrite(_meleeMulti, node["meleeMultiplier"]);
	statsModiferWrite(_throwMulti, node["throwMultiplier"]);

	_powerRangeReduction = node["powerRangeReduction"].as<float>(_powerRangeReduction);
	_psiReqiured = node["psiRequired"].as<bool>(_psiReqiured);

	if (!_listOrder)
	{
		_listOrder = listOrder;
	}
}

/**
 * Gets the item type. Each item has a unique type.
 * @return The item's type.
 */
std::string RuleItem::getType() const
{
	return _type;
}

/**
 * Gets the language string that names
 * this item. This is not necessarily unique.
 * @return  The item's name.
 */
std::string RuleItem::getName() const
{
	return _name;
}

/**
 * Gets the list of research required to
 * use this item.
 * @return The list of research IDs.
 */
const std::vector<std::string> &RuleItem::getRequirements() const
{
	return _requires;
}

/**
 * Gets the list of research required to
 * buy this item from market.
 * @return The list of research IDs.
 */
const std::vector<std::string> &RuleItem::getBuyRequirements() const
{
	return _requiresBuy;
}

/**
 * Gets the amount of space this item
 * takes up in a storage facility.
 * @return The storage size.
 */
double RuleItem::getSize() const
{
	return _size;
}

/**
 * Gets the amount of money this item
 * costs to purchase (0 if not purchasable).
 * @return The buy cost.
 */
int RuleItem::getBuyCost() const
{
	return _costBuy;
}

/**
 * Gets the amount of money this item
 * is worth to sell.
 * @return The sell cost.
 */
int RuleItem::getSellCost() const
{
	return _costSell;
}

/**
 * Gets the amount of time this item
 * takes to arrive at a base.
 * @return The time in hours.
 */
int RuleItem::getTransferTime() const
{
	return _transferTime;
}

/**
 * Gets the weight of the item.
 * @return The weight in strength units.
 */
int RuleItem::getWeight() const
{
	return _weight;
}

/**
 * Gets the reference in BIGOBS.PCK for use in inventory.
 * @return The sprite reference.
 */
int RuleItem::getBigSprite() const
{
	return _bigSprite;
}

/**
 * Gets the alternative reference in BIGOBS.PCK for use in inventory.
 * @return The sprite reference.
 */
int RuleItem::getBigSpriteAlt() const
{
	return _bigSpriteAlt;
}

/**
 * Gets the reference in FLOOROB.PCK for use in battlescape.
 * @return The sprite reference.
 */
int RuleItem::getFloorSprite() const
{
	return _floorSprite;
}

/**
 * Gets the alternative reference in FLOOROB.PCK for use in battlescape.
 * @return The sprite reference.
 */
int RuleItem::getFloorSpriteAlt() const
{
	return _floorSpriteAlt;
}

/**
 * Gets the reference in HANDOB.PCK for use in inventory.
 * @return The sprite reference.
 */
int RuleItem::getHandSprite() const
{
	return _handSprite;
}

/**
 * Returns whether this item is held with two hands.
 * @return True if it is two-handed.
 */
bool RuleItem::isTwoHanded() const
{
	return _twoHanded;
}

/**
 * Returns whether this item uses waypoints.
 * @return True if it uses waypoints.
 */
bool RuleItem::isWaypoint() const
{
	return _waypoint;
}

/**
 * Returns whether this item is a fixed weapon.
 * You can't move/throw/drop fixed weapons - e.g. HWP turrets.
 * @return True if it is a fixed weapon.
 */
bool RuleItem::isFixed() const
{
	return _fixedWeapon;
}

/**
 * Gets the item's bullet sprite reference.
 * @return The sprite reference.
 */
int RuleItem::getBulletSprite() const
{
	return _bulletSprite;
}

/**
 * Gets the item's fire sound.
 * @return The fire sound id.
 */
int RuleItem::getFireSound() const
{
	return _fireSound;
}

/**
 * Gets the item's hit sound.
 * @return The hit sound id.
 */
int RuleItem::getHitSound() const
{
	return _hitSound;
}

/**
 * Gets the item's hit animation.
 * @return The hit animation id.
 */
int RuleItem::getHitAnimation() const
{
	return _hitAnimation;
}

/**
 * Gets the item's power.
 * @return The power.
 */
int RuleItem::getPower() const
{
	return _power;
}

/**
 * Gets amount of power drop per voxel.
 * @return Reduction per voxel.
 */
float RuleItem::getPowerRangeReduction() const
{
	return _powerRangeReduction * 0.0625f;
}

/**
 * Gets the item's accuracy for snapshots.
 * @return The snapshot accuracy.
 */
int RuleItem::getAccuracySnap() const
{
	return _accuracySnap;
}

/**
 * Gets the item's accuracy for autoshots.
 * @return The autoshot accuracy.
 */
int RuleItem::getAccuracyAuto() const
{
	return _accuracyAuto;
}

/**
 * Gets the item's accuracy for aimed shots.
 * @return The aimed accuracy.
 */
int RuleItem::getAccuracyAimed() const
{
	return _accuracyAimed;
}

/**
 * Gets the item's accuracy for melee attacks.
 * @return The melee accuracy.
 */
int RuleItem::getAccuracyMelee() const
{
	return _accuracyMelee;
}

int RuleItem::getAccuracyUse() const
{
	return _accuracyUse;
}

int RuleItem::getAccuracyMind() const
{
	return _accuracyMind;
}

int RuleItem::getAccuracyPanic() const
{
	return _accuracyPanic;
}

int RuleItem::getAccuracyThrow() const
{
	return _accuracyThrow;
}

/**
 * Gets the item's time unit percentage for aimed shots.
 * @return The aimed shot TU percentage.
 */
int RuleItem::getTUAimed() const
{
	return _tuAimed;
}

/**
 * Gets the item's time unit percentage for autoshots.
 * @return The autoshot TU percentage.
 */
int RuleItem::getTUAuto() const
{
	return _tuAuto >= 0 ? _tuAuto : _tuAimed;
}

/**
 * Gets the item's time unit percentage for snapshots.
 * @return The snapshot TU percentage.
 */
int RuleItem::getTUSnap() const
{
	return _tuSnap >= 0 ? _tuSnap : _tuAimed;
}

/**
 * Gets the item's time unit percentage for melee attacks.
 * @return The melee TU percentage.
 */
int RuleItem::getTUMelee() const
{
	return _tuMelee;
}

/**
 * Gets the number of Time Units needed to use this item.
 * @return The number of Time Units needed to use this item.
 */
int RuleItem::getTUUse() const
{
	if (_battleType != BT_PSIAMP || !_psiAttackName.empty())
	{
		return _tuUse;
	}
	else
	{
		return 0;
	}
}

/**
 * Gets the number of Time Units needed to use mind control action.
 * @return The number of Time Units needed to mind control.
 */
int RuleItem::getTUMind() const
{
	return _tuMind >= 0 ? _tuMind : _tuUse;
}

/**
 * Gets the number of Time Units needed to use panic action.
 * @return The number of Time Units needed to panic.
 */
int RuleItem::getTUPanic() const
{
	return _tuPanic >= 0 ? _tuPanic : _tuUse;
}

/**
 * Gets the item's time unit percentage for throwing.
 * @return The throw TU percentage.
 */
int RuleItem::getTUThrow() const
{
	return _tuThrow;
}

/**
 * Gets the item's energy cost for aimed shots.
 * @return The aimed shot energy cost.
 */
int RuleItem::getEnergyAimed() const
{
	return _energyAimed;
}

/**
 * Gets the item's energy cost for autoshots.
 * @return The autoshot energy cost.
 */
int RuleItem::getEnergyAuto() const
{
	return _energyAuto >= 0 ? _energyAuto : _energyAimed;
}

/**
 * Gets the item's energy cost for snapshots.
 * @return The snapshot energy cost.
 */
int RuleItem::getEnergySnap() const
{
	return _energySnap >= 0 ? _energySnap : _energyAimed;
}

/**
 * Gets the item's energy cost for melee attacks.
 * @return The melee energy cost.
 */
int RuleItem::getEnergyMelee() const
{
	return _energyMelee;
}

/**
 * Gets the number of energy cost needed to use this item.
 * @return The number of energy cost needed to use this item.
 */
int RuleItem::getEnergyUse() const
{
	if (_battleType != BT_PSIAMP || !_psiAttackName.empty())
	{
		return _energyUse;
	}
	else
	{
		return 0;
	}
}

/**
 * Gets the number of energy cost needed to use mind control action.
 * @return The number of energy cost needed to mind control.
 */
int RuleItem::getEnergyMind() const
{
	return _energyMind >= 0 ? _energyMind : _energyUse;
}

/**
 * Gets the number of energy cost needed to use panic action.
 * @return The number of energy cost needed to panic.
 */
int RuleItem::getEnergyPanic() const
{
	return _energyPanic >= 0 ? _energyPanic : _energyUse;
}

/**
 * Gets the item's energy cost for throwing.
 * @return The throw energy cost.
 */
int RuleItem::getEnergyThrow() const
{
	return _energyThrow;
}

/**
 * Gets the item's time unit percentage for prime grenade.
 * @return The prime TU percentage.
 */
int RuleItem::getTUPrime() const
{
	return _tuPrime;
}

/**
 * Gets the item's time unit for loading weapon ammo.
 * @return The throw TU.
 */
int RuleItem::getTULoad() const
{
	return _tuLoad;
}

/**
 * Gets the item's time unit for unloading weapon ammo.
 * @return The throw TU.
 */
int RuleItem::getTUUnload() const
{
	return _tuUnload;
}

/**
 * Gets a list of compatible ammo.
 * @return Pointer to a list of compatible ammo.
 */
std::vector<std::string> *RuleItem::getCompatibleAmmo()
{
	return &_compatibleAmmo;
}

/**
 * Gets the item's damage type.
 * @return The damage type.
 */
const RuleDamageType *RuleItem::getDamageType() const
{
	return &_damageType;
}

/**
 * Gets the item's battlye type.
 * @return The battle type.
 */
BattleType RuleItem::getBattleType() const
{
	return _battleType;
}

/**
 * Gets the item's width in a soldier's inventory.
 * @return The width.
 */
int RuleItem::getInventoryWidth() const
{
	return _invWidth;
}

/**
 * Gets the item's height in a soldier's inventory.
 * @return The height.
 */
int RuleItem::getInventoryHeight() const
{
	return _invHeight;
}

/**
 * Gets the item's ammo clip size.
 * @return The ammo clip size.
 */
int RuleItem::getClipSize() const
{
	return _clipSize;
}

/**
 * Draws and centers the hand sprite on a surface
 * according to its dimensions.
 * @param texture Pointer to the surface set to get the sprite from.
 * @param surface Pointer to the surface to draw to.
 */
void RuleItem::drawHandSprite(SurfaceSet *texture, Surface *surface, bool alt) const
{
	Surface *frame = texture->getFrame(alt ? this->getBigSpriteAlt() : this->getBigSprite());
	frame->setX((RuleInventory::HAND_W - this->getInventoryWidth()) * RuleInventory::SLOT_W/2);
	frame->setY((RuleInventory::HAND_H - this->getInventoryHeight()) * RuleInventory::SLOT_H/2);
	frame->blit(surface);
}

/**
 * Gets the heal quantity of the item.
 * @return The new heal quantity.
 */
int RuleItem::getHealQuantity() const
{
	return _heal;
}

/**
 * Gets the pain killer quantity of the item.
 * @return The new pain killer quantity.
 */
int RuleItem::getPainKillerQuantity() const
{
	return _painKiller;
}

/**
 * Gets the stimulant quantity of the item.
 * @return The new stimulant quantity.
 */
int RuleItem::getStimulantQuantity() const
{
	return _stimulant;
}

/**
 * Gets the amount of fatal wound healed per usage.
 * @return The amount of fatal wound healed.
 */
int RuleItem::getWoundRecovery() const
{
	return _woundRecovery;
}

/**
 * Gets the amount of health added to a wounded soldier's health.
 * @return The amount of health to add.
 */
int RuleItem::getHealthRecovery() const
{
	return _healthRecovery;
}

/**
 * Gets the amount of energy added to a soldier's energy.
 * @return The amount of energy to add.
 */
int RuleItem::getEnergyRecovery() const
{
	return _energyRecovery;
}

/**
 * Gets the amount of stun removed from a soldier's stun level.
 * @return The amount of stun removed.
 */
int RuleItem::getStunRecovery() const
{
	return _stunRecovery;
}

/**
 * Returns the item's max explosion radius. Small explosions don't have a restriction.
 * Larger explosions are restricted using a formula, with a maximum of radius 10 no matter how large the explosion is.
 * @return The radius.
 */
int RuleItem::getExplosionRadius() const
{
	int radius = 0;

	if (_damageType.FixRadius == -1)
	{
		radius = _power * _damageType.RadiusEffectiveness;
		if (_damageType.FireBlastCalc)
		{
			radius += 1;
		}
		// cap the formula to 11
		if (radius > 11)
		{
			radius = 11;
		}
	}
	else
	{
		// unless a blast radius is actually defined.
		radius = _damageType.FixRadius;
	}

	return radius;
}

/**
 * Returns the item's recovery points.
 * This is used during the battlescape debriefing score calculation.
 * @return The recovery points.
 */
int RuleItem::getRecoveryPoints() const
{
	return _recoveryPoints;
}

/**
 * Returns the item's armor.
 * The item is destroyed when an explosion power bigger than its armor hits it.
 * @return The armor.
 */
int RuleItem::getArmor() const
{
	return _armor;
}

/**
 * Returns if the item should be recoverable
 * from the battlescape.
 * @return True if it is recoverable.
 */
bool RuleItem::isRecoverable() const
{
	return _recover;
}


/**
 * Returns the item's Turret Type.
 * @return The turret index (-1 for no turret).
 */
int RuleItem::getTurretType() const
{
	return _turretType;
}

/**
 * Returns if this is a live alien.
 * @return True if this is a live alien.
 */
bool RuleItem::isAlien() const
{
	return _liveAlien;
}

/**
 * Returns whether this item charges a flat TU rate.
 * @return True if this item charges a flat TU rate.
 */
bool RuleItem::getFlatRate() const
{
	return _flatRate;
}

/**
 * Returns if this weapon should arc its shots.
 * @return True if this weapon should arc its shots.
 */
bool RuleItem::getArcingShot() const
{
	return _arcingShot;
}

/**
 * Gets the attraction value for this item (for AI).
 * @return The attraction value.
 */
int RuleItem::getAttraction() const
{
	return _attraction;
}

/**
 * Gets the list weight for this research item
 * @return The list weight.
 */
int RuleItem::getListOrder() const
{
	return _listOrder;
}

/**
 * Gets the maximum range of this weapon
 * @return The maximum range.
 */
int RuleItem::getMaxRange() const
{
	return _maxRange;
}

/**
 * Gets the maximum effective range of this weapon when using Aimed Shot.
 * @return The maximum range.
 */
int RuleItem::getAimRange() const
{
	return _aimRange;
}

/**
 * Gets the maximim effective range of this weapon for Snap Shot.
 * @return The maximum range.
 */
int RuleItem::getSnapRange() const
{
	return _snapRange;
}

/**
 * Gets the maximim effective range of this weapon for Auto Shot.
 * @return The maximum range.
 */
int RuleItem::getAutoRange() const
{
	return _autoRange;
}

/**
 * Gets the minimum effective range of this weapon.
 * @return The minimum effective range.
 */
int RuleItem::getMinRange() const
{
	return _minRange;
}

/**
 * Gets the accuracy dropoff value of this weapon.
 * @return The per-tile dropoff.
 */
int RuleItem::getDropoff() const
{
	return _dropoff;
}

/**
 * Gets the speed at which this bullet travels.
 * @return The speed.
 */
int RuleItem::getBulletSpeed() const
{
	return _bulletSpeed;
}

/**
 * Gets the speed at which this bullet explodes.
 * @return The speed.
 */
int RuleItem::getExplosionSpeed() const
{
	return _explosionSpeed;
}

/**
* Gets the amount of auto shots fired by this weapon.
* @return The shots.
*/
int RuleItem::getAutoShots() const
{
	return _autoShots;
}

/**
 * Gets the name of psi attack for action popup list.
 * @return String Id.
 */
std::string RuleItem::getPsiAttackName() const
{
	return _psiAttackName;
}

/**
* is this item a rifle?
* @return whether or not it is a rifle.
*/
bool RuleItem::isRifle() const
{
	return (_battleType == BT_FIREARM || _battleType == BT_MELEE) && _twoHanded;
}

/**
* is this item a pistol?
* @return whether or not it is a pistol.
*/
bool RuleItem::isPistol() const
{
	return (_battleType == BT_FIREARM || _battleType == BT_MELEE) && !_twoHanded;
}

/**
 * Gets the number of projectiles this ammo shoots at once.
 * @return The number of projectiles.
 */
int RuleItem::getShotgunPellets() const
{
	return _shotgunPellets;
}

/**
 * Gets the unit that the victim is morphed into when attacked.
 * @return The weapon's zombie unit.
 */
std::string RuleItem::getZombieUnit() const
{
	return _zombieUnit;
}

/**
 * What sound does this weapon make when you swing this at someone?
 * @return The weapon's melee attack sound.
 */
int RuleItem::getMeleeAttackSound() const
{
	return _meleeSound;
}

/**
 * What sound does this weapon make when you punch someone in the face with it?
 * @return The weapon's melee hit sound.
 */
int RuleItem::getMeleeHitSound() const
{
	return _meleeHitSound;
}

/**
 * How much damage does this weapon do when you punch someone in the face with it?
 * @return The weapon's melee power.
 */
int RuleItem::getMeleePower() const
{
	return _meleePower;
}

/**
 * Is line of sight required for this psionic weapon to function?
 * @return If line of sight is required.
 */
bool RuleItem::isLOSRequired() const
{
	return _LOSRequired;
}

/**
 * What is the starting frame offset in hit.pck to use for the animation?
 * @return the starting frame offset in hit.pck to use for the animation.
 */
int RuleItem::getMeleeAnimation() const
{
	return _meleeAnimation;
}

/**
 * Can this item be used on land or is it underwater only?
 * @return if this is an underwater weapon or not.
 */
bool RuleItem::isWaterOnly() const
{
	return _underwaterOnly;
}

/**
 * Is psi skill is required to use this weapon.
 * @return If psi skill is required.
 */
bool RuleItem::isPsiRequired() const
{
	return _psiReqiured;
}
/**
 * Compute power bonus based on unit stats.
 * @param stats unit stats
 * @return bonus power.
 */
int RuleItem::getPowerBonus(const BattleUnit *unit) const
{
	return statsModiferRead(_damageBonus, unit);
}

/**
 * Compute multiplier of melee hit chance based on unit stats.
 * @param stats unit stats
 * @return multiplier.
 */
int RuleItem::getMeleeMultiplier(const BattleUnit *unit) const
{
	return statsModiferRead(_meleeMulti, unit);
}

/**
 * Compute multiplier of accuracy based on unit stats.
 * @param stats unit stats
 * @return multiplier.
 */
int RuleItem::getAccuracyMultiplier(const BattleUnit *unit) const
{
	return statsModiferRead(_accuracyMulti, unit);
}

/**
 * Compute multiplier of throw accuracy based on unit stats.
 * @param stats unit stats
 * @return multiplier.
 */
int RuleItem::getThrowMultiplier(const BattleUnit *unit) const
{
	return statsModiferRead(_throwMulti, unit);
}
/**
 * Gets the associated special type of this item.
 * note that type 14 is the alien brain, and types
 * 0 and 1 are "regular tile" and "starting point"
 * so try not to use those ones.
 * @return special type.
 */
int RuleItem::getSpecialType() const
{
	return _specialType;
}

/**
 * Gets the color offset to use for the vapor trail.
 * @return the color offset.
 */
int RuleItem::getVaporColor() const
{
	return _vaporColor;
}

/**
 * Gets the vapor cloud density for the vapor trail.
 * @return the vapor density.
 */
int RuleItem::getVaporDensity() const
{
	return _vaporDensity;
}

/**
 * Gets the vapor cloud probability for the vapor trail.
 * @return the vapor probability.
 */
int RuleItem::getVaporProbability() const
{
	return _vaporProbability;
}

}
