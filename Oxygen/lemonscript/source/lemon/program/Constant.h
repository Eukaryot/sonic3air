/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/utility/AnyBaseValue.h"
#include "lemon/utility/FlyweightString.h"


namespace lemon
{
	struct DataTypeDefinition;

	class API_EXPORT Constant
	{
	friend class CompilerFrontend;
	friend class Module;
	friend class ModuleSerializer;

	public:
		inline FlyweightString getName() const  { return mName; }
		inline const DataTypeDefinition* getDataType() const  { return mDataType; }
		inline AnyBaseValue getValue() const  { return mValue; }

	private:
		FlyweightString mName;
		const DataTypeDefinition* mDataType = nullptr;
		AnyBaseValue mValue { 0 };
	};
}
