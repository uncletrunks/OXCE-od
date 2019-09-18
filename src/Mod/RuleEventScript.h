#pragma once
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
#include <string>
#include <vector>
#include <map>
#include <yaml-cpp/yaml.h>
#include "../Savegame/WeightedOptions.h"

namespace OpenXcom
{

class RuleEventScript
{
private:
	std::string _type;
	std::vector<std::pair<size_t, WeightedOptions*> > _eventWeights;
	int _firstMonth, _lastMonth, _executionOdds, _minDifficulty, _maxDifficulty;
	std::map<std::string, bool> _researchTriggers;
public:
	/// Creates a blank RuleEventScript.
	RuleEventScript(const std::string &type);
	/// Cleans up the event script ruleset.
	~RuleEventScript();
	/// Loads an event script from YAML.
	void load(const YAML::Node &node);
	/// Gets the name of the script command.
	const std::string &getType() const { return _type; }
	/// Gets the first month this command will run.
	int getFirstMonth() const { return _firstMonth; }
	/// Gets the last month this command will run.
	int getLastMonth() const { return _lastMonth; }
	/// Gets the odds of this command executing.
	int getExecutionOdds() const { return _executionOdds; }
	/// Gets the minimum difficulty for this command to run.
	int getMinDifficulty() const { return _minDifficulty; }
	/// Gets the maximum difficulty for this command to run.
	int getMaxDifficulty() const { return _maxDifficulty; }
	/// Gets the research triggers that may apply to this command.
	const std::map<std::string, bool> &getResearchTriggers() const { return _researchTriggers; }
	/// Generates an event based on the month.
	std::string generate(const size_t monthsPassed) const;
};

}