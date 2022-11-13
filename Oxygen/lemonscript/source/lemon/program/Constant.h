/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/utility/FlyweightString.h"


namespace lemon
{
	struct DataTypeDefinition;

	class API_EXPORT Constant
	{
	friend class CompilerFrontend;
	friend class Module;

	public:
		inline FlyweightString getName() const  { return mName; }
		inline const DataTypeDefinition* getDataType() const  { return mDataType; }
		inline uint64 getValue() const  { return mValue; }

	private:
		FlyweightString mName;
		const DataTypeDefinition* mDataType = nullptr;
		uint64 mValue = 0;
	};
}
