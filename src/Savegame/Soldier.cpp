/*
 * Copyright 2010-2016 OpenXcom Developers.
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
#include <cmath>
#include <algorithm>
#include "Soldier.h"
#include "../Engine/RNG.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/ScriptBind.h"
#include "Craft.h"
#include "EquipmentLayoutItem.h"
#include "SoldierDeath.h"
#include "SoldierDiary.h"
#include "../Mod/SoldierNamePool.h"
#include "../Mod/RuleSoldier.h"
#include "../Mod/Armor.h"
#include "../Mod/Mod.h"
#include "../Mod/StatString.h"
#include "../Mod/RuleSoldierTransformation.h"

namespace OpenXcom
{

/**
 * Initializes a new soldier, either blank or randomly generated.
 * @param rules Soldier ruleset.
 * @param armor Soldier armor.
 * @param id Unique soldier id for soldier generation.
 */
Soldier::Soldier(RuleSoldier *rules, Armor *armor, int id) :
	_id(id), _nationality(0),
	_improvement(0), _psiStrImprovement(0), _rules(rules), _rank(RANK_ROOKIE), _craft(0),
	_gender(GENDER_MALE), _look(LOOK_BLONDE), _lookVariant(0), _missions(0), _kills(0), _recovery(0.0f),
	_recentlyPromoted(false), _psiTraining(false), _training(false),
	_armor(armor), _replacedArmor(0), _transformedArmor(0), _death(0), _diary(new SoldierDiary()),
	_corpseRecovered(false)
{
	if (id != 0)
	{
		UnitStats minStats = rules->getMinStats();
		UnitStats maxStats = rules->getMaxStats();

		_initialStats.tu = RNG::generate(minStats.tu, maxStats.tu);
		_initialStats.stamina = RNG::generate(minStats.stamina, maxStats.stamina);
		_initialStats.health = RNG::generate(minStats.health, maxStats.health);
		_initialStats.bravery = RNG::generate(minStats.bravery/10, maxStats.bravery/10)*10;
		_initialStats.reactions = RNG::generate(minStats.reactions, maxStats.reactions);
		_initialStats.firing = RNG::generate(minStats.firing, maxStats.firing);
		_initialStats.throwing = RNG::generate(minStats.throwing, maxStats.throwing);
		_initialStats.strength = RNG::generate(minStats.strength, maxStats.strength);
		_initialStats.psiStrength = RNG::generate(minStats.psiStrength, maxStats.psiStrength);
		_initialStats.melee = RNG::generate(minStats.melee, maxStats.melee);
		_initialStats.psiSkill = minStats.psiSkill;

		_currentStats = _initialStats;

		const std::vector<SoldierNamePool*> &names = rules->getNames();
		if (!names.empty())
		{
			_nationality = RNG::generate(0, names.size() - 1);
			_name = names.at(_nationality)->genName(&_gender, rules->getFemaleFrequency());
			_look = (SoldierLook)names.at(_nationality)->genLook(4); // Once we add the ability to mod in extra looks, this will need to reference the ruleset for the maximum amount of looks.
		}
		else
		{
			// No possible names, just wing it
			_gender = (RNG::percent(rules->getFemaleFrequency()) ? GENDER_FEMALE : GENDER_MALE);
			_look = (SoldierLook)RNG::generate(0,3);
			_name = (_gender == GENDER_FEMALE) ? L"Jane" : L"John";
			_name += L" Doe";
		}
	}
	_lookVariant = RNG::seedless(0, 15);
}

/**
 *
 */
Soldier::~Soldier()
{
	for (std::vector<EquipmentLayoutItem*>::iterator i = _equipmentLayout.begin(); i != _equipmentLayout.end(); ++i)
	{
		delete *i;
	}
	delete _death;
	delete _diary;
}

/**
 * Loads the soldier from a YAML file.
 * @param node YAML node.
 * @param mod Game mod.
 * @param save Pointer to savegame.
 */
