/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


class PersistentData : public SingleInstance<PersistentData>
{
public:
	void clear();
	bool loadFromFile(const std::wstring& filename);
	bool saveToFile();

	const std::vector<uint8>& getData(uint64 keyHash) const;
	void setData(std::string_view key, const std::vector<uint8>& data);

private:
	bool serialize(VectorBinarySerializer& serializer);

private:
	std::wstring mFilename;
	struct Entry
	{
		std::string mKey;
		std::vector<uint8> mData;
	};
	std::map<uint64, Entry> mEntries;
};
