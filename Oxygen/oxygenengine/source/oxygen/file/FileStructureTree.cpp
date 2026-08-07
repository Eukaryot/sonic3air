/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/file/FileStructureTree.h"


uint64 FileStructureTree::getLowercaseStringHash(const wchar_t* string, size_t length)
{
	// Get hash of the lowercase version of the input string, to allow for case insensitive comparisons
	// Note: This can produce different hashes on different platforms
	static std::vector<wchar_t> lowercaseString;
	lowercaseString.resize(std::max<size_t>(1, length));
	for (size_t k = 0; k < length; ++k)
	{
		wchar_t character = string[k];
		if (character >= 'A' && character <= 'Z')
			character += 32;
		lowercaseString[k] = character;
	}
	return rmx::getMurmur2_64((const uint8*)&lowercaseString[0], length * sizeof(wchar_t));
}

uint64 FileStructureTree::getLowercaseStringHash(const std::wstring& string)
{
	return getLowercaseStringHash(&string[0], string.length());
}


FileStructureTree::FileStructureTree()
{
	// Create the root node
	mNodes.emplace_back();
}

void FileStructureTree::clear()
{
	mNodes.resize(1);
}

void FileStructureTree::insertPath(const std::wstring& path, void* customData)
{
	// Split path into directory names and possibly a file name at the end
	int currentNodeIndex = 0;
	size_t pos = 0;
	while (pos < path.length())
	{
		const size_t startPos = pos;

		// Go on until the next slash
		while (pos < path.length() && (path[pos] != '/' && path[pos] != '\\'))
		{
			++pos;
		}

		// Ignore empty directory / file names (e.g. when there's two slashes in a row)
		if (pos == startPos)
		{
			++pos;
			continue;
		}

		const bool isFile = (pos == path.length());
		const uint64 hash = getLowercaseStringHash(&path[startPos], pos - startPos);

		// Is this file / directory name already known?
		bool isKnown = false;
		int* nodeIndexPtr = isFile ? &mNodes[currentNodeIndex].mChildFileIndex : &mNodes[currentNodeIndex].mChildDirectoryIndex;
		while (*nodeIndexPtr != -1)
		{
			Node& node = mNodes[*nodeIndexPtr];
			if (node.mHash == hash)
			{
				isKnown = true;
				break;
			}
			nodeIndexPtr = &node.mNextSiblingIndex;
		}

		if (!isKnown)
		{
			// Add as another sibling
			const int newNodeIndex = (int)mNodes.size();
			*nodeIndexPtr = newNodeIndex;
			currentNodeIndex = newNodeIndex;

			Node& newNode = vectorAdd(mNodes);			// Note that this can invalidate nodeIndexPtr
			newNode.mName = std::wstring(&path[startPos], pos - startPos);
			newNode.mHash = hash;
			newNode.mIsFile = isFile;
		}
		else
		{
			currentNodeIndex = *nodeIndexPtr;
		}

		if (isFile)
			break;

		// Skip the slash
		++pos;
	}

	// Set (or update) custom data for the found node
	mNodes[currentNodeIndex].mCustomData = customData;
}

void FileStructureTree::sortTreeNodes()
{
	std::vector<int> fileNodeIndices;
	fileNodeIndices.reserve(32);

	// Go through all directories
	std::vector<int> openDirectoryIndices;
	openDirectoryIndices.reserve(16);
	openDirectoryIndices.push_back(0);

	while (!openDirectoryIndices.empty())
	{
		Node& directoryNode = mNodes[openDirectoryIndices.back()];
		openDirectoryIndices.pop_back();

		// Collect directories
		int nodeIndex = directoryNode.mChildDirectoryIndex;
		while (nodeIndex != -1)
		{
			openDirectoryIndices.push_back(nodeIndex);
			nodeIndex = mNodes[nodeIndex].mNextSiblingIndex;
		}

		// Collect files
		nodeIndex = directoryNode.mChildFileIndex;
		if (nodeIndex != -1)
		{
			fileNodeIndices.clear();
			while (nodeIndex != -1)
			{
				fileNodeIndices.push_back(nodeIndex);
				nodeIndex = mNodes[nodeIndex].mNextSiblingIndex;
			}

			// Sort the file indices by file name
			std::sort(fileNodeIndices.begin(), fileNodeIndices.end(), [this](int a, int b) { return mNodes[a].mName < mNodes[b].mName; } );

			// Rebuild the linked list
			int* nodeIndexPtr = &directoryNode.mChildFileIndex;
			for (size_t k = 0; k < fileNodeIndices.size(); ++k)
			{
				const int fileNodeIndex = fileNodeIndices[k];
				*nodeIndexPtr = fileNodeIndex;		// Change previous index reference
				nodeIndexPtr = &mNodes[fileNodeIndex].mNextSiblingIndex;
			}
			*nodeIndexPtr = -1;		// Last file's index reference must be -1
		}
	}
}

