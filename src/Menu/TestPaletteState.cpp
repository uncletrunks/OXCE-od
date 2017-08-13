/*
 * Copyright 2010-2016 OpenXcom Developers.
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
#include "TestPaletteState.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Mod/Mod.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the test palette screen.
 */
TestPaletteState::TestPaletteState(const std::string &palette, bool highContrast)
{
	// Create objects
	_bg = new Surface(320, 200, 0, 0);
	_btnCancel = new TextButton(60, 9, 240, 190);

	// Set palette
	setPalette(palette);

	add(_bg);
	add(_btnCancel);

	centerAllSurfaces();

	_btnCancel->onMouseClick((ActionHandler)&TestPaletteState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&TestPaletteState::btnCancelClick, Options::keyCancel);

	Text text = Text(25, 9, 0, 0);
	text.setPalette(_bg->getPalette());
	text.initText(_game->getMod()->getFont("FONT_BIG"), _game->getMod()->getFont("FONT_SMALL"), _game->getLanguage());

	RuleInterface *rule = _game->getMod()->getInterface("inventory");

	text.setHighContrast(highContrast);
	for (int row = 0; row < 22; ++row)
	{
		for (int column = 0; column < 12; ++column)
		{
			int index = row * 12 + column;
			if (index > 255)
			{
				return;
			}
			text.setColor(index);
			text.setX(column * 26);
			text.setY(row * 9);
			std::wostringstream ss;
			ss << index;
			text.setText(ss.str().c_str());
			text.blit(_bg);
		}
	}
}

TestPaletteState::~TestPaletteState()
{
}

/**
* Returns to the previous screen.
* @param action Pointer to an action.
*/
void TestPaletteState::btnCancelClick(Action *action)
{
	_game->popState();
}

}
