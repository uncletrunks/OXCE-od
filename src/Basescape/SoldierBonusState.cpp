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
#include "SoldierBonusState.h"
#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"
#include "../Mod/ArticleDefinition.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleSoldierBonus.h"
#include "../Savegame/Base.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"
#include "../Ufopaedia/StatsForNerdsState.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the SoldierBonus window.
 * @param base Pointer to the base to get info from.
 * @param soldier ID of the selected soldier.
 */
SoldierBonusState::SoldierBonusState(Base *base, size_t soldier) : _base(base), _soldier(soldier)
{
	_screen = false;

	// Create objects
	_window = new Window(this, 192, 160, 64, 20, POPUP_BOTH);
	_btnCancel = new TextButton(140, 16, 90, 156);
	_txtTitle = new Text(182, 16, 69, 28);
	_txtType = new Text(90, 9, 80, 52);
	_lstBonuses = new TextList(160, 80, 73, 68);

	// Set palette
	setInterface("soldierBonus");

	add(_window, "window", "soldierBonus");
	add(_btnCancel, "button", "soldierBonus");
	add(_txtTitle, "text", "soldierBonus");
	add(_txtType, "text", "soldierBonus");
	add(_lstBonuses, "list", "soldierBonus");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "soldierBonus");

	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick((ActionHandler)&SoldierBonusState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&SoldierBonusState::btnCancelClick, Options::keyCancel);

	Soldier *s = _base ? _base->getSoldiers()->at(_soldier) : _game->getSavedGame()->getDeadSoldiers()->at(_soldier);
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_SOLDIER_BONUSES_FOR").arg(s->getName()));

	_txtType->setText(tr("STR_TYPE"));

	_lstBonuses->setColumns(1, 153);
	_lstBonuses->setSelectable(true);
	_lstBonuses->setBackground(_window);
	_lstBonuses->setMargin(8);

	_bonuses.clear();
	_lstBonuses->clearList();
	for (auto bonusRule : *s->getBonuses(nullptr))
	{
		_bonuses.push_back(bonusRule->getName());
		_lstBonuses->addRow(1, tr(bonusRule->getName()).c_str());
	}

	_lstBonuses->onMouseClick((ActionHandler)& SoldierBonusState::lstBonusesClick);
}

/**
 *
 */
SoldierBonusState::~SoldierBonusState()
{

}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void SoldierBonusState::btnCancelClick(Action *)
{
	_game->popState();
}

/**
 * Shows corresponding Stats for Nerds article.
 * @param action Pointer to an action.
 */
void SoldierBonusState::lstBonusesClick(Action *)
{
	std::string articleId = _bonuses[_lstBonuses->getSelectedRow()];
	_game->pushState(new StatsForNerdsState(UFOPAEDIA_TYPE_UNKNOWN, articleId, false, false, false));
}

}