void Soldier::load(const YAML::Node& node, const Mod *mod, SavedGame *save, const ScriptGlobal *shared)
{
	_id = node["id"].as<int>(_id);
	_name = Language::utf8ToWstr(node["name"].as<std::string>());
	_nationality = node["nationality"].as<int>(_nationality);
	_initialStats = node["initialStats"].as<UnitStats>(_initialStats);
	_currentStats = node["currentStats"].as<UnitStats>(_currentStats);
	_rank = (SoldierRank)node["rank"].as<int>();
	_gender = (SoldierGender)node["gender"].as<int>();
	_look = (SoldierLook)node["look"].as<int>();
	_lookVariant = node["lookVariant"].as<int>(_lookVariant);
	_missions = node["missions"].as<int>(_missions);
	_kills = node["kills"].as<int>(_kills);
	_recovery = node["recovery"].as<float>(_recovery);
	Armor *armor = mod->getArmor(node["armor"].as<std::string>());
	if (armor == 0)
	{
		armor = mod->getArmor(mod->getSoldier(mod->getSoldiersList().front())->getArmor());
	}
	_armor = armor;
	if (node["replacedArmor"])
		_replacedArmor = mod->getArmor(node["replacedArmor"].as<std::string>());;
	if (node["transformedArmor"])
		_transformedArmor = mod->getArmor(node["transformedArmor"].as<std::string>());;
	_psiTraining = node["psiTraining"].as<bool>(_psiTraining);
	_training = node["training"].as<bool>(_training);
	_improvement = node["improvement"].as<int>(_improvement);
	_psiStrImprovement = node["psiStrImprovement"].as<int>(_psiStrImprovement);
	if (const YAML::Node &layout = node["equipmentLayout"])
	{
		for (YAML::const_iterator i = layout.begin(); i != layout.end(); ++i)
		{
			EquipmentLayoutItem *layoutItem = new EquipmentLayoutItem(*i);
			if (mod->getInventory(layoutItem->getSlot()))
			{
				_equipmentLayout.push_back(layoutItem);
			}
			else
			{
				delete layoutItem;
			}
		}
	}
	if (node["death"])
	{
		_death = new SoldierDeath();
		_death->load(node["death"]);
	}
	if (node["diary"])
	{
		_diary = new SoldierDiary();
		_diary->load(node["diary"], mod);
	}
	calcStatString(mod->getStatStrings(), (Options::psiStrengthEval && save->isResearched(mod->getPsiRequirements())));
	_corpseRecovered = node["corpseRecovered"].as<bool>(_corpseRecovered);
	_previousTransformations = node["previousTransformations"].as<std::map<std::string, int > >(_previousTransformations);
	_scriptValues.load(node, shared);
}

/**
 * Saves the soldier to a YAML file.
 * @return YAML node.
 */
YAML::Node Soldier::save(const ScriptGlobal *shared) const
{
	YAML::Node node;
	node["type"] = _rules->getType();
	node["id"] = _id;
	node["name"] = Language::wstrToUtf8(_name);
	node["nationality"] = _nationality;
	node["initialStats"] = _initialStats;
	node["currentStats"] = _currentStats;
	node["rank"] = (int)_rank;
	if (_craft != 0)
	{
		node["craft"] = _craft->saveId();
	}
	node["gender"] = (int)_gender;
	node["look"] = (int)_look;
	node["lookVariant"] = _lookVariant;
	node["missions"] = _missions;
	node["kills"] = _kills;
	if (_recovery > 0.0f)
		node["recovery"] = _recovery;
	node["armor"] = _armor->getType();
	if (_replacedArmor != 0)
		node["replacedArmor"] = _replacedArmor->getType();
	if (_transformedArmor != 0)
		node["transformedArmor"] = _transformedArmor->getType();
	if (_psiTraining)
		node["psiTraining"] = _psiTraining;
	if (_training)
		node["training"] = _training;
	node["improvement"] = _improvement;
	node["psiStrImprovement"] = _psiStrImprovement;
	if (!_equipmentLayout.empty())
	{
		for (std::vector<EquipmentLayoutItem*>::const_iterator i = _equipmentLayout.begin(); i != _equipmentLayout.end(); ++i)
			node["equipmentLayout"].push_back((*i)->save());
	}
	if (_death != 0)
	{
		node["death"] = _death->save();
	}
	if (Options::soldierDiaries && (!_diary->getMissionIdList().empty() || !_diary->getSoldierCommendations()->empty()))
	{
		node["diary"] = _diary->save();
	}
	node["corpseRecovered"] = _corpseRecovered;
	node["previousTransformations"] = _previousTransformations;
	_scriptValues.save(node, shared);

	return node;
}

/**
 * Returns the soldier's full name (and, optionally, statString).
 * @param statstring Add stat string?
 * @param maxLength Restrict length to a certain value.
 * @return Soldier name.
 */
std::wstring Soldier::getName(bool statstring, unsigned int maxLength) const
{
	if (statstring && !_statString.empty())
	{
		if (_name.length() + _statString.length() > maxLength)
		{
			return _name.substr(0, maxLength - _statString.length()) + L"/" + _statString;
		}
		else
		{
			return _name + L"/" + _statString;
		}
	}
	else
	{
		return _name;
	}
}

/**
 * Changes the soldier's full name.
 * @param name Soldier name.
 */
void Soldier::setName(const std::wstring &name)
{
	_name = name;
}

/**
* Returns the soldier's nationality.
* @return Nationality ID.
*/
int Soldier::getNationality() const
{
	return _nationality;
}

/**
* Changes the soldier's nationality.
* @param nationality Nationality ID.
*/
void Soldier::setNationality(int nationality)
{
	_nationality = nationality;
}

/**
 * Returns the craft the soldier is assigned to.
 * @return Pointer to craft.
 */
Craft *Soldier::getCraft() const
{
	return _craft;
}

/**
 * Assigns the soldier to a new craft.
 * @param craft Pointer to craft.
 */
