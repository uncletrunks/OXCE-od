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
#include "Ufopaedia.h"
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
#include "../Mod/Armor.h"
#include "../Mod/ExtraSounds.h"
#include "../Mod/ExtraSprites.h"
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
StatsForNerdsState::StatsForNerdsState(const ArticleDefinition *article, size_t currentDetailIndex, bool debug, bool ids, bool defaults) : _counter(0), _indent(false)
{
	_typeId = article->getType();
	_topicId = article->id;
	_mainArticle = true;
	_currentDetailIndex = currentDetailIndex;

	buildUI(debug, ids, defaults);
}

/**
 * Initializes all the elements on the UI.
 */
StatsForNerdsState::StatsForNerdsState(const UfopaediaTypeId typeId, const std::string topicId, bool debug, bool ids, bool defaults) : _counter(0), _indent(false)
{
	_typeId = typeId;
	_topicId = topicId;
	_mainArticle = false;
	_currentDetailIndex = 0; // dummy

	buildUI(debug, ids, defaults);
}

/**
 * Builds the UI.
 */
void StatsForNerdsState::buildUI(bool debug, bool ids, bool defaults)
{
	// Create objects
	_window = new Window(this, 320, 200, 0, 0);
	_txtTitle = new Text(304, 17, 8, 7);
	_cbxRelatedStuff = new ComboBox(this, 148, 16, 164, 7);
	_txtArticle = new Text(230, 9, 8, 24);
	_btnPrev = new TextButton(33, 14, 242, 24);
	_btnNext = new TextButton(33, 14, 279, 24);
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
	add(_btnPrev, "button", "statsForNerds");
	add(_btnNext, "button", "statsForNerds");
	add(_cbxRelatedStuff, "comboBox", "statsForNerds");

	centerAllSurfaces();

	// Set up objects
	_window->setBackground(_game->getMod()->getSurface("BACK05.SCR"));

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_STATS_FOR_NERDS"));

	_txtArticle->setText(tr("STR_ARTICLE").arg(tr(_topicId)));

	_lstRawData->setColumns(2, 110, 177);
	_lstRawData->setSelectable(true);
	_lstRawData->setBackground(_window);
	_lstRawData->setWordWrap(true);

	_btnIncludeDebug->setText(tr("STR_INCLUDE_DEBUG"));
	_btnIncludeDebug->setPressed(debug);
	_btnIncludeDebug->onMouseClick((ActionHandler)&StatsForNerdsState::btnRefreshClick);

	_btnIncludeIds->setText(tr("STR_INCLUDE_IDS"));
	_btnIncludeIds->setPressed(ids);
	_btnIncludeIds->onMouseClick((ActionHandler)&StatsForNerdsState::btnRefreshClick);

	_btnIncludeDefaults->setText(tr("STR_INCLUDE_DEFAULTS"));
	_btnIncludeDefaults->setPressed(defaults);
	_btnIncludeDefaults->onMouseClick((ActionHandler)&StatsForNerdsState::btnRefreshClick);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&StatsForNerdsState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&StatsForNerdsState::btnOkClick, Options::keyCancel);
	_btnOk->onKeyboardPress((ActionHandler)&StatsForNerdsState::btnScrollUpClick, Options::keyGeoUp);
	_btnOk->onKeyboardPress((ActionHandler)&StatsForNerdsState::btnScrollDownClick, Options::keyGeoDown);

	_btnPrev->setText(L"<<");
	_btnPrev->onMouseClick((ActionHandler)&StatsForNerdsState::btnPrevClick);
	_btnPrev->onKeyboardPress((ActionHandler)&StatsForNerdsState::btnPrevClick, Options::keyGeoLeft);

	_btnNext->setText(L">>");
	_btnNext->onMouseClick((ActionHandler)&StatsForNerdsState::btnNextClick);
	_btnNext->onKeyboardPress((ActionHandler)&StatsForNerdsState::btnNextClick, Options::keyGeoRight);

	if (!_mainArticle)
	{
		_btnPrev->setVisible(false);
		_btnNext->setVisible(false);
	}

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
		_game->pushState(new StatsForNerdsState(UFOPAEDIA_TYPE_ITEM, _filterOptions.at(selIdx), _btnIncludeDebug->getPressed(), _btnIncludeIds->getPressed(), _btnIncludeDefaults->getPressed()));
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
	bool ctrlPressed = SDL_GetModState() & KMOD_CTRL;

	if (ctrlPressed)
	{
		Log(LOG_INFO) << Language::wstrToUtf8(_txtArticle->getText());
		for (int row = 0; row < _lstRawData->getTexts(); ++row)
		{
			Log(LOG_INFO) << Language::wstrToUtf8(_lstRawData->getCellText(row, 0)) << "\t\t" << Language::wstrToUtf8(_lstRawData->getCellText(row, 1));
		}
		return;
	}

	_game->popState();
}

/**
 * Shows the previous available article.
 * @param action Pointer to an action.
 */
void StatsForNerdsState::btnPrevClick(Action *)
{
	Ufopaedia::prevDetail(_game, _currentDetailIndex, _btnIncludeDebug->getPressed(), _btnIncludeIds->getPressed(), _btnIncludeDefaults->getPressed());
}

/**
 * Shows the next available article. Loops to the first.
 * @param action Pointer to an action.
 */
void StatsForNerdsState::btnNextClick(Action *)
{
	Ufopaedia::nextDetail(_game, _currentDetailIndex, _btnIncludeDebug->getPressed(), _btnIncludeIds->getPressed(), _btnIncludeDefaults->getPressed());
}

/**
 * Scrolls the main table up by one row.
 * @param action Pointer to an action.
 */
void StatsForNerdsState::btnScrollUpClick(Action *)
{
	_lstRawData->scrollUp(false);
}

/**
 * Scrolls the main table down by one row.
 * @param action Pointer to an action.
 */
void StatsForNerdsState::btnScrollDownClick(Action *)
{
	_lstRawData->scrollDown(false);
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
	_indent = false;

	switch (_typeId)
	{
	case UFOPAEDIA_TYPE_ITEM:
		initItemList();
		break;
	case UFOPAEDIA_TYPE_ARMOR:
		initArmorList();
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
 * Translates a property name.
 */
std::wstring StatsForNerdsState::trp(const std::string &propertyName)
{
	if (_showDebug)
	{
		if (_indent)
		{
			std::wstring indentation = L" ";
			std::wstring code = Language::utf8ToWstr(propertyName);
			return indentation + code;
		}
		else
		{
			return Language::utf8ToWstr(propertyName);
		}
	}
	else
	{
		if (_indent)
		{
			std::wstring indentation = L" ";
			std::wstring translation = tr(propertyName);
			return indentation + translation;
		}
		else
		{
			return tr(propertyName);
		}
	}
}

/**
 * Adds a section name to the table.
 */
void StatsForNerdsState::addSection(const std::wstring &name, const std::wstring &desc, Uint8 color, bool forceShow)
{
	if (_showDefaults || forceShow)
	{
		_lstRawData->addRow(2, name.c_str(), desc.c_str());
		_lstRawData->setRowColor(_lstRawData->getTexts() - 1, color);
	}

	// reset counter
	_counter = 0;
}

/**
 * Adds a group heading to the table.
 */
void StatsForNerdsState::addHeading(const std::string &propertyName)
{
	_lstRawData->addRow(2, trp(propertyName).c_str(), L"");
	_lstRawData->setCellColor(_lstRawData->getTexts() - 1, 0, _blue);

	// reset counter
	_counter = 0;
	_indent = true;
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
	_indent = false;
}

/**
 * Adds a single string value to the table.
 */
void StatsForNerdsState::addSingleString(std::wostringstream &ss, const std::string &id, const std::string &propertyName, const std::string &defaultId, bool translate)
{
	if (id == defaultId && !_showDefaults)
	{
		return;
	}
	resetStream(ss);
	if (translate)
	{
		addTranslation(ss, id);
	}
	else
	{
		ss << Language::utf8ToWstr(id);
	}
	_lstRawData->addRow(2, trp(propertyName).c_str(), ss.str().c_str());
	++_counter;
	if (id != defaultId)
	{
		_lstRawData->setCellColor(_lstRawData->getTexts() - 1, 1, _pink);
	}
}

/**
 * Adds a vector of strings to the table.
 */
void StatsForNerdsState::addVectorOfStrings(std::wostringstream &ss, const std::vector<std::string> &vec, const std::string &propertyName)
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
	_lstRawData->addRow(2, trp(propertyName).c_str(), ss.str().c_str());
	++_counter;
	if (!vec.empty())
	{
		_lstRawData->setCellColor(_lstRawData->getTexts() - 1, 1, _pink);
	}
}

/**
 * Adds a single boolean value to the table.
 */
void StatsForNerdsState::addBoolean(std::wostringstream &ss, const bool &value, const std::string &propertyName, const bool &defaultvalue)
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
	_lstRawData->addRow(2, trp(propertyName).c_str(), ss.str().c_str());
	++_counter;
	if (value != defaultvalue)
	{
		_lstRawData->setCellColor(_lstRawData->getTexts() - 1, 1, _pink);
	}
}

/**
 * Adds a single floating point number to the table.
 */
