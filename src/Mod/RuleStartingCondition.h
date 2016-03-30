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
#ifndef OPENXCOM_RULESTARTINGCONDITION_H
#define OPENXCOM_RULESTARTINGCONDITION_H

#include <vector>
#include <string>
#include <yaml-cpp/yaml.h>

namespace OpenXcom
{

class Craft;

/**
 * Represents a specific Starting Condition.
 */
class RuleStartingCondition
{
private:
	std::string _type;
	std::map<std::string, std::string> _armorTransformations;
	std::string _defaultArmor;
	std::vector<std::string> _allowedArmors;
	std::map<std::string, int> _defaultItems;
	std::vector<std::string> _allowedItems;
	std::vector<std::string> _allowedCraft;
public:
	/// Creates a blank Starting Conditions ruleset.
	RuleStartingCondition(const std::string &type);
	/// Cleans up the Starting Conditions ruleset.
	~RuleStartingCondition();
	/// Loads Starting Conditions data from YAML.
	void load(const YAML::Node& node);
	/// Gets the Starting Conditions's type.
	std::string getType() const;
	/// Checks if the craft type is allowed
	bool isCraftAllowed(const std::string &craftType) const;
};

}
#endif
