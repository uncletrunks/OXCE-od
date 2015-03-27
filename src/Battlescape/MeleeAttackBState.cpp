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
#include "MeleeAttackBState.h"
#include "ExplosionBState.h"
#include "BattlescapeGame.h"
#include "BattlescapeState.h"
#include "TileEngine.h"
#include "Pathfinding.h"
#include "Map.h"
#include "InfoboxState.h"
#include "Camera.h"
#include "AlienBAIState.h"
#include "../Savegame/Tile.h"
#include "../Engine/RNG.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/BattleItem.h"
#include "../Engine/Sound.h"
#include "../Resource/ResourcePack.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Armor.h"

namespace OpenXcom
{

/**
 * Sets up a MeleeAttackBState.
 */
MeleeAttackBState::MeleeAttackBState(BattlescapeGame *parent, BattleAction action) : BattleState(parent, action), _unit(0), _target(0), _weapon(0), _ammo(0), _initialized(false), _deathMessage(false)
{
}

/**
 * Deletes the MeleeAttackBState.
 */
MeleeAttackBState::~MeleeAttackBState()
{
}

/**
 * Initializes the sequence.
 * does a lot of validity checking.
 */
void MeleeAttackBState::init()
{
	if (_initialized) return;
	_initialized = true;

	_weapon = _action.weapon;

	if (!_weapon) // can't shoot without weapon
	{
		_parent->popState();
		return;
	}

	_ammo = _weapon->getAmmoItem();

	if (!_parent->getSave()->getTile(_action.target)) // invalid target position
	{
		_parent->popState();
		return;
	}

	_unit = _action.actor;

	if (_unit->isOut() || _unit->getHealth() <= 0 || _unit->getHealth() < _unit->getStunlevel())
	{
		// something went wrong - we can't shoot when dead or unconscious, or if we're about to fall over.
		_parent->popState();
		return;
	}

	// reaction fire
	if (_unit->getFaction() != _parent->getSave()->getSide())
	{
		// no ammo or target is dead: give the time units back and cancel the shot.
		if (_ammo == 0
			|| !_parent->getSave()->getTile(_action.target)->getUnit()
			|| _parent->getSave()->getTile(_action.target)->getUnit()->isOut()
			|| _parent->getSave()->getTile(_action.target)->getUnit() != _parent->getSave()->getSelectedUnit())
		{
			_action.rollbackTU();
			_parent->popState();
			return;
		}
		_unit->lookAt(_action.target, _unit->getTurretType() != -1);
		while (_unit->getStatus() == STATUS_TURNING)
		{
			_unit->turn();
		}
	}


	AlienBAIState *ai = dynamic_cast<AlienBAIState*>(_unit->getCurrentAIState());

	if (ai && ai->getTarget())
	{
		_target = ai->getTarget();
	}
	else
	{
		_target = _parent->getSave()->getTile(_action.target)->getUnit();
	}

	int height = _target->getFloatHeight() + (_target->getHeight() / 2) - _parent->getSave()->getTile(_action.target)->getTerrainLevel();
	_parent->getSave()->getPathfinding()->directionToVector(_unit->getDirection(), &_voxel);
	_voxel = _action.target.toVexel() + Position(8, 8, height) - _voxel;

	performMeleeAttack();
}
/**
 * Performs all the overall functions of the state, this code runs AFTER the explosion state pops.
 */
void MeleeAttackBState::think()
{
	if (_deathMessage)
	{
		Game *game = _parent->getSave()->getBattleState()->getGame();
		game->pushState(new InfoboxState(game->getLanguage()->getString("STR_HAS_BEEN_KILLED", _target->getGender()).arg(_target->getName(game->getLanguage()))));
		_parent->popState();
		return;
	}
	_parent->getSave()->getBattleState()->clearMouseScrollingState();

	// if the unit burns floortiles, burn floortiles
	if (_unit->getSpecialAbility() == SPECAB_BURNFLOOR || _unit->getSpecialAbility() == SPECAB_BURN_AND_EXPLODE)
	{
		_parent->getSave()->getTile(_action.target)->ignite(15);
	}
		// aliens
	if (_unit->getFaction() != FACTION_PLAYER &&
		// with enough TU for a second attack (*2 because they'll get charged for the initial attack when this state pops.)
		_action.haveMultipleTU(2) &&
		// whose target is still alive or at least conscious
		_target && _target->getHealth() > 0 &&
		_target->getHealth() > _target->getStunlevel() &&
		// and we still have ammo to make the attack
		_weapon->getAmmoItem())
	{
		// spend the TUs immediately
		_action.spendTU();
		performMeleeAttack();
	}
	else
	{
		if (_action.cameraPosition.z != -1)
		{
			_parent->getMap()->getCamera()->setMapOffset(_action.cameraPosition);
			_parent->getMap()->invalidate();
		}
//		melee doesn't trigger a reaction, remove comments to enable.
//		if (!_parent->getSave()->getUnitsFalling())
//		{
//			_parent->getTileEngine()->checkReactionFire(_unit);
//		}

		if (_parent->getSave()->getSide() == FACTION_PLAYER || _parent->getSave()->getDebugMode())
		{
			_parent->setupCursor();
		}
		if (_parent->convertInfected() && Options::battleNotifyDeath && _target && _target->getFaction() == FACTION_PLAYER)
		{
			_deathMessage = true;
			_parent->setStateInterval(BattlescapeState::DEFAULT_ANIM_SPEED);
		}
		else
		{
			_parent->popState();
		}
	}
}

/**
 * Sets up a melee attack, inserts an explosion into the map and make noises.
 */
void MeleeAttackBState::performMeleeAttack()
{
	// set the soldier in an aiming position
	_unit->aim(true);
	_unit->setCache(0);
	_parent->getMap()->cacheUnit(_unit);

	// use up ammo if applicable
	if (!_parent->getSave()->getDebugMode() && _weapon->getRules()->getBattleType() == BT_MELEE && _ammo && _ammo->spendBullet() == false)
	{
		_parent->getSave()->removeItem(_ammo);
		_action.weapon->setAmmoItem(0);
	}
	_parent->getMap()->setCursorType(CT_NONE);

	// make an explosion action
	_parent->statePushFront(new ExplosionBState(_parent, _voxel, BA_HIT, _action.weapon, _action.actor, 0, true));
}

}
