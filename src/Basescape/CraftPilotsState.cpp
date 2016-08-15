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
#include "CraftPilotsState.h"
#include <sstream>
#include "../Engine/Game.h"
#include "../Mod/Mod.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Mod/RuleCraft.h"
#include "../Savegame/Soldier.h"
#include "../Mod/RuleSoldier.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Craft Pilots screen.
 */
CraftPilotsState::CraftPilotsState(Base *base, size_t craft) : _base(base), _craft(craft)
{
	// Create objects
	_window = new Window(this, 320, 200, 0, 0);
	_btnOk = new TextButton(300, 20, 10, 170);
	_txtTitle = new Text(310, 17, 5, 12);
	_txtFiringAcc = new Text(108, 9, 74, 32);
	_txtReactions = new Text(58, 9, 182, 32);
	_txtBravery = new Text(58, 9, 240, 32);
	_txtPilots = new Text(150, 9, 10, 40);
	_lstPilots = new TextList(288, 32, 10, 48);

	_txtRequired = new Text(288, 9, 10, 84);
	_txtHint = new Text(288, 9, 10, 96);

	_txtAccuracyBonus = new Text(100, 9, 10, 124);
	_txtAccuracyBonusValue = new Text(150, 9, 110, 124);
	_txtDodgeBonus = new Text(100, 9, 10, 136);
	_txtDodgeBonusValue = new Text(150, 9, 110, 136);
	_txtApproachSpeed = new Text(100, 9, 10, 148);
	_txtApproachSpeedValue = new Text(150, 9, 110, 148);

	// Set palette
	setInterface("costsInfo");

	add(_window, "window", "costsInfo");
	add(_btnOk, "button", "costsInfo");
	add(_txtTitle, "text1", "costsInfo");
	add(_txtFiringAcc, "text1", "costsInfo");
	add(_txtReactions, "text1", "costsInfo");
	add(_txtBravery, "text1", "costsInfo");
	add(_txtPilots, "text1", "costsInfo");
	add(_lstPilots, "list", "costsInfo");
	add(_txtRequired, "text1", "costsInfo");
	add(_txtHint, "text1", "costsInfo");
	add(_txtAccuracyBonus, "text1", "costsInfo");
	add(_txtAccuracyBonusValue, "text2", "costsInfo");
	add(_txtDodgeBonus, "text1", "costsInfo");
	add(_txtDodgeBonusValue, "text2", "costsInfo");
	add(_txtApproachSpeed, "text1", "costsInfo");
	add(_txtApproachSpeedValue, "text2", "costsInfo");

	centerAllSurfaces();

	// Set up objects
	_window->setBackground(_game->getMod()->getSurface("BACK02.SCR"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&CraftPilotsState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&CraftPilotsState::btnOkClick, Options::keyOk);
	_btnOk->onKeyboardPress((ActionHandler)&CraftPilotsState::btnOkClick, Options::keyCancel);

	Craft *c = _base->getCrafts()->at(_craft);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_PILOTS_FOR_CRAFT").arg(c->getName(_game->getLanguage())));

	_txtFiringAcc->setText(tr("STR_FIRING_ACCURACY"));
	_txtFiringAcc->setAlign(ALIGN_RIGHT);

	_txtReactions->setText(tr("STR_REACTIONS"));
	_txtReactions->setAlign(ALIGN_RIGHT);

	_txtBravery->setText(tr("STR_BRAVERY"));
	_txtBravery->setAlign(ALIGN_RIGHT);

	_txtPilots->setText(tr("STR_PILOTS"));

	_lstPilots->setColumns(5, 114, 58, 58, 58, 0);
	_lstPilots->setAlign(ALIGN_RIGHT);
	_lstPilots->setAlign(ALIGN_LEFT, 0);
	_lstPilots->setDot(true);

	const std::vector<Soldier*> pilots = c->getPilotList();
	for (std::vector<Soldier*>::const_iterator i = pilots.begin(); i != pilots.end(); ++i)
	{
			std::wostringstream ss1;
			ss1 << (*i)->getCurrentStats()->firing;
			std::wostringstream ss2;
			ss2 << (*i)->getCurrentStats()->reactions;
			std::wostringstream ss3;
			ss3 << (*i)->getCurrentStats()->bravery;
			_lstPilots->addRow(5, (*i)->getName(false).c_str(), ss1.str().c_str(), ss2.str().c_str(), ss3.str().c_str(), L"");
	}

	_txtRequired->setText(tr("STR_PILOTS_REQUIRED").arg(c->getRules()->getPilots()));

	_txtHint->setText(tr("STR_HINT"));

	_txtAccuracyBonus->setText(tr("STR_ACCURACY_BONUS"));
	std::wostringstream ss1;
	int accBonus = c->getPilotAccuracyBonus(pilots);
	ss1 << (accBonus > 0 ? L"+" : L"") << accBonus << L"%";
	_txtAccuracyBonusValue->setText(ss1.str().c_str());

	_txtDodgeBonus->setText(tr("STR_DODGE_BONUS"));
	std::wostringstream ss2;
	int dodgeBonus = c->getPilotDodgeBonus(pilots);
	ss2 << (dodgeBonus > 0 ? L"+" : L"") << dodgeBonus << L"%";
	_txtDodgeBonusValue->setText(ss2.str().c_str());

	_txtApproachSpeed->setText(tr("STR_APPROACH_SPEED"));
	std::wostringstream ss3;
	int approachSpeed = c->getPilotApproachSpeedModifier(pilots);
	switch (approachSpeed)
	{
	case 1:
		ss3 << tr("STR_COWARDLY");
		break;
	case 2:
		ss3 << tr("STR_NORMAL");
		break;
	case 3:
		ss3 << tr("STR_BOLD");
		break;
	case 4:
		ss3 << tr("STR_VERY_BOLD");
		break;
	default:
		ss3 << tr("STR_UNKNOWN");
		break;
	}
	_txtApproachSpeedValue->setText(ss3.str().c_str());
}

/**
 *
 */
CraftPilotsState::~CraftPilotsState()
{

}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void CraftPilotsState::btnOkClick(Action *)
{
	_game->popState();
}

}