void Soldier::setCraft(Craft *craft)
{
	_craft = craft;
}

/**
 * Returns the soldier's craft string, which
 * is either the soldier's wounded status,
 * the assigned craft name, or none.
 * @param lang Language to get strings from.
 * @return Full name.
 */
std::wstring Soldier::getCraftString(Language *lang, float absBonus, float relBonus) const
{
	std::wstring s;
	if (_death)
	{
		if (_death->getCause())
		{
			s = lang->getString("STR_KILLED_IN_ACTION", _gender);
		}
		else
		{
			s = lang->getString("STR_MISSING_IN_ACTION", _gender);
		}
	}
	else if (isWounded())
	{
		std::wostringstream ss;
		ss << lang->getString("STR_WOUNDED");
		ss << L">";
		ss << getWoundRecovery(absBonus, relBonus);
		s = ss.str();
	}
	else if (_craft == 0)
	{
		s = lang->getString("STR_NONE_UC");
	}
	else
	{
		s = _craft->getName(lang);
	}
	return s;
}

/**
 * Returns a localizable-string representation of
 * the soldier's military rank.
 * @return String ID for rank.
 */
std::string Soldier::getRankString() const
{
	const std::vector<std::string> &rankStrings = _rules->getRankStrings();
	if (!_rules->getAllowPromotion())
	{
		// even if promotion is not allowed, we allow to use a different "Rookie" translation per soldier type
		if (rankStrings.empty())
		{
			return "STR_RANK_NONE";
		}
	}

	switch (_rank)
	{
	case RANK_ROOKIE:
		if (rankStrings.size() > 0)
			return rankStrings.at(0);
		return "STR_ROOKIE";
	case RANK_SQUADDIE:
		if (rankStrings.size() > 1)
			return rankStrings.at(1);
		return "STR_SQUADDIE";
	case RANK_SERGEANT:
		if (rankStrings.size() > 2)
			return rankStrings.at(2);
		return "STR_SERGEANT";
	case RANK_CAPTAIN:
		if (rankStrings.size() > 3)
			return rankStrings.at(3);
		return "STR_CAPTAIN";
	case RANK_COLONEL:
		if (rankStrings.size() > 4)
			return rankStrings.at(4);
		return "STR_COLONEL";
	case RANK_COMMANDER:
		if (rankStrings.size() > 5)
			return rankStrings.at(5);
		return "STR_COMMANDER";
	default:
		return "";
	}
}

/**
 * Returns a graphic representation of
 * the soldier's military rank from BASEBITS.PCK.
 * @note THE MEANING OF LIFE (is now obscured as a default)
 * @return Sprite ID for rank.
 */
int Soldier::getRankSprite() const
{
	return _rules->getRankSprite() + _rank;
}

/**
 * Returns a graphic representation of
 * the soldier's military rank from SMOKE.PCK
 * @return Sprite ID for rank.
 */
int Soldier::getRankSpriteBattlescape() const
{
	return _rules->getRankSpriteBattlescape() + _rank;
}

/**
 * Returns a graphic representation of
 * the soldier's military rank from TinyRanks
 * @return Sprite ID for rank.
 */
int Soldier::getRankSpriteTiny() const
{
	return _rules->getRankSpriteTiny() + _rank;
}

/**
 * Returns the soldier's military rank.
 * @return Rank enum.
 */
SoldierRank Soldier::getRank() const
{
	return _rank;
}

/**
 * Increase the soldier's military rank.
 */
void Soldier::promoteRank()
{
	if (!_rules->getAllowPromotion())
		return;

	const std::vector<std::string> &rankStrings = _rules->getRankStrings();
	if (!rankStrings.empty())
	{
		// stop if the soldier already has the maximum possible rank for his soldier type
		if ((int)_rank >= rankStrings.size() - 1)
		{
			return;
		}
	}

	_rank = (SoldierRank)((int)_rank + 1);
	if (_rank > RANK_SQUADDIE)
	{
		// only promotions above SQUADDIE are worth to be mentioned
		_recentlyPromoted = true;
	}
}

/**
 * Returns the soldier's amount of missions.
 * @return Missions.
 */
int Soldier::getMissions() const
{
	return _missions;
}

/**
 * Returns the soldier's amount of kills.
 * @return Kills.
 */
int Soldier::getKills() const
{
	return _kills;
}

/**
 * Returns the soldier's gender.
 * @return Gender.
 */
SoldierGender Soldier::getGender() const
{
	return _gender;
}

/**
 * Changes the soldier's gender (1/3 of avatar).
 * @param gender Gender.
 */
void Soldier::setGender(SoldierGender gender)
{
	_gender = gender;
}

/**
 * Returns the soldier's look.
 * @return Look.
 */
SoldierLook Soldier::getLook() const
{
	return _look;
}

/**
 * Changes the soldier's look (2/3 of avatar).
 * @param look Look.
 */
void Soldier::setLook(SoldierLook look)
{
	_look = look;
}

/**
 * Returns the soldier's look sub type.
 * @return Look.
 */
