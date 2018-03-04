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
#include "../Engine/State.h"
#include "../Mod/ArticleDefinition.h"
#include "../Mod/RuleDamageType.h"
#include "../Mod/RuleItem.h"

namespace OpenXcom
{

class Window;
class Text;
class ComboBox;
class TextList;
class ToggleTextButton;
class TextButton;

/**
 * A screen, where you can see the (almost) raw ruleset corresponding to the given Ufopedia article.
 */
class StatsForNerdsState : public State
{
private:
	Window *_window;
	Text *_txtTitle, *_txtArticle;
	ComboBox *_cbxRelatedStuff;
	TextList *_lstRawData;
	ToggleTextButton *_btnIncludeDebug, *_btnIncludeIds, *_btnIncludeDefaults;
	TextButton *_btnOk;

	Uint8 _purple, _pink, _blue, _white, _gold;

	UfopaediaTypeId _typeId;
	std::string _topicId;
	std::vector<std::string> _filterOptions;
	bool _showDebug, _showIds, _showDefaults;
	int _counter;

	void buildUI();
	void initLists();
	void resetStream(std::wostringstream &ss);
	void addTranslation(std::wostringstream &ss, const std::string &id);
	void addSection(const std::wstring &name, const std::wstring &desc, Uint8 color);
	void addHeading(const std::wstring &propertyName);
	void endHeading();
	void addSingleString(std::wostringstream &ss, const std::string &id, const std::wstring &propertyName, const std::string &defaultId = "");
	void addVectorOfStrings(std::wostringstream &ss, const std::vector<std::string> &vec, const std::wstring &propertyName);
	void addBoolean(std::wostringstream &ss, const bool &value, const std::wstring &propertyName, const bool &defaultvalue = false);
	void addFloat(std::wostringstream &ss, const float &value, const std::wstring &propertyName, const float &defaultvalue = 0.0f);
	void addDouble(std::wostringstream &ss, const double &value, const std::wstring &propertyName, const double &defaultvalue = 0.0);
	void addInteger(std::wostringstream &ss, const int &value, const std::wstring &propertyName, const int &defaultvalue = 0, bool formatAsMoney = false, const std::string &specialTranslation = "", const int &specialvalue = -1);
	void addIntegerPercent(std::wostringstream &ss, const int &value, const std::wstring &propertyName, const int &defaultvalue = 0);
	void addVectorOfIntegers(std::wostringstream &ss, const std::vector<int> &vec, const std::wstring &propertyName);
	void addBattleType(std::wostringstream &ss, const BattleType &value, const std::wstring &propertyName, const BattleType &defaultvalue = BT_NONE);
	void addDamageType(std::wostringstream &ss, const ItemDamageType &value, const std::wstring &propertyName, const ItemDamageType &defaultvalue = DT_NONE);
	void addDamageRandomType(std::wostringstream &ss, const ItemDamageRandomType &value, const std::wstring &propertyName, const ItemDamageRandomType &defaultvalue = DRT_DEFAULT);
	void addBattleFuseType(std::wostringstream &ss, const BattleFuseType &value, const std::wstring &propertyName, const BattleFuseType &defaultvalue = BFT_NONE);
	void addRuleItemUseCostBasic(std::wostringstream &ss, const RuleItemUseCost &value, const std::wstring &propertyName, const int &defaultvalue = 0);
	void addBoolOrInteger(std::wostringstream &ss, const int &value, bool formatAsBoolean);
	void addPercentageSignOrNothing(std::wostringstream &ss, const int &value, bool smartFormat);
	void addRuleItemUseCostFull(std::wostringstream &ss, const RuleItemUseCost &value, const std::wstring &propertyName, const RuleItemUseCost &defaultvalue = RuleItemUseCost(), bool smartFormat = false, const RuleItemUseCost &formatBy = RuleItemUseCost());
	void addBattleMediKitType(std::wostringstream &ss, const BattleMediKitType &value, const std::wstring &propertyName, const BattleMediKitType &defaultvalue = BMT_NORMAL);
	void addExperienceTrainingMode(std::wostringstream &ss, const ExperienceTrainingMode &value, const std::wstring &propertyName, const ExperienceTrainingMode &defaultvalue = ETM_DEFAULT);
	void addRuleStatBonus(std::wostringstream &ss, const RuleStatBonus &value, const std::wstring &propertyName);
	void initItemList();
public:
	/// Creates the StatsForNerdsState state.
	StatsForNerdsState(const ArticleDefinition *article);
	StatsForNerdsState(const UfopaediaTypeId typeId, const std::string topicId);
	/// Cleans up the StatsForNerdsState state.
	~StatsForNerdsState();
	/// Initializes the state.
	void init();
	/// Handler for selecting an item from the [Compatible ammo] combobox.
	void cbxAmmoSelect(Action *action);
	/// Handler for clicking the toggle buttons.
	void btnRefreshClick(Action *action);
	/// Handler for clicking the OK button.
	void btnOkClick(Action *action);
};

}
