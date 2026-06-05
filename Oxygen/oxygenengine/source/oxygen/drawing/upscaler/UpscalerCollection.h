/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/drawing/upscaler/UpscalerDefinition.h"


class UpscalerCollection : public SingleInstance<UpscalerCollection>
{
public:
	void loadUpscalers();

	inline const std::vector<UpscalerDefinition>& getUpscalers() const  { return mUpscalers; }

	int getUpscalerIndexByName(std::string_view name) const;
	int getUpscalerIndexByNameHash(uint64 hash) const;
	
	void setCurrentConfigUpscalerByIndex(int index);

	const UpscalerDefinition::Variant& getCurrentConfigVariant() const;
	void changeCurrentConfigVariant(int direction);

private:
	UpscalerDefinition& addUpscaler(std::string_view internalName, std::string_view displayName);

private:
	std::vector<UpscalerDefinition> mUpscalers;
};