int Soldier::getLookVariant() const
{
	return _lookVariant;
}

/**
 * Changes the soldier's look variant (3/3 of avatar).
 * @param lookVariant Look sub type.
 */
void Soldier::setLookVariant(int lookVariant)
{
	_lookVariant = lookVariant;
}

/**
 * Returns the soldier's rules.
 * @return rulesoldier
 */
RuleSoldier *Soldier::getRules() const
{
	return _rules;
}

/**
 * Returns the soldier's unique ID. Each soldier
 * can be identified by its ID. (not it's name)
 * @return Unique ID.
 */
int Soldier::getId() const
{
	return _id;
}

/**
 * Add a mission to the counter.
 */
void Soldier::addMissionCount()
{
	_missions++;
}

/**
 * Add a kill to the counter.
 */
void Soldier::addKillCount(int count)
{
	_kills += count;
}

/**
 * Get pointer to initial stats.
 */
UnitStats *Soldier::getInitStats()
{
	return &_initialStats;
}

/**
 * Get pointer to current stats.
 */
UnitStats *Soldier::getCurrentStats()
{
	return &_currentStats;
}

void Soldier::setBothStats(UnitStats *stats)
{
	_currentStats = *stats;
	_initialStats = *stats;
}

/**
 * Returns the unit's promotion status and resets it.
 * @return True if recently promoted, False otherwise.
 */
bool Soldier::isPromoted()
{
	bool promoted = _recentlyPromoted;
	_recentlyPromoted = false;
	return promoted;
}

/**
 * Returns the unit's current armor.
 * @return Pointer to armor data.
 */
Armor *Soldier::getArmor() const
{
	return _armor;
}

/**
 * Changes the unit's current armor.
 * @param armor Pointer to armor data.
 */
void Soldier::setArmor(Armor *armor)
{
	_armor = armor;
}

/**
 * Returns a list of armor layers (sprite names).
 */
const std::vector<std::string> Soldier::getArmorLayers(Armor *customArmor) const
{
	std::vector<std::string> ret;
	std::stringstream ss;

	const Armor *armor = customArmor ? customArmor : _armor;

	const std::string gender = _gender == GENDER_MALE ? "M" : "F";
	auto defaultPrefix = armor->getLayersDefaultPrefix();
	auto specificPrefix = armor->getLayersSpecificPrefix();
	auto layoutDefinition = armor->getLayersDefinition();
	std::vector<std::string> relevantLayer;
	int layerIndex = 0;
	bool isDefined = false;

	// find relevant layer
	for (int i = 0; i <= 4; ++i)
	{
		ss.str("");
		ss << gender;
		ss << (int)_look + (_lookVariant & (15 >> i)) * 4;
		isDefined = (layoutDefinition.find(ss.str()) != layoutDefinition.end());
		if (isDefined)
		{
			relevantLayer = layoutDefinition[ss.str()];
			break;
		}
	}
	if (!isDefined)
	{
		throw Exception("Layered armor sprite definition (" + armor->getType() + ") not found!");
	}
	for (auto layerItem : relevantLayer)
	{
		if (!layerItem.empty())
		{
			ss.str("");
			if (specificPrefix.find(layerIndex) != specificPrefix.end())
			{
				ss << specificPrefix[layerIndex];
			}
			else
			{
				ss << defaultPrefix;
			}
			ss << "__" << layerIndex << "__" << layerItem;
			ret.push_back(ss.str());
		}
		layerIndex++;
	}

	return ret;
}

/**
* Gets the soldier's original armor (before replacement).
* @return Pointer to armor data.
*/
Armor *Soldier::getReplacedArmor() const
{
	return _replacedArmor;
}

/**
* Backs up the soldier's original armor (before replacement).
* @param armor Pointer to armor data.
*/
void Soldier::setReplacedArmor(Armor *armor)
{
	_replacedArmor = armor;
}

/**
* Gets the soldier's original armor (before transformation).
* @return Pointer to armor data.
*/
Armor *Soldier::getTransformedArmor() const
{
	return _transformedArmor;
}

/**
* Backs up the soldier's original armor (before transformation).
* @param armor Pointer to armor data.
*/
void Soldier::setTransformedArmor(Armor *armor)
{
	_transformedArmor = armor;
}

/**
* Is the soldier wounded or not?.
* @return True if wounded.
*/
bool Soldier::isWounded() const
{
	return _recovery > 0.0f;
}

/**
* Is the soldier wounded or not?.
* @return False if wounded.
*/
bool Soldier::hasFullHealth() const
{
	return !isWounded();
}

/**
 * Returns the amount of time until the soldier is healed.
 * @return Number of days.
 */
int Soldier::getWoundRecoveryInt() const
{
	// Note: only for use in Yankes scripts!
	return (int)(std::ceil(_recovery));
}

int Soldier::getWoundRecovery(float absBonus, float relBonus) const
{
	float hpPerDay = 1.0f + absBonus + (relBonus * _currentStats.health * 0.01f);
	return (int)(std::ceil(_recovery / hpPerDay));
}

