#pragma once
/*
 * Copyright 2010-2018 OpenXcom Developers.
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
#include <map>
#include <yaml-cpp/yaml.h>
#include "Unit.h"

namespace OpenXcom
{

/**
 * Ruleset data structure for the information to transform a soldier.
 */
class RuleSoldierTransformation
{
private:
	std::string _name;
	std::vector<std::string > _requires, _requiredPreviousTransformations, _forbiddenPreviousTransformations, _requiresBaseFunc;
	std::string _producedSoldierType, _producedSoldierArmor;
	bool _keepSoldierArmor, _createsClone, _needsCorpseRecovered, _allowsDeadSoldiers, _allowsLiveSoldiers, _allowsWoundedSoldiers;
	std::vector<std::string > _allowedSoldierTypes;
	std::map<std::string, int> _requiredItems;
	int _listOrder, _cost, _transferTime, _recoveryTime;
	UnitStats _requiredMinStats, _flatOverallStatChange, _percentOverallStatChange, _percentGainedStatChange;
	bool _useRandomStats, _lowerBoundAtMinStats, _upperBoundAtMaxStats, _upperBoundAtStatCaps;

public:
	/// Default constructor
	RuleSoldierTransformation(const std::string &name);
	/// Loads the project data from YAML
	void load(const YAML::Node& node, int listOrder);
	/// Gets the unique name id of the project
	const std::string &getName() const;
	/// Gets the list weight of the project
	int getListOrder() const;
	/// Gets the list of research this project requires
	const std::vector<std::string > &getRequiredResearch() const;
	/// Gets the list of required base functions for this project
	const std::vector<std::string > &getRequiredBaseFuncs() const;
	/// Gets the type of soldier produced by this project
	const std::string &getProducedSoldierType() const;
	/// Gets the armor that the produced soldier should be wearing
	const std::string &getProducedSoldierArmor() const;
	/// Gets whether or not the soldier should keep their armor after the project
	bool isKeepingSoldierArmor() const;
	/// Gets whether or not the project should produce a clone (new id) of the input soldier
	bool isCreatingClone() const;
	/// Gets whether or not the project needs the body of the soldier to have been recovered
	bool needsCorpseRecovered() const;
	/// Gets whether or not the project allows input of dead soldiers
	bool isAllowingDeadSoldiers() const;
	/// Gets whether or not the project allows input of alive soldiers
	bool isAllowingAliveSoldiers() const;
	/// Gets whether or not the project allows input of wounded soldiers
	bool isAllowingWoundedSoldiers() const;
	/// Gets the list of soldier types eligible for this project
	const std::vector<std::string > &getAllowedSoldierTypes() const;
	/// Gets the list of previous soldier transformations a soldier needs for this project
	const std::vector<std::string > &getRequiredPreviousTransformations() const;
	/// Gets the list of previous soldier transformations that make a soldier ineligible for this project
	const std::vector<std::string > &getForbiddenPreviousTransformations() const;
	/// Gets the minimum stats a soldier needs to be eligible for this project
	const UnitStats &getRequiredMinStats() const;
	/// Gets the list of items necessary to complete this project
	const std::map<std::string, int> &getRequiredItems() const;
	/// Gets the cash cost of the project
	int getCost() const;
	/// Gets how long the transformed soldier should be in transit to the base after completion
	int getTransferTime() const;
	/// Gets how long the transformed soldier should take to recover after completion
	int getRecoveryTime() const;
	/// Gets the flat change to a soldier's overall stats when undergoing this project
	const UnitStats &getFlatOverallStatChange() const;
	/// Gets the percent change to a soldier's overall stats when undergoing this project
	const UnitStats &getPercentOverallStatChange() const;
	/// Gets the percent change to a soldier's gained stats when undergoing this project
	const UnitStats &getPercentGainedStatChange() const;
	/// Gets whether or not this project should use randomized stats from the produced RuleSoldier or the input soldier's stats
	bool isUsingRandomStats() const;
	/// Gets whether or not this project should bound stat penalties at the produced RuleSoldier's minStats
	bool hasLowerBoundAtMinStats() const;
	/// Gets whether or not this project should cap stats at the produced RuleSoldier's maxStats
	bool hasUpperBoundAtMaxStats() const;
	/// Gets whether or not this project should cap stats at the produced RuleSoldier's statCaps
	bool hasUpperBoundAtStatCaps() const;
};

}
