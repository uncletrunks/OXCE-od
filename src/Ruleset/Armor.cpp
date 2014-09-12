/*
 * Copyright 2010-2014 OpenXcom Developers.
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
#include "Armor.h"

namespace OpenXcom
{

/**
 * Creates a blank ruleset for a certain
 * type of armor.
 * @param type String defining the type.
 */
Armor::Armor(const std::string &type) :
	_type(type), _spriteSheet(""), _spriteInv(""), _corpseGeo(""), _storeItem(""), _corpseBattle(),
	_frontArmor(0), _sideArmor(0), _rearArmor(0), _underArmor(0),
	_drawingRoutine(0), _movementType(MT_WALK), _size(1), _weight(0), _visibilityAtDark(20),
	_deathFrames(3), _constantAnimation(false), _canHoldWeapon(false),
	_fearImmune(-1), _bleedImmune(-1), _painImmune(-1), _zombiImmune(-1)
{
	for (int i=0; i < DAMAGE_TYPES; i++)
		_damageModifier[i] = 1.0f;
}

/**
 *
 */
Armor::~Armor()
{

}

/**
 * Loads the armor from a YAML file.
 * @param node YAML node.
 */
void Armor::load(const YAML::Node &node)
{
	_type = node["type"].as<std::string>(_type);
	_spriteSheet = node["spriteSheet"].as<std::string>(_spriteSheet);
	_spriteInv = node["spriteInv"].as<std::string>(_spriteInv);
	if (node["corpseItem"])
	{
		_corpseBattle.clear();
		_corpseBattle.push_back(node["corpseItem"].as<std::string>());
		_corpseGeo = _corpseBattle[0];
	}
	else if (node["corpseBattle"])
	{
		_corpseBattle = node["corpseBattle"].as< std::vector<std::string> >();
		_corpseGeo = _corpseBattle[0];
	}
	_corpseGeo = node["corpseGeo"].as<std::string>(_corpseGeo);
	_storeItem = node["storeItem"].as<std::string>(_storeItem);
	_frontArmor = node["frontArmor"].as<int>(_frontArmor);
	_sideArmor = node["sideArmor"].as<int>(_sideArmor);
	_rearArmor = node["rearArmor"].as<int>(_rearArmor);
	_underArmor = node["underArmor"].as<int>(_underArmor);
	_drawingRoutine = node["drawingRoutine"].as<int>(_drawingRoutine);
	_movementType = (MovementType)node["movementType"].as<int>(_movementType);
	_size = node["size"].as<int>(_size);
	_weight = node["weight"].as<int>(_weight);
	_visibilityAtDark = node["visibilityAtDark"].as<int>(_visibilityAtDark);
	_stats.merge(node["stats"].as<UnitStats>(_stats));
	if (const YAML::Node &dmg = node["damageModifier"])
	{
		for (size_t i = 0; i < dmg.size() && i < DAMAGE_TYPES; ++i)
		{
			_damageModifier[i] = dmg[i].as<float>();
		}
	}
	_loftempsSet = node["loftempsSet"].as< std::vector<int> >(_loftempsSet);
	if (node["loftemps"])
		_loftempsSet.push_back(node["loftemps"].as<int>());
	_deathFrames = node["deathFrames"].as<int>(_deathFrames);
	_constantAnimation = node["constantAnimation"].as<bool>(_constantAnimation);
	if (_drawingRoutine == 0 ||
		_drawingRoutine == 1 ||
		_drawingRoutine == 4 ||
		_drawingRoutine == 6 ||
		_drawingRoutine == 10 ||
		_drawingRoutine == 13 ||
		_drawingRoutine == 14 ||
		_drawingRoutine == 16 ||
		_drawingRoutine == 17)
	{
		_canHoldWeapon = true;
	}
	else
	{
		_canHoldWeapon = false;
	}
	if (_size != 1)
	{
		_fearImmune = 1;
		_bleedImmune = 1;
		_painImmune = 1;
		_zombiImmune = 1;
	}
	if (node["fearImmune"])
	{
		_fearImmune = node["fearImmune"].as<bool>();
	}
	if (node["bleedImmune"])
	{
		_bleedImmune = node["bleedImmune"].as<bool>();
	}
	if (node["painImmune"])
	{
		_painImmune = node["painImmune"].as<bool>();
	}
	if (node["zombiImmune"] && _size == 1) //Big units are always immune, because game we don't have 2x2 unit zombie
	{
		_zombiImmune = node["zombiImmune"].as<bool>();
	}
}

