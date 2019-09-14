/*
 * Copyright 2010-2019 OpenXcom Developers.
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
#include "RuleSoldierBonus.h"

namespace OpenXcom
{

RuleSoldierBonus::RuleSoldierBonus(const std::string &name) : _name(name), _visibilityAtDark(0)
{
}

/**
 * Loads the soldier bonus definition from YAML.
 * @param node YAML node.
 */
void RuleSoldierBonus::load(const YAML::Node &node, const ModScript &parsers)
{
	if (const YAML::Node &parent = node["refNode"])
	{
		load(parent, parsers);
	}
	_name = node["name"].as<std::string>(_name);
	_visibilityAtDark = node["visibilityAtDark"].as<int>(_visibilityAtDark);
	_stats.merge(node["stats"].as<UnitStats>(_stats));
	const YAML::Node &rec = node["recovery"];
	{
		_timeRecovery.load(_name, rec, parsers.bonusStatsScripts.get<ModScript::TimeRecoveryStatBonus>());
		_energyRecovery.load(_name, rec, parsers.bonusStatsScripts.get<ModScript::EnergyRecoveryStatBonus>());
		_moraleRecovery.load(_name, rec, parsers.bonusStatsScripts.get<ModScript::MoraleRecoveryStatBonus>());
		_healthRecovery.load(_name, rec, parsers.bonusStatsScripts.get<ModScript::HealthRecoveryStatBonus>());
		_manaRecovery.load(_name, rec, parsers.bonusStatsScripts.get<ModScript::ManaRecoveryStatBonus>());
		_stunRecovery.load(_name, rec, parsers.bonusStatsScripts.get<ModScript::StunRecoveryStatBonus>());
	}
}

/**
 *  Gets the bonus TU recovery.
 */
int RuleSoldierBonus::getTimeRecovery(const BattleUnit *unit) const
{
	return _timeRecovery.getBonus(unit);
}

/**
 *  Gets the bonus Energy recovery.
 */
int RuleSoldierBonus::getEnergyRecovery(const BattleUnit *unit) const
{
	return _energyRecovery.getBonus(unit);
}

/**
 *  Gets the bonus Morale recovery.
 */
int RuleSoldierBonus::getMoraleRecovery(const BattleUnit *unit) const
{
	return _moraleRecovery.getBonus(unit);
}

/**
 *  Gets the bonus Health recovery.
 */
int RuleSoldierBonus::getHealthRecovery(const BattleUnit *unit) const
{
	return _healthRecovery.getBonus(unit);
}

/**
 *  Gets the bonus Mana recovery.
 */
int RuleSoldierBonus::getManaRecovery(const BattleUnit *unit) const
{
	return _manaRecovery.getBonus(unit);
}

/**
 *  Gets the bonus Stun recovery.
 */
int RuleSoldierBonus::getStunRegeneration(const BattleUnit *unit) const
{
	return _stunRecovery.getBonus(unit);
}

}