/**
 * Changes the amount of time until the soldier is healed.
 * @param recovery Number of days.
 */
void Soldier::setWoundRecovery(int recovery)
{
	_recovery = std::max(recovery, 0);
}

/**
 * Heals soldier wounds.
 */
void Soldier::heal(float absBonus, float relBonus)
{
	// 1 hp per day as minimum
	_recovery -= 1.0f;

	// absolute bonus from sick bay facilities
	_recovery -= absBonus;

	// relative bonus from sick bay facilities
	_recovery -= (relBonus * _currentStats.health * 0.01f);

	if (_recovery < 0.0f)
		_recovery = 0.0f;
}

/**
 * Returns the list of EquipmentLayoutItems of a soldier.
 * @return Pointer to the EquipmentLayoutItem list.
 */
std::vector<EquipmentLayoutItem*> *Soldier::getEquipmentLayout()
{
	return &_equipmentLayout;
}

/**
 * Trains a soldier's Psychic abilities after 1 month.
 */
void Soldier::trainPsi()
{
	UnitStats::Type psiSkillCap = _rules->getStatCaps().psiSkill;
	UnitStats::Type psiStrengthCap = _rules->getStatCaps().psiStrength;

	_improvement = _psiStrImprovement = 0;
	// -10 days - tolerance threshold for switch from anytimePsiTraining option.
	// If soldier has psiskill -10..-1, he was trained 20..59 days. 81.7% probability, he was trained more that 30 days.
	if (_currentStats.psiSkill < -10 + _rules->getMinStats().psiSkill)
		_currentStats.psiSkill = _rules->getMinStats().psiSkill;
	else if (_currentStats.psiSkill <= _rules->getMaxStats().psiSkill)
	{
		int max = _rules->getMaxStats().psiSkill + _rules->getMaxStats().psiSkill / 2;
		_improvement = RNG::generate(_rules->getMaxStats().psiSkill, max);
	}
	else
	{
		if (_currentStats.psiSkill <= (psiSkillCap / 2)) _improvement = RNG::generate(5, 12);
		else if (_currentStats.psiSkill < psiSkillCap) _improvement = RNG::generate(1, 3);

		if (Options::allowPsiStrengthImprovement)
		{
			if (_currentStats.psiStrength <= (psiStrengthCap / 2)) _psiStrImprovement = RNG::generate(5, 12);
			else if (_currentStats.psiStrength < psiStrengthCap) _psiStrImprovement = RNG::generate(1, 3);
		}
	}
	_currentStats.psiSkill = std::max(_currentStats.psiSkill, std::min<UnitStats::Type>(_currentStats.psiSkill+_improvement, psiSkillCap));
	_currentStats.psiStrength = std::max(_currentStats.psiStrength, std::min<UnitStats::Type>(_currentStats.psiStrength+_psiStrImprovement, psiStrengthCap));
}

/**
 * Trains a soldier's Psychic abilities after 1 day.
 * (anytimePsiTraining option)
 */
void Soldier::trainPsi1Day()
{
	if (!_psiTraining)
	{
		_improvement = 0;
		return;
	}

	if (_currentStats.psiSkill > 0) // yes, 0. _rules->getMinStats().psiSkill was wrong.
	{
		if (8 * 100 >= _currentStats.psiSkill * RNG::generate(1, 100) && _currentStats.psiSkill < _rules->getStatCaps().psiSkill)
		{
			++_improvement;
			++_currentStats.psiSkill;
		}

		if (Options::allowPsiStrengthImprovement)
		{
			if (8 * 100 >= _currentStats.psiStrength * RNG::generate(1, 100) && _currentStats.psiStrength < _rules->getStatCaps().psiStrength)
			{
				++_psiStrImprovement;
				++_currentStats.psiStrength;
			}
		}
	}
	else if (_currentStats.psiSkill < _rules->getMinStats().psiSkill)
	{
		if (++_currentStats.psiSkill == _rules->getMinStats().psiSkill)	// initial training is over
		{
			_improvement = _rules->getMaxStats().psiSkill + RNG::generate(0, _rules->getMaxStats().psiSkill / 2);
			_currentStats.psiSkill = _improvement;
		}
	}
	else // minStats.psiSkill <= 0 && _currentStats.psiSkill == minStats.psiSkill
		_currentStats.psiSkill -= RNG::generate(30, 60);	// set initial training from 30 to 60 days
}

/**
 * returns whether or not the unit is in psi training
 * @return true/false
 */
bool Soldier::isInPsiTraining() const
{
	return _psiTraining;
}

/**
 * toggles whether or not the unit is in psi training
 */
void Soldier::setPsiTraining()
{
	_psiTraining = !_psiTraining;
}

/**
 * returns this soldier's psionic skill improvement score for this month.
 * @return score
 */
int Soldier::getImprovement() const
{
	return _improvement;
}

