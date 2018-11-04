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
#include "SoldierTransformationState.h"
#include <sstream>
#include <algorithm>
#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/Screen.h"
#include "../Interface/ComboBox.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextEdit.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleSoldier.h"
#include "../Mod/RuleSoldierTransformation.h"
#include "../Savegame/Base.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/Transfer.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Soldier Transformation GUI
 * @param transformationRule Pointer to the transformation ruleset
 * @param base Pointer to the current base
 * @param sourceSoldier Pointer to the selected soldier
 * @param filteredListOfSoldiers Pointer to the list of available soldiers
 */
SoldierTransformationState::SoldierTransformationState(RuleSoldierTransformation *transformationRule, Base *base, Soldier *sourceSoldier, std::vector<Soldier *> *filteredListOfSoldiers) :
	_transformationRule(transformationRule), _base(base), _sourceSoldier(sourceSoldier), _filteredListOfSoldiers(filteredListOfSoldiers)
{
	_window = new Window(this, 320, 200, 0, 0);
	_btnCancel = new TextButton(148, 16, 8, 176);
	_btnStart = new TextButton(148, 16, 164, 176);

	_btnLeftArrow = new TextButton(16, 16, 8, 8);
	_btnRightArrow = new TextButton(16, 16, 296, 8);
	_edtSoldier = new TextEdit(this, 210, 16, 54, 8);

	_txtCost = new Text(290, 9, 16, 30);
	_txtTransferTime = new Text(290, 9, 16, 40);
	_txtRecoveryTime = new Text(290, 9, 16, 50);

	_txtRequiredItems = new Text(290, 9, 16, 64);
	_txtItemNameColumn = new Text(60, 16, 30, 72);
	_txtUnitRequiredColumn = new Text(60, 16, 155, 72);
	_txtUnitAvailableColumn = new Text(60, 16, 230, 72);
	_lstRequiredItems = new TextList(270, 48, 30, 88);

	_lstStatChanges = new TextList(288, 144, 16, 146);

	// Set palette
	setInterface("soldierTransformation");

	add(_window, "window", "soldierTransformation");
	add(_btnCancel, "button", "soldierTransformation");
	add(_btnStart, "button", "soldierTransformation");

	add(_btnLeftArrow, "button", "soldierTransformation");
	add(_btnRightArrow, "button", "soldierTransformation");
	add(_edtSoldier, "text", "soldierTransformation");

	add(_txtCost, "text", "soldierTransformation");
	add(_txtTransferTime, "text", "soldierTransformation");
	add(_txtRecoveryTime, "text", "soldierTransformation");

	add(_txtRequiredItems, "text", "soldierTransformation");
	add(_txtItemNameColumn, "text", "soldierTransformation");
	add(_txtUnitRequiredColumn, "text", "soldierTransformation");
	add(_txtUnitAvailableColumn, "text", "soldierTransformation");
	add(_lstRequiredItems, "list1", "soldierTransformation");

	add(_lstStatChanges, "list2", "soldierTransformation");

	centerAllSurfaces();

	// Set up objects
	_window->setBackground(_game->getMod()->getSurface("BACK02.SCR"));

	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick((ActionHandler)&SoldierTransformationState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&SoldierTransformationState::btnCancelClick, Options::keyCancel);

	_btnStart->setText(tr(_transformationRule->getName()));
	_btnStart->onMouseClick((ActionHandler)&SoldierTransformationState::btnStartClick);
	_btnStart->onKeyboardPress((ActionHandler)&SoldierTransformationState::btnStartClick, Options::keyOk);

	if (_filteredListOfSoldiers->size() > 1)
	{
		_btnLeftArrow->setText("<<");
		_btnLeftArrow->onMouseClick((ActionHandler)&SoldierTransformationState::btnLeftArrowClick);
		_btnLeftArrow->onKeyboardPress((ActionHandler)&SoldierTransformationState::btnLeftArrowClick, Options::keyGeoLeft);

		_btnRightArrow->setText(">>");
		_btnRightArrow->onMouseClick((ActionHandler)&SoldierTransformationState::btnRightArrowClick);
		_btnRightArrow->onKeyboardPress((ActionHandler)&SoldierTransformationState::btnRightArrowClick, Options::keyGeoRight);
	}
	else
	{
		_btnLeftArrow->setVisible(false);
		_btnRightArrow->setVisible(false);
	}

	_edtSoldier->setBig();
	_edtSoldier->setAlign(ALIGN_CENTER);

	_txtRequiredItems->setText(tr("STR_SPECIAL_MATERIALS_REQUIRED"));
	_txtRequiredItems->setAlign(ALIGN_CENTER);

	_txtItemNameColumn->setText(tr("STR_ITEM_REQUIRED"));
	_txtItemNameColumn->setWordWrap(true);

	_txtUnitRequiredColumn->setText(tr("STR_UNITS_REQUIRED"));
	_txtUnitRequiredColumn->setWordWrap(true);

	_txtUnitAvailableColumn->setText(tr("STR_UNITS_AVAILABLE"));
	_txtUnitAvailableColumn->setWordWrap(true);

	_lstRequiredItems->setColumns(3, 140, 75, 55);

	_lstStatChanges->setColumns(13, 90, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 0);
	_lstStatChanges->setAlign(ALIGN_RIGHT);
	_lstStatChanges->setAlign(ALIGN_LEFT, 0);

	initTransformationData();
}

