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
#include "StatsForNerdsState.h"
#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Interface/ComboBox.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/ToggleTextButton.h"
#include "../Interface/Window.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleInterface.h"
#include "../Mod/RuleItem.h"
#include "../fmath.h"

namespace OpenXcom
{

std::map<std::string, std::string> translationMap =
{
	{ "flatOne", "" }, // no translation
	{ "flatHundred", "" }, // no translation
	{ "strength", "STR_STRENGTH" },
	{ "psi", "STR_PSI_SKILL_AND_PSI_STRENGTH" }, // new, STR_PSIONIC_SKILL * STR_PSIONIC_STRENGTH
	{ "psiSkill", "STR_PSIONIC_SKILL" },
	{ "psiStrength", "STR_PSIONIC_STRENGTH" },
	{ "throwing", "STR_THROWING_ACCURACY" },
	{ "bravery", "STR_BRAVERY" },
	{ "firing", "STR_FIRING_ACCURACY" },
	{ "health", "STR_HEALTH" },
	{ "tu", "STR_TIME_UNITS" },
	{ "reactions", "STR_REACTIONS" },
	{ "stamina", "STR_STAMINA" },
	{ "melee", "STR_MELEE_ACCURACY" },
	{ "strengthMelee", "STR_STRENGTH_AND_MELEE_ACCURACY" }, // new, STR_STRENGTH * STR_MELEE_ACCURACY
	{ "strengthThrowing", "STR_STRENGTH_AND_THROWING_ACCURACY" }, // new, STR_STRENGTH * STR_THROWING_ACCURACY
	{ "firingReactions", "STR_FIRING_ACCURACY_AND_REACTIONS" }, // new, STR_FIRING_ACCURACY * STR_REACTIONS

	{ "rank", "STR_RANK" },
	{ "fatalWounds", "STR_FATAL_WOUNDS" },

	{ "healthCurrent", "STR_HEALTH_CURRENT" }, // new, current HP (i.e. not max HP)
	{ "tuCurrent", "STR_TIME_UNITS_CURRENT" }, // new
	{ "energyCurrent", "STR_ENERGY" },
	{ "moraleCurrent", "STR_MORALE" },
	{ "stunCurrent", "STR_STUN_LEVEL_CURRENT" }, // new

	{ "healthNormalized", "STR_HEALTH_NORMALIZED" }, // new, current HP normalized to [0, 1] interval
	{ "tuNormalized", "STR_TIME_UNITS_NORMALIZED" }, // new
	{ "energyNormalized", "STR_ENERGY_NORMALIZED" }, // new
	{ "moraleNormalized", "STR_MORALE_NORMALIZED" }, // new
	{ "stunNormalized", "STR_STUN_LEVEL_NORMALIZED" }, // new

	{ "energyRegen", "STR_ENERGY_REGENERATION" }, // new, special stat returning vanilla energy regen
};

/**
 * Initializes all the elements on the UI.
 */
StatsForNerdsState::StatsForNerdsState(const ArticleDefinition *article)
{
	_typeId = article->getType();
	_topicId = article->id;

	buildUI();
}

/**
 * Initializes all the elements on the UI.
 */
StatsForNerdsState::StatsForNerdsState(const UfopaediaTypeId typeId, const std::string topicId)
{
	_typeId = typeId;
	_topicId = topicId;

	buildUI();
}

/**
 * Builds the UI.
 */
void StatsForNerdsState::buildUI()
{
	// Create objects
	_window = new Window(this, 320, 200, 0, 0);
	_txtTitle = new Text(304, 17, 8, 7);
	_txtArticle = new Text(304, 9, 8, 24);
	_cbxRelatedStuff = new ComboBox(this, 148, 16, 164, 23);
	_lstRawData = new TextList(287, 128, 8, 40);
	_btnIncludeDebug = new ToggleTextButton(70, 16, 8, 176);
	_btnIncludeIds = new ToggleTextButton(70, 16, 86, 176);
	_btnIncludeDefaults = new ToggleTextButton(70, 16, 164, 176);
	_btnOk = new TextButton(70, 16, 242, 176);

	// Set palette
	setInterface("statsForNerds");

	_purple = _game->getMod()->getInterface("statsForNerds")->getElement("list")->color;
	_pink = _game->getMod()->getInterface("statsForNerds")->getElement("list")->color2;
	_blue = _game->getMod()->getInterface("statsForNerds")->getElement("list")->border;
	_white = _game->getMod()->getInterface("statsForNerds")->getElement("listExtended")->color;
	_gold = _game->getMod()->getInterface("statsForNerds")->getElement("listExtended")->color2;

	add(_window, "window", "statsForNerds");
	add(_txtTitle, "text", "statsForNerds");
	add(_txtArticle, "text", "statsForNerds");
	add(_lstRawData, "list", "statsForNerds");
	add(_btnIncludeDebug, "button", "statsForNerds");
	add(_btnIncludeIds, "button", "statsForNerds");
	add(_btnIncludeDefaults, "button", "statsForNerds");
	add(_btnOk, "button", "statsForNerds");
	add(_cbxRelatedStuff, "comboBox", "statsForNerds");

	centerAllSurfaces();

	// Set up objects
	_window->setBackground(_game->getMod()->getSurface("BACK05.SCR"));

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_STATS_FOR_NERDS"));

	_txtArticle->setText(tr("STR_ARTICLE").arg(L""));

	_lstRawData->setColumns(2, 128, 159);
	_lstRawData->setSelectable(true);
	_lstRawData->setBackground(_window);
	_lstRawData->setWordWrap(true);

	_btnIncludeDebug->setText(tr("STR_INCLUDE_DEBUG"));
	_btnIncludeDebug->onMouseClick((ActionHandler)&StatsForNerdsState::btnRefreshClick);

	_btnIncludeIds->setText(tr("STR_INCLUDE_IDS"));
	_btnIncludeIds->onMouseClick((ActionHandler)&StatsForNerdsState::btnRefreshClick);

	_btnIncludeDefaults->setText(tr("STR_INCLUDE_DEFAULTS"));
	_btnIncludeDefaults->onMouseClick((ActionHandler)&StatsForNerdsState::btnRefreshClick);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&StatsForNerdsState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&StatsForNerdsState::btnOkClick, Options::keyCancel);

	_filterOptions.clear();
	_filterOptions.push_back("STR_UNKNOWN");
	_cbxRelatedStuff->setOptions(_filterOptions);
	_cbxRelatedStuff->onChange((ActionHandler)&StatsForNerdsState::cbxAmmoSelect);
	_cbxRelatedStuff->setVisible(_filterOptions.size() > 1);
}

/**
 *
 */
StatsForNerdsState::~StatsForNerdsState()
{
}

/**
 * Initializes the screen (fills the data).
 */
void StatsForNerdsState::init()
{
	State::init();
	initLists();
}
/**
 * Opens the details for the selected ammo item.
 * @param action Pointer to an action.
 */
void StatsForNerdsState::cbxAmmoSelect(Action *)
{
	size_t selIdx = _cbxRelatedStuff->getSelected();
	if (selIdx > 0)
	{
		_game->pushState(new StatsForNerdsState(UFOPAEDIA_TYPE_ITEM, _filterOptions.at(selIdx)));
	}
}


/**
 * Refresh the displayed data including/exluding the raw IDs.
 * @param action Pointer to an action.
 */
