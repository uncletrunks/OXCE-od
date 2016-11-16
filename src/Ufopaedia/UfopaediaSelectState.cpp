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

#include "Ufopaedia.h"
#include "UfopaediaSelectState.h"
#include "../Mod/ArticleDefinition.h"
#include "../Engine/Game.h"
#include "../Engine/Options.h"
#include "../Engine/LocalizedText.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextEdit.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Mod/Mod.h"
#include "../Savegame/SavedGame.h"

namespace OpenXcom
{
	UfopaediaSelectState::UfopaediaSelectState(const std::string &section) : _section(section)
	{
		_screen = false;

		// set background window
		_window = new Window(this, 256, 180, 32, 10, POPUP_NONE);
		_btnQuickSearch = new TextEdit(this, 48, 9, 48, 30);

		// set title
		_txtTitle = new Text(224, 17, 48, 26);

		// set buttons
		_btnOk = new TextButton(108, 16, 164, 166);
		_btnMarkAllAsSeen = new TextButton(108, 16, 48, 166);
		_lstSelection = new TextList(224, 104, 40, 50);

		// Set palette
		setInterface("ufopaedia");

		add(_window, "window", "ufopaedia");
		add(_btnQuickSearch, "button2", "ufopaedia");
		add(_txtTitle, "text", "ufopaedia");
		add(_btnOk, "button2", "ufopaedia");
		add(_btnMarkAllAsSeen, "button2", "ufopaedia");
		add(_lstSelection, "list", "ufopaedia");

		centerAllSurfaces();

		_window->setBackground(_game->getMod()->getSurface("BACK01.SCR"));

		_txtTitle->setBig();
		_txtTitle->setAlign(ALIGN_CENTER);
		_txtTitle->setText(tr("STR_SELECT_ITEM"));

		_btnOk->setText(tr("STR_OK"));
		_btnOk->onMouseClick((ActionHandler)&UfopaediaSelectState::btnOkClick);
		_btnOk->onKeyboardPress((ActionHandler)&UfopaediaSelectState::btnOkClick,Options::keyCancel);

		_btnMarkAllAsSeen->setText(tr("MARK ALL AS SEEN"));
		_btnMarkAllAsSeen->onMouseClick((ActionHandler)&UfopaediaSelectState::btnMarkAllAsSeenClick);

		_lstSelection->setColumns(1, 206);
		_lstSelection->setSelectable(true);
		_lstSelection->setBackground(_window);
		_lstSelection->setMargin(18);
		_lstSelection->setAlign(ALIGN_CENTER);
		_lstSelection->onMouseClick((ActionHandler)&UfopaediaSelectState::lstSelectionClick);

		_btnQuickSearch->setText(L""); // redraw
		_btnQuickSearch->onEnter((ActionHandler)&UfopaediaSelectState::btnQuickSearchApply);
		_btnQuickSearch->setVisible(Options::showQuickSearch);

		_btnOk->onKeyboardRelease((ActionHandler)&UfopaediaSelectState::btnQuickSearchToggle, Options::keyToggleQuickSearch);
	}

	UfopaediaSelectState::~UfopaediaSelectState()
	{}
	
	/**
	 * Initializes the state.
	 */
	void UfopaediaSelectState::init()
	{
		State::init();
		loadSelectionList(false);
	}

	/**
	 * Returns to the previous screen.
	 * @param action Pointer to an action.
	 */
	void UfopaediaSelectState::btnOkClick(Action *)
	{
		_game->popState();
	}

	/**
	 * Marks all items as seen
	 * @param action Pointer to an action.
	 */
	void UfopaediaSelectState::btnMarkAllAsSeenClick(Action *)
	{
		loadSelectionList(true);
	}

	/**
	 *
	 * @param action Pointer to an action.
	 */
	void UfopaediaSelectState::lstSelectionClick(Action *)
	{
		Ufopaedia::openArticle(_game, _filtered_article_list[_lstSelection->getSelectedRow()]);
	}

	/**
	* Quick search toggle.
	* @param action Pointer to an action.
	*/
	void UfopaediaSelectState::btnQuickSearchToggle(Action *action)
	{
		if (_btnQuickSearch->getVisible())
		{
			_btnQuickSearch->setText(L"");
			_btnQuickSearch->setVisible(false);
			btnQuickSearchApply(action);
		}
		else
		{
			_btnQuickSearch->setVisible(true);
			_btnQuickSearch->setFocus(true);
		}
	}

	/**
	* Quick search.
	* @param action Pointer to an action.
	*/
	void UfopaediaSelectState::btnQuickSearchApply(Action *)
	{
		loadSelectionList(false);
	}

	void UfopaediaSelectState::loadSelectionList(bool markAllAsSeen)
	{
		std::wstring searchString = _btnQuickSearch->getText();
		for (auto & c : searchString) c = towupper(c);

		ArticleDefinitionList::iterator it;

		_lstSelection->clearList();
		_article_list.clear();
		Ufopaedia::list(_game->getSavedGame(), _game->getMod(), _section, _article_list);
		_filtered_article_list.clear();

		int row = 0;
		bool hasUnseen = false;
		for (it = _article_list.begin(); it!=_article_list.end(); ++it)
		{
			// quick search
			if (searchString != L"")
			{
				std::wstring projectName = tr((*it)->title);
				for (auto & c : projectName) c = towupper(c);
				if (projectName.find(searchString) == std::string::npos)
				{
					continue;
				}
			}

			_filtered_article_list.push_back((*it));
			_lstSelection->addRow(1, tr((*it)->title).c_str());

			if (markAllAsSeen)
			{
				// remember all listed articles as seen
				_game->getSavedGame()->addSeenUfopediaArticle((*it));
			}
			else if (!_game->getSavedGame()->isUfopediaArticleSeen((*it)->id))
			{
				// mark as unseen
				_lstSelection->setCellColor(row, 0, 90); // light green
				hasUnseen = true;
			}
			row++;
		}

		if (!hasUnseen)
		{
			_btnMarkAllAsSeen->setVisible(false);
			_btnOk->setWidth(_btnOk->getX()+_btnOk->getWidth()-_btnMarkAllAsSeen->getX());
			_btnOk->setX(_btnMarkAllAsSeen->getX());
		}
		else
		{
			_btnMarkAllAsSeen->setVisible(true);
			_btnOk->setWidth(_btnMarkAllAsSeen->getWidth());
			_btnOk->setX(_btnMarkAllAsSeen->getX()+_btnMarkAllAsSeen->getWidth()+8);
		}
	}

}
