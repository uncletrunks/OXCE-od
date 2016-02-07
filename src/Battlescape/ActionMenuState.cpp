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
#include "ActionMenuState.h"
#include "../Engine/Game.h"
#include "../Engine/Options.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Action.h"
#include "../Engine/Sound.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/BattleItem.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleItem.h"
#include "ActionMenuItem.h"
#include "PrimeGrenadeState.h"
#include "MedikitState.h"
#include "ScannerState.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"
#include "Pathfinding.h"
#include "TileEngine.h"
#include "../Interface/Text.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Action Menu window.
 * @param game Pointer to the core game.
 * @param action Pointer to the action.
 * @param x Position on the x-axis.
 * @param y position on the y-axis.
 */
ActionMenuState::ActionMenuState(BattleAction *action, int x, int y) : _action(action)
{
	_screen = false;

	// Set palette
	_game->getSavedGame()->getSavedBattle()->setPaletteByDepth(this);

	for (int i = 0; i < 6; ++i)
	{
		_actionMenu[i] = new ActionMenuItem(i, _game, x, y);
		add(_actionMenu[i]);
		_actionMenu[i]->setVisible(false);
		_actionMenu[i]->onMouseClick((ActionHandler)&ActionMenuState::btnActionMenuItemClick);
	}

	// Build up the popup menu
	int id = 0;
	RuleItem *weapon = _action->weapon->getRules();

	// throwing (if not a fixed weapon)
	if (!weapon->isFixed() && weapon->getCostThrow().Time > 0)
	{
		addItem(BA_THROW, "STR_THROW", &id);
	}

	// execute / break neck / cut throat / coup de grace
	if (Options::executeUnconsciousEnemies && (_action->weapon->getUnit() && _action->weapon->getUnit()->getStatus() == STATUS_UNCONSCIOUS))
	{
		BattleItem *otherWeapon = 0;

		// check left hand for secondary weapon
		BattleItem *leftHandWeapon = _game->getSavedGame()->getSavedBattle()->getSelectedUnit()->getItem("STR_LEFT_HAND");
		if (leftHandWeapon)
		{
			if (leftHandWeapon->getRules()->getCostMelee().Time > 0)
			{
				// melee weapons with melee attack (i.e. excl. stun weapons and others)
				if (leftHandWeapon->getRules()->getBattleType() == BT_MELEE && leftHandWeapon->getRules()->getDamageType()->ResistType == DT_MELEE)
				{
					otherWeapon = leftHandWeapon;
				}
			}
		}

		// check right hand for secondary weapon
		BattleItem *rightHandWeapon = _game->getSavedGame()->getSavedBattle()->getSelectedUnit()->getItem("STR_RIGHT_HAND");
		if (rightHandWeapon)
		{
			if (rightHandWeapon->getRules()->getCostMelee().Time > 0)
			{
				// melee weapons with melee attack (i.e. excl. stun weapons and others)
				if (rightHandWeapon->getRules()->getBattleType() == BT_MELEE && rightHandWeapon->getRules()->getDamageType()->ResistType == DT_MELEE)
				{
					otherWeapon = rightHandWeapon;
				}
			}
		}

		if (otherWeapon != 0)
		{
			addItem(BA_EXECUTE, "STR_CUT_THROAT", &id, otherWeapon);
		}
	}

	if (weapon->isPsiRequired() && _action->actor->getBaseStats()->psiSkill <= 0)
	{
		return;
	}

	// priming
	if (weapon->getFuseTimerDefault() >= 0 && _action->weapon->getFuseTimer() == -1)
	{
		addItem(BA_PRIME, "STR_PRIME_GRENADE", &id);
	}

	if (weapon->getBattleType() == BT_FIREARM)
	{
		if (weapon->isWaypoint() || (_action->weapon->getAmmoItem() && _action->weapon->getAmmoItem()->getRules()->isWaypoint()))
		{
			addItem(BA_LAUNCH, "STR_LAUNCH_MISSILE", &id);
		}
		else
		{
			if (weapon->getCostAuto().Time > 0)
			{
				addItem(BA_AUTOSHOT, "STR_AUTO_SHOT", &id);
			}
			if (weapon->getCostSnap().Time > 0)
			{
				addItem(BA_SNAPSHOT, "STR_SNAP_SHOT", &id);
			}
			if (weapon->getCostAimed().Time > 0)
			{
				addItem(BA_AIMEDSHOT, "STR_AIMED_SHOT", &id);
			}
		}
	}

	if (weapon->getCostMelee().Time > 0)
	{
		// stun rod
		if (weapon->getBattleType() == BT_MELEE && weapon->getDamageType()->ResistType == DT_STUN)
		{
			addItem(BA_HIT, "STR_STUN", &id);
		}
		else
		// melee weapon
		{
			addItem(BA_HIT, "STR_HIT_MELEE", &id);
		}
	}

	// special items
	if (weapon->getBattleType() == BT_MEDIKIT)
	{
		addItem(BA_USE, "STR_USE_MEDI_KIT", &id);
	}
	else if (weapon->getBattleType() == BT_SCANNER)
	{
		addItem(BA_USE, "STR_USE_SCANNER", &id);
	}
	else if (weapon->getBattleType() == BT_PSIAMP)
	{
		if (weapon->getCostMind().Time > 0)
		{
			addItem(BA_MINDCONTROL, "STR_MIND_CONTROL", &id);
		}
		if (weapon->getCostPanic().Time > 0)
		{
			addItem(BA_PANIC, "STR_PANIC_UNIT", &id);
		}
		if (weapon->getCostUse().Time > 0)
		{
			addItem(BA_USE, weapon->getPsiAttackName(), &id);
		}
	}
	else if (weapon->getBattleType() == BT_MINDPROBE)
	{
		addItem(BA_USE, "STR_USE_MIND_PROBE", &id);
	}

}