/**
 * returns this soldier's psionic strength improvement score for this month.
 */
int Soldier::getPsiStrImprovement() const
{
	return _psiStrImprovement;
}

/**
 * Returns the soldier's death details.
 * @return Pointer to death data. NULL if no death has occurred.
 */
SoldierDeath *Soldier::getDeath() const
{
	return _death;
}

/**
 * Kills the soldier in the Geoscape.
 * @param death Pointer to death data.
 */
void Soldier::die(SoldierDeath *death)
{
	delete _death;
	_death = death;

	// Clean up associations
	_craft = 0;
	_psiTraining = false;
	_training = false;
	_recentlyPromoted = false;
	_recovery = 0.0f;
	for (std::vector<EquipmentLayoutItem*>::iterator i = _equipmentLayout.begin(); i != _equipmentLayout.end(); ++i)
	{
		delete *i;
	}
	_equipmentLayout.clear();
}

/**
 * Returns the soldier's diary.
 * @return Diary.
 */
SoldierDiary *Soldier::getDiary()
{
	return _diary;
}

/**
* Resets the soldier's diary.
*/
void Soldier::resetDiary()
{
	delete _diary;
	_diary = new SoldierDiary();
}

/**
 * Calculates the soldier's statString
 * Calculates the soldier's statString.
 * @param statStrings List of statString rules.
 * @param psiStrengthEval Are psi stats available?
 */
void Soldier::calcStatString(const std::vector<StatString *> &statStrings, bool psiStrengthEval)
{
	if (_rules->getStatStrings().empty())
	{
		_statString = StatString::calcStatString(_currentStats, statStrings, psiStrengthEval, _psiTraining);
	}
	else
	{
		_statString = StatString::calcStatString(_currentStats, _rules->getStatStrings(), psiStrengthEval, _psiTraining);
	}
}

/**
 * Trains a soldier's Physical abilities
 */
void Soldier::trainPhys(int customTrainingFactor)
{
	UnitStats caps1 = _rules->getStatCaps();
	UnitStats caps2 = _rules->getTrainingStatCaps();
	// no P.T. for the wounded
	if (hasFullHealth())
	{
		if(_currentStats.firing < caps1.firing && RNG::generate(0, caps2.firing) > _currentStats.firing && RNG::percent(customTrainingFactor))
			_currentStats.firing++;
		if(_currentStats.health < caps1.health && RNG::generate(0, caps2.health) > _currentStats.health && RNG::percent(customTrainingFactor))
			_currentStats.health++;
		if(_currentStats.melee < caps1.melee && RNG::generate(0, caps2.melee) > _currentStats.melee && RNG::percent(customTrainingFactor))
			_currentStats.melee++;
		if(_currentStats.throwing < caps1.throwing && RNG::generate(0, caps2.throwing) > _currentStats.throwing && RNG::percent(customTrainingFactor))
			_currentStats.throwing++;
		if(_currentStats.strength < caps1.strength && RNG::generate(0, caps2.strength) > _currentStats.strength && RNG::percent(customTrainingFactor))
			_currentStats.strength++;
		if(_currentStats.tu < caps1.tu && RNG::generate(0, caps2.tu) > _currentStats.tu && RNG::percent(customTrainingFactor))
			_currentStats.tu++;
		if(_currentStats.stamina < caps1.stamina && RNG::generate(0, caps2.stamina) > _currentStats.stamina && RNG::percent(customTrainingFactor))
			_currentStats.stamina++;
	}
}

/**
 * Is the soldier already fully trained?
 * @return True, if the soldier cannot gain any more stats in the training facility.
 */
bool Soldier::isFullyTrained()
{
	UnitStats trainingCaps = _rules->getTrainingStatCaps();

	if (_currentStats.firing < trainingCaps.firing
		|| _currentStats.health < trainingCaps.health
		|| _currentStats.melee < trainingCaps.melee
		|| _currentStats.throwing < trainingCaps.throwing
		|| _currentStats.strength < trainingCaps.strength
		|| _currentStats.tu < trainingCaps.tu
		|| _currentStats.stamina < trainingCaps.stamina)
	{
		return false;
	}
	return true;
}

/**
 * returns whether or not the unit is in physical training
 */
bool Soldier::isInTraining()
{
	return _training;
}

/**
 * changes whether or not the unit is in physical training
 */
void Soldier::setTraining(bool training)
{
	_training = training;
}

/**
 * Sets whether or not the unit's corpse was recovered from a battle
 */
void Soldier::setCorpseRecovered(bool corpseRecovered)
{
	_corpseRecovered = corpseRecovered;
}

/**
 * Gets the previous transformations performed on this soldier
 * @return The map of previous transformations
 */
std::map<std::string, int> &Soldier::getPreviousTransformations()
{
	return _previousTransformations;
}

/**
 * Checks whether or not the soldier is eligible for a certain transformation
 */
