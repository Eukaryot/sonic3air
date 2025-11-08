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
	namespace detail
	{
		template<class ELEMENT> class ElementClassBase;
		template<class ELEMENT> class ElementClassCollection;
	}


	// Managed element base class
	template<class ELEMENT>
	class Element
	{
	public:
		typedef uint32 Type;

	public:
		inline Type getType() const  { return mType; }

		template<typename T> bool isA() const		{ return (getType() == T::TYPE); }

		template<typename T> T& as()				{ return *static_cast<T*>(this); }
		template<typename T> const T& as() const	{ return *static_cast<const T*>(this); }

		template<typename T> T* cast()				{ return isA<T>() ? static_cast<T*>(this) : nullptr; }
		template<typename T> const T* cast() const	{ return isA<T>() ? static_cast<const T*>(this) : nullptr; }

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
		friend class Manager<ELEMENT>;

		public:
			virtual ELEMENT& create() = 0;
			virtual void shrinkPool() {}

		protected:
			virtual void destroy(ELEMENT& element) = 0;
		};


		template<class ELEMENT, typename T>
		class ElementFactoryImpl : public ElementFactoryBase<ELEMENT>
		{
		protected:
			inline ELEMENT& create() override  { return mElementPool.createObject(); }
			inline void destroy(ELEMENT& element) override  { mElementPool.destroyObject(static_cast<T&>(element)); }
			inline void shrinkPool() override  { mElementPool.shrink(); }

		private:
			ObjectPool<T, 256> mElementPool;
		};


		template<class ELEMENT>
		class ElementClassBase
		{
		public:
			inline uint32 getType() const  { return mType; }
			inline ElementFactoryBase<ELEMENT>& getFactory() const  { return mFactory; }

		public:
			inline bool operator==(const ElementClassBase<ELEMENT>& other) const  { return this == &other; }

		private:
			const uint32 mType;
			ElementFactoryBase<ELEMENT>& mFactory;

		protected:
			inline ElementClassBase(uint32 type, ElementFactoryBase<ELEMENT>& factory) : mType(type), mFactory(factory)
			{
				ElementClassCollection<ELEMENT>::getInstance().registerClass(*this);
			}
		};


		template<class ELEMENT, class T, uint32 TYPE>
		class ElementClassImpl : public ElementClassBase<ELEMENT>
		{
		public:
			ElementFactoryImpl<ELEMENT, T> mFactoryImplementation;

			inline ElementClassImpl() : ElementClassBase<ELEMENT>(TYPE, mFactoryImplementation) {}
		};


		template<class ELEMENT>
		class ElementClassCollection
		{
		public:
			typedef uint32 Type;

		public:
			static inline ElementClassCollection<ELEMENT>& getInstance()
			{
				if (nullptr == mInstance)
					mInstance = new ElementClassCollection<ELEMENT>();
				return *mInstance;
			}

		public:
			void registerClass(ElementClassBase<ELEMENT>& newClass)
			{
				mClassList.push_back(&newClass);
				mClassMap[newClass.getType()] = &newClass;
			}

			void shrinkAllPools() const
			{
				for (ElementClassBase<ELEMENT>* elementClass : mClassList)
				{
					elementClass->getFactory().shrinkPool();
				}
			}

			ElementFactoryBase<ELEMENT>* getFactory(Type type) const
			{
				const auto it = mClassMap.find(type);
				return (it != mClassMap.end()) ? &(it->second)->getFactory() : nullptr;
			}

		private:
			static inline ElementClassCollection<ELEMENT>* mInstance = nullptr;

			std::vector<ElementClassBase<ELEMENT>*> mClassList;
			std::unordered_map<Type, ElementClassBase<ELEMENT>*> mClassMap;
		};
	}



	// Element manager base class
	template<class ELEMENT>
	class Manager
	{
	friend class Element<ELEMENT>;
	friend class detail::ElementClassBase<ELEMENT>;

	public:
		typedef uint32 Type;

	public:
		template<typename TYPE>
		static TYPE& create()
		{
			return static_cast<TYPE&>(TYPE::CLASS.getFactory().create());
		}

		static ELEMENT& create(Type type)
		{
			detail::ElementClassCollection<ELEMENT>& collection = detail::ElementClassCollection<ELEMENT>::getInstance();
			detail::ElementFactoryBase<ELEMENT>* factory = collection.getFactory(type);
			RMX_ASSERT(nullptr != factory, "Invalid type " << type);
			return factory->create();
		}

		static void shrinkAllPools()
		{
			detail::ElementClassCollection<ELEMENT>& collection = detail::ElementClassCollection<ELEMENT>::getInstance();
			collection.shrinkAllPools();
		}

	private:
		// Destroy is private as it should only be called by the reference counting mechanism
		static void destroy(Element<ELEMENT>& element)
		{
			RMX_ASSERT(element.getReferenceCounter() == 0, "Element still has references");
			detail::ElementClassCollection<ELEMENT>& collection = detail::ElementClassCollection<ELEMENT>::getInstance();
			detail::ElementFactoryBase<ELEMENT>* factory = collection.getFactory(element.getType());
			RMX_ASSERT(nullptr != factory, "Invalid factory");
			factory->destroy(static_cast<ELEMENT&>(element));
		}
	};



	// Smart pointer class
	template<class ELEMENT, typename BASE = ELEMENT>
	class ElementPtr
	{
	public:
		typedef uint32 Type;

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
		ELEMENT& create(Type type)		  { ELEMENT& element = Manager<BASE>::template create(type); set(element); return element; }

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

		void swapWith(ElementPtr& ptr)
		{
			std::swap(mElement, ptr.mElement);
		}

		inline void operator=(ELEMENT& element)		 { set(element); }
		inline void operator=(ELEMENT* element)		 { set(element); }
		inline void operator=(const ElementPtr& ptr) { set(ptr); }

		inline void operator=(ElementPtr&& ptr)
		{
			if (nullptr != mElement)
				mElement->removeReference();
			mElement = ptr.mElement;
			ptr.mElement = nullptr;
		}

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

		inline explicit ElementList(const ElementList& other)
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

		inline ElementList(ElementList&& other) :
			ElementList()
		{
			swapWith(other);
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

		void copyFrom(const ElementList& other)
		{
			copyFrom(other, 0, other.size());
		}

		void copyFrom(const ElementList& other, size_t startIndex, size_t count)
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
				mElements[mSize + i]->addReference();
			}
			for (size_t i = startIndex; i < other.mSize - count; ++i)
			{
				other.mElements[i] = other.mElements[i + count];
			}
			mSize += count;
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

		void operator=(const ElementList& other)
		{
			clear();
			copyFrom(other, 0, other.size());
		}

		void operator=(ElementList&& other)
		{
			clear();
			moveFrom(other, 0, other.size());
		}

	private:
		size_t mSize = 0;
		size_t mReserved = FIXEDSIZE;
		ELEMENT* mBuffer[FIXEDSIZE] = { nullptr };
		ELEMENT** mElements = nullptr;
	};

}


#define DEFINE_GENERIC_MANAGER_ELEMENT_TYPE(_element_, _base_, _class_, _type_) \
public: \
	static inline const uint32 TYPE = _type_; \
	static inline genericmanager::detail::ElementClassImpl<_element_, _class_, _type_> CLASS; \
	inline _class_() : _base_(TYPE) {} \
protected: \
	inline explicit _class_(uint32 TYPE) : _base_(TYPE) {}	// For sub-classes that want to inherit from this