/**
 * cleans up dynamic state
 */
SoldierTransformationState::~SoldierTransformationState()
{

}

/**
 * Creates a string for the soldier stats table
 */
std::string SoldierTransformationState::formatStat(int stat, bool plus, bool hide)
{
	if (hide) return "\x01?\x01"; //Unicode::TOK_COLOR_FLIP
	if (stat == 0) return "";

	std::ostringstream ss;
	ss << Unicode::TOK_COLOR_FLIP;
	if (plus && stat > 0) ss << '+';
	ss << stat << Unicode::TOK_COLOR_FLIP;
	return ss.str();
}

/**
 * Sets all the values for the data fields on the screen
 */
void SoldierTransformationState::initTransformationData()
{
	_edtSoldier->setText(_sourceSoldier->getName());

	_lstRequiredItems->clearList();
	_lstStatChanges->clearList();

	bool transformationPossible = _game->getSavedGame()->getFunds() >= _transformationRule->getCost();

	if (_base->getAvailableQuarters() <= _base->getUsedQuarters() &&
		(_transformationRule->isCreatingClone() ||
		(_transformationRule->isAllowingDeadSoldiers() && _sourceSoldier->getDeath())))
	{
		transformationPossible = false;
	}

	_txtCost->setText(tr("STR_COST_").arg(Unicode::formatFunding(_transformationRule->getCost())));

	int transferTime = 0;
	if (_transformationRule->getTransferTime() > 0 || _transformationRule->isCreatingClone() || _sourceSoldier->getDeath())
	{
		transferTime = _transformationRule->getTransferTime() > 0 ? _transformationRule->getTransferTime() : 24;
	}
	_txtTransferTime->setText(tr("STR_TRANSFER_TIME").arg(tr("STR_HOUR", transferTime)));
	_txtRecoveryTime->setText(tr("STR_RECOVERY_TIME").arg(tr("STR_DAY", _transformationRule->getRecoveryTime())));

	const std::map<std::string, int> & requiredItems(_transformationRule->getRequiredItems());

	int row = 0;
	for (std::map<std::string, int>::const_iterator iter = requiredItems.begin(); iter != requiredItems.end(); ++iter)
	{
		std::ostringstream s1, s2;
		s1 << iter->second;
		if (_game->getMod()->getItem(iter->first) != 0)
		{
			s2 << _base->getStorageItems()->getItem(iter->first);
			transformationPossible &= (_base->getStorageItems()->getItem(iter->first) >= iter->second);
		}

		_lstRequiredItems->addRow(3, tr(iter->first).c_str(), s1.str().c_str(), s2.str().c_str());
		_lstRequiredItems->setCellColor(row, 1, _lstRequiredItems->getSecondaryColor());
		_lstRequiredItems->setCellColor(row, 2, _lstRequiredItems->getSecondaryColor());
		row++;
	}

	UnitStats currentStats = *_sourceSoldier->getCurrentStats();
	UnitStats changedStats = _sourceSoldier->calculateStatChanges(_game->getMod(), _transformationRule, _sourceSoldier);

	_lstStatChanges->addRow(13, "",
		tr("STR_TIME_UNITS_ABBREVIATION").c_str(),
		tr("STR_STAMINA_ABBREVIATION").c_str(),
		tr("STR_HEALTH_ABBREVIATION").c_str(),
		tr("STR_BRAVERY_ABBREVIATION").c_str(),
		tr("STR_REACTIONS_ABBREVIATION").c_str(),
		tr("STR_FIRING_ACCURACY_ABBREVIATION").c_str(),
		tr("STR_THROWING_ACCURACY_ABBREVIATION").c_str(),
		tr("STR_MELEE_ACCURACY_ABBREVIATION").c_str(),
		tr("STR_STRENGTH_ABBREVIATION").c_str(),
		tr("STR_PSIONIC_STRENGTH_ABBREVIATION").c_str(),
		tr("STR_PSIONIC_SKILL_ABBREVIATION").c_str(),
		"");

	bool showPsi = currentStats.psiSkill > 0
		|| (Options::psiStrengthEval && _game->getSavedGame()->isResearched(_game->getMod()->getPsiRequirements()));
	bool isRandom = _transformationRule->isUsingRandomStats();

	_lstStatChanges->addRow(13, tr("STR_CURRENT_STATS").c_str(),
		formatStat(currentStats.tu, false, false).c_str(),
		formatStat(currentStats.stamina, false, false).c_str(),
		formatStat(currentStats.health, false, false).c_str(),
		formatStat(currentStats.bravery, false, false).c_str(),
		formatStat(currentStats.reactions, false, false).c_str(),
		formatStat(currentStats.firing, false, false).c_str(),
		formatStat(currentStats.throwing, false, false).c_str(),
		formatStat(currentStats.melee, false, false).c_str(),
		formatStat(currentStats.strength, false, false).c_str(),
		formatStat(currentStats.psiStrength, false, !showPsi).c_str(),
		formatStat(currentStats.psiSkill, false, !showPsi).c_str(),
		"");

	_lstStatChanges->addRow(13, tr("STR_CHANGES").c_str(),
		formatStat(changedStats.tu, true, isRandom).c_str(),
		formatStat(changedStats.stamina, true, isRandom).c_str(),
		formatStat(changedStats.health, true, isRandom).c_str(),
		formatStat(changedStats.bravery, true, isRandom).c_str(),
		formatStat(changedStats.reactions, true, isRandom).c_str(),
		formatStat(changedStats.firing, true, isRandom).c_str(),
		formatStat(changedStats.throwing, true, isRandom).c_str(),
		formatStat(changedStats.melee, true, isRandom).c_str(),
		formatStat(changedStats.strength, true, isRandom).c_str(),
		formatStat(changedStats.psiStrength, true, !showPsi || isRandom).c_str(),
		formatStat(changedStats.psiSkill, true, !showPsi || isRandom).c_str(),
		"");

	_btnStart->setVisible(transformationPossible);
}

