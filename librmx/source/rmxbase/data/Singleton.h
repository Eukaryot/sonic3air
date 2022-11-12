/*
*	rmx Library
*	Copyright (C) 2008-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once


/* ----- Singleton ----- */

template<class CLASS> class Singleton
{
public:
	virtual ~Singleton()
	{
		SAFE_DELETE(mInstance);
	}

	static bool valid()
	{
		return (mInstance != nullptr);
	}

	static CLASS& instance()
	{
		if (nullptr == mInstance)
			mInstance = new CLASS();
		return *mInstance;
	}

private:
	static CLASS* mInstance;
};

template <typename CLASS> CLASS* Singleton<CLASS>::mInstance = nullptr;



/* ----- SingletonPtr ----- */

template<class CLASS> class SingletonPtr
{
public:
//	void create()		{ if (nullptr == mObject) mObject = new CLASS(); }
//	void destroy()		{ SAFE_DELETE(mObject); }

	bool valid()		{ return Singleton<CLASS>::valid(); }

	CLASS* ptr()		{ return &Singleton<CLASS>::instance(); }
	CLASS& ref()		{ return Singleton<CLASS>::instance(); }

	CLASS& operator*()	{ return ref(); }
	operator CLASS*()	{ return ptr(); }
	CLASS* operator->()	{ return ptr(); }
};
