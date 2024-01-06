/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


class FileStructureTree
{
public:
	struct Entry
	{
		bool mIsFile = false;	// Set if this represents a file, otherwise it's a directory
		std::wstring mName;		// Name of the file or directory
		uint64 mHash = 0;		// String hash of the name
		void* mCustomData = nullptr;
	};

	// TODO: Move these somewhere else
	static uint64 getLowercaseStringHash(const wchar_t* string, size_t length);
	static uint64 getLowercaseStringHash(const std::wstring& string);

public:
	FileStructureTree();

	void clear();
	void insertPath(const std::wstring& path, void* customData);
	void sortTreeNodes();

	bool pathExists(const std::wstring& path) const;
	bool listFiles(std::vector<const Entry*>& outFiles, const std::wstring& directoryPath) const;
	bool listFilesByMask(std::vector<const Entry*>& outFiles, const std::wstring& filemask, bool recursive) const;
	bool listDirectories(std::vector<const Entry*>& outDirectories, const std::wstring& directoryPath) const;
	bool listDirectories(std::vector<std::wstring>& outDirectories, const std::wstring& directoryPath) const;

private:
	bool listEntriesInternal(std::vector<const Entry*>& outEntries, const std::wstring& directoryPath, bool listFiles) const;
	void listFilesByMaskInternal(std::vector<const Entry*>& outFiles, int directoryNodeIndex, const std::wstring& maskPrefix, const std::wstring& maskSuffix, bool recursive) const;

	int findNodeByHashInLinkedList(uint64 hash, int firstSiblingNodeIndex) const;
	int findDirectoryNodeIndexByPath(const std::wstring& path, size_t& outFileNameStartPos) const;

private:
	struct Node : public Entry
	{
		// Entry members + some internal stuff
		int mNextSiblingIndex = -1;			// Index of the next file / directory inside the same directory, forming a linked list; -1 if there's none
		int mChildDirectoryIndex = -1;		// Only if this is a directory: Index of first child directory, forming a linked list; -1 if there's none
		int mChildFileIndex = -1;			// Only if this is a directory: Index of first child file, forming a linked list; -1 if there's none
	};
	std::vector<Node> mNodes;

	mutable std::vector<const Entry*> mTempBuffer;		// For temporary internal use
};
