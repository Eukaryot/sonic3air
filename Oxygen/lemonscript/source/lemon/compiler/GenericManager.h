/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


namespace genericmanager
{

	template<class ELEMENT> class Manager;
	template<class ELEMENT> struct GetElementFactoryLookupType;


	// Managed element base class
	template<class ELEMENT>
	class Element
	{
	public:
		typedef uint32 Type;

	public:
		inline Type getType() const  { return mType; }

		template<typename T> T& as()  { return static_cast<T&>(*this); }
		template<typename T> const T& as() const  { return static_cast<const T&>(*this); }

		inline uint32 getReferenceCounter() const  { return mReferenceCounter; }

		void addReference()
		{
			++mReferenceCounter;
		}

		void removeReference()
		{
			if (mReferenceCounter >= 2)
			{
				--mReferenceCounter;
			}
			else
			{
				RMX_CHECK(mReferenceCounter == 1, "Invalid reference count of genericmanager::Element", RMX_REACT_THROW);
				mReferenceCounter = 0;
				Manager<ELEMENT>::destroy(static_cast<ELEMENT&>(*this));
			}
		}

	protected:
		inline explicit Element(Type type) : mType(type) {}
		inline virtual ~Element() {}

	private:
		const Type mType;
		uint32 mReferenceCounter = 0;
	};



	// Element factories
	namespace detail
	{
		template<class ELEMENT>
		class ElementFactoryBase
		{
		public:
			virtual ELEMENT& create() = 0;
			virtual void destroy(ELEMENT& element) = 0;
			virtual void shrinkPool() {}
		};


		template<class ELEMENT, typename T>
		class ElementFactory : public ElementFactoryBase<ELEMENT>
		{
		public:
			inline ELEMENT& create() override  { return mElementPool.createObject(); }
			inline void destroy(ELEMENT& element) override  { mElementPool.destroyObject(static_cast<T&>(element)); }
			inline void shrinkPool() override  { mElementPool.shrink(); }

		private:
			ObjectPool<T, 256> mElementPool;
		};


		template<class ELEMENT>
		class ElementFactoryMap
		{
		public:
			typedef uint32 Type;
			typedef ElementFactoryBase<ELEMENT> FactoryBase;

		public:
			template<typename T>
			FactoryBase& getOrCreateElementFactory()
			{
				const constexpr uint32 type = (uint32)T::TYPE;
				if constexpr (type < 0x80)
				{
					// Use std::vector
					if (nullptr != mClassFactoriesList[type])
					{
						return *mClassFactoriesList[type];
					}
					else
					{
						FactoryBase* factory = new ElementFactory<ELEMENT, T>();
						mClassFactoriesList[type] = factory;
						return *factory;
					}
				}
				else
				{
					// Use std::map
					const auto it = mClassFactoriesMap.find(type);
					if (it != mClassFactoriesMap.end())
					{
						return *it->second;
					}
					else
					{
						FactoryBase* factory = new ElementFactory<ELEMENT, T>();
						mClassFactoriesMap[type] = factory;
						return *factory;
					}
				}
			}

			FactoryBase& getElementFactory(Type type_)
			{
				const uint32 type = (uint32)type_;
				if (type < 0x80)
				{
					if (nullptr != mClassFactoriesList[type])
					{
						return *mClassFactoriesList[type];
					}
				}
				else
				{
					const auto it = mClassFactoriesMap.find(type);
					if (it != mClassFactoriesMap.end())
					{
						return *it->second;
					}
				}
				RMX_ERROR("Element class factory does not exist for type " << type, RMX_REACT_THROW);
				static FactoryBase* dummy = nullptr;
				return *dummy;
			}

			void shrinkAllPools()
			{
				for (size_t index = 0; index < 0x80; ++index)
				{
					if (nullptr != mClassFactoriesList[index])
						mClassFactoriesList[index]->shrinkPool();
				}
				for (const auto& pair : mClassFactoriesMap)
				{
					pair.second->shrinkPool();
				}
			}

		private:
			FactoryBase* mClassFactoriesList[0x80] = { nullptr };	// Used for types that are less than 0x80 as unsigned integer
			std::map<uint32, FactoryBase*> mClassFactoriesMap;		// Used for types that are 0x80 or higher as unsigned integer
		};
	}



	// Element manager base class
	template<class ELEMENT>
	class Manager
	{
	friend class Element<ELEMENT>;

	public:
		typedef uint32 Type;