/**
 * Deletes the ActionMenuState.
 */
ActionMenuState::~ActionMenuState()
{

}

/**
 * Adds a new menu item for an action.
 * @param ba Action type.
 * @param name Action description.
 * @param id Pointer to the new item ID.
 */
void ActionMenuState::addItem(BattleActionType ba, const std::string &name, int *id, BattleItem *secondaryWeapon)
{
	std::wstring s1, s2;
	int acc = _action->actor->getFiringAccuracy(ba, _action->weapon);
	if (secondaryWeapon != 0)
	{
		// for display only, this action will never miss anyway (alien is unconscious, how could you miss?)
		acc = 999;
		// backup the original "weapon" (i.e. unconscious alien) for later use (when we need to execute them)
		_action->origWeapon = _action->weapon;
		// this is actually important, so that we spend TUs (and other stats) correctly
		_action->weapon = secondaryWeapon;
	}
	int tu = _action->actor->getActionTUs(ba, (secondaryWeapon == 0) ? _action->weapon : secondaryWeapon).Time;

	if (ba == BA_THROW || ba == BA_AIMEDSHOT || ba == BA_SNAPSHOT || ba == BA_AUTOSHOT || ba == BA_LAUNCH || ba == BA_HIT || ba == BA_EXECUTE)
		s1 = tr("STR_ACCURACY_SHORT").arg(Text::formatPercentage(acc));
	s2 = tr("STR_TIME_UNITS_SHORT").arg(tu);
	_actionMenu[*id]->setAction(ba, tr(name), s1, s2, tu);
	_actionMenu[*id]->setVisible(true);
	(*id)++;
}

/**
 * Closes the window on right-click.
 * @param action Pointer to an action.
 */
void ActionMenuState::handle(Action *action)
{
	State::handle(action);
	if (action->getDetails()->type == SDL_MOUSEBUTTONDOWN && action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		_game->popState();
	}
	else if (action->getDetails()->type == SDL_KEYDOWN &&
		(action->getDetails()->key.keysym.sym == Options::keyCancel ||
		action->getDetails()->key.keysym.sym == Options::keyBattleUseLeftHand ||
		action->getDetails()->key.keysym.sym == Options::keyBattleUseRightHand))
	{
		_game->popState();
	}
}

