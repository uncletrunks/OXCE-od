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
#ifndef OPENXCOM_SOLDIERAVATARSTATE_H
#define OPENXCOM_SOLDIERAVATARSTATE_H

#include <vector>
#include "../Engine/State.h"

namespace OpenXcom
{

class Base;
class TextButton;
class Window;
class Text;
class TextList;
class SoldierAvatar;

/**
 * Select Avatar window that allows changing
 * of the soldier's avatar.
 */
class SoldierAvatarState : public State
{
private:
	Base *_base;
	size_t _soldier;

	TextButton *_btnCancel;
	Window *_window;
	Text *_txtTitle, *_txtType, *_txtQuantity;
	TextList *_lstAvatar;
	std::vector<SoldierAvatar> _avatars;
public:
	/// Creates the Soldier Avatar state.
	SoldierAvatarState(Base *base, size_t soldier);
	/// Cleans up the Soldier Avatar state.
	~SoldierAvatarState();
	/// Handler for clicking the Cancel button.
	void btnCancelClick(Action *action);
	/// Handler for clicking the Avatar list.
	void lstAvatarClick(Action *action);
};

}

#endif
