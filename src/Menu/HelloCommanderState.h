#pragma once
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
#include "../Engine/State.h"

namespace OpenXcom
{

class Surface;
class Text;
class TextButton;

/**
 * Easter Egg.
 */
class HelloCommanderState : public State
{
private:
	Surface *_bg;
	Text *_txtMessage;
	TextButton *_btnOk;
	bool _exit;
public:
	/// Creates the HelloCommander state.
	HelloCommanderState();
	/// Cleans up the HelloCommander state.
	~HelloCommanderState();
	/// Initializes the state.
	void init();
	/// Handler for clicking the OK button.
	void btnOkClick(Action *action);
};

}
