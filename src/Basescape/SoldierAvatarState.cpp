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
#include "SoldierAvatarState.h"
#include <sstream>
#include "../Engine/Game.h"
#include "../Mod/Mod.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/SoldierAvatar.h"
#include "../Savegame/Base.h"
#include "../Savegame/ItemContainer.h"
#include "../Mod/Mod.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Soldier Avatar window.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 * @param soldier ID of the selected soldier.
 */
SoldierAvatarState::SoldierAvatarState(Base *base, size_t soldier) : _base(base), _soldier(soldier)
{
	_screen = false;

	// Create objects
	_window = new Window(this, 192, 120, 64, 40, POPUP_BOTH);
	_btnCancel = new TextButton(140, 16, 90, 136);
	_txtTitle = new Text(182, 16, 69, 48);
	_txtType = new Text(90, 9, 80, 72);
	_txtQuantity = new Text(70, 9, 190, 72);
	_lstAvatar = new TextList(160, 40, 73, 88);

	// Set palette
	setInterface("soldierArmor");

	add(_window, "window", "soldierArmor");
	add(_btnCancel, "button", "soldierArmor");
	add(_txtTitle, "text", "soldierArmor");
	add(_txtType, "text", "soldierArmor");
	add(_txtQuantity, "text", "soldierArmor");
	add(_lstAvatar, "list", "soldierArmor");

	centerAllSurfaces();

	// Set up objects
	_window->setBackground(_game->getMod()->getSurface("BACK14.SCR"));

	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick((ActionHandler)&SoldierAvatarState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&SoldierAvatarState::btnCancelClick, Options::keyCancel);

	Soldier *s = _base->getSoldiers()->at(_soldier);
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("SELECT AVATAR FOR\n{0}").arg(s->getName()));

	_txtType->setText(tr("STR_TYPE"));

	_txtQuantity->setText(tr("STR_QUANTITY_UC"));

	_lstAvatar->setColumns(2, 132, 21);
	_lstAvatar->setSelectable(true);
	_lstAvatar->setBackground(_window);
	_lstAvatar->setMargin(8);

	// FIXME: 32 hard-coded avatars from X-PirateZ 0.97b
	_avatars.push_back(SoldierAvatar("STR_AVATAR_01", SoldierGender::GENDER_FEMALE, SoldierLook::LOOK_BLONDE, 0));
	_avatars.push_back(SoldierAvatar("STR_AVATAR_02", SoldierGender::GENDER_FEMALE, SoldierLook::LOOK_AFRICAN, 0));
	_avatars.push_back(SoldierAvatar("STR_AVATAR_03", SoldierGender::GENDER_MALE, SoldierLook::LOOK_BLONDE, 0));
	_avatars.push_back(SoldierAvatar("STR_AVATAR_04", SoldierGender::GENDER_MALE, SoldierLook::LOOK_AFRICAN, 0));
	_avatars.push_back(SoldierAvatar("STR_AVATAR_05", SoldierGender::GENDER_MALE, SoldierLook::LOOK_BROWNHAIR, 0));
	_avatars.push_back(SoldierAvatar("STR_AVATAR_06", SoldierGender::GENDER_MALE, SoldierLook::LOOK_ORIENTAL, 0));
	_avatars.push_back(SoldierAvatar("STR_AVATAR_07", SoldierGender::GENDER_FEMALE, SoldierLook::LOOK_BROWNHAIR, 0));
	_avatars.push_back(SoldierAvatar("STR_AVATAR_08", SoldierGender::GENDER_FEMALE, SoldierLook::LOOK_ORIENTAL, 0));
	_avatars.push_back(SoldierAvatar("STR_AVATAR_09", SoldierGender::GENDER_FEMALE, SoldierLook::LOOK_ORIENTAL, 1));
	_avatars.push_back(SoldierAvatar("STR_AVATAR_10", SoldierGender::GENDER_FEMALE, SoldierLook::LOOK_BLONDE, 1));
	_avatars.push_back(SoldierAvatar("STR_AVATAR_11", SoldierGender::GENDER_MALE, SoldierLook::LOOK_AFRICAN, 1));
	_avatars.push_back(SoldierAvatar("STR_AVATAR_12", SoldierGender::GENDER_MALE, SoldierLook::LOOK_BLONDE, 1));
	_avatars.push_back(SoldierAvatar("STR_AVATAR_13", SoldierGender::GENDER_MALE, SoldierLook::LOOK_ORIENTAL, 1));
	_avatars.push_back(SoldierAvatar("STR_AVATAR_14", SoldierGender::GENDER_MALE, SoldierLook::LOOK_BROWNHAIR, 1));
	_avatars.push_back(SoldierAvatar("STR_AVATAR_15", SoldierGender::GENDER_FEMALE, SoldierLook::LOOK_AFRICAN, 1));
	_avatars.push_back(SoldierAvatar("STR_AVATAR_16", SoldierGender::GENDER_FEMALE, SoldierLook::LOOK_BROWNHAIR, 1));
	_avatars.push_back(SoldierAvatar("STR_AVATAR_17", SoldierGender::GENDER_MALE, SoldierLook::LOOK_BLONDE, 2));
	_avatars.push_back(SoldierAvatar("STR_AVATAR_18", SoldierGender::GENDER_FEMALE, SoldierLook::LOOK_ORIENTAL, 2));
	_avatars.push_back(SoldierAvatar("STR_AVATAR_19", SoldierGender::GENDER_MALE, SoldierLook::LOOK_BROWNHAIR, 2));
	_avatars.push_back(SoldierAvatar("STR_AVATAR_20", SoldierGender::GENDER_MALE, SoldierLook::LOOK_ORIENTAL, 2));
	_avatars.push_back(SoldierAvatar("STR_AVATAR_21", SoldierGender::GENDER_FEMALE, SoldierLook::LOOK_BROWNHAIR, 2));
	_avatars.push_back(SoldierAvatar("STR_AVATAR_22", SoldierGender::GENDER_MALE, SoldierLook::LOOK_AFRICAN, 2));
	_avatars.push_back(SoldierAvatar("STR_AVATAR_23", SoldierGender::GENDER_FEMALE, SoldierLook::LOOK_BLONDE, 2));
	_avatars.push_back(SoldierAvatar("STR_AVATAR_24", SoldierGender::GENDER_FEMALE, SoldierLook::LOOK_AFRICAN, 2));
	_avatars.push_back(SoldierAvatar("STR_AVATAR_25", SoldierGender::GENDER_FEMALE, SoldierLook::LOOK_ORIENTAL, 3));
	_avatars.push_back(SoldierAvatar("STR_AVATAR_26", SoldierGender::GENDER_FEMALE, SoldierLook::LOOK_BLONDE, 3));
	_avatars.push_back(SoldierAvatar("STR_AVATAR_27", SoldierGender::GENDER_MALE, SoldierLook::LOOK_ORIENTAL, 3));
	_avatars.push_back(SoldierAvatar("STR_AVATAR_28", SoldierGender::GENDER_MALE, SoldierLook::LOOK_BLONDE, 3));
	_avatars.push_back(SoldierAvatar("STR_AVATAR_29", SoldierGender::GENDER_MALE, SoldierLook::LOOK_AFRICAN, 3));
	_avatars.push_back(SoldierAvatar("STR_AVATAR_30", SoldierGender::GENDER_MALE, SoldierLook::LOOK_BROWNHAIR, 3));
	_avatars.push_back(SoldierAvatar("STR_AVATAR_31", SoldierGender::GENDER_FEMALE, SoldierLook::LOOK_AFRICAN, 3));
	_avatars.push_back(SoldierAvatar("STR_AVATAR_32", SoldierGender::GENDER_FEMALE, SoldierLook::LOOK_BROWNHAIR, 3));

	for (std::vector<SoldierAvatar>::const_iterator i = _avatars.begin(); i != _avatars.end(); ++i)
	{
		_lstAvatar->addRow(1, tr(i->getAvatarName()).c_str());
	}
	_lstAvatar->onMouseClick((ActionHandler)&SoldierAvatarState::lstAvatarClick);
}

/**
 *
 */
SoldierAvatarState::~SoldierAvatarState()
{

}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void SoldierAvatarState::btnCancelClick(Action *)
{
	_game->popState();
}

/**
 * Changes the avatar on the soldier and returns to the previous screen.
 * @param action Pointer to an action.
 */
void SoldierAvatarState::lstAvatarClick(Action *)
{
	Soldier *soldier = _base->getSoldiers()->at(_soldier);

	// change the avatar
	soldier->setGender(_avatars[_lstAvatar->getSelectedRow()].getGender());
	soldier->setLook(_avatars[_lstAvatar->getSelectedRow()].getLook());
	soldier->setLookVariant(_avatars[_lstAvatar->getSelectedRow()].getLookVariant());

	_game->popState();
}

}
