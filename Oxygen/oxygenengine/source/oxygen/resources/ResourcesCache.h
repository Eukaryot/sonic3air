/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once


class ResourcesCache : public SingleInstance<ResourcesCache>
{
public:
	struct RawData
	{
		std::vector<uint8> mContent;
		uint32 mRomInjectAddress = 0xffffffff;
		bool mIsModded = false;
	};

	struct Palette
	{
		std::vector<Color> mColors;
		bool mIsModded = false;
	};

	struct CachedFont
	{
		std::string mKeyString;
		uint64 mKeyHash = 0;
		Font mFont;
	};

public:
	bool loadRom();
	bool loadRomFromFile(const std::wstring& filename);
	bool loadRomFromMemory(const std::vector<uint8>& content);

	void loadAllResources();

	inline const std::vector<uint8>& getUnmodifiedRom() const  { return mRom; }
	const std::vector<const RawData*>& getRawData(uint64 key) const;
	const Palette* getPalette(uint64 key, uint8 line) const;

	Font* getFontByKey(const std::string& keyString, uint64 keyHash);
	Font* registerFontSource(const std::string& filename);

	void applyRomInjections(uint8* rom, uint32 romSize) const;

private:
	bool loadRomFile(const std::wstring& filename);
	bool checkRomContent();
	void saveRomToAppData();

	void loadRawData(const std::wstring& path, bool isModded);
	void loadPalettes(const std::wstring& path, bool isModded);

private:
	std::vector<uint8> mRom;	// This is the original, unmodified ROM (i.e. without any raw data injections or ROM writes)

	std::map<uint64, std::vector<const RawData*>> mRawDataMap;
	std::vector<const RawData*> mRomInjections;
	ObjectPool<RawData> mRawDataPool;

	std::map<uint64, Palette> mPalettes;

	std::map<uint64, CachedFont> mCachedFonts;	// Using "mKeyHash" as map key
};