bool FileStructureTree::pathExists(const std::wstring& path) const
{
	size_t fileNameStartPos;
	const int directoryNodeIndex = findDirectoryNodeIndexByPath(path, fileNameStartPos);
	if (directoryNodeIndex == -1)
		return false;

	if (fileNameStartPos >= path.length())
	{
		// Path ended with a slash, so it's a directory name and it does indeed exist
		return true;
	}
	else
	{
		// There's also a file name, check if the file exists in the directory
		const uint64 hash = getLowercaseStringHash(&path[fileNameStartPos], path.length() - fileNameStartPos);
		const int fileNodeIndex = findNodeByHashInLinkedList(hash, mNodes[directoryNodeIndex].mChildFileIndex);
		return (fileNodeIndex != -1);
	}
}

bool FileStructureTree::listFiles(std::vector<const Entry*>& outFiles, const std::wstring& directoryPath) const
{
	return listEntriesInternal(outFiles, directoryPath, true);
}

bool FileStructureTree::listFilesByMask(std::vector<const Entry*>& outFiles, const std::wstring& filemask, bool recursive) const
{
	outFiles.clear();
	size_t fileNameStartPos;
	const int directoryNodeIndex = findDirectoryNodeIndexByPath(filemask, fileNameStartPos);
	if (directoryNodeIndex == -1)
	{
		// The base directory was not found
		return false;
	}

	// Note: The following also works if the file mask is actually just a directory name, i.e. ending with a slash
	//        -> In that case, both prefix and suffix are empty, so everything will be listed
	std::wstring maskPrefix;
	std::wstring maskSuffix;
	{
		const std::wstring mask = filemask.substr(fileNameStartPos);
		const size_t wildcardPosition = mask.find(L'*');
		if (wildcardPosition != std::string::npos)
		{
			maskPrefix = mask.substr(0, wildcardPosition);
			maskSuffix = mask.substr(wildcardPosition + 1);
		}
		else
		{
			maskSuffix = mask;
		}
	}

	// Collect the file / directory nodes
	listFilesByMaskInternal(outFiles, directoryNodeIndex, maskPrefix, maskSuffix, recursive);
	return true;
}

bool FileStructureTree::listDirectories(std::vector<const Entry*>& outDirectories, const std::wstring& directoryPath) const
{
	return listEntriesInternal(outDirectories, directoryPath, false);
}

bool FileStructureTree::listDirectories(std::vector<std::wstring>& outDirectories, const std::wstring& directoryPath) const
{
	mTempBuffer.clear();
	if (!listEntriesInternal(mTempBuffer, directoryPath, false))
		return false;

	if (!mTempBuffer.empty())
	{
		outDirectories.reserve(outDirectories.size() + mTempBuffer.size());
		for (size_t k = 0; k < mTempBuffer.size(); ++k)
		{
			outDirectories.emplace_back(mTempBuffer[k]->mName);
		}
	}
	return true;
}

