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

	Mod* _mod;
	std::map<std::string, ScriptParserBase*> _names;

	template<typename T>
	struct Warper : T
	{
		Warper(const std::string& name, Mod* mod, std::map<std::string, ScriptParserBase*>& names) : T{ name, mod }
		{
			this->logScriptMetadata();
			names.insert(std::make_pair(this->getName(), this));
		}
	};

	ModScript(Mod* mod) : _mod{ mod }
	{

	}
public:


	struct RecolorUnitParser : ScriptParser<const BattleUnit*, int, int, int, int>
	{
		RecolorUnitParser(const std::string& name,  Mod* mod);
	};
	struct SelectUnitParser : ScriptParser<const BattleUnit*, int, int, int>
	{
		SelectUnitParser(const std::string& name,  Mod* mod);
	};

	struct ReactionUnitParser : ScriptParser<const BattleUnit*, const BattleUnit*, const BattleItem*, int, const BattleUnit*>
	{
		ReactionUnitParser(const std::string& name,  Mod* mod);
	};

	struct RecolorItemParser : ScriptParser<const BattleItem*, int, int, int>
	{
		RecolorItemParser(const std::string& name,  Mod* mod);
	};
	struct SelectItemParser : ScriptParser<const BattleItem*, int, int, int>
	{
		SelectItemParser(const std::string& name,  Mod* mod);
	};

	Warper<RecolorUnitParser> recolorUnitSprite = { "recolorUnitSprite", _mod, _names };
	Warper<SelectUnitParser> selectUnitSprite = { "selectUnitSprite", _mod, _names };

	Warper<ReactionUnitParser> reactionWeaponAction = { "reactionWeaponAction", _mod, _names };
	Warper<ReactionUnitParser> reactionUnitAction = { "reactionUnitAction", _mod, _names };
	Warper<ReactionUnitParser> reactionUnitReaction = { "reactionUnitReaction", _mod, _names };

	Warper<RecolorItemParser> recolorItemSprite = { "recolorItemSprite", _mod, _names };
	Warper<SelectItemParser> selectItemSprite = { "selectItemSprite", _mod, _names };
};

}

#endif
