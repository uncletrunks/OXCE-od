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
#ifndef OPENXCOM_STORESSTATE_H
#define OPENXCOM_STORESSTATE_H

#include "../Engine/State.h"
#include <vector>

namespace OpenXcom
{

class Base;
class TextButton;
class ToggleTextButton;
class Window;
class Text;
class TextList;

struct StoredItem
{
	StoredItem(const std::string &_name, int _quantity, double _size, double _spaceUsed)
		: name(_name), quantity(_quantity), size(_size), spaceUsed(_spaceUsed)
	{
	}
	bool operator<(const StoredItem &rhs) const
	{
		return spaceUsed < rhs.spaceUsed;
	}
	std::string name;
	int quantity;
	double size;
	double spaceUsed;
};

/**
 * Stores window that displays all
 * the items currently stored in a base.
 */
class StoresState : public State
{
private:
	Base *_base;

	TextButton *_btnOk;
	ToggleTextButton *_btnSort;
	Window *_window;
	Text *_txtTitle, *_txtItem, *_txtQuantity, *_txtSize, *_txtSpaceUsed;
	TextList *_lstStores;
	std::vector<StoredItem> _sortedList;
	///initializes the display list
	void initList(bool sort);
public:
	/// Creates the Stores state.
	StoresState(Base *base);
	/// Cleans up the Stores state.
	~StoresState();
	/// Handler for clicking the OK button.
	void btnOkClick(Action *action);
	/// Handler for clicking the Sort button.
	void btnSortClick(Action *action);
};

}

#endif
