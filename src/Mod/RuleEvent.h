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
#include <yaml-cpp/yaml.h>

namespace OpenXcom
{

/**
 * Represents a custom Geoscape event.
 * Events are spawned using Event Script ruleset.
 */
class RuleEvent
{
private:
	std::string _name, _description, _background;
	std::vector<std::string> _regionList;
	bool _city;
	int _points, _funds;
	bool _randomItem;
	std::vector<std::string> _itemList, _researchList;
	std::string _interruptResearch;
	int _timer, _timerRandom;
public:
	/// Creates a blank RuleEvent.
	RuleEvent(const std::string &name);
	/// Cleans up the event ruleset.
	~RuleEvent() = default;
	/// Loads the event definition from YAML.
	void load(const YAML::Node &node);
	/// Gets the event's name.
	const std::string &getName() const { return _name; }
	/// Gets the event's description.
	const std::string &getDescription() const { return _description; }
	/// Gets the event's background sprite name.
	const std::string &getBackground() const { return _background; }
	/// Gets a list of regions where this event can occur.
	const std::vector<std::string> &getRegionList() const { return _regionList; }
	/// Is this event city specific?
	bool isCitySpecific() const { return _city; }
	/// Gets the amount of score points awarded when this event pops up.
	int getPoints() const { return _points; }
	/// Gets the amount of funds awarded when this event pops up.
	int getFunds() const { return _funds; }
	/// Should all items be awarded or only one randomly?
	bool isRandomItem() const { return _randomItem; }
	/// Gets a list of items; they are transferred to HQ stores when this event pops up.
	const std::vector<std::string> &getItemList() const { return _itemList; }
	/// Gets a list of research projects; one of them will be randomly discovered when this event pops up.
	const std::vector<std::string> &getResearchList() const { return _researchList; }
	/// Gets the research project that will interrupt/terminate an already generated (but not yet popped up) event.
	const std::string &getInterruptResearch() const { return _interruptResearch; }
	/// Gets the timer of delay for this event, for it occurring after being spawned with eventScripts ruleset.
	int getTimer() const { return _timer; }
	/// Gets value for calculation of random part of delay for this event.
	int getTimerRandom() const { return _timerRandom; }
};

}
