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

class Mod;
class BattleUnit;
class BattleItem;

class ModScript
{
	friend class Mod;

	ScriptGlobal* _shared;
	Mod* _mod;

	template<typename T>
	struct Warper : T
	{
		Warper(const std::string& name, Mod* mod, ScriptGlobal* shared) : T{ shared, name, mod }
		{
			shared->pushParser(this);
		}
	};

	ModScript(ScriptGlobal* shared, Mod* mod) : _shared{ shared }, _mod{ mod }
	{

	}

	using Output = ScriptOutputArgs<int&, int>;
public:

	const ScriptGlobal* getShared() const
	{
		return _shared;
	}

	struct RecolorUnitParser : ScriptParser<Output, const BattleUnit*, int, int, int, int>
	{
		RecolorUnitParser(ScriptGlobal* shared, const std::string& name, Mod* mod);
	};
	struct SelectUnitParser : ScriptParserEvents<Output, const BattleUnit*, int, int, int>
	{
		SelectUnitParser(ScriptGlobal* shared, const std::string& name, Mod* mod);
	};

	struct ReactionUnitParser : ScriptParserEvents<Output, const BattleUnit*, const BattleUnit*, const BattleItem*, int, const BattleUnit*, int>
	{
		ReactionUnitParser(ScriptGlobal* shared, const std::string& name, Mod* mod);
	};

	struct RecolorItemParser : ScriptParser<Output, const BattleItem*, int, int, int>
	{
		RecolorItemParser(ScriptGlobal* shared, const std::string& name, Mod* mod);
	};
	struct SelectItemParser : ScriptParserEvents<Output, const BattleItem*, int, int, int>
	{
		SelectItemParser(ScriptGlobal* shared, const std::string& name, Mod* mod);
	};

	Warper<RecolorUnitParser> recolorUnitSprite = { "recolorUnitSprite", _mod, _shared };
	Warper<SelectUnitParser> selectUnitSprite = { "selectUnitSprite", _mod, _shared };

	Warper<ReactionUnitParser> reactionWeaponAction = { "reactionWeaponAction", _mod, _shared };
	Warper<ReactionUnitParser> reactionUnitAction = { "reactionUnitAction", _mod, _shared };
	Warper<ReactionUnitParser> reactionUnitReaction = { "reactionUnitReaction", _mod, _shared };

	Warper<RecolorItemParser> recolorItemSprite = { "recolorItemSprite", _mod, _shared };
	Warper<SelectItemParser> selectItemSprite = { "selectItemSprite", _mod, _shared };
};

}

#endif
