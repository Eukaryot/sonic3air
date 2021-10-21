/*
*	rmx Library
*	Copyright (C) 2008-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*
*	Thread
*		Helper classes for multi-threading (using SDL).
*/

#pragma once


namespace rmx
{

	class ThreadBase;


	// Simple mutex wrapper
	class Mutex
	{
	public:
		inline Mutex()		 { mMutex = SDL_CreateMutex(); }
		inline ~Mutex()		 { SDL_DestroyMutex(mMutex); }

		inline void lock()	 { SDL_LockMutex(mMutex); }
		inline void unlock() { SDL_UnlockMutex(mMutex); }

	private:
		SDL_mutex* mMutex = nullptr;
	};


	// Thread manager
	class ThreadManager
	{
	public:
		void registerThread(ThreadBase& thread);
		void unregisterThread(ThreadBase& thread);

	private:
		std::set<ThreadBase*> mThreads;
	};


	// Base class for threads
	class ThreadBase
	{
	public:
		ThreadBase();
		ThreadBase(const std::string& name);
		~ThreadBase();

		void startThread();
		void signalStopThread(bool join = false);

		void joinThread();

		bool isThreadRunning() const  { return mIsThreadRunning; }

	protected:
		// This is the method to override
		virtual void threadFunc() = 0;

	private:
		static int runThreadStatic(void* data);
		void runThreadInternal();

	protected:
		bool mShouldBeRunning = false;		// If set to false, the thread should stop itself; this has to be implemented in the sub-class

	private:
		SDL_Thread* mSDLThread = nullptr;
		std::string mName;
		bool mIsThreadRunning = false;		// Set as long as the actual thread is running
		SinglePtr<ThreadManager> mManager;
	};

}
