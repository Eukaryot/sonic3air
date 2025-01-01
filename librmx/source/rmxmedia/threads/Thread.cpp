/*
*	rmx Library
*	Copyright (C) 2008-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "rmxmedia.h"


namespace rmx
{

	void ThreadManager::registerThread(ThreadBase& thread)
	{
		mThreads.insert(&thread);
	}

	void ThreadManager::unregisterThread(ThreadBase& thread)
	{
		mThreads.erase(&thread);
	}


	ThreadBase::ThreadBase() :
		mName("rmx Thread")
	{
		mManager->registerThread(*this);
	}

	ThreadBase::ThreadBase(const std::string& name) :
		mName(name)
	{
		mManager->registerThread(*this);
	}

	ThreadBase::~ThreadBase()
	{
		if (mIsThreadRunning)
		{
			signalStopThread(true);
		}
		mManager->unregisterThread(*this);
	}

	int ThreadBase::runThreadStatic(void* data)
	{
		ThreadBase* thread = reinterpret_cast<ThreadBase*>(data);
		if (nullptr == thread)
			return 0;

		thread->runThreadInternal();
		return 1;
	}

	void ThreadBase::runThreadInternal()
	{
		mIsThreadRunning = true;
		mShouldBeRunning = true;
		threadFunc();
		mShouldBeRunning = false;
		mIsThreadRunning = false;
	}

	void ThreadBase::startThread()
	{
		if (!mIsThreadRunning)
		{
		#if !defined(PLATFORM_VITA)
			mSDLThread = SDL_CreateThread(ThreadBase::runThreadStatic, mName.c_str(), this);
		#else
			mSDLThread = SDL_CreateThreadWithStackSize(ThreadBase::runThreadStatic, mName.c_str(), 4 * 1024 * 1024, this);
		#endif
		}
	}

	void ThreadBase::signalStopThread(bool join)
	{
		if (mIsThreadRunning)
		{
			mShouldBeRunning = false;
			if (join)
				joinThread();
		}
	}

	void ThreadBase::joinThread()
	{
		if (mIsThreadRunning && nullptr != mSDLThread)
		{
			SDL_WaitThread(mSDLThread, nullptr);
		}
	}

}