void StatsForNerdsState::addFloat(std::wostringstream &ss, const float &value, const std::string &propertyName, const float &defaultvalue)
{
	if (AreSame(value, defaultvalue) && !_showDefaults)
	{
		return;
	}
	resetStream(ss);
	ss << value;
	_lstRawData->addRow(2, trp(propertyName).c_str(), ss.str().c_str());
	++_counter;
	if (!AreSame(value, defaultvalue))
	{
		_lstRawData->setCellColor(_lstRawData->getTexts() - 1, 1, _pink);
	}
}

/**
 * Adds a single floating point number (formatted as percentage) to the table.
 */
void StatsForNerdsState::addFloatAsPercentage(std::wostringstream &ss, const float &value, const std::string &propertyName, const float &defaultvalue)
{
	if (AreSame(value, defaultvalue) && !_showDefaults)
	{
		return;
	}
	resetStream(ss);
	ss << (value * 100.0f) << L"%";
	if (_showIds)
	{
		ss << L" [" << value << L"]";
	}
	_lstRawData->addRow(2, trp(propertyName).c_str(), ss.str().c_str());
	++_counter;
	if (!AreSame(value, defaultvalue))
	{
		_lstRawData->setCellColor(_lstRawData->getTexts() - 1, 1, _pink);
	}
}

/**
 * Adds a single floating point (double precision) number to the table.
 */
void StatsForNerdsState::addDouble(std::wostringstream &ss, const double &value, const std::string &propertyName, const double &defaultvalue)
{
	if (AreSame(value, defaultvalue) && !_showDefaults)
	{
		return;
	}
	resetStream(ss);
	ss << value;
	_lstRawData->addRow(2, trp(propertyName).c_str(), ss.str().c_str());
	++_counter;
	if (!AreSame(value, defaultvalue))
	{
		_lstRawData->setCellColor(_lstRawData->getTexts() - 1, 1, _pink);
	}
}

/**
 * Adds a single integer number to the table.
 */
void StatsForNerdsState::addInteger(std::wostringstream &ss, const int &value, const std::string &propertyName, const int &defaultvalue, bool formatAsMoney, const std::string &specialTranslation, const int &specialvalue)
{
	if (value == defaultvalue && !_showDefaults)
	{
		return;
	}
	resetStream(ss);
	if (value == specialvalue && !specialTranslation.empty())
	{
		ss << tr(specialTranslation);
		if (_showIds)
		{
			ss << L" [" << specialvalue << L"]";
		}
	}
	else
	{
		if (formatAsMoney)
		{
			ss << Text::formatFunding(value);
		}
		else
		{
			ss << value;
		}
	}
	_lstRawData->addRow(2, trp(propertyName).c_str(), ss.str().c_str());
	++_counter;
	if (value != defaultvalue)
	{
		_lstRawData->setCellColor(_lstRawData->getTexts() - 1, 1, _pink);
	}
}

/**
 * Adds a script tag to the table.
 */
void StatsForNerdsState::addIntegerScriptTag(std::wostringstream &ss, const int &value, const std::string &propertyName, const int &defaultvalue)
{
	if (value == defaultvalue && !_showDefaults)
	{
		return;
	}
	resetStream(ss);
	ss << trp(propertyName);
	ss << L": ";
	ss << value;
	_lstRawData->setFlooding(true);
	_lstRawData->addRow(1, ss.str().c_str());
	_lstRawData->setFlooding(false);
	++_counter;
	if (value != defaultvalue)
	{
		_lstRawData->setCellColor(_lstRawData->getTexts() - 1, 0, _pink);
	}
}

/**
 * Adds a single integer number (representing a percentage) to the table.
 */
void StatsForNerdsState::addIntegerPercent(std::wostringstream &ss, const int &value, const std::string &propertyName, const int &defaultvalue)
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
	_lstRawData->addRow(2, trp(propertyName).c_str(), ss.str().c_str());
	++_counter;
	if (value != defaultvalue)
	{
		_lstRawData->setCellColor(_lstRawData->getTexts() - 1, 1, _pink);
	}
}

/**
 * Adds a vector of integer numbers to the table.
 */
void StatsForNerdsState::addVectorOfIntegers(std::wostringstream &ss, const std::vector<int> &vec, const std::string &propertyName)
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
	_lstRawData->addRow(2, trp(propertyName).c_str(), ss.str().c_str());
	++_counter;
	if (!vec.empty())
	{
		_lstRawData->setCellColor(_lstRawData->getTexts() - 1, 1, _pink);
	}
}

/**
 * Adds a BattleType to the table.
 */
void StatsForNerdsState::addBattleType(std::wostringstream &ss, const BattleType &value, const std::string &propertyName, const BattleType &defaultvalue)
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
	_lstRawData->addRow(2, trp(propertyName).c_str(), ss.str().c_str());
	++_counter;
	if (value != defaultvalue)
	{
		_lstRawData->setCellColor(_lstRawData->getTexts() - 1, 1, _pink);
	}
}

/**
 * Adds a DamageType to the table.
 */
void StatsForNerdsState::addDamageType(std::wostringstream &ss, const ItemDamageType &value, const std::string &propertyName, const ItemDamageType &defaultvalue)
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
	_lstRawData->addRow(2, trp(propertyName).c_str(), ss.str().c_str());
	++_counter;
	if (value != defaultvalue)
	{
		_lstRawData->setCellColor(_lstRawData->getTexts() - 1, 1, _pink);
	}
}

/**
 * Adds a DamageRandomType to the table.
 */
void StatsForNerdsState::addDamageRandomType(std::wostringstream &ss, const ItemDamageRandomType &value, const std::string &propertyName, const ItemDamageRandomType &defaultvalue)
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
	_lstRawData->addRow(2, trp(propertyName).c_str(), ss.str().c_str());
	++_counter;
	if (value != defaultvalue)
	{
		_lstRawData->setCellColor(_lstRawData->getTexts() - 1, 1, _pink);
	}
}

/**
 * Adds a BattleFuseType to the table.
 */
void StatsForNerdsState::addBattleFuseType(std::wostringstream &ss, const BattleFuseType &value, const std::string &propertyName, const BattleFuseType &defaultvalue)
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
		default:
		{
			if (value >= BFT_FIX_MIN && value < BFT_FIX_MAX)
			{
				ss << tr("BFT_FIXED");
			}
			else
			{
				ss << tr("STR_UNKNOWN");
			}
			break;
		}
	}
	if (_showIds)
	{
		ss << L" [" << value << L"]";
	}
	_lstRawData->addRow(2, trp(propertyName).c_str(), ss.str().c_str());
	++_counter;
	if (value != defaultvalue)
	{
		_lstRawData->setCellColor(_lstRawData->getTexts() - 1, 1, _pink);
	}
}

/**
 * Adds a single RuleItemUseCost to the table.
 */
void StatsForNerdsState::addRuleItemUseCostBasic(std::wostringstream &ss, const RuleItemUseCost &value, const std::string &propertyName, const int &defaultvalue)
{
	if (value.Time == defaultvalue && !_showDefaults)
	{
		return;
	}
	resetStream(ss);
	ss << value.Time;
	_lstRawData->addRow(2, trp(propertyName).c_str(), ss.str().c_str());
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
void StatsForNerdsState::addRuleItemUseCostFull(std::wostringstream &ss, const RuleItemUseCost &value, const std::string &propertyName, const RuleItemUseCost &defaultvalue, bool smartFormat, const RuleItemUseCost &formatBy)
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
	bool isFlatAttribute = propertyName.find("flat") == 0;
	resetStream(ss);
	bool isFirst = true;
	// always show non-zero TUs, even if it's a default value
	if (value.Time != 0 || _showDefaults)
	{
		ss << tr("STR_COST_TIME") << L": ";
		addBoolOrInteger(ss, value.Time, isFlatAttribute);
		addPercentageSignOrNothing(ss, formatBy.Time, smartFormat);
		isFirst = false;
	}
	if (value.Energy != defaultvalue.Energy || _showDefaults)
	{
		if (!isFirst) ss << L", ";
		ss << tr("STR_COST_ENERGY") << L": ";
		addBoolOrInteger(ss, value.Energy, isFlatAttribute);
		addPercentageSignOrNothing(ss, formatBy.Energy, smartFormat);
		isFirst = false;
	}
	if (value.Morale != defaultvalue.Morale || _showDefaults)
	{
		if (!isFirst) ss << L", ";
		ss << tr("STR_COST_MORALE") << L": ";
		addBoolOrInteger(ss, value.Morale, isFlatAttribute);
		addPercentageSignOrNothing(ss, formatBy.Morale, smartFormat);
		isFirst = false;
	}
	if (value.Health != defaultvalue.Health || _showDefaults)
	{
		if (!isFirst) ss << L", ";
		ss << tr("STR_COST_HEALTH") << L": ";
		addBoolOrInteger(ss, value.Health, isFlatAttribute);
		addPercentageSignOrNothing(ss, formatBy.Health, smartFormat);
		isFirst = false;
	}
	if (value.Stun != defaultvalue.Stun || _showDefaults)
	{
		if (!isFirst) ss << L", ";
		ss << tr("STR_COST_STUN") << L": ";
		addBoolOrInteger(ss, value.Stun, isFlatAttribute);
		addPercentageSignOrNothing(ss, formatBy.Stun, smartFormat);
		isFirst = false;
	}
	_lstRawData->addRow(2, trp(propertyName).c_str(), ss.str().c_str());
	++_counter;
	if (!isDefault)
	{
		_lstRawData->setCellColor(_lstRawData->getTexts() - 1, 1, _pink);
	}
}

