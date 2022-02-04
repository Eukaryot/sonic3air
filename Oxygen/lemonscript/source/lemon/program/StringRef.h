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

	class API_EXPORT StringLookup
	{
	public:
		inline size_t size() const  { return mStrings.size(); }

		inline void clear()  { mStrings.clear(); }
		inline const FlyweightString* getStringByHash(uint64 hash) const  { return mapFind(mStrings, hash); }
		inline void addString(FlyweightString str)  { mStrings[str.getHash()] = str; }

		void addFromList(const std::vector<FlyweightString>& list);

	private:
		std::unordered_map<uint64, FlyweightString> mStrings;
	};


	struct API_EXPORT StringRef : public FlyweightString
	{
		inline StringRef() : FlyweightString() {}
		inline explicit StringRef(uint64 hash) : FlyweightString(hash) {}
		inline explicit StringRef(FlyweightString str) : FlyweightString(str) {}
	};

}
