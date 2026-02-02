/*
*	rmx Library
*	Copyright (C) 2008-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once


namespace rmx
{

	class WeakPtrBase;

	class API_EXPORT WeakPtrTarget
	{
	friend class WeakPtrBase;

	protected:
		virtual ~WeakPtrTarget();

	private:
		WeakPtrBase* mWeakPtrList = nullptr;
	};


	class API_EXPORT WeakPtrBase
	{
	friend class WeakPtrTarget;

	public:
		inline bool isValid() const  { return nullptr != mTarget; }

		inline void clear()  { clearTarget(); }

	protected:
		WeakPtrBase()  {}	// Protected constructor so this base class can't be instanced
		WeakPtrBase(WeakPtrTarget* target)		{ setTarget(target); }
		WeakPtrBase(WeakPtrTarget& target)		{ setTarget(target); }
		WeakPtrBase(const WeakPtrBase&& ptr)	{ setTarget(ptr.mTarget); }
		WeakPtrBase(WeakPtrBase&& ptr)			{ setTarget(ptr.mTarget); ptr.clearTarget(); }
		virtual ~WeakPtrBase()  { clearTarget(); }

		void clearTarget();
		void setTarget(WeakPtrTarget& target);
		void setTarget(WeakPtrTarget* target);

		inline void operator=(const WeakPtrBase& ptr)  { setTarget(ptr.mTarget); }

	protected:
		WeakPtrTarget* mTarget = nullptr;
		WeakPtrBase* mPrev = nullptr;
		WeakPtrBase* mNext = nullptr;
	};

}



template<typename T>
class WeakPtr : public rmx::WeakPtrBase
{
public:
	WeakPtr()  {}
	WeakPtr(T* target)				{ setTarget(target); }
	WeakPtr(T& target)				{ setTarget(target); }
	WeakPtr(const WeakPtr<T>& ptr)	{ setTarget(ptr.mTarget); }
	WeakPtr(WeakPtr<T>&& ptr)		{ setTarget(ptr.mTarget); ptr.clearTarget(); }

	inline T* get() const  { return static_cast<T*>(mTarget); }

	inline void operator=(T& target)  { setTarget(target); }
	inline void operator=(T* target)  { setTarget(target); }

	inline operator T*() const    { return static_cast<T*>(mTarget); }

	inline T& operator*() const   { RMX_ASSERT(nullptr != mTarget, "Dereferencing invalid weak pointer"); return static_cast<T&>(*mTarget); }
	inline T* operator->() const  { return static_cast<T*>(mTarget); }

	inline void operator=(const WeakPtr<T>& ptr)   { setTarget(ptr.mTarget); }

	inline bool operator==(const WeakPtr<T>& ptr) const  { return mTarget == ptr.mTarget; }
	inline bool operator!=(const WeakPtr<T>& ptr) const  { return mTarget != ptr.mTarget; }
	inline bool operator==(const T* target) const  { return mTarget == target; }
	inline bool operator!=(const T* target) const  { return mTarget != target; }
	inline bool operator==(const T& target) const  { return mTarget == &target; }
	inline bool operator!=(const T& target) const  { return mTarget != &target; }
};