bool FileStructureTree::listEntriesInternal(std::vector<const Entry*>& outEntries, const std::wstring& directoryPath, bool listFiles) const
{
	size_t fileNameStartPos;
	int directoryNodeIndex = findDirectoryNodeIndexByPath(directoryPath, fileNameStartPos);
	if (directoryNodeIndex == -1)
	{
		// The base directory was not found
		return false;
	}

	// If there's a remaining part after the last slash, interpret it as a directory name
	if (fileNameStartPos < directoryPath.length())
	{
		const uint64 hash = getLowercaseStringHash(&directoryPath[fileNameStartPos], directoryPath.length() - fileNameStartPos);
		directoryNodeIndex = findNodeByHashInLinkedList(hash, mNodes[directoryNodeIndex].mChildDirectoryIndex);
		if (directoryNodeIndex == -1)
			return false;
	}

	// Collect the file / directory nodes
	int nodeIndex = listFiles ? mNodes[directoryNodeIndex].mChildFileIndex : mNodes[directoryNodeIndex].mChildDirectoryIndex;
	while (nodeIndex != -1)
	{
		const Node& node = mNodes[nodeIndex];
		outEntries.push_back(&node);
		nodeIndex = node.mNextSiblingIndex;
	}

	// Base directory found, and file entries were written - but it's possible there were no files inside at all
	return true;
}

void FileStructureTree::listFilesByMaskInternal(std::vector<const Entry*>& outFiles, int directoryNodeIndex, const std::wstring& maskPrefix, const std::wstring& maskSuffix, bool recursive) const
{
	int nodeIndex = mNodes[directoryNodeIndex].mChildFileIndex;
	while (nodeIndex != -1)
	{
		const Node& node = mNodes[nodeIndex];
		if (rmx::startsWith(node.mName, maskPrefix) && rmx::endsWith(node.mName, maskSuffix) && node.mName.length() >= maskPrefix.length() + maskSuffix.length())
		{
			outFiles.push_back(&node);
		}
		nodeIndex = node.mNextSiblingIndex;
	}

	if (recursive)
	{
		// Go through the subdirectories
		nodeIndex = mNodes[directoryNodeIndex].mChildDirectoryIndex;
		while (nodeIndex != -1)
		{
			const Node& node = mNodes[nodeIndex];
			listFilesByMaskInternal(outFiles, nodeIndex, maskPrefix, maskSuffix, recursive);
			nodeIndex = node.mNextSiblingIndex;
		}
	}
}

int FileStructureTree::findNodeByHashInLinkedList(uint64 hash, int firstSiblingNodeIndex) const
{
	int nodeIndex = firstSiblingNodeIndex;
	while (nodeIndex != -1)
	{
		const Node& node = mNodes[nodeIndex];
		if (node.mHash == hash)
			return nodeIndex;

		nodeIndex = node.mNextSiblingIndex;
	}

	// Not found
	return -1;
}

int FileStructureTree::findDirectoryNodeIndexByPath(const std::wstring& path, size_t& outFileNameStartPos) const
{
	// Split path into directory names and possibly a file name at the end
	int currentDirectoryNodeIndex = 0;
	size_t pos = 0;
	while (pos < path.length())
	{
		const size_t startPos = pos;

		// Go on until the next slash
		while (pos < path.length() && (path[pos] != '/' && path[pos] != '\\'))
		{
			++pos;
		}

		// Ignore empty directory / file names (e.g. when there's two slashes in a row)
		if (pos == startPos)
		{
			++pos;
			continue;
		}

		const bool isFile = (pos == path.length());
		if (isFile)
		{
			// Reached the file
			outFileNameStartPos = startPos;
			return currentDirectoryNodeIndex;
		}

		// Search for this sub-directory
		const uint64 hash = getLowercaseStringHash(&path[startPos], pos - startPos);
		const int directoryNodeIndex = findNodeByHashInLinkedList(hash, mNodes[currentDirectoryNodeIndex].mChildDirectoryIndex);
		if (directoryNodeIndex == -1)
		{
			// Sub-directory was not found, path search failed
			return -1;
		}

		// Set as new directory
		currentDirectoryNodeIndex = directoryNodeIndex;

		// Skip the slash
		++pos;
	}

	// There was no file name at the end
	outFileNameStartPos = path.length();
	return currentDirectoryNodeIndex;
}
