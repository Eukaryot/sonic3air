/*
*	rmx Library
*	Copyright (C) 2008-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once


template<class CLASS>
class GlobalObjectPtr
{
public:
	~GlobalObjectPtr()
	{
		clear();
	}

	inline bool hasInstance() const { return (nullptr != mInstance); }
	inline CLASS& instance() const  { return *mInstance; }

	void clear()
	{
		SAFE_DELETE(mInstance);
	}

	template<typename T>
	void create()
	{
		clear();
		mInstance = new T();
	}

	void createDefault()
	{
		create<CLASS>();
	}

	CLASS* operator->()  { return mInstance; }

private:
	CLASS* mInstance = nullptr;
};
