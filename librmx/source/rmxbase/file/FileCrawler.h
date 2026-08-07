/*
*	rmx Library
*	Copyright (C) 2008-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once


class API_EXPORT FileCrawler
{
public:
	typedef rmx::FileIO::FileEntry FileEntry;

	enum class SortMode
	{
		BY_FILENAME = 0,
		BY_EXTENSION,
		BY_TIME,
		BY_SIZE
	};

public:
	static std::vector<std::wstring> getSubdirectories(const std::wstring& parentDirectory);

public:
	FileCrawler();
	~FileCrawler();

	void clear();

	// Gather entries
	void addFiles(const WString& filemask, bool recursive = false);

	// Change order of entries
	void sort(SortMode mode);
	void invertOrder();

	// Access entries
	inline const std::vector<FileEntry>& getFileEntries() const  { return mFileEntries; }

	size_t size() const  { return mFileEntries.size(); }
	const FileEntry* getFileEntry(size_t index) const;
	inline const FileEntry* operator[](size_t index) const  { return getFileEntry(index); }

	// Load file
	bool loadFile(size_t num, std::vector<uint8>& buffer);

protected:
	void addFilesInternal(const WString& filemask, bool recursive);

protected:
	// List of entries
	std::vector<FileEntry> mFileEntries;
};
