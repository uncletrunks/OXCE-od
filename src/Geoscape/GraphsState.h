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
#ifndef OPENXCOM_GRAPHSSTATE_H
#define OPENXCOM_GRAPHSSTATE_H

#include "../Engine/State.h"

namespace OpenXcom
{

class Surface;
class InteractiveSurface;
class Text;
class TextButton;
class ToggleTextButton;
class TextList;
class Region;
struct GraphButInfo;

/**
 * Graphs screen for displaying graphs of various
 * monthly game data like activity and funding.
 */
class GraphsState : public State
{
private:
	InteractiveSurface *_bg;
	InteractiveSurface *_btnGeoscape;
	InteractiveSurface *_btnXcomCountry, *_btnUfoCountry;
	InteractiveSurface *_btnXcomRegion, *_btnUfoRegion;
	InteractiveSurface *_btnIncome, *_btnFinance;
	Text *_txtTitle, *_txtFactor;
	TextList *_txtMonths, *_txtYears;
	std::vector<Text *> _txtScale;
	std::vector<ToggleTextButton *> _btnRegions, _btnCountries, _btnFinances;
	std::vector<GraphButInfo *>  _regionToggles, _countryToggles;
	std::vector<bool> _financeToggles;
	ToggleTextButton *_btn100, *_btn200, *_btn400, *_btn800, *_btn1600;
	ToggleTextButton *_btnRegionTotal, *_btnCountryTotal;
	std::vector<Surface *> _alienRegionLines, _alienCountryLines;
	std::vector<Surface *> _xcomRegionLines, _xcomCountryLines;
	std::vector<Surface *> _financeLines, _incomeLines;
	bool _alien, _income, _country, _finance;
	bool _is100Pressed, _is200Pressed, _is400Pressed, _is800Pressed, _is1600Pressed;
	static const size_t GRAPH_MAX_BUTTONS=16;
	//will be only between 0 and size()
	size_t _butRegionsOffset, _butCountriesOffset;
	//scroll and repaint buttons functions
	void scrollButtons(std::vector<GraphButInfo *> &toggles, std::vector<ToggleTextButton *> &buttons, size_t &offset, int step);
	void updateButton(GraphButInfo *from,ToggleTextButton *to);
	/// Handler for clicking the 100 button.
	void btnScale100Click(Action *action);
	/// Handler for clicking the 200 button.
	void btnScale200Click(Action *action);
	/// Handler for clicking the 400 button.
	void btnScale400Click(Action *action);
	/// Handler for clicking the 800 button.
	void btnScale800Click(Action *action);
	/// Handler for clicking the 1600 button.
	void btnScale1600Click(Action *action);
public:
	/// Creates the Graphs state.
	GraphsState();
	/// Cleans up the Graphs state.
	~GraphsState();
	/// Handler for clicking the Geoscape icon.
	void btnGeoscapeClick(Action *action);
	/// Handler for clicking the ufo region icon.
	void btnUfoRegionClick(Action *action);
	/// Handler for clicking the ufo country icon.
	void btnUfoCountryClick(Action *action);
	/// Handler for clicking the xcom region icon.
	void btnXcomRegionClick(Action *action);
	/// Handler for clicking the xcom country icon.
	void btnXcomCountryClick(Action *action);
	/// Handler for clicking the income icon.
	void btnIncomeClick(Action *action);
	/// Handler for clicking the finance icon.
	void btnFinanceClick(Action *action);
	/// Handler for clicking on a region button.
	void btnRegionListClick(Action *action);
	/// Handler for clicking on a country button.
	void btnCountryListClick(Action *action);
	/// Handler for clicking  on a finances button.
	void btnFinanceListClick(Action *action);
	/// Mouse wheel handler for shifting up/down the buttons
	void shiftButtons(Action *action);
	/// Reset all the elements on screen.
	void resetScreen();
	/// Update the scale 
	void updateScale(double lowerLimit, double upperLimit);
	/// Decide which lines to draw
	void drawLines();
	/// Draw Region Lines.
	void drawRegionLines();
	/// Draw Country Lines.
	void drawCountryLines();
	/// Draw Finances Lines.
	void drawFinanceLines();
	/// Scroll button lists
};

}

#endif
