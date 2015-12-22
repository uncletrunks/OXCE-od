/*
 * Copyright 2010-2014 OpenXcom Developers.
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
#include <sstream>
#include "TrainingState.h"
#include "AllocateTrainingState.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Palette.h"
#include "../Mod/RuleInterface.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Base.h"
#include "../Interface/TextList.h"
#include "GeoscapeState.h"
#include "../Savegame/Soldier.h"
#include "../Engine/Action.h"
#include "../Engine/Options.h"
#include "../Mod/Mod.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Psi Training screen.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to handle.
 */
AllocateTrainingState::AllocateTrainingState(Base *base) : _sel(0)
{
	_base = base;
	// Create objects
	_window = new Window(this, 320, 200, 0, 0);
	_txtTitle = new Text(300, 17, 10, 8);
	_txtRemaining = new Text(300, 10, 10, 24);
	_txtName = new Text(64, 10, 10, 40);
	_txtTraining = new Text(48, 20, 270, 32);
	_btnOk = new TextButton(160, 14, 80, 174);
	_lstSoldiers = new TextList(290, 112, 8, 52);
	_txtTu = new Text(18, 10, 124, 40);
	_txtStamina = new Text(18, 10, 144, 40);
	_txtHealth = new Text(18, 10, 164, 40);
	_txtFiring = new Text(18, 10, 184, 40);
	_txtThrowing = new Text(18, 10, 204, 40);
	_txtMelee = new Text(18, 10, 224, 40);
	_txtStrength = new Text(18, 10, 244, 40);

	// Set palette
	setPalette("PAL_BASESCAPE", _game->getMod()->getInterface("allocatePsi")->getElement("palette")->color);

	add(_window, "window", "allocatePsi");
	add(_btnOk, "button", "allocatePsi");
	add(_txtName, "text", "allocatePsi");
	add(_txtTitle, "text", "allocatePsi");
	add(_txtRemaining, "text", "allocatePsi");
	add(_txtTraining, "text", "allocatePsi");
	add(_lstSoldiers, "list", "allocatePsi");
	add(_txtTu, "text", "allocatePsi");
	add(_txtStamina, "text", "allocatePsi");
	add(_txtHealth, "text", "allocatePsi");
	add(_txtFiring, "text", "allocatePsi");
	add(_txtThrowing, "text", "allocatePsi");
	add(_txtMelee, "text", "allocatePsi");
	add(_txtStrength, "text", "allocatePsi");

	centerAllSurfaces();

	// Set up objects
	_window->setBackground(_game->getMod()->getSurface("BACK02.SCR"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&AllocateTrainingState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&AllocateTrainingState::btnOkClick, Options::keyCancel);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_PHYSICAL_TRAINING"));

	_space = base->getAvailableTraining() - base->getUsedTraining();
	_txtRemaining->setText(tr("STR_REMAINING_TRAINING_FACILITY_CAPACITY").arg(_space));

	_txtName->setText(tr("STR_NAME"));
	_txtTu->setText(tr("TU"));
	_txtStamina->setText(tr("STA"));
	_txtHealth->setText(tr("HP"));
	_txtFiring->setText(tr("ACC"));
	_txtThrowing->setText(tr("THR"));
	_txtMelee->setText(tr("MEL"));
	_txtStrength->setText(tr("STR"));
	_txtTraining->setText(tr("STR_IN_TRAINING"));

	_lstSoldiers->setColumns(9, 114, 20, 20, 20, 20, 20, 20, 26, 40);
	_lstSoldiers->setSelectable(true);
	_lstSoldiers->setBackground(_window);
	_lstSoldiers->setMargin(2);
	_lstSoldiers->onMouseClick((ActionHandler)&AllocateTrainingState::lstSoldiersClick);
	int row = 0;
	for (std::vector<Soldier*>::const_iterator s = base->getSoldiers()->begin(); s != base->getSoldiers()->end(); ++s)
	{
		std::wostringstream tu;
		tu << (*s)->getCurrentStats()->tu;
		std::wostringstream stamina;
		stamina << (*s)->getCurrentStats()->stamina;
		std::wostringstream health;
		health << (*s)->getCurrentStats()->health;
		std::wostringstream firing;
		firing << (*s)->getCurrentStats()->firing;
		std::wostringstream throwing;
		throwing << (*s)->getCurrentStats()->throwing;
		std::wostringstream melee;
		melee << (*s)->getCurrentStats()->melee;
		std::wostringstream strength;
		strength << (*s)->getCurrentStats()->strength;

		if ((*s)->isInTraining())
		{
			_lstSoldiers->addRow(9, (*s)->getName(true).c_str(), tu.str().c_str(), stamina.str().c_str(), health.str().c_str(), firing.str().c_str(), throwing.str().c_str(), melee.str().c_str(), strength.str().c_str(), tr("STR_YES").c_str());
			_lstSoldiers->setRowColor(row, _lstSoldiers->getSecondaryColor());
		}
		else
		{
			_lstSoldiers->addRow(9, (*s)->getName(true).c_str(), tu.str().c_str(), stamina.str().c_str(), health.str().c_str(), firing.str().c_str(), throwing.str().c_str(), melee.str().c_str(), strength.str().c_str(), tr("STR_NO").c_str());
			_lstSoldiers->setRowColor(row, _lstSoldiers->getColor());
		}
		row++;
	}
}

/**
 *
 */
AllocateTrainingState::~AllocateTrainingState()
{

}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void AllocateTrainingState::btnOkClick(Action *)
{
	_game->popState();
}

/**
 * Assigns / removes a soldier from Psi Training.
 * @param action Pointer to an action.
 */
void AllocateTrainingState::lstSoldiersClick(Action *action)
{
	_sel = _lstSoldiers->getSelectedRow();
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		if (!_base->getSoldiers()->at(_sel)->isInTraining())
		{
			if (_base->getUsedTraining() < _base->getAvailableTraining())
			{
				_lstSoldiers->setCellText(_sel, 8, tr("STR_YES").c_str());
				_lstSoldiers->setRowColor(_sel, _lstSoldiers->getSecondaryColor());
				_space--;
				_txtRemaining->setText(tr("STR_REMAINING_TRAINING_FACILITY_CAPACITY").arg(_space));
				_base->getSoldiers()->at(_sel)->setTraining(true);
			}
		}
		else
		{
			_lstSoldiers->setCellText(_sel, 8, tr("STR_NO").c_str());
			_lstSoldiers->setRowColor(_sel, _lstSoldiers->getColor());
			_space++;
			_txtRemaining->setText(tr("STR_REMAINING_TRAINING_FACILITY_CAPACITY").arg(_space));
			_base->getSoldiers()->at(_sel)->setTraining(false);
		}
	}
}

}