/**
 * Executes the action corresponding to this action menu item.
 * @param action Pointer to an action.
 */
void ActionMenuState::btnActionMenuItemClick(Action *action)
{
	_game->getSavedGame()->getSavedBattle()->getPathfinding()->removePreview();

	int btnID = -1;
	RuleItem *weapon = _action->weapon->getRules();

	// got to find out which button was pressed
	for (size_t i = 0; i < sizeof(_actionMenu)/sizeof(_actionMenu[0]) && btnID == -1; ++i)
	{
		if (action->getSender() == _actionMenu[i])
		{
			btnID = i;
		}
	}

	if (btnID != -1)
	{
		_action->type = _actionMenu[btnID]->getAction();
		_action->updateTU();
		if (_action->type != BA_THROW &&
			_action->actor->getOriginalFaction() == FACTION_PLAYER &&
			!_game->getSavedGame()->isResearched(weapon->getRequirements()))
		{
			_action->result = "STR_UNABLE_TO_USE_ALIEN_ARTIFACT_UNTIL_RESEARCHED";
			_game->popState();
		}
		else if (weapon->isWaterOnly() &&
			_game->getSavedGame()->getSavedBattle()->getDepth() == 0 &&
			_action->type != BA_THROW)
		{
			_action->result = "STR_UNDERWATER_EQUIPMENT";
			_game->popState();
		}
		else if (_action->type != BA_THROW &&
			_action->actor->getFaction() == FACTION_PLAYER &&
			weapon->isBlockingBothHands() &&
			_action->actor->getItem("STR_LEFT_HAND") != 0 &&
			_action->actor->getItem("STR_RIGHT_HAND") != 0)
		{
			_action->result = "STR_MUST_USE_BOTH_HANDS";
			_game->popState();
		}
		else if (_action->type == BA_PRIME)
		{
			const BattleFuseType fuseType = weapon->getFuseTimerType();
			if (fuseType == BFT_SET)
			{
				_game->pushState(new PrimeGrenadeState(_action, false, 0));
			}
			else
			{
				_action->value = weapon->getFuseTimerDefault();
				_game->popState();
			}
		}
		else if (_action->type == BA_USE && weapon->getBattleType() == BT_MEDIKIT)
		{
			BattleUnit *targetUnit = 0;
			TileEngine *tileEngine = _game->getSavedGame()->getSavedBattle()->getTileEngine();
			const std::vector<BattleUnit*> *units = _game->getSavedGame()->getSavedBattle()->getUnits();
			for (std::vector<BattleUnit*>::const_iterator i = units->begin(); i != units->end() && !targetUnit; ++i)
			{
				// we can heal a unit that is at the same position, unconscious and healable(=woundable)
				if ((*i)->getPosition() == _action->actor->getPosition() && *i != _action->actor && (*i)->getStatus() == STATUS_UNCONSCIOUS && (*i)->isWoundable())
				{
					targetUnit = *i;
				}
			}
			if (!targetUnit)
			{
				if (tileEngine->validMeleeRange(
					_action->actor->getPosition(),
					_action->actor->getDirection(),
					_action->actor,
					0, &_action->target, false))
				{
					Tile *tile = _game->getSavedGame()->getSavedBattle()->getTile(_action->target);
					if (tile != 0 && tile->getUnit() && tile->getUnit()->isWoundable())
					{
						targetUnit = tile->getUnit();
					}
				}
			}
			if (!targetUnit && weapon->getAllowSelfHeal())
			{
				targetUnit = _action->actor;
			}
			if (targetUnit)
			{
				_game->popState();
				BattleMediKitType type = weapon->getMediKitType();
				if (type)
				{
					if ((type == BMT_HEAL && _action->weapon->getHealQuantity() > 0) ||
						(type == BMT_STIMULANT && _action->weapon->getStimulantQuantity() > 0) ||
						(type == BMT_PAINKILLER && _action->weapon->getPainKillerQuantity() > 0))
					{
						if (_action->spendTU(&_action->result))
						{
							switch (type)
							{
							case BMT_HEAL:
								if (targetUnit->getFatalWounds())
								{
									for (int i = 0; i < BODYPART_MAX; ++i)
									{
										if (targetUnit->getFatalWound(i))
										{
											tileEngine->medikitHeal(_action, targetUnit, i);
											tileEngine->medikitRemoveIfEmpty(_action);
											break;
										}
									}
								}
								else
								{
									tileEngine->medikitHeal(_action, targetUnit, BODYPART_TORSO);
									tileEngine->medikitRemoveIfEmpty(_action);
								}
								break;
							case BMT_STIMULANT:
								tileEngine->medikitStimulant(_action, targetUnit);
								tileEngine->medikitRemoveIfEmpty(_action);
								break;
							case BMT_PAINKILLER:
								tileEngine->medikitPainKiller(_action, targetUnit);
								tileEngine->medikitRemoveIfEmpty(_action);
								break;
							case BMT_NORMAL:
								break;
							}
						}
					}
					else
					{
						_action->result = "STR_NO_USES_LEFT";
					}
				}
				else
				{
					_game->pushState(new MedikitState(targetUnit, _action, tileEngine));
				}
			}
			else
			{
				_action->result = "STR_THERE_IS_NO_ONE_THERE";
				_game->popState();
			}
		}
		else if (_action->type == BA_USE && weapon->getBattleType() == BT_SCANNER)
		{
			// spend TUs first, then show the scanner
			if (_action->spendTU(&_action->result))
			{
				_game->popState();
				_game->pushState (new ScannerState(_action));
			}
			else
			{
				_game->popState();
			}
		}
		else if (_action->type == BA_LAUNCH)
		{
			// check beforehand if we have enough time units
			if (!_action->haveTU(&_action->result))
			{
				//nothing
			}
			else if (_action->weapon->getAmmoItem() ==0 || (_action->weapon->getAmmoItem() && _action->weapon->getAmmoItem()->getAmmoQuantity() == 0))
			{
				_action->result = "STR_NO_AMMUNITION_LOADED";
			}
			else
			{
				_action->targeting = true;
			}
			_game->popState();
		}
		else if (_action->type == BA_HIT)
		{
			// check beforehand if we have enough time units
			if (!_action->haveTU(&_action->result))
			{
				//nothing
			}
			else if (!_game->getSavedGame()->getSavedBattle()->getTileEngine()->validMeleeRange(
				_action->actor->getPosition(),
				_action->actor->getDirection(),
				_action->actor,
				0, &_action->target))
			{
				_action->result = "STR_THERE_IS_NO_ONE_THERE";
			}
			_game->popState();
		}
		else if (_action->type == BA_EXECUTE)
		{
			if (_action->spendTU(&_action->result))
			{
				if (_action->origWeapon != 0)
				{
					// kill unit
					_action->origWeapon->getUnit()->instaKill();
					// convert inventory item to corpse
					RuleItem *corpseRules = _game->getMod()->getItem(_action->origWeapon->getUnit()->getArmor()->getCorpseBattlescape()[0]); // we're in an inventory, so we must be a 1x1 unit
					_action->origWeapon->convertToCorpse(corpseRules);
					// inform the player
					_action->result = "STR_TARGET_WAS_EXECUTED";
					// audio feedback
					_game->getMod()->getSoundByDepth(_game->getSavedGame()->getSavedBattle()->getDepth(), _action->weapon->getRules()->getMeleeHitSound())->play();
				}
			}
			_game->popState();
		}
		else
		{
			_action->targeting = true;
			_game->popState();
		}
	}
}

/**
 * Updates the scale.
 * @param dX delta of X;
 * @param dY delta of Y;
 */
void ActionMenuState::resize(int &dX, int &dY)
{
	State::recenter(dX, dY * 2);
}

}
