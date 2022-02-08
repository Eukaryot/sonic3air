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

	struct PreprocessorDefinition
	{
		FlyweightString mIdentifier;
		int64 mValue = 1;
	};

	class PreprocessorDefinitionMap
	{
	public:
		void clear()
		{
			mDefinitions.clear();
		}

		PreprocessorDefinition* getDefinition(uint64 hash)
		{
			const auto it = mDefinitions.find(hash);
			return (it == mDefinitions.end()) ? nullptr : &it->second;
		}

		const PreprocessorDefinition* getDefinition(uint64 hash) const
		{
			const auto it = mDefinitions.find(hash);
			return (it == mDefinitions.end()) ? nullptr : &it->second;
		}

		int64 getValue(uint64 hash) const
		{
			const PreprocessorDefinition* definition = getDefinition(hash);
			return (nullptr == definition) ? 0 : definition->mValue;
		}

		PreprocessorDefinition& setDefinition(FlyweightString name, int64 value = 1)
		{
			const auto it = mDefinitions.find(name.getHash());
			if (it == mDefinitions.end())
			{
				PreprocessorDefinition& newDefinition = mDefinitions[name.getHash()];
				newDefinition.mIdentifier = name;
				newDefinition.mValue = value;
				return newDefinition;
			}
			else
			{
				return it->second;
			}
		}

		inline PreprocessorDefinition* getDefinition(FlyweightString name)				{ return getDefinition(name.getHash()); }
		inline const PreprocessorDefinition* getDefinition(FlyweightString name) const	{ return getDefinition(name.getHash()); }
		inline int64 getValue(FlyweightString name) const								{ return getValue(name.getHash()); }

	private:
		std::map<uint64, PreprocessorDefinition> mDefinitions;
	};

}