bool Soldier::isEligibleForTransformation(RuleSoldierTransformation *transformationRule)
{
	// alive and well
	if (!_death && !isWounded() && !transformationRule->isAllowingAliveSoldiers())
		return false;

	// alive and wounded
	if (!_death && isWounded() && !transformationRule->isAllowingWoundedSoldiers())
		return false;

	// dead
	if (_death && !transformationRule->isAllowingDeadSoldiers())
		return false;

	// dead and vaporized, or missing in action
	if (_death && !_corpseRecovered && transformationRule->needsCorpseRecovered())
		return false;

	// Is the soldier of the correct type?
	const std::vector<std::string> &allowedTypes = transformationRule->getAllowedSoldierTypes();
	std::vector<std::string >::const_iterator it;
	it = std::find(allowedTypes.begin(), allowedTypes.end(), _rules->getType());
	if (it == allowedTypes.end())
		return false;

	// Does this soldier's transformation history preclude this new project?
	const std::vector<std::string> &requiredTransformations = transformationRule->getRequiredPreviousTransformations();
	const std::vector<std::string> &forbiddenTransformations = transformationRule->getForbiddenPreviousTransformations();
	for (auto it : requiredTransformations)
	{
		if (_previousTransformations.find(it) == _previousTransformations.end())
			return false;
	}

	for (auto it : forbiddenTransformations)
	{
		if (_previousTransformations.find(it) != _previousTransformations.end())
			return false;
	}

	// Does this soldier meet the minimum stat requirements for the project?
	UnitStats minStats = transformationRule->getRequiredMinStats();
	if (_currentStats.tu < minStats.tu ||
		_currentStats.stamina < minStats.stamina ||
		_currentStats.health < minStats.health ||
		_currentStats.bravery < minStats.bravery ||
		_currentStats.reactions < minStats.reactions ||
		_currentStats.firing < minStats.firing ||
		_currentStats.throwing < minStats.throwing ||
		_currentStats.melee < minStats.melee ||
		_currentStats.strength < minStats.strength ||
		_currentStats.psiStrength < minStats.psiStrength ||
		(_currentStats.psiSkill < minStats.psiSkill && minStats.psiSkill != 0)) // The != 0 is required for the "psi training at any time" option, as it sets skill to negative in training
		return false;

	return true;
}

/**
 * Performs a transformation on this unit
 */
void Soldier::transform(const Mod *mod, RuleSoldierTransformation *transformationRule, Soldier *sourceSoldier)
{
	if (_death)
	{
		_corpseRecovered = false; // They're not a corpse anymore!
		delete _death;
		_death = 0;
	}

	if (transformationRule->getRecoveryTime() > 0)
	{
		_recovery = transformationRule->getRecoveryTime();
	}
	_training = false;
	_psiTraining = false;

	// needed, because the armor size may change (also, it just makes sense)
	sourceSoldier->setCraft(0);

	if (transformationRule->isCreatingClone())
	{
		// a clone already has the correct soldier type, but random stats
		// if we don't want random stats, let's copy them from the source soldier
		if (!transformationRule->isUsingRandomStats())
		{
			UnitStats newStats = *sourceSoldier->getCurrentStats() + calculateStatChanges(mod, transformationRule, sourceSoldier);
			setBothStats(&newStats);
		}
	}
	else
	{
		// change soldier type if needed
		if (!transformationRule->getProducedSoldierType().empty() && _rules->getType() != transformationRule->getProducedSoldierType())
		{
			_rules = mod->getSoldier(transformationRule->getProducedSoldierType());

			// demote soldier if needed (i.e. when new soldier type doesn't support the current rank)
			if (!_rules->getAllowPromotion())
			{
				_rank = RANK_ROOKIE;
			}
			else if (!_rules->getRankStrings().empty() && (int)_rank > _rules->getRankStrings().size() - 1)
			{
				switch (_rules->getRankStrings().size() - 1)
				{
				case 1:
					_rank = RANK_SQUADDIE;
					break;
				case 2:
					_rank = RANK_SERGEANT;
					break;
				case 3:
					_rank = RANK_CAPTAIN;
					break;
				case 4:
					_rank = RANK_COLONEL;
					break;
				case 5:
					_rank = RANK_COMMANDER; // I hereby demote you to commander! :P
					break;
				default:
					_rank = RANK_ROOKIE;
					break;
				}
			}
		}

		// change stats
		if (transformationRule->isUsingRandomStats())
		{
			Soldier *tmpSoldier = new Soldier(_rules, 0, _id);
			setBothStats(tmpSoldier->getCurrentStats());
			delete tmpSoldier;
			tmpSoldier = 0;
		}
		else
		{
			_currentStats += calculateStatChanges(mod, transformationRule, sourceSoldier);
		}
	}

	if (!transformationRule->isKeepingSoldierArmor())
	{
		if (transformationRule->getProducedSoldierArmor().empty())
		{
			// default armor of the soldier's type
			_armor = mod->getArmor(_rules->getArmor());
		}
		else
		{
			// explicitly defined armor
			_armor = mod->getArmor(transformationRule->getProducedSoldierArmor());
		}
	}

	// Remember the performed transformation (on the source soldier)
	auto& history = sourceSoldier->getPreviousTransformations();
	auto it = history.find(transformationRule->getName());
	if (it != history.end())
	{
		it->second += 1;
	}
	else
	{
		history[transformationRule->getName()] = 1;
	}
}

