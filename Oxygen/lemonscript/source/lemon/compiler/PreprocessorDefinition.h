/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


namespace lemon
{

	struct PreprocessorDefinition
	{
		std::string mIdentifierName;
		uint64 mIdentifierHash = 0;
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

		PreprocessorDefinition& setDefinition(uint64 hash, const std::string& name, int64 value = 1)
		{
			const auto it = mDefinitions.find(hash);
			if (it == mDefinitions.end())
			{
				PreprocessorDefinition& newDefinition = mDefinitions[hash];
				newDefinition.mIdentifierHash = hash;
				newDefinition.mIdentifierName = name;
				newDefinition.mValue = value;
				return newDefinition;
			}
			else
			{
				return it->second;
			}
		}

		inline PreprocessorDefinition* getDefinition(const std::string& name)					{ return getDefinition(rmx::getMurmur2_64(name)); }
		inline const PreprocessorDefinition* getDefinition(const std::string& name) const		{ return getDefinition(rmx::getMurmur2_64(name)); }
		inline int64 getValue(const std::string& name) const									{ return getValue(rmx::getMurmur2_64(name)); }
		inline PreprocessorDefinition& setDefinition(const std::string& name, int64 value = 1)	{ return setDefinition(rmx::getMurmur2_64(name), name, value); }

	private:
		std::map<uint64, PreprocessorDefinition> mDefinitions;
	};

}
