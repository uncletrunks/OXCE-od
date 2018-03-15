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
#include "../Mod/Armor.h"
#include "../Mod/MapData.h"
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
	Text *_txtTitle;
	ComboBox *_cbxRelatedStuff;
	Text *_txtArticle;
	TextButton *_btnPrev, *_btnNext;
	TextList *_lstRawData;
	ToggleTextButton *_btnIncludeDebug, *_btnIncludeIds, *_btnIncludeDefaults;
	TextButton *_btnOk;

	Uint8 _purple, _pink, _blue, _white, _gold;

	UfopaediaTypeId _typeId;
	std::string _topicId;
	bool _mainArticle;
	size_t _currentDetailIndex;

	std::vector<std::string> _filterOptions;
	bool _showDebug, _showIds, _showDefaults;
	int _counter;
	bool _indent;

	void buildUI(bool debug, bool ids, bool defaults);
	void initLists();
	void resetStream(std::wostringstream &ss);
	void addTranslation(std::wostringstream &ss, const std::string &id);
	std::wstring trp(const std::string &propertyName);
	void addSection(const std::wstring &name, const std::wstring &desc, Uint8 color, bool forceShow = false);
	void addHeading(const std::string &propertyName);
	void endHeading();
	void addSingleString(std::wostringstream &ss, const std::string &id, const std::string &propertyName, const std::string &defaultId = "", bool translate = true);
	void addVectorOfStrings(std::wostringstream &ss, const std::vector<std::string> &vec, const std::string &propertyName);
	void addBoolean(std::wostringstream &ss, const bool &value, const std::string &propertyName, const bool &defaultvalue = false);
	void addFloat(std::wostringstream &ss, const float &value, const std::string &propertyName, const float &defaultvalue = 0.0f);
	void addFloatAsPercentage(std::wostringstream &ss, const float &value, const std::string &propertyName, const float &defaultvalue = 0.0f);
	void addDouble(std::wostringstream &ss, const double &value, const std::string &propertyName, const double &defaultvalue = 0.0);
	void addInteger(std::wostringstream &ss, const int &value, const std::string &propertyName, const int &defaultvalue = 0, bool formatAsMoney = false, const std::string &specialTranslation = "", const int &specialvalue = -1);
	void addIntegerScriptTag(std::wostringstream &ss, const int &value, const std::string &propertyName, const int &defaultvalue = 0);
	void addIntegerPercent(std::wostringstream &ss, const int &value, const std::string &propertyName, const int &defaultvalue = 0);
	void addVectorOfIntegers(std::wostringstream &ss, const std::vector<int> &vec, const std::string &propertyName);
	void addBattleType(std::wostringstream &ss, const BattleType &value, const std::string &propertyName, const BattleType &defaultvalue = BT_NONE);
	void addDamageType(std::wostringstream &ss, const ItemDamageType &value, const std::string &propertyName, const ItemDamageType &defaultvalue = DT_NONE);
	void addDamageRandomType(std::wostringstream &ss, const ItemDamageRandomType &value, const std::string &propertyName, const ItemDamageRandomType &defaultvalue = DRT_DEFAULT);
	void addBattleFuseType(std::wostringstream &ss, const BattleFuseType &value, const std::string &propertyName, const BattleFuseType &defaultvalue = BFT_NONE);
	void addRuleItemUseCostBasic(std::wostringstream &ss, const RuleItemUseCost &value, const std::string &propertyName, const int &defaultvalue = 0);
	void addBoolOrInteger(std::wostringstream &ss, const int &value, bool formatAsBoolean);
	void addPercentageSignOrNothing(std::wostringstream &ss, const int &value, bool smartFormat);
	void addRuleItemUseCostFull(std::wostringstream &ss, const RuleItemUseCost &value, const std::string &propertyName, const RuleItemUseCost &defaultvalue = RuleItemUseCost(), bool smartFormat = false, const RuleItemUseCost &formatBy = RuleItemUseCost());
	void addBattleMediKitType(std::wostringstream &ss, const BattleMediKitType &value, const std::string &propertyName, const BattleMediKitType &defaultvalue = BMT_NORMAL);
	void addExperienceTrainingMode(std::wostringstream &ss, const ExperienceTrainingMode &value, const std::string &propertyName, const ExperienceTrainingMode &defaultvalue = ETM_DEFAULT);
	void addRuleStatBonus(std::wostringstream &ss, const RuleStatBonus &value, const std::string &propertyName);
	void addSpriteResourcePath(std::wostringstream &ss, Mod *mod, const std::string &resourceSetName, const int &resourceId);
	void addSoundVectorResourcePaths(std::wostringstream &ss, Mod *mod, const std::string &resourceSetName, const std::vector<int> &resourceIds);
	void initItemList();
	void addUnitStatFormatted(std::wostringstream &ss, const int &value, const std::string &label, bool &isFirst);
	void addUnitStatBonus(std::wostringstream &ss, const UnitStats &value, const std::string &propertyName);
	void addArmorDamageModifiers(std::wostringstream &ss, const std::vector<float> &vec, const std::string &propertyName);
	void addMovementType(std::wostringstream &ss, const MovementType &value, const std::string &propertyName, const MovementType &defaultvalue = MT_WALK);
	void addForcedTorso(std::wostringstream &ss, const ForcedTorso &value, const std::string &propertyName, const ForcedTorso &defaultvalue = TORSO_USE_GENDER);
	void addDrawingRoutine(std::wostringstream &ss, const int &value, const std::string &propertyName, const int &defaultvalue = 0);
	void initArmorList();
public:
	/// Creates the StatsForNerdsState state.
	StatsForNerdsState(const ArticleDefinition *article, size_t currentDetailIndex, bool debug, bool ids, bool defaults);
	StatsForNerdsState(const UfopaediaTypeId typeId, const std::string topicId, bool debug, bool ids, bool defaults);
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
	/// Handler for clicking the [Previous] button.
	void btnPrevClick(Action *action);
	/// Handler for clicking the [Next] button.
	void btnNextClick(Action *action);
	/// Handler for clicking the [Scroll Up] button.
	void btnScrollUpClick(Action *action);
	/// Handler for clicking the [Scroll Down] button.
	void btnScrollDownClick(Action *action);
};

}