	public:
		template<typename TYPE>
		static TYPE& create()
		{
			detail::ElementFactoryBase<ELEMENT>& factory = mFactoryMap.template getOrCreateElementFactory<TYPE>();
			return static_cast<TYPE&>(factory.create());
		}

		static void shrinkAllPools()
		{
			mFactoryMap.shrinkAllPools();
		}

	private:
		static void destroy(Element<ELEMENT>& element)
		{
			RMX_ASSERT(element.getReferenceCounter() == 0, "Element still has references");
			detail::ElementFactoryBase<ELEMENT>& factory = mFactoryMap.getElementFactory(element.getType());
			factory.destroy(static_cast<ELEMENT&>(element));
		}

	private:
		static inline detail::ElementFactoryMap<ELEMENT> mFactoryMap;
	};



	// Smart pointer class
	template<class ELEMENT, typename BASE = ELEMENT>
	class ElementPtr
	{
	public:
		inline ElementPtr() : mElement(nullptr)  {}
		inline explicit ElementPtr(ELEMENT& element) : mElement(nullptr)  { set(&element); }
		inline explicit ElementPtr(ELEMENT* element) : mElement(nullptr)  { set(element); }
		inline explicit ElementPtr(const ElementPtr& other) : mElement(nullptr) { set(other.mElement); }
		inline explicit ElementPtr(ElementPtr&& other) : mElement(other.mElement) { other.mElement = nullptr; }
		inline virtual ~ElementPtr() { clear(); }

		inline bool valid() const  { return (nullptr != mElement); }
		inline ELEMENT* get() const  { return mElement; }
		template<typename T> T* as() const  { return static_cast<T*>(mElement); }

		template<typename T> T& create()  { T& element = Manager<BASE>::template create<T>(); set(element); return element; }

		inline void clear()  { set(nullptr); }
		inline void set(ELEMENT& element)		{ set(&element); }
		inline void set(const ElementPtr& ptr)  { set(ptr.mElement); }

		void set(ELEMENT* element)
		{
			if (nullptr != mElement)
				mElement->removeReference();
			mElement = element;
			if (nullptr != mElement)
				mElement->addReference();
		}

		inline void operator=(ELEMENT& element)		 { set(element); }
		inline void operator=(ELEMENT* element)		 { set(element); }
		inline void operator=(const ElementPtr& ptr) { set(ptr); }

		inline ELEMENT& operator*()				  { return *mElement; }
		inline const ELEMENT& operator*() const	  { return *mElement; }

		inline ELEMENT* operator->()			  { return mElement; }
		inline const ELEMENT* operator->() const  { return mElement; }

	private:
		ELEMENT* mElement = nullptr;
	};



	// Smart list class
	template<class ELEMENT, int FIXEDSIZE>
	class ElementList
	{
	public:
		inline ElementList() :
			mElements(mBuffer)
		{}

		explicit ElementList(const ElementList& other)
		{
			const size_t size = other.mSize;
			reserve(size);
			mSize = size;
			for (size_t i = 0; i < size; ++i)
			{
				mElements[i] = other.mElements[i];
				//RMX_CHECK(nullptr != mElements[i], "Invalid entry in element list", RMX_REACT_THROW);
				mElements[i]->addReference();
			}
		}

		inline virtual ~ElementList()
		{
			clear();
			if (mElements != mBuffer)
				delete[] mElements;
		}

		inline void clear()
		{
			for (size_t i = 0; i < mSize; ++i)
			{
				mElements[i]->removeReference();
			}
			mSize = 0;
		}

		inline bool empty() const  { return mSize == 0; }
		inline size_t size() const { return mSize; }

		inline void reserve(size_t num)
		{
			if (num <= mReserved)
				return;

			ELEMENT** oldElements = mElements;
			while (mReserved < num)
				mReserved *= 2;
			mElements = new ELEMENT*[mReserved];
			for (size_t i = 0; i < mSize; ++i)
			{
				mElements[i] = oldElements[i];
			}
			if (oldElements != mBuffer)
				delete[] oldElements;
		}

		template<typename T> T& create()
		{
			T& element = Manager<ELEMENT>::template create<T>();
			add(element);
			return element;
		}

		template<typename T> T& createAt(size_t index)
		{
			T& element = Manager<ELEMENT>::template create<T>();
			insert(element, index);
			return element;
		}

		template<typename T> T& createFront()
		{
			return createAt<T>(0);
		}

		template<typename T> T& createBack()
		{
			return create<T>();
		}

		template<typename T> T& createReplaceAt(size_t index)
		{
			T& element = Manager<ELEMENT>::template create<T>();
			replace(element, index);
			return element;
		}