/**
 * Returns the language string that names
 * this armor. Each armor has a unique name. Coveralls, Power Suit,...
 * @return The armor name.
 */
std::string Armor::getType() const
{
	return _type;
}

/**
 * Gets the unit's sprite sheet.
 * @return The sprite sheet name.
 */
std::string Armor::getSpriteSheet() const
{
	return _spriteSheet;
}

/**
 * Gets the unit's inventory sprite.
 * @return The inventory sprite name.
 */
std::string Armor::getSpriteInventory() const
{
	return _spriteInv;
}

/**
 * Gets the front armor level.
 * @return The front armor level.
 */
int Armor::getFrontArmor() const
{
	return _frontArmor;
}

/**
 * Gets the side armor level.
 * @return The side armor level.
 */
int Armor::getSideArmor() const
{
	return _sideArmor;
}

/**
 * Gets the rear armor level.
 * @return The rear armor level.
 */
int Armor::getRearArmor() const
{
	return _rearArmor;
}

/**
 * Gets the under armor level.
 * @return The under armor level.
 */
int Armor::getUnderArmor() const
{
	return _underArmor;
}


/**
 * Gets the corpse item used in the Geoscape.
 * @return The name of the corpse item.
 */
std::string Armor::getCorpseGeoscape() const
{
	return _corpseGeo;
}

/**
 * Gets the list of corpse items dropped by the unit
 * in the Battlescape (one per unit tile).
 * @return The list of corpse items.
 */
const std::vector<std::string> &Armor::getCorpseBattlescape() const
{
	return _corpseBattle;
}

/**
 * Gets the storage item needed to equip this.
 * @return The name of the store item.
 */
std::string Armor::getStoreItem() const
{
	return _storeItem;
}

/**
 * Gets the drawing routine ID.
 * @return The drawing routine ID.
 */
int Armor::getDrawingRoutine() const
{
	return _drawingRoutine;
}

/**
 * Gets the movement type of this armor.
 * Useful for determining whether the armor can fly.
 * @return The movement type.
 */
MovementType Armor::getMovementType() const
{
	return _movementType;
}

/**
 * Gets the size of the unit. Normally this is 1 (small) or 2 (big).
 * @return The unit's size.
 */
int Armor::getSize() const
{
	return _size;
}

/**
 * Gets the damage modifier for a certain damage type.
 * @param dt The damageType.
 * @return The damage modifier 0->1.
 */
float Armor::getDamageModifier(ItemDamageType dt) const
{
	return _damageModifier[(int)dt];
}

/** Gets the loftempSet.
 * @return The loftempsSet.
 */
const std::vector<int>& Armor::getLoftempsSet() const
{
	return _loftempsSet;
}

/**
  * Gets pointer to the armor's stats.
  * @return stats Pointer to the armor's stats.
  */
const UnitStats *Armor::getStats() const
{
	return &_stats;
}

/**
 * Gets the armor's weight.
 * @return the weight of the armor.
 */
int Armor::getWeight() const
{
	return _weight;
}

/**
 * Gets number of death frames.
 * @return number of death frames.
 */
int Armor::getDeathFrames() const
{
	return _deathFrames;
}

/**
 * Gets if armor uses constant animation.
 * @return if it uses constant animation
 */
bool Armor::getConstantAnimation() const
{
	return _constantAnimation;
}

/**
 * Gets if armor can hold weapon.
 * @return if it can hold weapon
 */
bool Armor::getCanHoldWeapon() const
{
	return _canHoldWeapon;
}

/**
 * Gets max view distance at dark in BattleScape.
 * @return The distance to see at dark.
 */
int Armor::getVisibilityAtDark() const
{
	return _visibilityAtDark;
}

/**
 * Gets how armor react to fear.
 * @param def Default value.
 * @return Can ignored fear?
 */
bool Armor::getFearImmune(bool def) const
{
	return _fearImmune != -1 ? _fearImmune : def;
}

/**
 * Gets how armor react to bleeding.
 * @param def Default value.
 * @return Can ignore bleed?
 */
bool Armor::getBleedImmune(bool def) const
{
	return _bleedImmune != -1 ? _bleedImmune : def;
}

/**
 * Gets how armor react to inflicted pain.
 * @param def
 * @return Can ignore pain?
 */
bool Armor::getPainImmune(bool def) const
{
	return _painImmune != -1 ? _painImmune : def;
}

/**
 * Gets how armor react to zombification.
 * @param def Default value.
 * @return Can't be turn to zombie?
 */
bool Armor::getZombiImmune(bool def) const
{
	return _zombiImmune != -1 ? _zombiImmune : def;
}


}