/**
 * Calculates the stat changes a soldier undergoes from this project
 * @param mod Pointer to the mod
 * @return The stat changes
 */
UnitStats Soldier::calculateStatChanges(const Mod *mod, RuleSoldierTransformation *transformationRule, Soldier *sourceSoldier)
{
	UnitStats statChange;

	// If this project uses random stats from the produced soldier type's rules, we don't need calculations!
	if (transformationRule->isUsingRandomStats())
		return statChange;

	UnitStats initialStats = *sourceSoldier->getInitStats();
	UnitStats currentStats = *sourceSoldier->getCurrentStats();
	UnitStats gainedStats = currentStats - initialStats;

	// Flat stat changes
	statChange += transformationRule->getFlatOverallStatChange();

	// Stat changes based on current stats
	statChange += UnitStats::percent(currentStats, transformationRule->getPercentOverallStatChange());

	// Stat changes based on gained stats
	statChange += UnitStats::percent(gainedStats, transformationRule->getPercentGainedStatChange());

	// round (mathematically) to whole tens
	int sign = statChange.bravery < 0 ? -1 : 1;
	statChange.bravery = ((statChange.bravery + (sign * 5)) / 10) * 10;

	RuleSoldier *transformationSoldierType = _rules;
	if (!transformationRule->getProducedSoldierType().empty())
	{
		transformationSoldierType = mod->getSoldier(transformationRule->getProducedSoldierType());
	}

	if (transformationRule->hasLowerBoundAtMinStats())
	{
		UnitStats lowerBound = transformationSoldierType->getMinStats();
		UnitStats cappedChange = lowerBound - currentStats;

		statChange = UnitStats::max(statChange, cappedChange);
	}

	if (transformationRule->hasUpperBoundAtMaxStats() || transformationRule->hasUpperBoundAtStatCaps())
	{
		UnitStats upperBound = transformationRule->hasUpperBoundAtMaxStats()
			? transformationSoldierType->getMaxStats()
			: transformationSoldierType->getStatCaps();
		UnitStats cappedChange = upperBound - currentStats;

		statChange = UnitStats::min(statChange, cappedChange);
	}

	return statChange;
}


////////////////////////////////////////////////////////////
//					Script binding
////////////////////////////////////////////////////////////

namespace
{

void getGenderScript(const Soldier *so, int &ret)
{
	if (so)
	{
		ret = so->getGender();
		return;
	}
	ret = 0;
}
void getRankScript(const Soldier *so, int &ret)
{
	if (so)
	{
		ret = so->getRank();
		return;
	}
	ret = 0;
}
void getLookScript(const Soldier *so, int &ret)
{
	if (so)
	{
		ret = so->getLook();
		return;
	}
	ret = 0;
}
void getLookVariantScript(const Soldier *so, int &ret)
{
	if (so)
	{
		ret = so->getLookVariant();
		return;
	}
	ret = 0;
}
struct getRuleSoldierScript
{
	static RetEnum func(const Soldier *so, const RuleSoldier* &ret)
	{
		if (so)
		{
			ret = so->getRules();
		}
		else
		{
			ret = nullptr;
		}
		return RetContinue;
	}
};

std::string debugDisplayScript(const Soldier* so)
{
	if (so)
	{
		std::string s;
		s += Soldier::ScriptName;
		s += "(type: \"";
		s += so->getRules()->getType();
		s += "\" id: ";
		s += std::to_string(so->getId());
		s += ")";
		return s;
	}
	else
	{
		return "null";
	}
}

} // namespace

/**
 * Register Soldier in script parser.
 * @param parser Script parser.
 */
void Soldier::ScriptRegister(ScriptParserBase* parser)
{
	parser->registerPointerType<RuleSoldier>();

	Bind<Soldier> so = { parser };


	so.addField<&Soldier::_id>("getId");
	so.add<&getRankScript>("getRank");
	so.add<&getGenderScript>("getGender");
	so.add<&getLookScript>("getLook");
	so.add<&getLookVariantScript>("getLookVariant");


	UnitStats::addGetStatsScript<Soldier, &Soldier::_currentStats>(so, "Stats.");
	UnitStats::addSetStatsScript<Soldier, &Soldier::_currentStats>(so, "Stats.");


	so.addFunc<getRuleSoldierScript>("getRuleSoldier");
	so.add<&Soldier::getWoundRecoveryInt>("getWoundRecovery");
	so.add<&Soldier::setWoundRecovery>("setWoundRecovery");


	so.addScriptValue<&Soldier::_scriptValues>();
	so.addDebugDisplay<&debugDisplayScript>();
}

} // namespace OpenXcom
