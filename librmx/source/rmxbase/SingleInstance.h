/*
*	rmx Library
*	Copyright (C) 2008-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once


template<class CLASS> class SingleInstance
{
public:
	static bool hasInstance()
	{
		return (mSingleInstance != nullptr);
	}

	static CLASS& instance()
	{
		return *mSingleInstance;
	}

protected:
	SingleInstance()
	{
		// TODO: Sanity check: (mSingleInstance == nullptr)
		mSingleInstance = static_cast<CLASS*>(this);
	}

	virtual ~SingleInstance()
	{
		// TODO: Sanity check: (mSingleInstance == this)
		mSingleInstance = nullptr;
	}

private:
	static CLASS* mSingleInstance;
};

template<typename CLASS> CLASS* SingleInstance<CLASS>::mSingleInstance = nullptr;