/**
 * Adds a BattleMediKitType to the table.
 */
void StatsForNerdsState::addBattleMediKitType(std::wostringstream &ss, const BattleMediKitType &value, const std::string &propertyName, const BattleMediKitType &defaultvalue)
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
	_lstRawData->addRow(2, trp(propertyName).c_str(), ss.str().c_str());
	++_counter;
	if (value != defaultvalue)
	{
		_lstRawData->setCellColor(_lstRawData->getTexts() - 1, 1, _pink);
	}
}

/**
 * Adds a ExperienceTrainingMode to the table.
 */
void StatsForNerdsState::addExperienceTrainingMode(std::wostringstream &ss, const ExperienceTrainingMode &value, const std::string &propertyName, const ExperienceTrainingMode &defaultvalue)
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
	_lstRawData->addRow(2, trp(propertyName).c_str(), ss.str().c_str());
	++_counter;
	if (value != defaultvalue)
	{
		_lstRawData->setCellColor(_lstRawData->getTexts() - 1, 1, _pink);
	}
}

/**
 * Adds a RuleStatBonus info to the table.
 */
void StatsForNerdsState::addRuleStatBonus(std::wostringstream &ss, const RuleStatBonus &value, const std::string &propertyName)
{
	if (!value.isModded() && !_showDefaults)
	{
		return;
	}
	resetStream(ss);
	bool isFirst = true;
	for (RuleStatBonusDataOrig item : *value.getBonusRaw())
	{
		int power = 0;
		for (float number : item.second)
		{
			++power;
			if (!AreSame(number, 0.0f))
			{
				float numberAbs = number;
				if (!isFirst)
				{
					if (number > 0.0f)
					{
						ss << L" + ";
					}
					else
					{
						ss << L" - ";
						numberAbs = std::abs(number);
					}
				}
				if (item.first == "flatOne")
				{
					ss << numberAbs * 1;
				}
				if (item.first == "flatHundred")
				{
					ss << numberAbs * pow(100, power);
				}
				else
				{
					if (!AreSame(numberAbs, 1.0f))
					{
						ss << numberAbs << L"*";
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
	_lstRawData->addRow(2, trp(propertyName).c_str(), ss.str().c_str());
	++_counter;
	if (value.isModded())
	{
		_lstRawData->setCellColor(_lstRawData->getTexts() - 1, 1, _pink);
	}
}

/**
 * Adds a sprite resource path to the table.
 */
void StatsForNerdsState::addSpriteResourcePath(std::wostringstream &ss, Mod *mod, const std::string &resourceSetName, const int &resourceId)
{
	// go through all extra sprites from all mods
	for (auto resourceSet : mod->getExtraSprites())
	{
		// only check the relevant ones (e.g. "BIGOBS.PCK")
		if (resourceSet.first == resourceSetName)
		{
			// strip mod offset from the in-game ID
			int originalSpriteId = resourceId - (resourceSet.second->getModIndex());

			auto mapOfSprites = resourceSet.second->getSprites();
			auto individualSprite = mapOfSprites->find(originalSpriteId);
			if (individualSprite != mapOfSprites->end())
			{
				std::wostringstream numbers;
				resetStream(numbers);
				numbers << L"  " << originalSpriteId << L" + " << resourceSet.second->getModIndex();

				resetStream(ss);
				ss << Language::utf8ToWstr(individualSprite->second);

				_lstRawData->addRow(2, numbers.str().c_str(), ss.str().c_str());
				++_counter;
				_lstRawData->setRowColor(_lstRawData->getTexts() - 1, _gold);
				return;
			}
		}
	}
}

/**
 * Adds sound resource paths to the table.
 */
void StatsForNerdsState::addSoundVectorResourcePaths(std::wostringstream &ss, Mod *mod, const std::string &resourceSetName, const std::vector<int> &resourceIds)
{
	if (resourceIds.empty())
	{
		return;
	}

	// go through all extra sounds from all mods
	for (auto resourceSet : mod->getExtraSounds())
	{
		// only check the relevant ones (e.g. "BATTLE.CAT")
		if (resourceSet.first == resourceSetName)
		{
			for (auto resourceId : resourceIds)
			{
				// strip mod offset from the in-game ID
				int originalSoundId = resourceId - (resourceSet.second->getModIndex());

				auto mapOfSounds = resourceSet.second->getSounds();
				auto individualSound = mapOfSounds->find(originalSoundId);
				if (individualSound != mapOfSounds->end())
				{
					std::wostringstream numbers;
					resetStream(numbers);
					numbers << L"  " << originalSoundId << L" + " << resourceSet.second->getModIndex();

					resetStream(ss);
					ss << Language::utf8ToWstr(individualSound->second);

					_lstRawData->addRow(2, numbers.str().c_str(), ss.str().c_str());
					++_counter;
					_lstRawData->setRowColor(_lstRawData->getTexts() - 1, _gold);
				}
			}
		}
	}
}

/**
 * Shows the "raw" RuleItem data.
 */
void StatsForNerdsState::initItemList()
{
	_lstRawData->clearList();
	_lstRawData->setIgnoreSeparators(true);

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
	if (_filterOptions.size() > 1)
	{
		_txtTitle->setAlign(ALIGN_LEFT);
	}

	std::wostringstream ss;

	addBattleType(ss, itemRule->getBattleType(), "battleType");
	addExperienceTrainingMode(ss, itemRule->getExperienceTrainingMode(), "experienceTrainingMode");
	addBoolean(ss, itemRule->getArcingShot(), "arcingShot");
	addBoolean(ss, itemRule->isFireExtinguisher(), "isFireExtinguisher");
	addInteger(ss, itemRule->getWaypoints(), "waypoints");

	addInteger(ss, itemRule->getShotgunPellets(), "shotgunPellets");
	addInteger(ss, itemRule->getShotgunBehaviorType(), "shotgunBehavior", 0, false, "STR_SHOTGUN_BEHAVIOR_OXCE", 1);
	addIntegerPercent(ss, itemRule->getShotgunSpread(), "shotgunSpread", 100);
	addIntegerPercent(ss, itemRule->getShotgunChoke(), "shotgunChoke", 100);

	addBoolean(ss, itemRule->isWaterOnly(), "underwaterOnly");
	addBoolean(ss, itemRule->isLandOnly(), "landOnly");
	int psiRequiredDefault = itemRule->getBattleType() == BT_PSIAMP ? true : false;
	addBoolean(ss, itemRule->isPsiRequired(), "psiRequired", psiRequiredDefault);
	addBoolean(ss, itemRule->isLOSRequired(), "LOSRequired");

	if (itemRule->getBattleType() == BT_FIREARM
		|| itemRule->getBattleType() == BT_GRENADE
		|| itemRule->getBattleType() == BT_PROXIMITYGRENADE
		|| itemRule->getBattleType() == BT_FLARE
		|| _showDebug)
	{
		addIntegerPercent(ss, itemRule->getNoLOSAccuracyPenalty(mod), "noLOSAccuracyPenalty", -1); // not raw!
	}
	if (itemRule->getBattleType() == BT_FIREARM || _showDebug)
	{
		addIntegerPercent(ss, itemRule->getKneelBonus(mod), "kneelBonus", 115); // not raw!
	}
	addBoolean(ss, itemRule->isTwoHanded(), "twoHanded");
	addBoolean(ss, itemRule->isBlockingBothHands(), "blockBothHands");
	int oneHandedPenaltyCurrent = itemRule->getOneHandedPenalty(mod);
	int oneHandedPenaltyDefault = itemRule->isTwoHanded() ? 80 : oneHandedPenaltyCurrent;
	addIntegerPercent(ss, oneHandedPenaltyCurrent, "oneHandedPenalty", oneHandedPenaltyDefault); // not raw!

	addInteger(ss, itemRule->getMinRange(), "minRange");
	addInteger(ss, itemRule->getMaxRange(), "maxRange", 200);
	int aimRangeDefault = itemRule->getBattleType() == BT_PSIAMP ? 0 : 200;
	addInteger(ss, itemRule->getAimRange(), "aimRange", aimRangeDefault);
	addInteger(ss, itemRule->getAutoRange(), "autoRange", 7);
	addInteger(ss, itemRule->getSnapRange(), "snapRange", 15);
	int dropoffDefault = itemRule->getBattleType() == BT_PSIAMP ? 1 : 2;
	addInteger(ss, itemRule->getDropoff(), "dropoff", dropoffDefault);

	addRuleStatBonus(ss, *itemRule->getAccuracyMultiplierRaw(), "accuracyMultiplier");
	addIntegerPercent(ss, itemRule->getConfigAimed()->accuracy, "accuracyAimed");
	addIntegerPercent(ss, itemRule->getConfigAuto()->accuracy, "accuracyAuto");
	addIntegerPercent(ss, itemRule->getConfigSnap()->accuracy, "accuracySnap");
	addRuleItemUseCostFull(ss, itemRule->getCostAimed(), "costAimed", RuleItemUseCost(), true, itemRule->getFlatAimed());
	addRuleItemUseCostFull(ss, itemRule->getCostAuto(), "costAuto", RuleItemUseCost(), true, itemRule->getFlatAuto());
	addRuleItemUseCostFull(ss, itemRule->getCostSnap(), "costSnap", RuleItemUseCost(), true, itemRule->getFlatSnap());

	addRuleStatBonus(ss, *itemRule->getMeleeMultiplierRaw(), "meleeMultiplier");
	addIntegerPercent(ss, itemRule->getConfigMelee()->accuracy, "accuracyMelee");
	addRuleItemUseCostFull(ss, itemRule->getCostMelee(), "costMelee", RuleItemUseCost(), true, itemRule->getFlatMelee());

	addSingleString(ss, itemRule->getPsiAttackName(), "psiAttackName");
	addIntegerPercent(ss, itemRule->getAccuracyUse(), "accuracyUse");
	if (itemRule->getBattleType() == BT_PSIAMP || _showDebug)
	{
		addIntegerPercent(ss, itemRule->getAccuracyMind(), "accuracyMindControl");
		addIntegerPercent(ss, itemRule->getAccuracyPanic(), "accuracyPanic", 20);
	}
	int tuUseDefault = (itemRule->getBattleType() == BT_PSIAMP/* && itemRule->getPsiAttackName().empty()*/) ? 0 : 25;
	addRuleItemUseCostFull(ss, itemRule->getCostUse(), "costUse", RuleItemUseCost(tuUseDefault), true, itemRule->getFlatUse());
	if (itemRule->getBattleType() == BT_PSIAMP || _showDebug)
	{
		// using flatUse! there are no flatMindcontrol and flatPanic
		// always show! as if default was 0 instead of 25
		// don't show if Time == 0, for the game it means disabled (even if other costs are non-zero)
		if (itemRule->getCostMind().Time > 0 || _showDebug)
		{
			addRuleItemUseCostFull(ss, itemRule->getCostMind(), "costMindcontrol", RuleItemUseCost(0), true, itemRule->getFlatUse());
		}
		if (itemRule->getCostPanic().Time > 0 || _showDebug)
		{
			addRuleItemUseCostFull(ss, itemRule->getCostPanic(), "costPanic", RuleItemUseCost(0), true, itemRule->getFlatUse());
		}
	}

	addInteger(ss, itemRule->getWeight(), "weight", 3);

	addRuleStatBonus(ss, *itemRule->getThrowMultiplierRaw(), "throwMultiplier");
	addIntegerPercent(ss, itemRule->getAccuracyThrow(), "accuracyThrow", 100);
	addRuleItemUseCostFull(ss, itemRule->getCostThrow(), "costThrow", RuleItemUseCost(25), true, itemRule->getFlatThrow());
	addRuleItemUseCostFull(ss, itemRule->getCostPrime(), "costPrime", RuleItemUseCost(50), true, itemRule->getFlatPrime());
	addRuleItemUseCostFull(ss, itemRule->getCostUnprime(), "costUnprime", RuleItemUseCost(25), true, itemRule->getFlatUnprime());

	if ((mod->getEnableCloseQuartersCombat() && itemRule->getBattleType() == BT_FIREARM) || _showDebug)
	{
		addRuleStatBonus(ss, *itemRule->getCloseQuartersMultiplierRaw(), "closeQuartersMultiplier");
		addIntegerPercent(ss, itemRule->getAccuracyCloseQuarters(mod), "accuracyCloseQuarters", 100); // not raw!
	}

	for (int i = 0; i < 2; i++)
	{
		const RuleDamageType *rule = (i == 0) ? itemRule->getDamageType() : itemRule->getMeleeType();
		if (i == 0)
		{
			ItemDamageType damageTypeDefault = DT_NONE;
			if (rule->ResistType == DT_NONE)
			{
				if (itemRule->getBattleType() == BT_FIREARM || itemRule->getBattleType() == BT_AMMO || itemRule->getBattleType() == BT_MELEE)
				{
					if (itemRule->getClipSize() != 0)
					{
						damageTypeDefault = DAMAGE_TYPES;
					}
				}
				else if (itemRule->getBattleType() == BT_PSIAMP)
				{
					if (!itemRule->getPsiAttackName().empty())
					{
						damageTypeDefault = DAMAGE_TYPES;
					}
				}
			}
			addDamageType(ss, rule->ResistType, "damageType", damageTypeDefault);
			addInteger(ss, itemRule->getPower(), "power");
			addFloat(ss, itemRule->getPowerRangeReductionRaw(), "powerRangeReduction");
			addFloat(ss, itemRule->getPowerRangeThresholdRaw(), "powerRangeThreshold");
			addRuleStatBonus(ss, *itemRule->getDamageBonusRaw(), "damageBonus");
			if (!_showDebug)
			{
				addInteger(ss, rule->FixRadius, "blastRadius", mod->getDamageType(rule->ResistType)->FixRadius);
			}
			addHeading("damageAlter");
		}
		else
		{
			addDamageType(ss, rule->ResistType, "meleeType", DT_MELEE);
			addInteger(ss, itemRule->getMeleePower(), "meleePower");
			addRuleStatBonus(ss, *itemRule->getMeleeBonusRaw(), "meleeBonus");
			addHeading("meleeAlter");
		}

		if (_showDebug || i > 0)
		{
			addInteger(ss, rule->FixRadius, "FixRadius", mod->getDamageType(rule->ResistType)->FixRadius);
		}

		addDamageRandomType(ss, rule->RandomType, "RandomType", mod->getDamageType(rule->ResistType)->RandomType);

		addBoolean(ss, rule->FireBlastCalc, "FireBlastCalc", mod->getDamageType(rule->ResistType)->FireBlastCalc);
		addBoolean(ss, rule->IgnoreDirection, "IgnoreDirection", mod->getDamageType(rule->ResistType)->IgnoreDirection);
		addBoolean(ss, rule->IgnoreSelfDestruct, "IgnoreSelfDestruct", mod->getDamageType(rule->ResistType)->IgnoreSelfDestruct);
		addBoolean(ss, rule->IgnorePainImmunity, "IgnorePainImmunity", mod->getDamageType(rule->ResistType)->IgnorePainImmunity);
		addBoolean(ss, rule->IgnoreNormalMoraleLose, "IgnoreNormalMoraleLose", mod->getDamageType(rule->ResistType)->IgnoreNormalMoraleLose);
		addBoolean(ss, rule->IgnoreOverKill, "IgnoreOverKill", mod->getDamageType(rule->ResistType)->IgnoreOverKill);

		addFloatAsPercentage(ss, rule->ArmorEffectiveness, "ArmorEffectiveness", mod->getDamageType(rule->ResistType)->ArmorEffectiveness);
		addFloatAsPercentage(ss, rule->RadiusEffectiveness, "RadiusEffectiveness", mod->getDamageType(rule->ResistType)->RadiusEffectiveness);
		addFloat(ss, rule->RadiusReduction, "RadiusReduction", mod->getDamageType(rule->ResistType)->RadiusReduction);

		addInteger(ss, rule->FireThreshold, "FireThreshold", mod->getDamageType(rule->ResistType)->FireThreshold);
		addInteger(ss, rule->SmokeThreshold, "SmokeThreshold", mod->getDamageType(rule->ResistType)->SmokeThreshold);

		addFloatAsPercentage(ss, rule->ToArmorPre, "ToArmorPre", mod->getDamageType(rule->ResistType)->ToArmorPre);
		addBoolean(ss, rule->RandomArmorPre, "RandomArmorPre", mod->getDamageType(rule->ResistType)->RandomArmorPre);

		addFloatAsPercentage(ss, rule->ToArmor, "ToArmor", mod->getDamageType(rule->ResistType)->ToArmor);
		addBoolean(ss, rule->RandomArmor, "RandomArmor", mod->getDamageType(rule->ResistType)->RandomArmor);

		addFloatAsPercentage(ss, rule->ToHealth, "ToHealth", mod->getDamageType(rule->ResistType)->ToHealth);
		addBoolean(ss, rule->RandomHealth, "RandomHealth", mod->getDamageType(rule->ResistType)->RandomHealth);

		addFloatAsPercentage(ss, rule->ToStun, "ToStun", mod->getDamageType(rule->ResistType)->ToStun);
		addBoolean(ss, rule->RandomStun, "RandomStun", mod->getDamageType(rule->ResistType)->RandomStun);

		addFloatAsPercentage(ss, rule->ToWound, "ToWound", mod->getDamageType(rule->ResistType)->ToWound);
		addBoolean(ss, rule->RandomWound, "RandomWound", mod->getDamageType(rule->ResistType)->RandomWound);

		addFloatAsPercentage(ss, rule->ToTime, "ToTime", mod->getDamageType(rule->ResistType)->ToTime);
		addBoolean(ss, rule->RandomTime, "RandomTime", mod->getDamageType(rule->ResistType)->RandomTime);

		addFloatAsPercentage(ss, rule->ToEnergy, "ToEnergy", mod->getDamageType(rule->ResistType)->ToEnergy);
		addBoolean(ss, rule->RandomEnergy, "RandomEnergy", mod->getDamageType(rule->ResistType)->RandomEnergy);

		addFloatAsPercentage(ss, rule->ToMorale, "ToMorale", mod->getDamageType(rule->ResistType)->ToMorale);
		addBoolean(ss, rule->RandomMorale, "RandomMorale", mod->getDamageType(rule->ResistType)->RandomMorale);

		addFloatAsPercentage(ss, rule->ToItem, "ToItem", mod->getDamageType(rule->ResistType)->ToItem);
		addBoolean(ss, rule->RandomItem, "RandomItem", mod->getDamageType(rule->ResistType)->RandomItem);

		addFloatAsPercentage(ss, rule->ToTile, "ToTile", mod->getDamageType(rule->ResistType)->ToTile);
		addBoolean(ss, rule->RandomTile, "RandomTile", mod->getDamageType(rule->ResistType)->RandomTile);

		endHeading();
	}

	// skillApplied*
	// strengthApplied*
	// autoShots*

	addHeading("confAimed");
	{
		addInteger(ss, itemRule->getConfigAimed()->shots, "shots", 1);
		addSingleString(ss, itemRule->getConfigAimed()->name, "name", "STR_AIMED_SHOT");
		addInteger(ss, itemRule->getConfigAimed()->ammoSlot, "ammoSlot");
		endHeading();
	}

	addHeading("confAuto");
	{
		addInteger(ss, itemRule->getConfigAuto()->shots, "shots", 3);
		addSingleString(ss, itemRule->getConfigAuto()->name, "name", "STR_AUTO_SHOT");
		addInteger(ss, itemRule->getConfigAuto()->ammoSlot, "ammoSlot");
		endHeading();
	}

	addHeading("confSnap");
	{
		addInteger(ss, itemRule->getConfigSnap()->shots, "shots", 1);
		addSingleString(ss, itemRule->getConfigSnap()->name, "name", "STR_SNAP_SHOT");
		addInteger(ss, itemRule->getConfigSnap()->ammoSlot, "ammoSlot");
		endHeading();
	}

	addHeading("confMelee");
	{
		addInteger(ss, itemRule->getConfigMelee()->shots, "shots", 1);
		addSingleString(ss, itemRule->getConfigMelee()->name, "name");
		int ammoSlotCurrent = itemRule->getConfigMelee()->ammoSlot;
		int ammoSlotDefault = itemRule->getBattleType() == BT_MELEE ? 0 : -1;
		if (itemRule->getBattleType() == BT_NONE)
		{
			// exception for unspecified battle type, e.g. Elerium-115 in vanilla
			if (ammoSlotCurrent <= 0)
			{
				ammoSlotDefault = ammoSlotCurrent;
			}
		}
		addInteger(ss, ammoSlotCurrent, "ammoSlot", ammoSlotDefault);
		endHeading();
	}

	addInteger(ss, itemRule->getClipSize(), "clipSize", 0, false, "STR_CLIP_SIZE_UNLIMITED", -1);

	// compatibleAmmo*
	// tuLoad*
	// tuUnload*

	addHeading("primaryAmmo");
	{
		addVectorOfStrings(ss, *itemRule->getCompatibleAmmoForSlot(0), "compatibleAmmo");
		addInteger(ss, itemRule->getTULoad(0), "tuLoad", 15);
		addInteger(ss, itemRule->getTUUnload(0), "tuUnload", 8);
		endHeading();
	}

	addHeading("ammo[1]");
	{
		addVectorOfStrings(ss, *itemRule->getCompatibleAmmoForSlot(1), "compatibleAmmo");
		addInteger(ss, itemRule->getTULoad(1), "tuLoad", 15);
		addInteger(ss, itemRule->getTUUnload(1), "tuUnload", 8);
		endHeading();
	}

	addHeading("ammo[2]");
	{
		addVectorOfStrings(ss, *itemRule->getCompatibleAmmoForSlot(2), "compatibleAmmo");
		addInteger(ss, itemRule->getTULoad(2), "tuLoad", 15);
		addInteger(ss, itemRule->getTUUnload(2), "tuUnload", 8);
		endHeading();
	}

	addHeading("ammo[3]");
	{
		addVectorOfStrings(ss, *itemRule->getCompatibleAmmoForSlot(3), "compatibleAmmo");
		addInteger(ss, itemRule->getTULoad(3), "tuLoad", 15);
		addInteger(ss, itemRule->getTUUnload(3), "tuUnload", 8);
		endHeading();
	}

	addInteger(ss, itemRule->getArmor(), "armor", 20);

	addBattleMediKitType(ss, itemRule->getMediKitType(), "medikitType");
	addBoolean(ss, itemRule->getAllowSelfHeal(), "allowSelfHeal");
	addBoolean(ss, itemRule->isConsumable(), "isConsumable");
	addInteger(ss, itemRule->getPainKillerQuantity(), "painKiller");
	addInteger(ss, itemRule->getHealQuantity(), "heal");
	addInteger(ss, itemRule->getStimulantQuantity(), "stimulant");
	addInteger(ss, itemRule->getWoundRecovery(), "woundRecovery");
	addInteger(ss, itemRule->getHealthRecovery(), "healthRecovery");
	addInteger(ss, itemRule->getStunRecovery(), "stunRecovery");
	addInteger(ss, itemRule->getEnergyRecovery(), "energyRecovery");
	addInteger(ss, itemRule->getMoraleRecovery(), "moraleRecovery");
	addFloatAsPercentage(ss, itemRule->getPainKillerRecovery(), "painKillerRecovery", 1.0f);

	addVectorOfStrings(ss, itemRule->getRequirements(), "requires");
	addVectorOfStrings(ss, itemRule->getBuyRequirements(), "requiresBuy");
	addVectorOfStrings(ss, itemRule->getCategories(), "categories");
	addVectorOfStrings(ss, itemRule->getSupportedInventorySections(), "supportedInventorySections");

	addDouble(ss, itemRule->getSize(), "size");
	addInteger(ss, itemRule->getBuyCost(), "costBuy", 0, true);
	addInteger(ss, itemRule->getSellCost(), "costSell", 0, true);
	addInteger(ss, itemRule->getTransferTime(), "transferTime", 24);
	addInteger(ss, itemRule->getMonthlySalary(), "monthlySalary", 0, true);
	addInteger(ss, itemRule->getMonthlyMaintenance(), "monthlyMaintenance", 0, true);

	if (_showDebug)
	{
		addSection(L"{Modding section}", L"You don't need this info as a player", _white, true);

		addSection(L"{Naming}", L"", _white);
		addSingleString(ss, itemRule->getType(), "type");
		addSingleString(ss, itemRule->getName(), "name", itemRule->getType());
		addSingleString(ss, itemRule->getNameAsAmmo(), "nameAsAmmo");
		addInteger(ss, itemRule->getListOrder(), "listOrder");

		addSection(L"{Inventory}", L"", _white);
		addInteger(ss, itemRule->getCustomItemPreviewIndex(), "customItemPreviewIndex");
		addInteger(ss, itemRule->getInventoryWidth(), "invWidth", -1); // always show!
		addInteger(ss, itemRule->getInventoryHeight(), "invHeight", -1); // always show!
		addSingleString(ss, itemRule->getDefaultInventorySlot(), "defaultInventorySlot");
		addBoolean(ss, itemRule->isFixed(), "fixedWeapon");

		addSection(L"{Recovery}", L"", _white);
		addBoolean(ss, !itemRule->canBeEquippedBeforeBaseDefense(), "ignoreInBaseDefense"); // negated!
		addInteger(ss, itemRule->getSpecialType(), "specialType", -1);
		addBoolean(ss, itemRule->isRecoverable(), "recover", true);
		addBoolean(ss, itemRule->isCorpseRecoverable(), "recoverCorpse", true);
		addInteger(ss, itemRule->getRecoveryPoints(), "recoveryPoints");
		addBoolean(ss, itemRule->isAlien(), "liveAlien");
		addInteger(ss, itemRule->getPrisonType(), "prisonType");

		addSection(L"{Explosives}", L"", _white);
		addBoolean(ss, itemRule->isHiddenOnMinimap(), "hiddenOnMinimap");
		addSingleString(ss, itemRule->getPrimeActionName(), "primeActionName", "STR_PRIME_GRENADE");
		addSingleString(ss, itemRule->getPrimeActionMessage(), "primeActionMessage", "STR_GRENADE_IS_ACTIVATED");
		addSingleString(ss, itemRule->getUnprimeActionName(), "unprimeActionName");
		addSingleString(ss, itemRule->getUnprimeActionMessage(), "unprimeActionMessage", "STR_GRENADE_IS_DEACTIVATED");
		BattleFuseType fuseTypeDefault = BFT_NONE;
		if (itemRule->getBattleType() == BT_PROXIMITYGRENADE)
		{
			fuseTypeDefault = BFT_INSTANT;
		}
		else if (itemRule->getBattleType() == BT_GRENADE)
		{
			fuseTypeDefault = BFT_SET;
		}
		addBattleFuseType(ss, itemRule->getFuseTimerType(), "fuseType", fuseTypeDefault);
		addBoolean(ss, itemRule->isExplodingInHands(), "isExplodingInHands");
		addHeading("fuseTriggerEvents:");
		{
			addBoolean(ss, itemRule->getFuseTriggerEvent()->defaultBehavior, "defaultBehavior", true);
			addBoolean(ss, itemRule->getFuseTriggerEvent()->throwTrigger, "throwTrigger");
			addBoolean(ss, itemRule->getFuseTriggerEvent()->throwExplode, "throwExplode");
			addBoolean(ss, itemRule->getFuseTriggerEvent()->proximityTrigger, "proximityTrigger");
			addBoolean(ss, itemRule->getFuseTriggerEvent()->proximityExplode, "proximityExplode");
			endHeading();
		}
		addIntegerPercent(ss, itemRule->getSpecialChance(), "specialChance", 100);
		addSingleString(ss, itemRule->getZombieUnit(), "zombieUnit");

		addSection(L"{Sprites}", L"", _white);
		addBoolean(ss, itemRule->getFixedShow(), "fixedWeaponShow");
		addInteger(ss, itemRule->getTurretType(), "turretType", -1);

		addInteger(ss, itemRule->getBigSprite(), "bigSprite", -1);
		addSpriteResourcePath(ss, mod, "BIGOBS.PCK", itemRule->getBigSprite());
		addInteger(ss, itemRule->getFloorSprite(), "floorSprite", -1);
		addSpriteResourcePath(ss, mod, "FLOOROB.PCK", itemRule->getFloorSprite());
		addInteger(ss, itemRule->getHandSprite(), "handSprite", 120);
		addSpriteResourcePath(ss, mod, "HANDOB.PCK", itemRule->getHandSprite());
		addInteger(ss, itemRule->getBulletSprite(), "bulletSprite", -1);

		addSection(L"{Sounds}", L"", _white);
		addVectorOfIntegers(ss, itemRule->getFireSoundRaw(), "fireSound");
		addSoundVectorResourcePaths(ss, mod, "BATTLE.CAT", itemRule->getFireSoundRaw());
		addVectorOfIntegers(ss, itemRule->getHitSoundRaw(), "hitSound");
		addSoundVectorResourcePaths(ss, mod, "BATTLE.CAT", itemRule->getHitSoundRaw());
		addVectorOfIntegers(ss, itemRule->getHitMissSoundRaw(), "hitMissSound");
		addSoundVectorResourcePaths(ss, mod, "BATTLE.CAT", itemRule->getHitMissSoundRaw());
		addVectorOfIntegers(ss, itemRule->getMeleeSoundRaw(), "meleeSound");
		addSoundVectorResourcePaths(ss, mod, "BATTLE.CAT", itemRule->getMeleeSoundRaw());
		addVectorOfIntegers(ss, itemRule->getMeleeHitSoundRaw(), "meleeHitSound");
		addSoundVectorResourcePaths(ss, mod, "BATTLE.CAT", itemRule->getMeleeHitSoundRaw());
		addVectorOfIntegers(ss, itemRule->getMeleeMissSoundRaw(), "meleeMissSound");
		addSoundVectorResourcePaths(ss, mod, "BATTLE.CAT", itemRule->getMeleeMissSoundRaw());
		addVectorOfIntegers(ss, itemRule->getPsiSoundRaw(), "psiSound");
		addSoundVectorResourcePaths(ss, mod, "BATTLE.CAT", itemRule->getPsiSoundRaw());
		addVectorOfIntegers(ss, itemRule->getPsiMissSoundRaw(), "psiMissSound");
		addSoundVectorResourcePaths(ss, mod, "BATTLE.CAT", itemRule->getPsiMissSoundRaw());
		addVectorOfIntegers(ss, itemRule->getExplosionHitSoundRaw(), "explosionHitSound");
		addSoundVectorResourcePaths(ss, mod, "BATTLE.CAT", itemRule->getExplosionHitSoundRaw());

		addSection(L"{Animations}", L"", _white);
		addInteger(ss, itemRule->getHitAnimation(), "hitAnimation");
		addInteger(ss, itemRule->getHitMissAnimation(), "hitMissAnimation", -1);
		addInteger(ss, itemRule->getMeleeAnimation(), "meleeAnimation");
		addInteger(ss, itemRule->getMeleeMissAnimation(), "meleeMissAnimation", -1);
		addInteger(ss, itemRule->getPsiAnimation(), "psiAnimation", -1);
		addInteger(ss, itemRule->getPsiMissAnimation(), "psiMissAnimation", -1);

		addInteger(ss, itemRule->getBulletSpeed(), "bulletSpeed");
		addInteger(ss, itemRule->getExplosionSpeed(), "explosionSpeed");

		addInteger(ss, itemRule->getVaporColor(), "vaporColor", -1);
		addInteger(ss, itemRule->getVaporDensity(), "vaporDensity");
		addIntegerPercent(ss, itemRule->getVaporProbability(), "vaporProbability", 15);

		addSection(L"{AI}", L"", _white);
		addInteger(ss, itemRule->getAttraction(), "attraction");
		addHeading("ai:");
		{
			int useDelayCurrent = itemRule->getAIUseDelay(mod);
			int useDelayDefault = useDelayCurrent > 0 ? -1 : useDelayCurrent;
			addInteger(ss, useDelayCurrent, "useDelay", useDelayDefault); // not raw!
			addInteger(ss, itemRule->getAIMeleeHitCount(), "meleeHitCount", 25);
			endHeading();
		}

		addSection(L"{TU/flat info}", L"", _white);
		addRuleItemUseCostBasic(ss, itemRule->getCostAimed(), "tuAimed");
		addRuleItemUseCostBasic(ss, itemRule->getCostAuto(), "tuAuto");
		addRuleItemUseCostBasic(ss, itemRule->getCostSnap(), "tuSnap");
		addRuleItemUseCostBasic(ss, itemRule->getCostMelee(), "tuMelee");
		int tuUseDefault = (itemRule->getBattleType() == BT_PSIAMP/* && itemRule->getPsiAttackName().empty()*/) ? 0 : 25;
		addRuleItemUseCostBasic(ss, itemRule->getCostUse(), "tuUse", tuUseDefault);
		if (itemRule->getBattleType() == BT_PSIAMP || _showDebug)
		{
			// always show! as if default was 0 instead of 25
			addRuleItemUseCostBasic(ss, itemRule->getCostMind(), "tuMindcontrol", 0);
			addRuleItemUseCostBasic(ss, itemRule->getCostPanic(), "tuPanic", 0);
		}
		addRuleItemUseCostBasic(ss, itemRule->getCostThrow(), "tuThrow", 25);
		addRuleItemUseCostBasic(ss, itemRule->getCostPrime(), "tuPrime", 50);
		addRuleItemUseCostBasic(ss, itemRule->getCostUnprime(), "tuUnprime", 25);

		// flatRate*

		addRuleItemUseCostFull(ss, itemRule->getFlatAimed(), "flatAimed", RuleItemUseCost(0, 1));
		addRuleItemUseCostFull(ss, itemRule->getFlatAuto(), "flatAuto", RuleItemUseCost(0, 1));
		addRuleItemUseCostFull(ss, itemRule->getFlatSnap(), "flatSnap", RuleItemUseCost(0, 1));
		addRuleItemUseCostFull(ss, itemRule->getFlatMelee(), "flatMelee", RuleItemUseCost(0, 1));
		addRuleItemUseCostFull(ss, itemRule->getFlatUse(), "flatUse", RuleItemUseCost(0, 1));
		addRuleItemUseCostFull(ss, itemRule->getFlatThrow(), "flatThrow", RuleItemUseCost(0, 1));
		addRuleItemUseCostFull(ss, itemRule->getFlatPrime(), "flatPrime", RuleItemUseCost(0, 1));
		addRuleItemUseCostFull(ss, itemRule->getFlatUnprime(), "flatUnprime", RuleItemUseCost(0, 1));

		addSection(L"{Script tags}", L"", _white, true);
		{
			auto tagValues = itemRule->getScriptValuesRaw().getValuesRaw();
			ArgEnum index = ScriptParserBase::getArgType<ScriptTag<RuleItem>>();
			auto tagNames = mod->getScriptGlobal()->getTagNames().at(index);
			for (size_t i = 0; i < tagValues.size(); ++i)
			{
				auto nameAsString = tagNames.values[i].name.toString().substr(4);
				addIntegerScriptTag(ss, tagValues.at(i), nameAsString);
			}
			endHeading();
		}
	}
}

/**
 * Adds a unit stat to the string stream, formatted with a label.
 */
void StatsForNerdsState::addUnitStatFormatted(std::wostringstream &ss, const int &value, const std::string &label, bool &isFirst)
{
	if (value != 0 || _showDefaults)
	{
		if (!isFirst) ss << L", ";
		ss << tr(label) << L":" << value;
		isFirst = false;
	}
}

/**
 * Adds a UnitStats info to the table.
 */
void StatsForNerdsState::addUnitStatBonus(std::wostringstream &ss, const UnitStats &value, const std::string &propertyName)
{
	bool isDefault = value.tu == 0 && value.stamina == 0 && value.health == 0 && value.strength == 0
		&& value.reactions == 0 && value.firing == 0 && value.melee == 0 && value.throwing == 0
		&& value.psiSkill == 0 && value.psiStrength == 0 && value.bravery == 0;
	if (isDefault && !_showDefaults)
	{
		return;
	}
	resetStream(ss);
	bool isFirst = true;
	addUnitStatFormatted(ss, value.tu, "STR_TIME_UNITS_ABBREVIATION", isFirst);
	addUnitStatFormatted(ss, value.stamina, "STR_STAMINA_ABBREVIATION", isFirst);
	addUnitStatFormatted(ss, value.health, "STR_HEALTH_ABBREVIATION", isFirst);
	addUnitStatFormatted(ss, value.strength, "STR_STRENGTH_ABBREVIATION", isFirst);
	addUnitStatFormatted(ss, value.reactions, "STR_REACTIONS_ABBREVIATION", isFirst);
	addUnitStatFormatted(ss, value.firing, "STR_FIRING_ACCURACY_ABBREVIATION", isFirst);
	addUnitStatFormatted(ss, value.melee, "STR_MELEE_ACCURACY_ABBREVIATION", isFirst);
	addUnitStatFormatted(ss, value.throwing, "STR_THROWING_ACCURACY_ABBREVIATION", isFirst);
	addUnitStatFormatted(ss, value.psiStrength, "STR_PSIONIC_STRENGTH_ABBREVIATION", isFirst);
	addUnitStatFormatted(ss, value.psiSkill, "STR_PSIONIC_SKILL_ABBREVIATION", isFirst);
	addUnitStatFormatted(ss, value.bravery, "STR_BRAVERY_ABBREVIATION", isFirst);
	_lstRawData->addRow(2, trp(propertyName).c_str(), ss.str().c_str());
	++_counter;
	if (!isDefault)
	{
		_lstRawData->setCellColor(_lstRawData->getTexts() - 1, 1, _pink);
	}
}

/**
 * Adds a vector of damage modifiers to the table.
 */
void StatsForNerdsState::addArmorDamageModifiers(std::wostringstream &ss, const std::vector<float> &vec, const std::string &propertyName)
{
	bool isDefault = true;
	for (auto &item : vec)
	{
		if (!AreSame(item, 1.0f))
		{
			isDefault = false;
		}
	}
	if (isDefault && !_showDefaults)
	{
		return;
	}
	resetStream(ss);
	int index = 0;
	bool isFirst = true;
	ss << L"{";
	for (auto &item : vec)
	{
		if (!AreSame(item, 1.0f) || _showDefaults)
		{
			if (!isFirst)
			{
				ss << L", ";
			}
			switch (index)
			{
				case 0: ss << tr("STR_DAMAGE_NONE"); break;
				case 1: ss << tr("STR_DAMAGE_ARMOR_PIERCING"); break;
				case 2: ss << tr("STR_DAMAGE_INCENDIARY"); break;
				case 3: ss << tr("STR_DAMAGE_HIGH_EXPLOSIVE"); break;
				case 4: ss << tr("STR_DAMAGE_LASER_BEAM"); break;
				case 5: ss << tr("STR_DAMAGE_PLASMA_BEAM"); break;
				case 6: ss << tr("STR_DAMAGE_STUN"); break;
				case 7: ss << tr("STR_DAMAGE_MELEE"); break;
				case 8: ss << tr("STR_DAMAGE_ACID"); break;
				case 9: ss << tr("STR_DAMAGE_SMOKE"); break;
				case 10: ss << tr("STR_DAMAGE_10"); break;
				case 11: ss << tr("STR_DAMAGE_11"); break;
				case 12: ss << tr("STR_DAMAGE_12"); break;
				case 13: ss << tr("STR_DAMAGE_13"); break;
				case 14: ss << tr("STR_DAMAGE_14"); break;
				case 15: ss << tr("STR_DAMAGE_15"); break;
				case 16: ss << tr("STR_DAMAGE_16"); break;
				case 17: ss << tr("STR_DAMAGE_17"); break;
				case 18: ss << tr("STR_DAMAGE_18"); break;
				case 19: ss << tr("STR_DAMAGE_19"); break;
				default: ss << tr("STR_UNKNOWN"); break;
			}
			ss << L": " << item * 100 << L"%";
			isFirst = false;
		}
		index++;
	}
	ss << L"}";
	_lstRawData->addRow(2, trp(propertyName).c_str(), ss.str().c_str());
	++_counter;
	if (!isDefault)
	{
		_lstRawData->setCellColor(_lstRawData->getTexts() - 1, 1, _pink);
	}
}

/**
 * Adds a MovementType to the table.
 */
void StatsForNerdsState::addMovementType(std::wostringstream &ss, const MovementType &value, const std::string &propertyName, const MovementType &defaultvalue)
{
	if (value == defaultvalue && !_showDefaults)
	{
		return;
	}
	resetStream(ss);
	switch (value)
	{
		case MT_WALK: ss << tr("MT_WALK"); break;
		case MT_FLY: ss << tr("MT_FLY"); break;
		case MT_SLIDE: ss << tr("MT_SLIDE"); break;
		case MT_FLOAT: ss << tr("MT_FLOAT"); break;
		case MT_SINK: ss << tr("MT_SINK"); break;
		default: ss << tr("STR_UNKNOWN"); break;
	}
	if (_showIds)
	{
		ss << L" [" << value << L"]";
	}
	_lstRawData->addRow(2, trp(propertyName).c_str(), ss.str().c_str());
	++_counter;
	if (value != defaultvalue)
	{
		_lstRawData->setCellColor(_lstRawData->getTexts() - 1, 1, _pink);
	}
}

/**
 * Adds a ForcedTorso to the table.
 */
void StatsForNerdsState::addForcedTorso(std::wostringstream &ss, const ForcedTorso &value, const std::string &propertyName, const ForcedTorso &defaultvalue)
{
	if (value == defaultvalue && !_showDefaults)
	{
		return;
	}
	resetStream(ss);
	switch (value)
	{
		case TORSO_USE_GENDER: ss << tr("TORSO_USE_GENDER"); break;
		case TORSO_ALWAYS_MALE: ss << tr("TORSO_ALWAYS_MALE"); break;
		case TORSO_ALWAYS_FEMALE: ss << tr("TORSO_ALWAYS_FEMALE"); break;
		default: ss << tr("STR_UNKNOWN"); break;
	}
	if (_showIds)
	{
		ss << L" [" << value << L"]";
	}
	_lstRawData->addRow(2, trp(propertyName).c_str(), ss.str().c_str());
	++_counter;
	if (value != defaultvalue)
	{
		_lstRawData->setCellColor(_lstRawData->getTexts() - 1, 1, _pink);
	}
}

/**
 * Adds a DrawingRoutine to the table.
 */
void StatsForNerdsState::addDrawingRoutine(std::wostringstream &ss, const int &value, const std::string &propertyName, const int &defaultvalue)
{
	if (value == defaultvalue && !_showDefaults)
	{
		return;
	}
	resetStream(ss);
	switch (value)
	{
		case 0: ss << tr("DRAWING_ROUTINE_SOLDIER_SECTOID"); break;
		case 1: ss << tr("DRAWING_ROUTINE_FLOATER"); break;
		case 2: ss << tr("DRAWING_ROUTINE_HWP"); break;
		case 3: ss << tr("DRAWING_ROUTINE_CYBERDISC"); break;
		case 4: ss << tr("DRAWING_ROUTINE_CIVILIAN_ETHEREAL"); break;
		case 5: ss << tr("DRAWING_ROUTINE_SECTOPOD_REAPER"); break;
		case 6: ss << tr("DRAWING_ROUTINE_SNAKEMAN"); break;
		case 7: ss << tr("DRAWING_ROUTINE_CHRYSSALID"); break;
		case 8: ss << tr("DRAWING_ROUTINE_SILACOID"); break;
		case 9: ss << tr("DRAWING_ROUTINE_CELATID"); break;
		case 10: ss << tr("DRAWING_ROUTINE_MUTON"); break;
		case 11: ss << tr("DRAWING_ROUTINE_SWS"); break;
		case 12: ss << tr("DRAWING_ROUTINE_HALLUCINOID"); break;
		case 13: ss << tr("DRAWING_ROUTINE_AQUANAUTS"); break;
		case 14: ss << tr("DRAWING_ROUTINE_CALCINITE_AND_MORE"); break;
		case 15: ss << tr("DRAWING_ROUTINE_AQUATOID"); break;
		case 16: ss << tr("DRAWING_ROUTINE_BIO_DRONE"); break;
		case 17: ss << tr("DRAWING_ROUTINE_TFTD_CIVILIAN_A"); break;
		case 18: ss << tr("DRAWING_ROUTINE_TFTD_CIVILIAN_B"); break;
		case 19: ss << tr("DRAWING_ROUTINE_TENTACULAT"); break;
		case 20: ss << tr("DRAWING_ROUTINE_TRISCENE"); break;
		case 21: ss << tr("DRAWING_ROUTINE_XARQUID"); break;
		case 22: ss << tr("DRAWING_ROUTINE_INVERTED_CYBERDISC"); break;
		default: ss << tr("STR_UNKNOWN"); break;
	}
	if (_showIds)
	{
		ss << L" [" << value << L"]";
	}
	_lstRawData->addRow(2, trp(propertyName).c_str(), ss.str().c_str());
	++_counter;
	if (value != defaultvalue)
	{
		_lstRawData->setCellColor(_lstRawData->getTexts() - 1, 1, _pink);
	}
}

/**
 * Shows the "raw" (Rule)Armor data.
 */
void StatsForNerdsState::initArmorList()
{
	_lstRawData->clearList();
	_lstRawData->setIgnoreSeparators(true);

	std::wostringstream ssTopic;
	ssTopic << tr(_topicId);
	if (_showIds)
	{
		ssTopic << L" [" << Language::utf8ToWstr(_topicId) << L"]";
	}

	_txtArticle->setText(tr("STR_ARTICLE").arg(ssTopic.str()));

	Mod *mod = _game->getMod();
	Armor *armorRule = mod->getArmor(_topicId);
	if (!armorRule)
		return;

	_filterOptions.clear();
	_cbxRelatedStuff->setVisible(false);

	std::wostringstream ss;

	addUnitStatBonus(ss, *armorRule->getStats(), "stats");

	addInteger(ss, armorRule->getWeight(), "weight");

	addInteger(ss, armorRule->getSize(), "size", 1);

	addBoolean(ss, armorRule->allowsRunning(), "allowsRunning", true);
	addBoolean(ss, armorRule->allowsStrafing(), "allowsStrafing", true);
	addBoolean(ss, armorRule->allowsKneeling(), "allowsKneeling", true);

	bool fearImmuneDefault = false;
	bool bleedImmuneDefault = false;
	bool painImmuneDefault = false;
	bool zombiImmuneDefault = false;
	bool ignoresMeleeThreatDefault = false;
	bool createsMeleeThreatDefault = true;

	if (armorRule->getSize() != 1)
	{
		fearImmuneDefault = true;
		bleedImmuneDefault = true;
		painImmuneDefault = true;
		zombiImmuneDefault = true;
		ignoresMeleeThreatDefault = true;
		createsMeleeThreatDefault = false;
	}

	addBoolean(ss, armorRule->getFearImmune(), "fearImmune", fearImmuneDefault);
	addBoolean(ss, armorRule->getBleedImmune(), "bleedImmune", bleedImmuneDefault); // not considering alienBleeding option!
	addBoolean(ss, armorRule->getPainImmune(), "painImmune", painImmuneDefault);
	addBoolean(ss, armorRule->getZombiImmune(), "zombiImmune", zombiImmuneDefault);
	addBoolean(ss, armorRule->getIgnoresMeleeThreat(), "ignoresMeleeThreat", ignoresMeleeThreatDefault);
	addBoolean(ss, armorRule->getCreatesMeleeThreat(), "createsMeleeThreat", createsMeleeThreatDefault);

	_filterOptions.clear();
	for (auto biw : armorRule->getBuiltInWeapons())
	{
		// ignore dummy inventory padding
		if (biw.find("INV_NULL") == std::string::npos || _showDebug)
		{
			_filterOptions.push_back(biw);
		}
	}
	addVectorOfStrings(ss, _filterOptions, "builtInWeapons");
	addSingleString(ss, armorRule->getSpecialWeapon(), "specialWeapon");

	if (!armorRule->getSpecialWeapon().empty())
	{
		_filterOptions.push_back(armorRule->getSpecialWeapon());
	}

	_filterOptions.insert(_filterOptions.begin(), "STR_BUILT_IN_ITEMS");
	_cbxRelatedStuff->setOptions(_filterOptions);
	_cbxRelatedStuff->setVisible(_filterOptions.size() > 1);
	if (_filterOptions.size() > 1)
	{
		_txtTitle->setAlign(ALIGN_LEFT);
	}

	addIntegerPercent(ss, armorRule->getHeatVision(), "heatVision");
	addInteger(ss, armorRule->getPsiVision(), "psiVision");

	addInteger(ss, armorRule->getVisibilityAtDay(), "visibilityAtDay");
	addInteger(ss, armorRule->getVisibilityAtDark(), "visibilityAtDark");
	addInteger(ss, armorRule->getCamouflageAtDay(), "camouflageAtDay");
	addInteger(ss, armorRule->getCamouflageAtDark(), "camouflageAtDark");
	addInteger(ss, armorRule->getAntiCamouflageAtDay(), "antiCamouflageAtDay");
	addInteger(ss, armorRule->getAntiCamouflageAtDark(), "antiCamouflageAtDark");

	addRuleStatBonus(ss, *armorRule->getPsiDefenceRaw(), "psiDefence");
	addRuleStatBonus(ss, *armorRule->getMeleeDodgeRaw(), "meleeDodge");
	addFloatAsPercentage(ss, armorRule->getMeleeDodgeBackPenalty(), "meleeDodgeBackPenalty");

	addHeading("recovery");
	{
		addRuleStatBonus(ss, *armorRule->getTimeRecoveryRaw(), "time");
		addRuleStatBonus(ss, *armorRule->getEnergyRecoveryRaw(), "energy");
		addRuleStatBonus(ss, *armorRule->getMoraleRecoveryRaw(), "morale");
		addRuleStatBonus(ss, *armorRule->getHealthRecoveryRaw(), "health");
		addRuleStatBonus(ss, *armorRule->getStunRegenerationRaw(), "stun");
		endHeading();
	}

	addVectorOfStrings(ss, armorRule->getUnits(), "units");

	if (_showDebug)
	{
		addSection(L"{Modding section}", L"You don't need this info as a player", _white, true);

		addSection(L"{Naming}", L"", _white);
		addSingleString(ss, armorRule->getType(), "type");

		addSection(L"{Recovery}", L"", _white);
		addVectorOfStrings(ss, armorRule->getCorpseBattlescape(), "corpseBattle");
		addSingleString(ss, armorRule->getCorpseGeoscape(), "corpseGeo");
		addSingleString(ss, armorRule->getStoreItem(), "storeItem");

		addSection(L"{Inventory}", L"", _white);
		addSingleString(ss, armorRule->getSpriteInventory(), "spriteInv", "", false);
		addBoolean(ss, armorRule->hasInventory(), "allowInv", true);

		addSection(L"{Sprites}", L"", _white);
		addInteger(ss, armorRule->getCustomArmorPreviewIndex(), "customArmorPreviewIndex");
		addSingleString(ss, armorRule->getSpriteSheet(), "spriteSheet", "", false);
		addInteger(ss, armorRule->getFaceColorGroup(), "spriteFaceGroup");
		addVectorOfIntegers(ss, armorRule->getFaceColorRaw(), "spriteFaceColor");
		addInteger(ss, armorRule->getHairColorGroup(), "spriteHairGroup");
		addVectorOfIntegers(ss, armorRule->getHairColorRaw(), "spriteHairColor");
		addInteger(ss, armorRule->getUtileColorGroup(), "spriteUtileGroup");
		addVectorOfIntegers(ss, armorRule->getUtileColorRaw(), "spriteUtileColor");
		addInteger(ss, armorRule->getRankColorGroup(), "spriteRankGroup");
		addVectorOfIntegers(ss, armorRule->getRankColorRaw(), "spriteRankColor");

		addSection(L"{Sounds}", L"", _white);
		addInteger(ss, armorRule->getMoveSound(), "moveSound", -1);
		std::vector<int> tmpSoundVector;
		tmpSoundVector.push_back(armorRule->getMoveSound());
		addSoundVectorResourcePaths(ss, mod, "BATTLE.CAT", tmpSoundVector);

		addSection(L"{Animations}", L"", _white);
		addDrawingRoutine(ss, armorRule->getDrawingRoutine(), "drawingRoutine");
		addBoolean(ss, armorRule->getConstantAnimation(), "constantAnimation");
		addMovementType(ss, armorRule->getMovementType(), "movementType");
		addForcedTorso(ss, armorRule->getForcedTorso(), "forcedTorso");
		addInteger(ss, armorRule->getDeathFrames(), "deathFrames", 3);

		addSection(L"{Calculations}", L"", _white);
		addVectorOfIntegers(ss, armorRule->getLoftempsSet(), "loftempsSet");
		addInteger(ss, armorRule->getPersonalLight(), "personalLight", 15);
		addInteger(ss, armorRule->getStandHeight(), "standHeight", -1);
		addInteger(ss, armorRule->getKneelHeight(), "kneelHeight", -1);
		addInteger(ss, armorRule->getFloatHeight(), "floatHeight", -1);
		addFloat(ss, armorRule->getOverKill(), "overKill", 0.5f);
		addBoolean(ss, armorRule->getInstantWoundRecovery(), "instantWoundRecovery");

		addSection(L"{Basics}", L"Stuff from the main article", _white, true);
		addInteger(ss, armorRule->getFrontArmor(), "frontArmor");
		addInteger(ss, armorRule->getRightSideArmor(), "sideArmor");
		addInteger(ss, armorRule->getLeftSideArmor() - armorRule->getRightSideArmor(), "leftArmorDiff");
		addInteger(ss, armorRule->getRearArmor(), "rearArmor");
		addInteger(ss, armorRule->getUnderArmor(), "underArmor");

		addArmorDamageModifiers(ss, armorRule->getDamageModifiersRaw(), "damageModifier");

		addSection(L"{Script tags}", L"", _white, true);
		{
			auto tagValues = armorRule->getScriptValuesRaw().getValuesRaw();
			ArgEnum index = ScriptParserBase::getArgType<ScriptTag<Armor>>();
			auto tagNames = mod->getScriptGlobal()->getTagNames().at(index);
			for (size_t i = 0; i < tagValues.size(); ++i)
			{
				auto nameAsString = tagNames.values[i].name.toString().substr(4);
				addIntegerScriptTag(ss, tagValues.at(i), nameAsString);
			}
			endHeading();
		}
	}
}

}