void StatsForNerdsState::btnRefreshClick(Action *)
{
	initLists();
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void StatsForNerdsState::btnOkClick(Action *)
{
	_game->popState();
}

/**
 * Shows the "raw" data.
 */
void StatsForNerdsState::initLists()
{
	_showDebug = _btnIncludeDebug->getPressed();
	_showIds = _btnIncludeIds->getPressed();
	_showDefaults = _btnIncludeDefaults->getPressed();

	_counter = 0;

	switch (_typeId)
	{
	case UFOPAEDIA_TYPE_ITEM:
		initItemList();
		break;
	default:
		break;
	}
}

/**
 * Resets the string stream.
 */
void StatsForNerdsState::resetStream(std::wostringstream &ss)
{
	ss.str(L"");
	ss.clear();
}

/**
 * Adds a translatable item to the string stream, optionally with string ID.
 */
void StatsForNerdsState::addTranslation(std::wostringstream &ss, const std::string &id)
{
	ss << tr(id);
	if (_showIds)
	{
		ss << L" [" << Language::utf8ToWstr(id) << L"]";
	}
}

/**
 * Adds a section name to the table.
 */
void StatsForNerdsState::addSection(const std::wstring &name, const std::wstring &desc, Uint8 color)
{
	_lstRawData->addRow(2, name.c_str(), desc.c_str());
	_lstRawData->setRowColor(_lstRawData->getTexts() - 1, color);
}

/**
 * Adds a group heading to the table.
 */
void StatsForNerdsState::addHeading(const std::wstring &propertyName)
{
	_lstRawData->addRow(2, propertyName.c_str(), L"");
	_lstRawData->setCellColor(_lstRawData->getTexts() - 1, 0, _blue);

	// reset counter
	_counter = 0;
}

/**
 * Ends a group with a heading, potentially removing the heading... if the group turned out to be empty.
 */
void StatsForNerdsState::endHeading()
{
	if (_counter == 0)
	{
		_lstRawData->removeLastRow();
	}
}

/**
 * Adds a single string value to the table.
 */
void StatsForNerdsState::addSingleString(std::wostringstream &ss, const std::string &id, const std::wstring &propertyName, const std::string &defaultId)
{
	if (id == defaultId && !_showDefaults)
	{
		return;
	}
	resetStream(ss);
	addTranslation(ss, id);
	_lstRawData->addRow(2, propertyName.c_str(), ss.str().c_str());
	++_counter;
	if (id != defaultId)
	{
		_lstRawData->setCellColor(_lstRawData->getTexts() - 1, 1, _pink);
	}
}

/**
 * Adds a vector of strings to the table.
 */
void StatsForNerdsState::addVectorOfStrings(std::wostringstream &ss, const std::vector<std::string> &vec, const std::wstring &propertyName)
{
	if (vec.empty() && !_showDefaults)
	{
		return;
	}
	resetStream(ss);
	int i = 0;
	ss << L"{";
	for (auto &item : vec)
	{
		if (i > 0)
		{
			ss << L", ";
		}
		addTranslation(ss, item);
		i++;
	}
	ss << L"}";
	_lstRawData->addRow(2, propertyName.c_str(), ss.str().c_str());
	++_counter;
	if (!vec.empty())
	{
		_lstRawData->setCellColor(_lstRawData->getTexts() - 1, 1, _pink);
	}
}

/**
 * Adds a single boolean value to the table.
 */
void StatsForNerdsState::addBoolean(std::wostringstream &ss, const bool &value, const std::wstring &propertyName, const bool &defaultvalue)
{
	if (value == defaultvalue && !_showDefaults)
	{
		return;
	}
	resetStream(ss);
	if (value)
	{
		ss << L"true";
	}
	else
	{
		ss << L"false";
	}
	_lstRawData->addRow(2, propertyName.c_str(), ss.str().c_str());
	++_counter;
	if (value != defaultvalue)
	{
		_lstRawData->setCellColor(_lstRawData->getTexts() - 1, 1, _pink);
	}
}

/**
 * Adds a single floating point number to the table.
 */
void StatsForNerdsState::addFloat(std::wostringstream &ss, const float &value, const std::wstring &propertyName, const float &defaultvalue)
{
	if (AreSame(value, defaultvalue) && !_showDefaults)
	{
		return;
	}
	resetStream(ss);
	ss << value;
	_lstRawData->addRow(2, propertyName.c_str(), ss.str().c_str());
	++_counter;
	if (!AreSame(value, defaultvalue))
	{
		_lstRawData->setCellColor(_lstRawData->getTexts() - 1, 1, _pink);
	}
}

/**
 * Adds a single floating point (double precision) number to the table.
 */
void StatsForNerdsState::addDouble(std::wostringstream &ss, const double &value, const std::wstring &propertyName, const double &defaultvalue)
{
	if (AreSame(value, defaultvalue) && !_showDefaults)
	{
		return;
	}
	resetStream(ss);
	ss << value;
	_lstRawData->addRow(2, propertyName.c_str(), ss.str().c_str());
	++_counter;
	if (!AreSame(value, defaultvalue))
	{
		_lstRawData->setCellColor(_lstRawData->getTexts() - 1, 1, _pink);
	}
}

/**
 * Adds a single integer number to the table.
 */
void StatsForNerdsState::addInteger(std::wostringstream &ss, const int &value, const std::wstring &propertyName, const int &defaultvalue)
{
	if (value == defaultvalue && !_showDefaults)
	{
		return;
	}
	resetStream(ss);
	ss << value;
	_lstRawData->addRow(2, propertyName.c_str(), ss.str().c_str());
	++_counter;
	if (value != defaultvalue)
	{
		_lstRawData->setCellColor(_lstRawData->getTexts() - 1, 1, _pink);
	}
}

/**
 * Adds a single integer number (representing a percentage) to the table.
 */
void StatsForNerdsState::addIntegerPercent(std::wostringstream &ss, const int &value, const std::wstring &propertyName, const int &defaultvalue)
{
	if (value == defaultvalue && !_showDefaults)
	{
		return;
	}
	resetStream(ss);
	ss << value;
	// -1 is not a percentage, usually means "take value from somewhere else"
	if (value >= 0)
	{
		ss << L"%";
	}
	_lstRawData->addRow(2, propertyName.c_str(), ss.str().c_str());
	++_counter;
	if (value != defaultvalue)
	{
		_lstRawData->setCellColor(_lstRawData->getTexts() - 1, 1, _pink);
	}
}

/**
 * Adds a vector of integer numbers to the table.
 */
void StatsForNerdsState::addVectorOfIntegers(std::wostringstream &ss, const std::vector<int> &vec, const std::wstring &propertyName)
{
	if (vec.empty() && !_showDefaults)
	{
		return;
	}
	resetStream(ss);
	int i = 0;
	ss << L"{";
	for (auto &item : vec)
	{
		if (i > 0)
		{
			ss << L", ";
		}
		ss << item;
		i++;
	}
	ss << L"}";
	_lstRawData->addRow(2, propertyName.c_str(), ss.str().c_str());
	++_counter;
	if (!vec.empty())
	{
		_lstRawData->setCellColor(_lstRawData->getTexts() - 1, 1, _pink);
	}
}

/**
 * Adds a BattleType to the table.
 */
void StatsForNerdsState::addBattleType(std::wostringstream &ss, const BattleType &value, const std::wstring &propertyName, const BattleType &defaultvalue)
{
	if (value == defaultvalue && !_showDefaults)
	{
		return;
	}
	resetStream(ss);
	switch (value)
	{
		case BT_NONE: ss << tr("BT_NONE"); break;
		case BT_FIREARM: ss << tr("BT_FIREARM"); break;
		case BT_AMMO: ss << tr("BT_AMMO"); break;
		case BT_MELEE: ss << tr("BT_MELEE"); break;
		case BT_GRENADE: ss << tr("BT_GRENADE"); break;
		case BT_PROXIMITYGRENADE: ss << tr("BT_PROXIMITYGRENADE"); break;
		case BT_MEDIKIT: ss << tr("BT_MEDIKIT"); break;
		case BT_SCANNER: ss << tr("BT_SCANNER"); break;
		case BT_MINDPROBE: ss << tr("BT_MINDPROBE"); break;
		case BT_PSIAMP: ss << tr("BT_PSIAMP"); break;
		case BT_FLARE: ss << tr("BT_FLARE"); break;
		case BT_CORPSE: ss << tr("BT_CORPSE"); break;
		default: ss << tr("STR_UNKNOWN"); break;
	}
	if (_showIds)
	{
		ss << L" [" << value << L"]";
	}
	_lstRawData->addRow(2, propertyName.c_str(), ss.str().c_str());
	++_counter;
	if (value != defaultvalue)
	{
		_lstRawData->setCellColor(_lstRawData->getTexts() - 1, 1, _pink);
	}
}

/**
 * Adds a DamageType to the table.
 */
void StatsForNerdsState::addDamageType(std::wostringstream &ss, const ItemDamageType &value, const std::wstring &propertyName, const ItemDamageType &defaultvalue)
{
	if (value == defaultvalue && !_showDefaults)
	{
		return;
	}
	resetStream(ss);
	switch (value)
	{
		case DT_NONE: ss << tr( "STR_DAMAGE_NONE"); break;
		case DT_AP: ss << tr("STR_DAMAGE_ARMOR_PIERCING"); break;
		case DT_IN: ss << tr("STR_DAMAGE_INCENDIARY"); break;
		case DT_HE: ss << tr("STR_DAMAGE_HIGH_EXPLOSIVE"); break;
		case DT_LASER: ss << tr("STR_DAMAGE_LASER_BEAM"); break;
		case DT_PLASMA: ss << tr("STR_DAMAGE_PLASMA_BEAM"); break;
		case DT_STUN: ss << tr("STR_DAMAGE_STUN"); break;
		case DT_MELEE: ss << tr("STR_DAMAGE_MELEE"); break;
		case DT_ACID: ss << tr("STR_DAMAGE_ACID"); break;
		case DT_SMOKE: ss << tr("STR_DAMAGE_SMOKE"); break;
		case DT_10: ss << tr("STR_DAMAGE_10"); break;
		case DT_11: ss << tr("STR_DAMAGE_11"); break;
		case DT_12: ss << tr("STR_DAMAGE_12"); break;
		case DT_13: ss << tr("STR_DAMAGE_13"); break;
		case DT_14: ss << tr("STR_DAMAGE_14"); break;
		case DT_15: ss << tr("STR_DAMAGE_15"); break;
		case DT_16: ss << tr("STR_DAMAGE_16"); break;
		case DT_17: ss << tr("STR_DAMAGE_17"); break;
		case DT_18: ss << tr("STR_DAMAGE_18"); break;
		case DT_19: ss << tr("STR_DAMAGE_19"); break;
		default: ss << tr("STR_UNKNOWN"); break;
	}
	if (_showIds)
	{
		ss << L" [" << value << L"]";
	}
	_lstRawData->addRow(2, propertyName.c_str(), ss.str().c_str());
	++_counter;
	if (value != defaultvalue)
	{
		_lstRawData->setCellColor(_lstRawData->getTexts() - 1, 1, _pink);
	}
}

/**
 * Adds a DamageRandomType to the table.
 */
void StatsForNerdsState::addDamageRandomType(std::wostringstream &ss, const ItemDamageRandomType &value, const std::wstring &propertyName, const ItemDamageRandomType &defaultvalue)
{
	if (value == defaultvalue && !_showDefaults)
	{
		return;
	}
	resetStream(ss);
	switch (value)
	{
		case DRT_DEFAULT: ss << tr("DRT_DEFAULT"); break;
		case DRT_UFO: ss << tr("DRT_UFO"); break;
		case DRT_TFTD: ss << tr("DRT_TFTD"); break;
		case DRT_FLAT: ss << tr("DRT_FLAT"); break;
		case DRT_FIRE: ss << tr("DRT_FIRE"); break;
		case DRT_NONE: ss << tr("DRT_NONE"); break;
		case DRT_UFO_WITH_TWO_DICE: ss << tr("DRT_UFO_WITH_TWO_DICE"); break;
		default: ss << tr("STR_UNKNOWN"); break;
	}
	if (_showIds)
	{
		ss << L" [" << value << L"]";
	}
	_lstRawData->addRow(2, propertyName.c_str(), ss.str().c_str());
	++_counter;
	if (value != defaultvalue)
	{
		_lstRawData->setCellColor(_lstRawData->getTexts() - 1, 1, _pink);
	}
}

/**
 * Adds a BattleFuseType to the table.
 */
void StatsForNerdsState::addBattleFuseType(std::wostringstream &ss, const BattleFuseType &value, const std::wstring &propertyName, const BattleFuseType &defaultvalue)
{
	if (value == defaultvalue && !_showDefaults)
	{
		return;
	}
	resetStream(ss);
	switch (value)
	{
		case BFT_NONE: ss << tr("BFT_NONE"); break;
		case BFT_INSTANT: ss << tr("BFT_INSTANT"); break;
		case BFT_SET: ss << tr("BFT_SET"); break;
		case BFT_FIX_MIN: ss << tr("BFT_FIX_MIN"); break;
		case BFT_FIX_MAX: ss << tr("BFT_FIX_MAX"); break;
		default: ss << tr("STR_UNKNOWN"); break;
	}
	if (_showIds)
	{
		ss << L" [" << value << L"]";
	}
	_lstRawData->addRow(2, propertyName.c_str(), ss.str().c_str());
	++_counter;
	if (value != defaultvalue)
	{
		_lstRawData->setCellColor(_lstRawData->getTexts() - 1, 1, _pink);
	}
}

/**
 * Adds a single RuleItemUseCost to the table.
 */
void StatsForNerdsState::addRuleItemUseCostBasic(std::wostringstream &ss, const RuleItemUseCost &value, const std::wstring &propertyName, const int &defaultvalue)
{
	if (value.Time == defaultvalue && !_showDefaults)
	{
		return;
	}
	resetStream(ss);
	ss << value.Time;
	_lstRawData->addRow(2, propertyName.c_str(), ss.str().c_str());
	++_counter;
	if (value.Time != defaultvalue)
	{
		_lstRawData->setCellColor(_lstRawData->getTexts() - 1, 1, _pink);
	}
}

/**
 * Adds a number to the string stream, optionally formatted as boolean.
 */
void StatsForNerdsState::addBoolOrInteger(std::wostringstream &ss, const int &value, bool formatAsBoolean)
{
	if (formatAsBoolean)
	{
		if (value == 1)
			ss << L"true";
		else if (value == 0)
			ss << L"false";
		else
			ss << value;
	}
	else
	{
		ss << value;
	}
}

/**
 * Adds a number to the string stream, optionally formatted as boolean.
 */
void StatsForNerdsState::addPercentageSignOrNothing(std::wostringstream &ss, const int &value, bool smartFormat)
{
	if (smartFormat)
	{
		// 1 means flat: true
		if (value != 1)
		{
			ss << L"%";
		}
	}
}

/**
 * Adds a full RuleItemUseCost to the table.
 */
void StatsForNerdsState::addRuleItemUseCostFull(std::wostringstream &ss, const RuleItemUseCost &value, const std::wstring &propertyName, const RuleItemUseCost &defaultvalue, bool smartFormat, const RuleItemUseCost &formatBy)
{
	bool isDefault = false;
	if (value.Time == defaultvalue.Time &&
		value.Energy == defaultvalue.Energy &&
		value.Morale == defaultvalue.Morale &&
		value.Health == defaultvalue.Health &&
		value.Stun == defaultvalue.Stun)
	{
		isDefault = true;
	}
	if (isDefault && !_showDefaults)
	{
		return;
	}
	bool isFlatAttribute = propertyName.find(L"flat") == 0;
	resetStream(ss);
	bool isFirst = true;
	// always show non-zero TUs, even if it's a default value
	if (value.Time != 0 || _showDefaults)
	{
		ss << L"time:";
		addBoolOrInteger(ss, value.Time, isFlatAttribute);
		addPercentageSignOrNothing(ss, formatBy.Time, smartFormat);
		isFirst = false;
	}
	if (value.Energy != defaultvalue.Energy || _showDefaults)
	{
		if (!isFirst) ss << L", ";
		ss << L"energy:";
		addBoolOrInteger(ss, value.Energy, isFlatAttribute);
		addPercentageSignOrNothing(ss, formatBy.Energy, smartFormat);
		isFirst = false;
	}
	if (value.Morale != defaultvalue.Morale || _showDefaults)
	{
		if (!isFirst) ss << L", ";
		ss << L"morale:";
		addBoolOrInteger(ss, value.Morale, isFlatAttribute);
		addPercentageSignOrNothing(ss, formatBy.Morale, smartFormat);
		isFirst = false;
	}
	if (value.Health != defaultvalue.Health || _showDefaults)
	{
		if (!isFirst) ss << L", ";
		ss << L"health:";
		addBoolOrInteger(ss, value.Health, isFlatAttribute);
		addPercentageSignOrNothing(ss, formatBy.Health, smartFormat);
		isFirst = false;
	}
	if (value.Stun != defaultvalue.Stun || _showDefaults)
	{
		if (!isFirst) ss << L", ";
		ss << L"stun:";
		addBoolOrInteger(ss, value.Stun, isFlatAttribute);
		addPercentageSignOrNothing(ss, formatBy.Stun, smartFormat);
		isFirst = false;
	}
	_lstRawData->addRow(2, propertyName.c_str(), ss.str().c_str());
	++_counter;
	if (!isDefault)
	{
		_lstRawData->setCellColor(_lstRawData->getTexts() - 1, 1, _pink);
	}
}

/**
 * Adds a BattleMediKitType to the table.
 */
void StatsForNerdsState::addBattleMediKitType(std::wostringstream &ss, const BattleMediKitType &value, const std::wstring &propertyName, const BattleMediKitType &defaultvalue)
{
	if (value == defaultvalue && !_showDefaults)
	{
		return;
	}
	resetStream(ss);
	switch (value)
	{
		case BMT_NORMAL: ss << tr("BMT_NORMAL"); break;
		case BMT_HEAL: ss << tr("BMT_HEAL"); break;
		case BMT_STIMULANT: ss << tr("BMT_STIMULANT"); break;
		case BMT_PAINKILLER: ss << tr("BMT_PAINKILLER"); break;
		default: ss << tr("STR_UNKNOWN"); break;
	}
	if (_showIds)
	{
		ss << L" [" << value << L"]";
	}
	_lstRawData->addRow(2, propertyName.c_str(), ss.str().c_str());
	++_counter;
	if (value != defaultvalue)
	{
		_lstRawData->setCellColor(_lstRawData->getTexts() - 1, 1, _pink);
	}
}

/**
 * Adds a ExperienceTrainingMode to the table.
 */
void StatsForNerdsState::addExperienceTrainingMode(std::wostringstream &ss, const ExperienceTrainingMode &value, const std::wstring &propertyName, const ExperienceTrainingMode &defaultvalue)
{
	if (value == defaultvalue && !_showDefaults)
	{
		return;
	}
	resetStream(ss);
	switch (value)
	{
		case ETM_DEFAULT: ss << tr("ETM_DEFAULT"); break;
		case ETM_MELEE_100: ss << tr("ETM_MELEE_100"); break;
		case ETM_MELEE_50: ss << tr("ETM_MELEE_50"); break;
		case ETM_MELEE_33: ss << tr("ETM_MELEE_33"); break;
		case ETM_FIRING_100: ss << tr("ETM_FIRING_100"); break;
		case ETM_FIRING_50: ss << tr("ETM_FIRING_50"); break;
		case ETM_FIRING_33: ss << tr("ETM_FIRING_33"); break;
		case ETM_THROWING_100: ss << tr("ETM_THROWING_100"); break;
		case ETM_THROWING_50: ss << tr("ETM_THROWING_50"); break;
		case ETM_THROWING_33: ss << tr("ETM_THROWING_33"); break;
		case ETM_FIRING_AND_THROWING: ss << tr("ETM_FIRING_AND_THROWING"); break;
		case ETM_FIRING_OR_THROWING: ss << tr("ETM_FIRING_OR_THROWING"); break;
		case ETM_REACTIONS: ss << tr("ETM_REACTIONS"); break;
		case ETM_REACTIONS_AND_MELEE: ss << tr("ETM_REACTIONS_AND_MELEE"); break;
		case ETM_REACTIONS_AND_FIRING: ss << tr("ETM_REACTIONS_AND_FIRING"); break;
		case ETM_REACTIONS_AND_THROWING: ss << tr("ETM_REACTIONS_AND_THROWING"); break;
		case ETM_REACTIONS_OR_MELEE: ss << tr("ETM_REACTIONS_OR_MELEE"); break;
		case ETM_REACTIONS_OR_FIRING: ss << tr("ETM_REACTIONS_OR_FIRING"); break;
		case ETM_REACTIONS_OR_THROWING: ss << tr("ETM_REACTIONS_OR_THROWING"); break;
		case ETM_BRAVERY: ss << tr("ETM_BRAVERY"); break;
		case ETM_BRAVERY_2X: ss << tr("ETM_BRAVERY_2X"); break;
		case ETM_BRAVERY_AND_REACTIONS: ss << tr("ETM_BRAVERY_AND_REACTIONS"); break;
		case ETM_BRAVERY_OR_REACTIONS: ss << tr("ETM_BRAVERY_OR_REACTIONS"); break;
		case ETM_BRAVERY_OR_REACTIONS_2X: ss << tr("ETM_BRAVERY_OR_REACTIONS_2X"); break;
		case ETM_PSI_STRENGTH: ss << tr("ETM_PSI_STRENGTH"); break;
		case ETM_PSI_STRENGTH_2X: ss << tr("ETM_PSI_STRENGTH_2X"); break;
		case ETM_PSI_SKILL: ss << tr("ETM_PSI_SKILL"); break;
		case ETM_PSI_SKILL_2X: ss << tr("ETM_PSI_SKILL_2X"); break;
		case ETM_PSI_STRENGTH_AND_SKILL: ss << tr("ETM_PSI_STRENGTH_AND_SKILL"); break;
		case ETM_PSI_STRENGTH_AND_SKILL_2X: ss << tr("ETM_PSI_STRENGTH_AND_SKILL_2X"); break;
		case ETM_PSI_STRENGTH_OR_SKILL: ss << tr("ETM_PSI_STRENGTH_OR_SKILL"); break;
		case ETM_PSI_STRENGTH_OR_SKILL_2X: ss << tr("ETM_PSI_STRENGTH_OR_SKILL_2X"); break;
		case ETM_NOTHING: ss << tr("ETM_NOTHING"); break;
		default: ss << tr("STR_UNKNOWN"); break;
	}
	if (_showIds)
	{
		ss << L" [" << value << L"]";
	}
	_lstRawData->addRow(2, propertyName.c_str(), ss.str().c_str());
	++_counter;
	if (value != defaultvalue)
	{
		_lstRawData->setCellColor(_lstRawData->getTexts() - 1, 1, _pink);
	}
}

/**
 * Adds a RuleStatBonus info to the table.
 */
void StatsForNerdsState::addRuleStatBonus(std::wostringstream &ss, const RuleStatBonus &value, const std::wstring &propertyName)
{
	if (!value.isModded() && !_showDefaults)
	{
		return;
	}
	resetStream(ss);
	bool isFirst = true;
	for (RuleStatBonusDataOrig item : *value.getBonusRaw())
	{
		bool isEmpty = true;
		for (float n : item.second)
		{
			if (!AreSame(n, 0.0f))
			{
				isEmpty = false;
				break;
			}
		}
		if (!isFirst && !isEmpty)
		{
			ss << L" + ";
		}
		int power = 0;
		for (float number : item.second)
		{
			++power;
			if (!AreSame(number, 0.0f))
			{
				if (item.first == "flatOne")
				{
					ss << number * 1;
				}
				if (item.first == "flatHundred")
				{
					ss << number * pow(100, power);
				}
				else
				{
					if (!AreSame(number, 1.0f))
					{
						ss << number << L"*";
					}
					addTranslation(ss, translationMap[item.first]);
					if (power > 1)
					{
						ss << L"^" << power;
					}
				}
				isFirst = false;
			}
		}
	}
	_lstRawData->addRow(2, propertyName.c_str(), ss.str().c_str());
	++_counter;
	if (value.isModded())
	{
		_lstRawData->setCellColor(_lstRawData->getTexts() - 1, 1, _pink);
	}
}


/**
 * Shows the "raw" RuleItem data.
 */
void StatsForNerdsState::initItemList()
{
	_lstRawData->clearList();

	std::wostringstream ssTopic;
	ssTopic << tr(_topicId);
	if (_showIds)
	{
		ssTopic << L" [" << Language::utf8ToWstr(_topicId) << L"]";
	}

	_txtArticle->setText(tr("STR_ARTICLE").arg(ssTopic.str()));

	Mod *mod = _game->getMod();
	RuleItem *itemRule = mod->getItem(_topicId);
	if (!itemRule)
		return;

	_filterOptions.clear();
	_filterOptions.push_back("STR_COMPATIBLE_AMMO");
	for (int slot = 0; slot < RuleItem::AmmoSlotMax; ++slot)
	{
		for (auto ammo : *itemRule->getCompatibleAmmoForSlot(slot))
		{
			_filterOptions.push_back(ammo);
		}
	}
	_cbxRelatedStuff->setOptions(_filterOptions);
	_cbxRelatedStuff->setVisible(_filterOptions.size() > 1);

	std::wostringstream ss;

	addSingleString(ss, itemRule->getName(), L"name", itemRule->getType());
	addSingleString(ss, itemRule->getNameAsAmmo(), L"nameAsAmmo");

	addBattleType(ss, itemRule->getBattleType(), L"battleType");
	addInteger(ss, itemRule->getWeight(), L"weight", 3);

	for (int i = 0; i < 2; i++)
	{
		const RuleDamageType *rule = (i == 0) ? itemRule->getDamageType() : itemRule->getMeleeType();
		if (i == 0)
		{
			addDamageType(ss, rule->ResistType, L"damageType", DAMAGE_TYPES); // always show damageType
			// blastRadius*
			addHeading(L"damageAlter:");
		}
		else
		{
			addDamageType(ss, rule->ResistType, L"meleeType", DT_MELEE);
			addHeading(L"meleeAlter:");
		}

		addInteger(ss, rule->FixRadius, L"  FixRadius", mod->getDamageType(rule->ResistType)->FixRadius);
		addDamageRandomType(ss, rule->RandomType, L"  RandomType", mod->getDamageType(rule->ResistType)->RandomType);

		addBoolean(ss, rule->FireBlastCalc, L"  FireBlastCalc", mod->getDamageType(rule->ResistType)->FireBlastCalc);
		addBoolean(ss, rule->IgnoreDirection, L"  IgnoreDirection", mod->getDamageType(rule->ResistType)->IgnoreDirection);
		addBoolean(ss, rule->IgnoreSelfDestruct, L"  IgnoreSelfDestruct", mod->getDamageType(rule->ResistType)->IgnoreSelfDestruct);
		addBoolean(ss, rule->IgnorePainImmunity, L"  IgnorePainImmunity", mod->getDamageType(rule->ResistType)->IgnorePainImmunity);
		addBoolean(ss, rule->IgnoreNormalMoraleLose, L"  IgnoreNormalMoraleLose", mod->getDamageType(rule->ResistType)->IgnoreNormalMoraleLose);
		addBoolean(ss, rule->IgnoreOverKill, L"  IgnoreOverKill", mod->getDamageType(rule->ResistType)->IgnoreOverKill);

		addFloat(ss, rule->ArmorEffectiveness, L"  ArmorEffectiveness", mod->getDamageType(rule->ResistType)->ArmorEffectiveness);
		addFloat(ss, rule->RadiusEffectiveness, L"  RadiusEffectiveness", mod->getDamageType(rule->ResistType)->RadiusEffectiveness);
		addFloat(ss, rule->RadiusReduction, L"  RadiusReduction", mod->getDamageType(rule->ResistType)->RadiusReduction);

		addInteger(ss, rule->FireThreshold, L"  FireThreshold", mod->getDamageType(rule->ResistType)->FireThreshold);
		addInteger(ss, rule->SmokeThreshold, L"  SmokeThreshold", mod->getDamageType(rule->ResistType)->SmokeThreshold);

		addFloat(ss, rule->ToHealth, L"  ToHealth", mod->getDamageType(rule->ResistType)->ToHealth);
		addFloat(ss, rule->ToArmor, L"  ToArmor", mod->getDamageType(rule->ResistType)->ToArmor);
		addFloat(ss, rule->ToArmorPre, L"  ToArmorPre", mod->getDamageType(rule->ResistType)->ToArmorPre);
		addFloat(ss, rule->ToWound, L"  ToWound", mod->getDamageType(rule->ResistType)->ToWound);
		addFloat(ss, rule->ToItem, L"  ToItem", mod->getDamageType(rule->ResistType)->ToItem);
		addFloat(ss, rule->ToTile, L"  ToTile", mod->getDamageType(rule->ResistType)->ToTile);
		addFloat(ss, rule->ToStun, L"  ToStun", mod->getDamageType(rule->ResistType)->ToStun);
		addFloat(ss, rule->ToEnergy, L"  ToEnergy", mod->getDamageType(rule->ResistType)->ToEnergy);
		addFloat(ss, rule->ToTime, L"  ToTime", mod->getDamageType(rule->ResistType)->ToTime);
		addFloat(ss, rule->ToMorale, L"  ToMorale", mod->getDamageType(rule->ResistType)->ToMorale);

		addBoolean(ss, rule->RandomHealth, L"  RandomHealth", mod->getDamageType(rule->ResistType)->RandomHealth);
		addBoolean(ss, rule->RandomArmor, L"  RandomArmor", mod->getDamageType(rule->ResistType)->RandomArmor);
		addBoolean(ss, rule->RandomArmorPre, L"  RandomArmorPre", mod->getDamageType(rule->ResistType)->RandomArmorPre);
		addBoolean(ss, rule->RandomWound, L"  RandomWound", mod->getDamageType(rule->ResistType)->RandomWound);
		addBoolean(ss, rule->RandomItem, L"  RandomItem", mod->getDamageType(rule->ResistType)->RandomItem);
		addBoolean(ss, rule->RandomTile, L"  RandomTile", mod->getDamageType(rule->ResistType)->RandomTile);
		addBoolean(ss, rule->RandomStun, L"  RandomStun", mod->getDamageType(rule->ResistType)->RandomStun);
		addBoolean(ss, rule->RandomEnergy, L"  RandomEnergy", mod->getDamageType(rule->ResistType)->RandomEnergy);
		addBoolean(ss, rule->RandomTime, L"  RandomTime", mod->getDamageType(rule->ResistType)->RandomTime);
		addBoolean(ss, rule->RandomMorale, L"  RandomMorale", mod->getDamageType(rule->ResistType)->RandomMorale);

		endHeading();
	}

	// skillApplied*
	// strengthApplied*

	addInteger(ss, itemRule->getPower(), L"power");

	BattleFuseType fuseTypeDefault = BFT_NONE;
	if (itemRule->getBattleType() == BT_PROXIMITYGRENADE)
	{
		fuseTypeDefault = BFT_INSTANT;
	}
	else if (itemRule->getBattleType() == BT_GRENADE)
	{
		fuseTypeDefault = BFT_SET;
	}
	addBattleFuseType(ss, itemRule->getFuseTimerType(), L"fuseType", fuseTypeDefault);
	addBoolean(ss, itemRule->isHiddenOnMinimap(), L"hiddenOnMinimap");
	addInteger(ss, itemRule->getClipSize(), L"clipSize");

	addHeading(L"fuseTriggerEvents:");
	{
		addBoolean(ss, itemRule->getFuseTriggerEvent()->defaultBehavior, L"  defaultBehavior", true);
		addBoolean(ss, itemRule->getFuseTriggerEvent()->throwTrigger, L"  throwTrigger");
		addBoolean(ss, itemRule->getFuseTriggerEvent()->throwExplode, L"  throwExplode");
		addBoolean(ss, itemRule->getFuseTriggerEvent()->proximityTrigger, L"  proximityTrigger");
		addBoolean(ss, itemRule->getFuseTriggerEvent()->proximityExplode, L"  proximityExplode");
		endHeading();
	}

	addIntegerPercent(ss, itemRule->getConfigAimed()->accuracy, L"accuracyAimed");
	addIntegerPercent(ss, itemRule->getConfigAuto()->accuracy, L"accuracyAuto");
	addIntegerPercent(ss, itemRule->getConfigSnap()->accuracy, L"accuracySnap");
	addIntegerPercent(ss, itemRule->getConfigMelee()->accuracy, L"accuracyMelee");
	addIntegerPercent(ss, itemRule->getAccuracyUse(), L"accuracyUse");
	addIntegerPercent(ss, itemRule->getAccuracyMind(), L"accuracyMindControl");
	addIntegerPercent(ss, itemRule->getAccuracyPanic(), L"accuracyPanic", 20);
	addIntegerPercent(ss, itemRule->getAccuracyThrow(), L"accuracyThrow", 100);
	addIntegerPercent(ss, itemRule->getAccuracyCloseQuarters(mod), L"accuracyCloseQuarters", 100); // not raw!
	addIntegerPercent(ss, itemRule->getNoLOSAccuracyPenalty(mod), L"noLOSAccuracyPenalty", -1); // not raw!

	addRuleItemUseCostFull(ss, itemRule->getCostAimed(), L"costAimed", RuleItemUseCost(), true, itemRule->getFlatAimed());
	addRuleItemUseCostFull(ss, itemRule->getCostAuto(), L"costAuto", RuleItemUseCost(), true, itemRule->getFlatAuto());
	addRuleItemUseCostFull(ss, itemRule->getCostSnap(), L"costSnap", RuleItemUseCost(), true, itemRule->getFlatSnap());
	addRuleItemUseCostFull(ss, itemRule->getCostMelee(), L"costMelee", RuleItemUseCost(), true, itemRule->getFlatMelee());
	int tuUseDefault = (itemRule->getBattleType() == BT_PSIAMP && itemRule->getPsiAttackName().empty()) ? 0 : 25;
	addRuleItemUseCostFull(ss, itemRule->getCostUse(), L"costUse", RuleItemUseCost(tuUseDefault), true, itemRule->getFlatUse());
	addRuleItemUseCostFull(ss, itemRule->getCostMind(), L"costMindcontrol", RuleItemUseCost(25), true, itemRule->getFlatUse()); // using flatUse!
	addRuleItemUseCostFull(ss, itemRule->getCostPanic(), L"costPanic", RuleItemUseCost(25), true, itemRule->getFlatUse()); // using flatUse!
	addRuleItemUseCostFull(ss, itemRule->getCostThrow(), L"costThrow", RuleItemUseCost(25), true, itemRule->getFlatThrow());
	addRuleItemUseCostFull(ss, itemRule->getCostPrime(), L"costPrime", RuleItemUseCost(50), true, itemRule->getFlatPrime());
	addRuleItemUseCostFull(ss, itemRule->getCostUnprime(), L"costUnprime", RuleItemUseCost(25), true, itemRule->getFlatUnprime());

	addHeading(L"confAimed:");
	{
		addInteger(ss, itemRule->getConfigAimed()->shots, L"  shots", 1);
		addSingleString(ss, itemRule->getConfigAimed()->name, L"  name", "STR_AIMED_SHOT");
		addInteger(ss, itemRule->getConfigAimed()->ammoSlot, L"  ammoSlot");
		endHeading();
	}

	addHeading(L"confAuto:");
	{
		addInteger(ss, itemRule->getConfigAuto()->shots, L"  shots", 3);
		addSingleString(ss, itemRule->getConfigAuto()->name, L"  name", "STR_AUTO_SHOT");
		addInteger(ss, itemRule->getConfigAuto()->ammoSlot, L"  ammoSlot");
		endHeading();
	}

	addHeading(L"confSnap:");
	{
		addInteger(ss, itemRule->getConfigSnap()->shots, L"  shots", 1);
		addSingleString(ss, itemRule->getConfigSnap()->name, L"  name", "STR_SNAP_SHOT");
		addInteger(ss, itemRule->getConfigSnap()->ammoSlot, L"  ammoSlot");
		endHeading();
	}

	addHeading(L"confMelee:");
	{
		addInteger(ss, itemRule->getConfigMelee()->shots, L"  shots", 1);
		addSingleString(ss, itemRule->getConfigMelee()->name, L"  name");
		int ammoSlotDefault = itemRule->getBattleType() == BT_MELEE ? 0 : -1;
		addInteger(ss, itemRule->getConfigMelee()->ammoSlot, L"  ammoSlot", ammoSlotDefault);
		endHeading();
	}

	// compatibleAmmo*
	// tuLoad*
	// tuUnload*

	addHeading(L"primaryAmmo:");
	{
		addVectorOfStrings(ss, *itemRule->getCompatibleAmmoForSlot(0), L"  compatibleAmmo");
		addInteger(ss, itemRule->getTULoad(0), L"  tuLoad", 15);
		addInteger(ss, itemRule->getTUUnload(0), L"  tuUnload", 8);
		endHeading();
	}

	addHeading(L"ammo[1]:");
	{
		addVectorOfStrings(ss, *itemRule->getCompatibleAmmoForSlot(1), L"  compatibleAmmo");
		addInteger(ss, itemRule->getTULoad(1), L"  tuLoad", 15);
		addInteger(ss, itemRule->getTUUnload(1), L"  tuUnload", 8);
		endHeading();
	}

	addHeading(L"ammo[2]:");
	{
		addVectorOfStrings(ss, *itemRule->getCompatibleAmmoForSlot(2), L"  compatibleAmmo");
		addInteger(ss, itemRule->getTULoad(2), L"  tuLoad", 15);
		addInteger(ss, itemRule->getTUUnload(2), L"  tuUnload", 8);
		endHeading();
	}

	addHeading(L"ammo[3]:");
	{
		addVectorOfStrings(ss, *itemRule->getCompatibleAmmoForSlot(3), L"  compatibleAmmo");
		addInteger(ss, itemRule->getTULoad(3), L"  tuLoad", 15);
		addInteger(ss, itemRule->getTUUnload(3), L"  tuUnload", 8);
		endHeading();
	}

	addInteger(ss, itemRule->getSpecialChance(), L"specialChance", 100);
	addInteger(ss, itemRule->getWaypoints(), L"waypoints");
	addBoolean(ss, itemRule->isFixed(), L"fixedWeapon");
	addBoolean(ss, itemRule->getFixedShow(), L"fixedWeaponShow");
	addSingleString(ss, itemRule->getDefaultInventorySlot(), L"defaultInventorySlot");
	addVectorOfStrings(ss, itemRule->getSupportedInventorySections(), L"supportedInventorySections");
	addBoolean(ss, itemRule->getAllowSelfHeal(), L"allowSelfHeal");
	addBoolean(ss, itemRule->isConsumable(), L"isConsumable");
	addBoolean(ss, itemRule->isFireExtinguisher(), L"isFireExtinguisher");
	addBoolean(ss, itemRule->isExplodingInHands(), L"isExplodingInHands");

	addInteger(ss, itemRule->getPainKillerQuantity(), L"painKiller");
	addInteger(ss, itemRule->getHealQuantity(), L"heal");
	addInteger(ss, itemRule->getStimulantQuantity(), L"stimulant");
	addInteger(ss, itemRule->getWoundRecovery(), L"woundRecovery");
	addInteger(ss, itemRule->getHealthRecovery(), L"healthRecovery");
	addInteger(ss, itemRule->getStunRecovery(), L"stunRecovery");
	addInteger(ss, itemRule->getEnergyRecovery(), L"energyRecovery");
	addInteger(ss, itemRule->getMoraleRecovery(), L"moraleRecovery");
	addFloat(ss, itemRule->getPainKillerRecovery(), L"painKillerRecovery", 1.0f);
	addBattleMediKitType(ss, itemRule->getMediKitType(), L"medikitType");

	addInteger(ss, itemRule->getArmor(), L"armor", 20);
	addInteger(ss, itemRule->getTurretType(), L"turretType", -1);

	addBoolean(ss, itemRule->isRecoverable(), L"recover", true);
	addBoolean(ss, itemRule->isCorpseRecoverable(), L"recoverCorpse", true);
	addBoolean(ss, !itemRule->canBeEquippedBeforeBaseDefense(), L"ignoreInBaseDefense"); // negated!
	addBoolean(ss, itemRule->isAlien(), L"liveAlien");
	addInteger(ss, itemRule->getPrisonType(), L"prisonType");
	addInteger(ss, itemRule->getAttraction(), L"attraction");
	addBoolean(ss, itemRule->getArcingShot(), L"arcingShot");

	addExperienceTrainingMode(ss, itemRule->getExperienceTrainingMode(), L"experienceTrainingMode");

	addInteger(ss, itemRule->getMaxRange(), L"maxRange", 200);
	int aimRangeDefault = itemRule->getBattleType() == BT_PSIAMP ? 0 : 200;
	addInteger(ss, itemRule->getAimRange(), L"aimRange", aimRangeDefault);
	addInteger(ss, itemRule->getAutoRange(), L"autoRange", 7);
	addInteger(ss, itemRule->getSnapRange(), L"snapRange", 15);
	addInteger(ss, itemRule->getMinRange(), L"minRange");
	int dropoffDefault = itemRule->getBattleType() == BT_PSIAMP ? 1 : 2;
	addInteger(ss, itemRule->getDropoff(), L"dropoff", dropoffDefault);

	// autoShots*

	addInteger(ss, itemRule->getShotgunPellets(), L"shotgunPellets");
	addInteger(ss, itemRule->getShotgunBehaviorType(), L"shotgunBehavior");
	addInteger(ss, itemRule->getShotgunSpread(), L"shotgunSpread", 100);
	addInteger(ss, itemRule->getShotgunChoke(), L"shotgunChoke", 100);
	addSingleString(ss, itemRule->getZombieUnit(), L"zombieUnit");
	addBoolean(ss, itemRule->isLOSRequired(), L"LOSRequired");
	addInteger(ss, itemRule->getMeleePower(), L"meleePower");
	addBoolean(ss, itemRule->isWaterOnly(), L"underwaterOnly");
	addBoolean(ss, itemRule->isLandOnly(), L"landOnly");

	addInteger(ss, itemRule->getSpecialType(), L"specialType", -1);
	addInteger(ss, itemRule->getCustomItemPreviewIndex(), L"customItemPreviewIndex");
	addIntegerPercent(ss, itemRule->getKneelBonus(mod), L"kneelBonus", 115); // not raw!
	addBoolean(ss, itemRule->isTwoHanded(), L"twoHanded");
	addBoolean(ss, itemRule->isBlockingBothHands(), L"blockBothHands");
	int oneHandedPenaltyCurrent = itemRule->getOneHandedPenalty(mod);
	int oneHandedPenaltyDefault = itemRule->isTwoHanded() ? 80 : oneHandedPenaltyCurrent;
	addIntegerPercent(ss, oneHandedPenaltyCurrent, L"oneHandedPenalty", oneHandedPenaltyDefault); // not raw!
	addInteger(ss, itemRule->getMonthlySalary(), L"monthlySalary");
	addInteger(ss, itemRule->getMonthlyMaintenance(), L"monthlyMaintenance");

	addRuleStatBonus(ss, *itemRule->getDamageBonusRaw(), L"damageBonus");
	addRuleStatBonus(ss, *itemRule->getMeleeBonusRaw(), L"meleeBonus");
	addRuleStatBonus(ss, *itemRule->getAccuracyMultiplierRaw(), L"accuracyMultiplier");
	addRuleStatBonus(ss, *itemRule->getMeleeMultiplierRaw(), L"meleeMultiplier");
	addRuleStatBonus(ss, *itemRule->getThrowMultiplierRaw(), L"throwMultiplier");
	addRuleStatBonus(ss, *itemRule->getCloseQuartersMultiplierRaw(), L"closeQuartersMultiplier");

	addFloat(ss, itemRule->getPowerRangeReductionRaw(), L"powerRangeReduction");
	addFloat(ss, itemRule->getPowerRangeThresholdRaw(), L"powerRangeThreshold");
	int psiRequiredDefault = itemRule->getBattleType() == BT_PSIAMP ? true : false;
	addBoolean(ss, itemRule->isPsiRequired(), L"psiRequired", psiRequiredDefault);

	addDouble(ss, itemRule->getSize(), L"size");
	addInteger(ss, itemRule->getBuyCost(), L"costBuy");
	addInteger(ss, itemRule->getSellCost(), L"costSell");
	addInteger(ss, itemRule->getTransferTime(), L"transferTime", 24);

	addVectorOfStrings(ss, itemRule->getRequirements(), L"requires");
	addVectorOfStrings(ss, itemRule->getBuyRequirements(), L"requiresBuy");
	addVectorOfStrings(ss, itemRule->getCategories(), L"categories");

	if (_showDebug)
	{
		addSection(L"DEBUG section", L"Intended for modders only", _white);

		addInteger(ss, itemRule->getBigSprite(), L"bigSprite", -1);
		addInteger(ss, itemRule->getFloorSprite(), L"floorSprite", -1);
		addInteger(ss, itemRule->getHandSprite(), L"handSprite", 120);
		addInteger(ss, itemRule->getBulletSprite(), L"bulletSprite", -1);

		addVectorOfIntegers(ss, itemRule->getFireSoundRaw(), L"fireSound");
		addVectorOfIntegers(ss, itemRule->getHitSoundRaw(), L"hitSound");
		addVectorOfIntegers(ss, itemRule->getHitMissSoundRaw(), L"hitMissSound");
		addVectorOfIntegers(ss, itemRule->getMeleeSoundRaw(), L"meleeSound");
		addVectorOfIntegers(ss, itemRule->getMeleeMissSoundRaw(), L"meleeMissSound");
		addVectorOfIntegers(ss, itemRule->getPsiSoundRaw(), L"psiSound");
		addVectorOfIntegers(ss, itemRule->getPsiMissSoundRaw(), L"psiMissSound");

		addInteger(ss, itemRule->getHitAnimation(), L"hitAnimation");
		addInteger(ss, itemRule->getHitMissAnimation(), L"hitMissAnimation", -1);
		addInteger(ss, itemRule->getMeleeAnimation(), L"meleeAnimation");
		addInteger(ss, itemRule->getMeleeMissAnimation(), L"meleeMissAnimation", -1);
		addInteger(ss, itemRule->getPsiAnimation(), L"psiAnimation", -1);
		addInteger(ss, itemRule->getPsiMissAnimation(), L"psiMissAnimation", -1);

		addVectorOfIntegers(ss, itemRule->getMeleeHitSoundRaw(), L"meleeHitSound");
		addVectorOfIntegers(ss, itemRule->getExplosionHitSoundRaw(), L"explosionHitSound");

		addInteger(ss, itemRule->getBulletSpeed(), L"bulletSpeed");
		addInteger(ss, itemRule->getExplosionSpeed(), L"explosionSpeed");

		addInteger(ss, itemRule->getVaporColor(), L"vaporColor", -1);
		addInteger(ss, itemRule->getVaporDensity(), L"vaporDensity");
		addInteger(ss, itemRule->getVaporProbability(), L"vaporProbability", 15);

		addSingleString(ss, itemRule->getPsiAttackName(), L"psiAttackName");
		addSingleString(ss, itemRule->getPrimeActionName(), L"primeActionName", "STR_PRIME_GRENADE");
		addSingleString(ss, itemRule->getPrimeActionMessage(), L"primeActionMessage", "STR_GRENADE_IS_ACTIVATED");
		addSingleString(ss, itemRule->getUnprimeActionName(), L"unprimeActionName");
		addSingleString(ss, itemRule->getUnprimeActionMessage(), L"unprimeActionMessage", "STR_GRENADE_IS_DEACTIVATED");

		addInteger(ss, itemRule->getInventoryWidth(), L"invWidth", 1);
		addInteger(ss, itemRule->getInventoryHeight(), L"invHeight", 1);

		addHeading(L"ai:");
		{
			int useDelayCurrent = itemRule->getAIUseDelay(mod);
			int useDelayDefault = useDelayCurrent > 0 ? -1 : useDelayCurrent;
			addInteger(ss, useDelayCurrent, L"  useDelay", useDelayDefault); // not raw!
			addInteger(ss, itemRule->getAIMeleeHitCount(), L"  meleeHitCount", 25);
			endHeading();
		}

		addRuleItemUseCostBasic(ss, itemRule->getCostAimed(), L"tuAimed");
		addRuleItemUseCostBasic(ss, itemRule->getCostAuto(), L"tuAuto");
		addRuleItemUseCostBasic(ss, itemRule->getCostSnap(), L"tuSnap");
		addRuleItemUseCostBasic(ss, itemRule->getCostMelee(), L"tuMelee");
		int tuUseDefault = (itemRule->getBattleType() == BT_PSIAMP && itemRule->getPsiAttackName().empty()) ? 0 : 25;
		addRuleItemUseCostBasic(ss, itemRule->getCostUse(), L"tuUse", tuUseDefault);
		addRuleItemUseCostBasic(ss, itemRule->getCostMind(), L"tuMindcontrol", 25);
		addRuleItemUseCostBasic(ss, itemRule->getCostPanic(), L"tuPanic", 25);
		addRuleItemUseCostBasic(ss, itemRule->getCostThrow(), L"tuThrow", 25);
		addRuleItemUseCostBasic(ss, itemRule->getCostPrime(), L"tuPrime", 50);
		addRuleItemUseCostBasic(ss, itemRule->getCostUnprime(), L"tuUnprime", 25);

		// flatRate*

		addRuleItemUseCostFull(ss, itemRule->getFlatAimed(), L"flatAimed", RuleItemUseCost(0, 1));
		addRuleItemUseCostFull(ss, itemRule->getFlatAuto(), L"flatAuto", RuleItemUseCost(0, 1));
		addRuleItemUseCostFull(ss, itemRule->getFlatSnap(), L"flatSnap", RuleItemUseCost(0, 1));
		addRuleItemUseCostFull(ss, itemRule->getFlatMelee(), L"flatMelee", RuleItemUseCost(0, 1));
		addRuleItemUseCostFull(ss, itemRule->getFlatUse(), L"flatUse", RuleItemUseCost(0, 1));
		addRuleItemUseCostFull(ss, itemRule->getFlatThrow(), L"flatThrow", RuleItemUseCost(0, 1));
		addRuleItemUseCostFull(ss, itemRule->getFlatPrime(), L"flatPrime", RuleItemUseCost(0, 1));
		addRuleItemUseCostFull(ss, itemRule->getFlatUnprime(), L"flatUnprime", RuleItemUseCost(0, 1));

		addSingleString(ss, itemRule->getType(), L"type");
		addInteger(ss, itemRule->getRecoveryPoints(), L"recoveryPoints");
		addInteger(ss, itemRule->getListOrder(), L"listOrder");
	}
}

}
