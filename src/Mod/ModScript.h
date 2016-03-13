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
#ifndef OPENXCOM_MODSCRIPT_H
#define OPENXCOM_MODSCRIPT_H

#include "../Engine/Script.h"

namespace OpenXcom
{

class Surface;
class SurfaceSet;
class Soldier;
class RuleCountry;
class RuleRegion;
class RuleBaseFacility;
class RuleCraft;
class RuleCraftWeapon;
class RuleItem;
class RuleDamageType;
class RuleUfo;
class RuleTerrain;
class MapDataSet;
class RuleSoldier;
class Unit;
class Armor;
class RuleInventory;
class RuleResearch;
class RuleManufacture;
class AlienRace;
class RuleAlienMission;
class Base;
class MapScript;
class RuleVideo;

class BattleUnit;
class BattleItem;

struct ModScript
{
	ModScript()
	{
		recolorUnitSprite.LogInfo();
	}
	struct RecolorUnitParser : ScriptParser<const BattleUnit*, int, int, int, int>
	{
		RecolorUnitParser();
	};
	struct SelectUnitParser : ScriptParser<const BattleUnit*, int, int, int>
	{
		SelectUnitParser();
	};

	struct RecolorItemParser : ScriptParser<const BattleItem*, int, int, int>
	{
		RecolorItemParser();
	};
	struct SelectItemParser : ScriptParser<const BattleItem*, int, int, int>
	{
		SelectItemParser();
	};

	RecolorUnitParser recolorUnitSprite;
	SelectUnitParser selectUnitSprite;

	RecolorItemParser recolorItemSprite;
	SelectItemParser selectItemSprite;
};

}

#endif