/**
 * Returns to previous screen.
 * @param action A pointer to an Action.
 */
void SoldierTransformationState::btnCancelClick(Action *action)
{
	_game->popState();
}

/**
 * Performs the selected transformation on the selected soldier
 * @param action A pointer to an Action.
 */
void SoldierTransformationState::btnStartClick(Action *action)
{
	// Pay upfront, no refunds
	_game->getSavedGame()->setFunds(_game->getSavedGame()->getFunds() - _transformationRule->getCost());

	for (std::map<std::string, int>::const_iterator i = _transformationRule->getRequiredItems().begin(); i != _transformationRule->getRequiredItems().end(); ++i)
	{
		if (_game->getMod()->getItem(i->first) != 0)
		{
			_base->getStorageItems()->removeItem(i->first, i->second);
		}
	}

	// Here we go
	performTransformation();

	_game->popState();
}

void SoldierTransformationState::performTransformation()
{
	Soldier *destinationSoldier = 0;

	if (_transformationRule->isCreatingClone())
	{
		int newId = _game->getSavedGame()->getId("STR_SOLDIER");
		RuleSoldier *newSoldierType = _game->getMod()->getSoldier(_sourceSoldier->getRules()->getType());
		if (!_transformationRule->getProducedSoldierType().empty())
		{
			newSoldierType = _game->getMod()->getSoldier(_transformationRule->getProducedSoldierType());
		}
		destinationSoldier = new Soldier(
			newSoldierType,
			_game->getMod()->getArmor(newSoldierType->getArmor()),
			newId);

		// copy stuff that is not influenced by transformation ruleset
		destinationSoldier->setGender(_sourceSoldier->getGender());
		destinationSoldier->setLook(_sourceSoldier->getLook());
		destinationSoldier->setLookVariant(_sourceSoldier->getLookVariant());

		// preserve nationality only for soldiers of the same type
		if (_sourceSoldier->getRules() == newSoldierType)
		{
			destinationSoldier->setNationality(_sourceSoldier->getNationality());
		}

		// allow easy name handling when cloning (e.g. John Doe III => John Doe IV)
		destinationSoldier->setName(_edtSoldier->getText());
	}
	else
	{
		destinationSoldier = _sourceSoldier;

		if (_sourceSoldier->getDeath())
		{
			// true resurrect = remove from Memorial Wall
			std::vector<Soldier*>::iterator it = find(_game->getSavedGame()->getDeadSoldiers()->begin(), _game->getSavedGame()->getDeadSoldiers()->end(), _sourceSoldier);
			if (it != _game->getSavedGame()->getDeadSoldiers()->end())
			{
				_game->getSavedGame()->getDeadSoldiers()->erase(it);
			}
		}
		else if (_transformationRule->getTransferTime() > 0)
		{
			// transfer time on a live soldier already at the base (doesn't make much sense, but we need to handle it anyway)
			std::vector<Soldier*>::iterator it = find(_base->getSoldiers()->begin(), _base->getSoldiers()->end(), _sourceSoldier);
			if (it != _base->getSoldiers()->end())
			{
				_base->getSoldiers()->erase(it);
			}
		}
	}

	if (_transformationRule->getTransferTime() > 0 || _transformationRule->isCreatingClone() || _sourceSoldier->getDeath())
	{
		int transferTime = _transformationRule->getTransferTime() > 0 ? _transformationRule->getTransferTime() : 24;
		Transfer *transfer = new Transfer(transferTime);
		transfer->setSoldier(destinationSoldier);
		_base->getTransfers()->push_back(transfer);
	}

	destinationSoldier->transform(_game->getMod(), _transformationRule, _sourceSoldier);
}

/**
 * Handles pressing the Left arrow button
 * @param action A pointer to an Action.
 */
void SoldierTransformationState::btnLeftArrowClick(Action *action)
{
	std::vector<Soldier*>::const_iterator iter = find(_filteredListOfSoldiers->begin(), _filteredListOfSoldiers->end(), _sourceSoldier);
	if (iter == _filteredListOfSoldiers->begin())
	{
		iter = _filteredListOfSoldiers->end();
	}

	--iter;
	_sourceSoldier = *iter;
	initTransformationData();
}

/**
 * Handles pressing the Right arrow button
 * @param action A pointer to an Action.
 */
void SoldierTransformationState::btnRightArrowClick(Action *action)
{
	std::vector<Soldier*>::const_iterator iter = find(_filteredListOfSoldiers->begin(), _filteredListOfSoldiers->end(), _sourceSoldier);
	if (iter == _filteredListOfSoldiers->end() - 1)
	{
		iter = _filteredListOfSoldiers->begin();
	}
	else
	{
		++iter;
	}

	_sourceSoldier = *iter;
	initTransformationData();
}

}
