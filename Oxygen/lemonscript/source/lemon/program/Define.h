/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/compiler/Token.h"
#include "lemon/utility/FlyweightString.h"


namespace lemon
{

	class API_EXPORT Define
	{
	friend class Module;

	public:
		inline FlyweightString getName() const  { return mName; }
		inline const DataTypeDefinition* getDataType() const  { return mDataType; }

		void invalidateResolvedIdentifiers();

	public:
		TokenList mContent;

	private:
		FlyweightString mName;
		const DataTypeDefinition* mDataType = nullptr;
	};

}
