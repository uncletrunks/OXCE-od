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
#include "RuleStartingCondition.h"
#include <algorithm>

namespace OpenXcom
{

/**
 * Creates a blank ruleset for a certain type of Starting Condition.
 * @param type String defining the type.
 */
RuleStartingCondition::RuleStartingCondition(const std::string &type) : _type(type)
{
}

/**
 *
 */
RuleStartingCondition::~RuleStartingCondition()
{
}

/**
 * Loads the Starting Conditions from a YAML file.
 * @param node YAML node.
 */
void RuleStartingCondition::load(const YAML::Node &node)
{
	if (const YAML::Node &parent = node["refNode"])
	{
		load(parent);
	}
	_type = node["type"].as<std::string>(_type);
	_allowedCraft = node["allowedCraft"].as< std::vector<std::string> >(_allowedCraft);
}

/**
 * Returns the language string that names the Starting Condition. Each type has a unique name.
 * @return Starting Condition name.
 */
std::string RuleStartingCondition::getType() const
{
	return _type;
}

/**
* Gets bonus statistic of UFO based on race.
* @param s Race name.
* @return Bonus stats.
*/
bool RuleStartingCondition::isCraftAllowed(const std::string &craftType) const
{
	return _allowedCraft.empty() || (std::find(_allowedCraft.begin(), _allowedCraft.end(), craftType) != _allowedCraft.end());
}

}