		void add(ELEMENT& element)
		{
			reserve(mSize + 1);
			mElements[mSize] = &element;
			element.addReference();
			++mSize;
		}

		void insert(ELEMENT& element, size_t beforeIndex)
		{
			if (beforeIndex >= mSize)
			{
				add(element);
			}
			else
			{
				reserve(mSize + 1);
				for (size_t i = mSize; i > beforeIndex; --i)
				{
					mElements[i] = mElements[i-1];
				}
				mElements[beforeIndex] = &element;
				element.addReference();
				++mSize;
			}
		}

		void replace(ELEMENT& element, size_t index)
		{
			if (index >= mSize)
			{
				add(element);
			}
			else
			{
				mElements[index]->removeReference();
				mElements[index] = &element;
				element.addReference();
			}
		}

		void erase(size_t index)
		{
			if (index >= mSize)
				return;

			//RMX_CHECK(nullptr != mElements[index], "Invalid entry in element list", RMX_REACT_THROW);
			mElements[index]->removeReference();
			for (size_t i = index; i < mSize-1; ++i)
			{
				mElements[i] = mElements[i+1];
			}
			--mSize;
		}

		void erase(size_t index, size_t count)
		{
			if (index + count > mSize)
			{
				if (index >= mSize)
					return;
				count = mSize - index;
			}
			const size_t limit = index + count;
			for (size_t i = index; i < limit; ++i)
			{
				//RMX_CHECK(nullptr != mElements[i], "Invalid entry in element list", RMX_REACT_THROW);
				mElements[i]->removeReference();
			}
			for (size_t i = index; i < mSize - count; ++i)
			{
				mElements[i] = mElements[i + count];
			}
			mSize -= count;
		}

		void erase(const std::vector<size_t>& indices)
		{
			if (indices.empty())
				return;

			size_t actuallyRemoved = 0;
			for (size_t index : indices)
			{
				RMX_ASSERT(index < mSize, "Invalid index " << index);
				if (nullptr != mElements[index])	// In case the index is twice in the list
				{
					mElements[index]->removeReference();
					mElements[index] = nullptr;
					++actuallyRemoved;
				}
			}

			size_t newIndex = 0;
			for (; newIndex < mSize; ++newIndex)
			{
				if (nullptr == mElements[newIndex])
					break;
			}

			// Fill the gaps
			for (size_t oldIndex = newIndex + 1; oldIndex < mSize; ++oldIndex)
			{
				if (nullptr != mElements[oldIndex])
				{
					mElements[newIndex] = mElements[oldIndex];
					++newIndex;
				}
			}
			mSize -= actuallyRemoved;
		}

		void moveFrom(ElementList& other, size_t startIndex, size_t count)
		{
			if (startIndex + count > other.mSize)
			{
				if (startIndex >= other.mSize)
					return;
				count = other.mSize - startIndex;
			}
			reserve(mSize + count);
			for (size_t i = 0; i < count; ++i)
			{
				mElements[mSize + i] = other.mElements[startIndex + i];
			}
			for (size_t i = startIndex; i < other.mSize - count; ++i)
			{
				other.mElements[i] = other.mElements[i + count];
			}
			mSize += count;
			other.mSize -= count;
		}

		void swapWith(ElementList& other)
		{
			if (mElements == mBuffer)
			{
				if (other.mElements == other.mBuffer)
				{
					for (size_t i = 0; i < std::max(mSize, other.mSize); ++i)
						std::swap(mElements[i], other.mElements[i]);
				}
				else
				{
					for (size_t i = 0; i < mSize; ++i)
						other.mBuffer[i] = mElements[i];
					mElements = other.mElements;
					other.mElements = other.mBuffer;
				}
			}
			else
			{
				if (other.mElements == other.mBuffer)
				{
					for (size_t i = 0; i < mSize; ++i)
						mBuffer[i] = other.mElements[i];
					other.mElements = mElements;
					mElements = mBuffer;
				}
				else
				{
					std::swap(mElements, other.mElements);
				}
			}
			std::swap(mSize, other.mSize);
			std::swap(mReserved, other.mReserved);
		}

		ELEMENT& front() const { return *mElements[0]; }
		ELEMENT& back() const  { return *mElements[mSize-1]; }
		ELEMENT& operator[](size_t index) const { return *mElements[index]; }

	private:
		size_t mSize = 0;
		size_t mReserved = FIXEDSIZE;
		ELEMENT* mBuffer[FIXEDSIZE] = { nullptr };
		ELEMENT** mElements = nullptr;
	};

}
