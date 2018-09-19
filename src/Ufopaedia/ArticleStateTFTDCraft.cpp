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

#include "../Mod/ArticleDefinition.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleCraft.h"
#include "ArticleStateTFTD.h"
#include "ArticleStateTFTDCraft.h"
#include "../Engine/Game.h"
#include "../Engine/Palette.h"
#include "../Engine/LocalizedText.h"
#include "../Interface/TextButton.h"
#include "../Interface/Text.h"

namespace OpenXcom
{

	ArticleStateTFTDCraft::ArticleStateTFTDCraft(ArticleDefinitionTFTD *defs) : ArticleStateTFTD(defs)
	{
		_btnInfo->setVisible(_game->getMod()->getShowPediaInfoButton());

		RuleCraft *craft = _game->getMod()->getCraft(defs->id, true);
		_txtStats = new Text(131, 56, 187, 116);
		add(_txtStats);

		_txtStats->setColor(_textColor);
		_txtStats->setSecondaryColor(_textColor2);

		std::wostringstream ss;
		ss << tr("STR_MAXIMUM_SPEED_UC").arg(Text::formatNumber(craft->getMaxSpeed())) << L'\n';
		ss << tr("STR_ACCELERATION").arg(craft->getAcceleration()) << L'\n';
		switch (_game->getMod()->getPediaReplaceCraftFuelWithRangeType())
		{
			int range;

			// Both max range alone and average range get rounded
			case 0:
			case 2:
				range = craft->calculateRange(_game->getMod()->getPediaReplaceCraftFuelWithRangeType());
				if (range == -1)
				{
					ss << tr("STR_MAXIMUM_RANGE").arg(tr("STR_INFINITE_RANGE")) << L'\n';
					break;
				}

				// Round the answer to
				if (range < 100)
				{
					// don't round if it's small!
				}
				else if (range < 1000)
				{
					// nearest 10 nautical miles
					range += 10 / 2;
					range -= range % 10;
				}
				else
				{
					// nearest 100 nautical miles
					range += 100 / 2;
					range -= range % 100;
				}

				ss << tr("STR_MAXIMUM_RANGE").arg(Text::formatNumber(range)) << L'\n';
				break;
			// Min-maxxers can fret over exact numbers
			case 1:
				if (craft->calculateRange(0) == -1)
				{
					ss << tr("STR_MAXIMUM_RANGE").arg(tr("STR_INFINITE_RANGE")) << L'\n';
					break;
				}

				ss << tr("STR_MINIMUM_RANGE").arg(Text::formatNumber(craft->calculateRange(1))) << L'\n';
				ss << tr("STR_MAXIMUM_RANGE").arg(Text::formatNumber(craft->calculateRange(0))) << L'\n';
				break;
			default :
				ss << tr("STR_FUEL_CAPACITY").arg(Text::formatNumber(craft->getMaxFuel())) << L'\n';
				break;
		}
		ss << tr("STR_WEAPON_PODS").arg(craft->getWeapons()) << L'\n';
		ss << tr("STR_DAMAGE_CAPACITY_UC").arg(Text::formatNumber(craft->getMaxDamage())) << L'\n';
		ss << tr("STR_CARGO_SPACE").arg(craft->getSoldiers()) << L'\n';
		ss << tr("STR_HWP_CAPACITY").arg(craft->getVehicles());
		_txtStats->setText(ss.str());

		centerAllSurfaces();
	}

	ArticleStateTFTDCraft::~ArticleStateTFTDCraft()
	{}

}
