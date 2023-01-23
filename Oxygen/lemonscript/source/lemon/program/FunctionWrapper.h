/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/program/Function.h"
#include "lemon/runtime/Runtime.h"
#include "lemon/utility/AnyBaseValue.h"


namespace lemon
{

	struct AnyTypeWrapper
	{
		const DataTypeDefinition* mType = nullptr;
		AnyBaseValue mValue;
	};


	namespace traits
	{
		template<typename T> const DataTypeDefinition* getDataType()  { return T::UNKNOWN_TYPE; }
		template<> const DataTypeDefinition* getDataType<void>();
		template<> const DataTypeDefinition* getDataType<bool>();
		template<> const DataTypeDefinition* getDataType<int8>();
		template<> const DataTypeDefinition* getDataType<uint8>();
		template<> const DataTypeDefinition* getDataType<int16>();
		template<> const DataTypeDefinition* getDataType<uint16>();
		template<> const DataTypeDefinition* getDataType<int32>();
		template<> const DataTypeDefinition* getDataType<uint32>();
		template<> const DataTypeDefinition* getDataType<int64>();
		template<> const DataTypeDefinition* getDataType<uint64>();
		template<> const DataTypeDefinition* getDataType<float>();
		template<> const DataTypeDefinition* getDataType<double>();
		template<> const DataTypeDefinition* getDataType<StringRef>();
		template<> const DataTypeDefinition* getDataType<AnyTypeWrapper>();
	}


	namespace internal
	{

		// Stack interactions templates for base types - these functions are used as a basis for return type and parameter type handling

		template<typename R>
		void pushStackGeneric(R value, const NativeFunction::Context context)
		{
			context.mControlFlow.pushValueStack<R>(value);
		};

		template<typename T>
		T popStackGeneric(const NativeFunction::Context context)
		{
			return static_cast<T>(context.mControlFlow.popValueStack<T>());
		}



		// Template specializations for StringRef, representing the "string" type in script

		template<>
		void pushStackGeneric<StringRef>(StringRef value, const NativeFunction::Context context);

		template<>
		StringRef popStackGeneric(const NativeFunction::Context context);



		// Template specializations for AnyTypeWrapper, representing the "any" type in script

		template<>
		void pushStackGeneric<AnyTypeWrapper>(AnyTypeWrapper value, const NativeFunction::Context context);

		template<>
		AnyTypeWrapper popStackGeneric(const NativeFunction::Context context);



		// Return type handlers for functions

		template<typename R>
		struct ReturnTypeHandler0
		{
			void call(R(*pointer)(), const NativeFunction::Context context)
			{
				pushStackGeneric(pointer(), context);
			}
		};

		template<>
		struct ReturnTypeHandler0<void>
		{
			void call(void(*pointer)(), const NativeFunction::Context context)
			{
				pointer();
			}
		};

		template<typename R, typename A>
		struct ReturnTypeHandler1
		{
			void call(R(*pointer)(A), const NativeFunction::Context context, A a)
			{
				pushStackGeneric(pointer(a), context);
			}
		};

		template<typename A>
		struct ReturnTypeHandler1<void, A>
		{
			void call(void(*pointer)(A), const NativeFunction::Context context, A a)
			{
				pointer(a);
			}
		};

		template<typename R, typename A, typename B>
		struct ReturnTypeHandler2
		{
			void call(R(*pointer)(A, B), const NativeFunction::Context context, A a, B b)
			{
				pushStackGeneric(pointer(a, b), context);
			}
		};

		template<typename A, typename B>
		struct ReturnTypeHandler2<void, A, B>
		{
			void call(void(*pointer)(A, B), const NativeFunction::Context context, A a, B b)
			{
				pointer(a, b);
			}
		};

		template<typename R, typename A, typename B, typename C>
		struct ReturnTypeHandler3
		{
			void call(R(*pointer)(A, B, C), const NativeFunction::Context context, A a, B b, C c)
			{
				pushStackGeneric(pointer(a, b, c), context);
			}
		};

		template<typename A, typename B, typename C>
		struct ReturnTypeHandler3<void, A, B, C>
		{
			void call(void(*pointer)(A, B, C), const NativeFunction::Context context, A a, B b, C c)
			{
				pointer(a, b, c);
			}
		};

		template<typename R, typename A, typename B, typename C, typename D>
		struct ReturnTypeHandler4
		{
			void call(R(*pointer)(A, B, C, D), const NativeFunction::Context context, A a, B b, C c, D d)
			{
				pushStackGeneric(pointer(a, b, c, d), context);
			}
		};

		template<typename A, typename B, typename C, typename D>
		struct ReturnTypeHandler4<void, A, B, C, D>
		{
			void call(void(*pointer)(A, B, C, D), const NativeFunction::Context context, A a, B b, C c, D d)
			{
				pointer(a, b, c, d);
			}
		};

		template<typename R, typename A, typename B, typename C, typename D, typename E>
		struct ReturnTypeHandler5
		{
			void call(R(*pointer)(A, B, C, D, E), const NativeFunction::Context context, A a, B b, C c, D d, E e)
			{
				pushStackGeneric(pointer(a, b, c, d, e), context);
			}
		};

		template<typename A, typename B, typename C, typename D, typename E>
		struct ReturnTypeHandler5<void, A, B, C, D, E>
		{
			void call(void(*pointer)(A, B, C, D, E), const NativeFunction::Context context, A a, B b, C c, D d, E e)
			{
				pointer(a, b, c, d, e);
			}
		};

		template<typename R, typename A, typename B, typename C, typename D, typename E, typename F>
		struct ReturnTypeHandler6
		{
			void call(R(*pointer)(A, B, C, D, E, F), const NativeFunction::Context context, A a, B b, C c, D d, E e, F f)
			{
				pushStackGeneric(pointer(a, b, c, d, e, f), context);
			}
		};

		template<typename A, typename B, typename C, typename D, typename E, typename F>
		struct ReturnTypeHandler6<void, A, B, C, D, E, F>
		{
			void call(void(*pointer)(A, B, C, D, E, F), const NativeFunction::Context context, A a, B b, C c, D d, E e, F f)
			{
				pointer(a, b, c, d, e, f);
			}
		};

		template<typename R, typename A, typename B, typename C, typename D, typename E, typename F, typename G>
		struct ReturnTypeHandler7
		{
			void call(R(*pointer)(A, B, C, D, E, F, G), const NativeFunction::Context context, A a, B b, C c, D d, E e, F f, G g)
			{
				pushStackGeneric(pointer(a, b, c, d, e, f, g), context);
			}
		};

		template<typename A, typename B, typename C, typename D, typename E, typename F, typename G>
		struct ReturnTypeHandler7<void, A, B, C, D, E, F, G>
		{
			void call(void(*pointer)(A, B, C, D, E, F, G), const NativeFunction::Context context, A a, B b, C c, D d, E e, F f, G g)
			{
				pointer(a, b, c, d, e, f, g);
			}
		};

		template<typename R, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H>
		struct ReturnTypeHandler8
		{
			void call(R(*pointer)(A, B, C, D, E, F, G, H), const NativeFunction::Context context, A a, B b, C c, D d, E e, F f, G g, H h)
			{
				pushStackGeneric(pointer(a, b, c, d, e, f, g, h), context);
			}
		};

		template<typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H>
		struct ReturnTypeHandler8<void, A, B, C, D, E, F, G, H>
		{
			void call(void(*pointer)(A, B, C, D, E, F, G, H), const NativeFunction::Context context, A a, B b, C c, D d, E e, F f, G g, H h)
			{
				pointer(a, b, c, d, e, f, g, h);
			}
		};

		template<typename R, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I>
		struct ReturnTypeHandler9
		{
			void call(R(*pointer)(A, B, C, D, E, F, G, H, I), const NativeFunction::Context context, A a, B b, C c, D d, E e, F f, G g, H h, I i)
			{
				pushStackGeneric(pointer(a, b, c, d, e, f, g, h, i), context);
			}
		};

		template<typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I>
		struct ReturnTypeHandler9<void, A, B, C, D, E, F, G, H, I>
		{
			void call(void(*pointer)(A, B, C, D, E, F, G, H, I), const NativeFunction::Context context, A a, B b, C c, D d, E e, F f, G g, H h, I i)
			{
				pointer(a, b, c, d, e, f, g, h, i);
			}
		};

		template<typename R, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I, typename J>
		struct ReturnTypeHandler10
		{
			void call(R(*pointer)(A, B, C, D, E, F, G, H, I, J), const NativeFunction::Context context, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j)
			{
				pushStackGeneric(pointer(a, b, c, d, e, f, g, h, i, j), context);
			}
		};

		template<typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I, typename J>
		struct ReturnTypeHandler10<void, A, B, C, D, E, F, G, H, I, J>
		{
			void call(void(*pointer)(A, B, C, D, E, F, G, H, I, J), const NativeFunction::Context context, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j)
			{
				pointer(a, b, c, d, e, f, g, h, i, j);
			}
		};

		template<typename R, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I, typename J, typename K>
		struct ReturnTypeHandler11
		{
			void call(R(*pointer)(A, B, C, D, E, F, G, H, I, J, K), const NativeFunction::Context context, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k)
			{
				pushStackGeneric(pointer(a, b, c, d, e, f, g, h, i, j, k), context);
			}
		};

		template<typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I, typename J, typename K>
		struct ReturnTypeHandler11<void, A, B, C, D, E, F, G, H, I, J, K>
		{
			void call(void(*pointer)(A, B, C, D, E, F, G, H, I, J, K), const NativeFunction::Context context, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k)
			{
				pointer(a, b, c, d, e, f, g, h, i, j, k);
			}
		};



		// Return type handlers for class methods

		template<typename CLASS, typename R>
		struct ReturnTypeHandlerM0
		{
			void call(CLASS& object, R(CLASS::*pointer)(), const NativeFunction::Context context)
			{
				pushStackGeneric((object.*pointer)(), context);
			}
		};

		template<typename CLASS>
		struct ReturnTypeHandlerM0<CLASS, void>
		{
			void call(CLASS& object, void(CLASS::*pointer)(), const NativeFunction::Context context)
			{
				(object.*pointer)();
			}
		};

		template<typename CLASS, typename R, typename A>
		struct ReturnTypeHandlerM1
		{
			void call(CLASS& object, R(CLASS::*pointer)(A), const NativeFunction::Context context, A a)
			{
				pushStackGeneric((object.*pointer)(a), context);
			}
		};

		template<typename CLASS, typename A>
		struct ReturnTypeHandlerM1<CLASS, void, A>
		{
			void call(CLASS& object, void(CLASS::*pointer)(A), const NativeFunction::Context context, A a)
			{
				(object.*pointer)(a);
			}
		};

		template<typename CLASS, typename R, typename A, typename B>
		struct ReturnTypeHandlerM2
		{
			void call(CLASS& object, R(CLASS::*pointer)(A, B), const NativeFunction::Context context, A a, B b)
			{
				pushStackGeneric((object.*pointer)(a, b), context);
			}
		};

		template<typename CLASS, typename A, typename B>
		struct ReturnTypeHandlerM2<CLASS, void, A, B>
		{
			void call(CLASS& object, void(CLASS::*pointer)(A, B), const NativeFunction::Context context, A a, B b)
			{
				(object.*pointer)(a, b);
			}
		};

		template<typename CLASS, typename R, typename A, typename B, typename C>
		struct ReturnTypeHandlerM3
		{
			void call(CLASS& object, R(CLASS::*pointer)(A, B, C), const NativeFunction::Context context, A a, B b, C c)
			{
				pushStackGeneric((object.*pointer)(a, b, c), context);
			}
		};

		template<typename CLASS, typename A, typename B, typename C>
		struct ReturnTypeHandlerM3<CLASS, void, A, B, C>
		{
			void call(CLASS& object, void(CLASS::*pointer)(A, B, C), const NativeFunction::Context context, A a, B b, C c)
			{
				(object.*pointer)(a, b, c);
			}
		};

		template<typename CLASS, typename R, typename A, typename B, typename C, typename D>
		struct ReturnTypeHandlerM4
		{
			void call(CLASS& object, R(CLASS::*pointer)(A, B, C, D), const NativeFunction::Context context, A a, B b, C c, D d)
			{
				pushStackGeneric((object.*pointer)(a, b, c, d), context);
			}
		};

		template<typename CLASS, typename A, typename B, typename C, typename D>
		struct ReturnTypeHandlerM4<CLASS, void, A, B, C, D>
		{
			void call(CLASS& object, void(CLASS::*pointer)(A, B, C, D), const NativeFunction::Context context, A a, B b, C c, D d)
			{
				(object.*pointer)(a, b, c, d);
			}
		};

		template<typename CLASS, typename R, typename A, typename B, typename C, typename D, typename E>
		struct ReturnTypeHandlerM5
		{
			void call(CLASS& object, R(CLASS::*pointer)(A, B, C, D, E), const NativeFunction::Context context, A a, B b, C c, D d, E e)
			{
				pushStackGeneric((object.*pointer)(a, b, c, d, e), context);
			}
		};

		template<typename CLASS, typename A, typename B, typename C, typename D, typename E>
		struct ReturnTypeHandlerM5<CLASS, void, A, B, C, D, E>
		{
			void call(CLASS& object, void(CLASS::*pointer)(A, B, C, D, E), const NativeFunction::Context context, A a, B b, C c, D d, E e)
			{
				(object.*pointer)(a, b, c, d, e);
			}
		};

		template<typename CLASS, typename R, typename A, typename B, typename C, typename D, typename E, typename F>
		struct ReturnTypeHandlerM6
		{
			void call(CLASS& object, R(CLASS::*pointer)(A, B, C, D, E, F), const NativeFunction::Context context, A a, B b, C c, D d, E e, F f)
			{
				pushStackGeneric((object.*pointer)(a, b, c, d, e, f), context);
			}
		};

		template<typename CLASS, typename A, typename B, typename C, typename D, typename E, typename F>
		struct ReturnTypeHandlerM6<CLASS, void, A, B, C, D, E, F>
		{
			void call(CLASS& object, void(CLASS::*pointer)(A, B, C, D, E, F), const NativeFunction::Context context, A a, B b, C c, D d, E e, F f)
			{
				(object.*pointer)(a, b, c, d, e, f);
			}
		};

		template<typename CLASS, typename R, typename A, typename B, typename C, typename D, typename E, typename F, typename G>
		struct ReturnTypeHandlerM7
		{
			void call(CLASS& object, R(CLASS::*pointer)(A, B, C, D, E, F, G), const NativeFunction::Context context, A a, B b, C c, D d, E e, F f, G g)
			{
				pushStackGeneric((object.*pointer)(a, b, c, d, e, f, g), context);
			}
		};

		template<typename CLASS, typename A, typename B, typename C, typename D, typename E, typename F, typename G>
		struct ReturnTypeHandlerM7<CLASS, void, A, B, C, D, E, F, G>
		{
			void call(CLASS& object, void(CLASS::*pointer)(A, B, C, D, E, F, G), const NativeFunction::Context context, A a, B b, C c, D d, E e, F f, G g)
			{
				(object.*pointer)(a, b, c, d, e, f, g);
			}
		};

		template<typename CLASS, typename R, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H>
		struct ReturnTypeHandlerM8
		{
			void call(CLASS& object, R(CLASS::*pointer)(A, B, C, D, E, F, G, H), const NativeFunction::Context context, A a, B b, C c, D d, E e, F f, G g, H h)
			{
				pushStackGeneric((object.*pointer)(a, b, c, d, e, f, g, h), context);
			}
		};

		template<typename CLASS, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H>
		struct ReturnTypeHandlerM8<CLASS, void, A, B, C, D, E, F, G, H>
		{
			void call(CLASS& object, void(CLASS::*pointer)(A, B, C, D, E, F, G, H), const NativeFunction::Context context, A a, B b, C c, D d, E e, F f, G g, H h)
			{
				(object.*pointer)(a, b, c, d, e, f, g, h);
			}
		};

		template<typename CLASS, typename R, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I>
		struct ReturnTypeHandlerM9
		{
			void call(CLASS& object, R(CLASS::*pointer)(A, B, C, D, E, F, G, H, I), const NativeFunction::Context context, A a, B b, C c, D d, E e, F f, G g, H h, I i)
			{
				pushStackGeneric((object.*pointer)(a, b, c, d, e, f, g, h, i), context);
			}
		};

		template<typename CLASS, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I>
		struct ReturnTypeHandlerM9<CLASS, void, A, B, C, D, E, F, G, H, I>
		{
			void call(CLASS& object, void(CLASS::*pointer)(A, B, C, D, E, F, G, H, I), const NativeFunction::Context context, A a, B b, C c, D d, E e, F f, G g, H h, I i)
			{
				(object.*pointer)(a, b, c, d, e, f, g, h, i);
			}
		};

		template<typename CLASS, typename R, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I, typename J>
		struct ReturnTypeHandlerM10
		{
			void call(CLASS& object, R(CLASS::*pointer)(A, B, C, D, E, F, G, H, I, J), const NativeFunction::Context context, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j)
			{
				pushStackGeneric((object.*pointer)(a, b, c, d, e, f, g, h, i, j), context);
			}
		};

		template<typename CLASS, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I, typename J>
		struct ReturnTypeHandlerM10<CLASS, void, A, B, C, D, E, F, G, H, I, J>
		{
			void call(CLASS& object, void(CLASS::*pointer)(A, B, C, D, E, F, G, H, I, J), const NativeFunction::Context context, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j)
			{
				(object.*pointer)(a, b, c, d, e, f, g, h, i, j);
			}
		};

		template<typename CLASS, typename R, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I, typename J, typename K>
		struct ReturnTypeHandlerM11
		{
			void call(CLASS& object, R(CLASS::*pointer)(A, B, C, D, E, F, G, H, I, J, K), const NativeFunction::Context context, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k)
			{
				pushStackGeneric((object.*pointer)(a, b, c, d, e, f, g, h, i, j, k), context);
			}
		};

		template<typename CLASS, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I, typename J, typename K>
		struct ReturnTypeHandlerM11<CLASS, void, A, B, C, D, E, F, G, H, I, J, K>
		{
			void call(CLASS& object, void(CLASS::*pointer)(A, B, C, D, E, F, G, H, I, J, K), const NativeFunction::Context context, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k)
			{
				(object.*pointer)(a, b, c, d, e, f, g, h, i, j, k);
			}
		};



		// Function wrappers

		template<typename R>
		class FunctionWrapperBase : public NativeFunction::FunctionWrapper
		{
		protected:
			virtual const DataTypeDefinition* getReturnType() const override
			{
				return traits::getDataType<R>();
			}

			template<typename T>
			static inline T popStack(const NativeFunction::Context context)
			{
				return popStackGeneric<T>(context);
			}
		};


		template<typename R>
		class FunctionWrapper0 : public FunctionWrapperBase<R>
		{
		public:
			typedef R(*Pointer)();

		public:
			inline FunctionWrapper0(Pointer pointer) : mPointer(pointer) {}

		protected:
			virtual void execute(const NativeFunction::Context context) const override
			{
				// Call the actual function
				ReturnTypeHandler0<R>().call(mPointer, context);
			}

			virtual std::vector<const DataTypeDefinition*> getParameterTypes() const override
			{
				return {};
			}

		protected:
			Pointer mPointer;
		};


		template<typename R, typename A>
		class FunctionWrapper1 : public FunctionWrapperBase<R>
		{
		public:
			typedef R(*Pointer)(A);

		public:
			inline FunctionWrapper1(Pointer pointer) : mPointer(pointer) {}

		protected:
			virtual void execute(const NativeFunction::Context context) const override
			{
				// Pop parameters
				const A a = FunctionWrapperBase<R>::template popStack<A>(context);

				// Call the actual function
				ReturnTypeHandler1<R, A>().call(mPointer, context, a);
			}

			virtual std::vector<const DataTypeDefinition*> getParameterTypes() const override
			{
				return { traits::getDataType<A>() };
			}

		protected:
			Pointer mPointer;
		};


		template<typename R, typename A, typename B>
		class FunctionWrapper2 : public FunctionWrapperBase<R>
		{
		public:
			typedef R(*Pointer)(A, B);

		public:
			inline FunctionWrapper2(Pointer pointer) : mPointer(pointer) {}

		protected:
			virtual void execute(const NativeFunction::Context context) const override
			{
				// Pop parameters
				const B b = FunctionWrapperBase<R>::template popStack<B>(context);
				const A a = FunctionWrapperBase<R>::template popStack<A>(context);

				// Call the actual function
				ReturnTypeHandler2<R, A, B>().call(mPointer, context, a, b);
			}

			virtual std::vector<const DataTypeDefinition*> getParameterTypes() const override
			{
				return { traits::getDataType<A>(), traits::getDataType<B>() };
			}

		protected:
			Pointer mPointer;
		};


		template<typename R, typename A, typename B, typename C>
		class FunctionWrapper3 : public FunctionWrapperBase<R>
		{
		public:
			typedef R(*Pointer)(A, B, C);

		public:
			inline FunctionWrapper3(Pointer pointer) : mPointer(pointer) {}

		protected:
			virtual void execute(const NativeFunction::Context context) const override
			{
				// Pop parameters
				const C c = FunctionWrapperBase<R>::template popStack<C>(context);
				const B b = FunctionWrapperBase<R>::template popStack<B>(context);
				const A a = FunctionWrapperBase<R>::template popStack<A>(context);

				// Call the actual function
				ReturnTypeHandler3<R, A, B, C>().call(mPointer, context, a, b, c);
			}

			virtual std::vector<const DataTypeDefinition*> getParameterTypes() const override
			{
				return { traits::getDataType<A>(), traits::getDataType<B>(), traits::getDataType<C>() };
			}

		protected:
			Pointer mPointer;
		};


		template<typename R, typename A, typename B, typename C, typename D>
		class FunctionWrapper4 : public FunctionWrapperBase<R>
		{
		public:
			typedef R(*Pointer)(A, B, C, D);

		public:
			inline FunctionWrapper4(Pointer pointer) : mPointer(pointer) {}

		protected:
			virtual void execute(const NativeFunction::Context context) const override
			{
				// Pop parameters
				const D d = FunctionWrapperBase<R>::template popStack<D>(context);
				const C c = FunctionWrapperBase<R>::template popStack<C>(context);
				const B b = FunctionWrapperBase<R>::template popStack<B>(context);
				const A a = FunctionWrapperBase<R>::template popStack<A>(context);

				// Call the actual function
				ReturnTypeHandler4<R, A, B, C, D>().call(mPointer, context, a, b, c, d);
			}

			virtual std::vector<const DataTypeDefinition*> getParameterTypes() const override
			{
				return { traits::getDataType<A>(), traits::getDataType<B>(), traits::getDataType<C>(), traits::getDataType<D>() };
			}

		protected:
			Pointer mPointer;
		};


		template<typename R, typename A, typename B, typename C, typename D, typename E>
		class FunctionWrapper5 : public FunctionWrapperBase<R>
		{
		public:
			typedef R(*Pointer)(A, B, C, D, E);

		public:
			inline FunctionWrapper5(Pointer pointer) : mPointer(pointer) {}

		protected:
			virtual void execute(const NativeFunction::Context context) const override
			{
				// Pop parameters
				const E e = FunctionWrapperBase<R>::template popStack<E>(context);
				const D d = FunctionWrapperBase<R>::template popStack<D>(context);
				const C c = FunctionWrapperBase<R>::template popStack<C>(context);
				const B b = FunctionWrapperBase<R>::template popStack<B>(context);
				const A a = FunctionWrapperBase<R>::template popStack<A>(context);

				// Call the actual function
				ReturnTypeHandler5<R, A, B, C, D, E>().call(mPointer, context, a, b, c, d, e);
			}

			virtual std::vector<const DataTypeDefinition*> getParameterTypes() const override
			{
				return { traits::getDataType<A>(), traits::getDataType<B>(), traits::getDataType<C>(), traits::getDataType<D>(), traits::getDataType<E>() };
			}

		protected:
			Pointer mPointer;
		};


		template<typename R, typename A, typename B, typename C, typename D, typename E, typename F>
		class FunctionWrapper6 : public FunctionWrapperBase<R>
		{
		public:
			typedef R(*Pointer)(A, B, C, D, E, F);

		public:
			inline FunctionWrapper6(Pointer pointer) : mPointer(pointer) {}

		protected:
			virtual void execute(const NativeFunction::Context context) const override
			{
				// Pop parameters
				const F f = FunctionWrapperBase<R>::template popStack<F>(context);
				const E e = FunctionWrapperBase<R>::template popStack<E>(context);
				const D d = FunctionWrapperBase<R>::template popStack<D>(context);
				const C c = FunctionWrapperBase<R>::template popStack<C>(context);
				const B b = FunctionWrapperBase<R>::template popStack<B>(context);
				const A a = FunctionWrapperBase<R>::template popStack<A>(context);

				// Call the actual function
				ReturnTypeHandler6<R, A, B, C, D, E, F>().call(mPointer, context, a, b, c, d, e, f);
			}

			virtual std::vector<const DataTypeDefinition*> getParameterTypes() const override
			{
				return { traits::getDataType<A>(), traits::getDataType<B>(), traits::getDataType<C>(), traits::getDataType<D>(), traits::getDataType<E>(), traits::getDataType<F>() };
			}

		protected:
			Pointer mPointer;
		};


		template<typename R, typename A, typename B, typename C, typename D, typename E, typename F, typename G>
		class FunctionWrapper7 : public FunctionWrapperBase<R>
		{
		public:
			typedef R(*Pointer)(A, B, C, D, E, F, G);

		public:
			inline FunctionWrapper7(Pointer pointer) : mPointer(pointer) {}

		protected:
			virtual void execute(const NativeFunction::Context context) const override
			{
				// Pop parameters
				const G g = FunctionWrapperBase<R>::template popStack<G>(context);
				const F f = FunctionWrapperBase<R>::template popStack<F>(context);
				const E e = FunctionWrapperBase<R>::template popStack<E>(context);
				const D d = FunctionWrapperBase<R>::template popStack<D>(context);
				const C c = FunctionWrapperBase<R>::template popStack<C>(context);
				const B b = FunctionWrapperBase<R>::template popStack<B>(context);
				const A a = FunctionWrapperBase<R>::template popStack<A>(context);

				// Call the actual function
				ReturnTypeHandler7<R, A, B, C, D, E, F, G>().call(mPointer, context, a, b, c, d, e, f, g);
			}

			virtual std::vector<const DataTypeDefinition*> getParameterTypes() const override
			{
				return { traits::getDataType<A>(), traits::getDataType<B>(), traits::getDataType<C>(), traits::getDataType<D>(), traits::getDataType<E>(), traits::getDataType<F>(), traits::getDataType<G>() };
			}

		protected:
			Pointer mPointer;
		};


		template<typename R, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H>
		class FunctionWrapper8 : public FunctionWrapperBase<R>
		{
		public:
			typedef R(*Pointer)(A, B, C, D, E, F, G, H);

		public:
			inline FunctionWrapper8(Pointer pointer) : mPointer(pointer) {}

		protected:
			virtual void execute(const NativeFunction::Context context) const override
			{
				// Pop parameters
				const H h = FunctionWrapperBase<R>::template popStack<H>(context);
				const G g = FunctionWrapperBase<R>::template popStack<G>(context);
				const F f = FunctionWrapperBase<R>::template popStack<F>(context);
				const E e = FunctionWrapperBase<R>::template popStack<E>(context);
				const D d = FunctionWrapperBase<R>::template popStack<D>(context);
				const C c = FunctionWrapperBase<R>::template popStack<C>(context);
				const B b = FunctionWrapperBase<R>::template popStack<B>(context);
				const A a = FunctionWrapperBase<R>::template popStack<A>(context);

				// Call the actual function
				ReturnTypeHandler8<R, A, B, C, D, E, F, G, H>().call(mPointer, context, a, b, c, d, e, f, g, h);
			}

			virtual std::vector<const DataTypeDefinition*> getParameterTypes() const override
			{
				return { traits::getDataType<A>(), traits::getDataType<B>(), traits::getDataType<C>(), traits::getDataType<D>(), traits::getDataType<E>(), traits::getDataType<F>(), traits::getDataType<G>(), traits::getDataType<H>() };
			}

		protected:
			Pointer mPointer;
		};


		template<typename R, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I>
		class FunctionWrapper9 : public FunctionWrapperBase<R>
		{
		public:
			typedef R(*Pointer)(A, B, C, D, E, F, G, H, I);

		public:
			inline FunctionWrapper9(Pointer pointer) : mPointer(pointer) {}

		protected:
			virtual void execute(const NativeFunction::Context context) const override
			{
				// Pop parameters
				const I i = FunctionWrapperBase<R>::template popStack<I>(context);
				const H h = FunctionWrapperBase<R>::template popStack<H>(context);
				const G g = FunctionWrapperBase<R>::template popStack<G>(context);
				const F f = FunctionWrapperBase<R>::template popStack<F>(context);
				const E e = FunctionWrapperBase<R>::template popStack<E>(context);
				const D d = FunctionWrapperBase<R>::template popStack<D>(context);
				const C c = FunctionWrapperBase<R>::template popStack<C>(context);
				const B b = FunctionWrapperBase<R>::template popStack<B>(context);
				const A a = FunctionWrapperBase<R>::template popStack<A>(context);

				// Call the actual function
				ReturnTypeHandler9<R, A, B, C, D, E, F, G, H, I>().call(mPointer, context, a, b, c, d, e, f, g, h, i);
			}

			virtual std::vector<const DataTypeDefinition*> getParameterTypes() const override
			{
				return { traits::getDataType<A>(), traits::getDataType<B>(), traits::getDataType<C>(), traits::getDataType<D>(), traits::getDataType<E>(), traits::getDataType<F>(), traits::getDataType<G>(), traits::getDataType<H>(), traits::getDataType<I>() };
			}

		protected:
			Pointer mPointer;
		};


		template<typename R, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I, typename J>
		class FunctionWrapper10 : public FunctionWrapperBase<R>
		{
		public:
			typedef R(*Pointer)(A, B, C, D, E, F, G, H, I, J);

		public:
			inline FunctionWrapper10(Pointer pointer) : mPointer(pointer) {}

		protected:
			virtual void execute(const NativeFunction::Context context) const override
			{
				// Pop parameters
				const J j = FunctionWrapperBase<R>::template popStack<J>(context);
				const I i = FunctionWrapperBase<R>::template popStack<I>(context);
				const H h = FunctionWrapperBase<R>::template popStack<H>(context);
				const G g = FunctionWrapperBase<R>::template popStack<G>(context);
				const F f = FunctionWrapperBase<R>::template popStack<F>(context);
				const E e = FunctionWrapperBase<R>::template popStack<E>(context);
				const D d = FunctionWrapperBase<R>::template popStack<D>(context);
				const C c = FunctionWrapperBase<R>::template popStack<C>(context);
				const B b = FunctionWrapperBase<R>::template popStack<B>(context);
				const A a = FunctionWrapperBase<R>::template popStack<A>(context);

				// Call the actual function
				ReturnTypeHandler10<R, A, B, C, D, E, F, G, H, I, J>().call(mPointer, context, a, b, c, d, e, f, g, h, i, j);
			}

			virtual std::vector<const DataTypeDefinition*> getParameterTypes() const override
			{
				return { traits::getDataType<A>(), traits::getDataType<B>(), traits::getDataType<C>(), traits::getDataType<D>(), traits::getDataType<E>(), traits::getDataType<F>(), traits::getDataType<G>(), traits::getDataType<H>(), traits::getDataType<I>(), traits::getDataType<J>() };
			}

		protected:
			Pointer mPointer;
		};


		template<typename R, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I, typename J, typename K>
		class FunctionWrapper11 : public FunctionWrapperBase<R>
		{
		public:
			typedef R(*Pointer)(A, B, C, D, E, F, G, H, I, J, K);

		public:
			inline FunctionWrapper11(Pointer pointer) : mPointer(pointer) {}

		protected:
			virtual void execute(const NativeFunction::Context context) const override
			{
				// Pop parameters
				const K k = FunctionWrapperBase<R>::template popStack<K>(context);
				const J j = FunctionWrapperBase<R>::template popStack<J>(context);
				const I i = FunctionWrapperBase<R>::template popStack<I>(context);
				const H h = FunctionWrapperBase<R>::template popStack<H>(context);
				const G g = FunctionWrapperBase<R>::template popStack<G>(context);
				const F f = FunctionWrapperBase<R>::template popStack<F>(context);
				const E e = FunctionWrapperBase<R>::template popStack<E>(context);
				const D d = FunctionWrapperBase<R>::template popStack<D>(context);
				const C c = FunctionWrapperBase<R>::template popStack<C>(context);
				const B b = FunctionWrapperBase<R>::template popStack<B>(context);
				const A a = FunctionWrapperBase<R>::template popStack<A>(context);

				// Call the actual function
				ReturnTypeHandler11<R, A, B, C, D, E, F, G, H, I, J, K>().call(mPointer, context, a, b, c, d, e, f, g, h, i, j, k);
			}

			virtual std::vector<const DataTypeDefinition*> getParameterTypes() const override
			{
				return { traits::getDataType<A>(), traits::getDataType<B>(), traits::getDataType<C>(), traits::getDataType<D>(), traits::getDataType<E>(), traits::getDataType<F>(), traits::getDataType<G>(), traits::getDataType<H>(), traits::getDataType<I>(), traits::getDataType<J>(), traits::getDataType<K>() };
			}

		protected:
			Pointer mPointer;
		};



		// Class method wrappers

		template<typename CLASS, typename R>
		class FunctionWrapperM0 : public FunctionWrapperBase<R>
		{
		public:
			typedef R(CLASS::*Pointer)();

		public:
			inline FunctionWrapperM0(CLASS& object, Pointer pointer) : mObject(object), mPointer(pointer) {}

		protected:
			virtual void execute(const NativeFunction::Context context) const override
			{
				// Call the actual function
				ReturnTypeHandlerM0<CLASS, R>().call(mObject, mPointer, context);
			}

			virtual std::vector<const DataTypeDefinition*> getParameterTypes() const override
			{
				return {};
			}

		protected:
			CLASS& mObject;
			Pointer mPointer;
		};


		template<typename CLASS, typename R, typename A>
		class FunctionWrapperM1 : public FunctionWrapperBase<R>
		{
		public:
			typedef R(CLASS::*Pointer)(A);

		public:
			inline FunctionWrapperM1(CLASS& object, Pointer pointer) : mObject(object), mPointer(pointer) {}

		protected:
			virtual void execute(const NativeFunction::Context context) const override
			{
				// Pop parameters
				const A a = FunctionWrapperBase<R>::template popStack<A>(context);

				// Call the actual function
				ReturnTypeHandlerM1<CLASS, R, A>().call(mObject, mPointer, context, a);
			}

			virtual std::vector<const DataTypeDefinition*> getParameterTypes() const override
			{
				return { traits::getDataType<A>() };
			}

		protected:
			CLASS& mObject;
			Pointer mPointer;
		};


		template<typename CLASS, typename R, typename A, typename B>
		class FunctionWrapperM2 : public FunctionWrapperBase<R>
		{
		public:
			typedef R(CLASS::*Pointer)(A, B);

		public:
			inline FunctionWrapperM2(CLASS& object, Pointer pointer) : mObject(object), mPointer(pointer) {}

		protected:
			virtual void execute(const NativeFunction::Context context) const override
			{
				// Pop parameters
				const B b = FunctionWrapperBase<R>::template popStack<B>(context);
				const A a = FunctionWrapperBase<R>::template popStack<A>(context);

				// Call the actual function
				ReturnTypeHandlerM2<CLASS, R, A, B>().call(mObject, mPointer, context, a, b);
			}

			virtual std::vector<const DataTypeDefinition*> getParameterTypes() const override
			{
				return { traits::getDataType<A>(), traits::getDataType<B>() };
			}

		protected:
			CLASS& mObject;
			Pointer mPointer;
		};


		template<typename CLASS, typename R, typename A, typename B, typename C>
		class FunctionWrapperM3 : public FunctionWrapperBase<R>
		{
		public:
			typedef R(CLASS::*Pointer)(A, B, C);

		public:
			inline FunctionWrapperM3(CLASS& object, Pointer pointer) : mObject(object), mPointer(pointer) {}

		protected:
			virtual void execute(const NativeFunction::Context context) const override
			{
				// Pop parameters
				const C c = FunctionWrapperBase<R>::template popStack<C>(context);
				const B b = FunctionWrapperBase<R>::template popStack<B>(context);
				const A a = FunctionWrapperBase<R>::template popStack<A>(context);

				// Call the actual function
				ReturnTypeHandlerM3<CLASS, R, A, B, C>().call(mObject, mPointer, context, a, b, c);
			}

			virtual std::vector<const DataTypeDefinition*> getParameterTypes() const override
			{
				return { traits::getDataType<A>(), traits::getDataType<B>(), traits::getDataType<C>() };
			}

		protected:
			CLASS& mObject;
			Pointer mPointer;
		};


		template<typename CLASS, typename R, typename A, typename B, typename C, typename D>
		class FunctionWrapperM4 : public FunctionWrapperBase<R>
		{
		public:
			typedef R(CLASS::*Pointer)(A, B, C, D);

		public:
			inline FunctionWrapperM4(CLASS& object, Pointer pointer) : mObject(object), mPointer(pointer) {}

		protected:
			virtual void execute(const NativeFunction::Context context) const override
			{
				// Pop parameters
				const D d = FunctionWrapperBase<R>::template popStack<D>(context);
				const C c = FunctionWrapperBase<R>::template popStack<C>(context);
				const B b = FunctionWrapperBase<R>::template popStack<B>(context);
				const A a = FunctionWrapperBase<R>::template popStack<A>(context);

				// Call the actual function
				ReturnTypeHandlerM4<CLASS, R, A, B, C, D>().call(mObject, mPointer, context, a, b, c, d);
			}

			virtual std::vector<const DataTypeDefinition*> getParameterTypes() const override
			{
				return { traits::getDataType<A>(), traits::getDataType<B>(), traits::getDataType<C>(), traits::getDataType<D>() };
			}

		protected:
			CLASS& mObject;
			Pointer mPointer;
		};


		template<typename CLASS, typename R, typename A, typename B, typename C, typename D, typename E>
		class FunctionWrapperM5 : public FunctionWrapperBase<R>
		{
		public:
			typedef R(CLASS::*Pointer)(A, B, C, D, E);

		public:
			inline FunctionWrapperM5(CLASS& object, Pointer pointer) : mObject(object), mPointer(pointer) {}

		protected:
			virtual void execute(const NativeFunction::Context context) const override
			{
				// Pop parameters
				const E e = FunctionWrapperBase<R>::template popStack<E>(context);
				const D d = FunctionWrapperBase<R>::template popStack<D>(context);
				const C c = FunctionWrapperBase<R>::template popStack<C>(context);
				const B b = FunctionWrapperBase<R>::template popStack<B>(context);
				const A a = FunctionWrapperBase<R>::template popStack<A>(context);

				// Call the actual function
				ReturnTypeHandlerM5<CLASS, R, A, B, C, D, E>().call(mObject, mPointer, context, a, b, c, d, e);
			}

			virtual std::vector<const DataTypeDefinition*> getParameterTypes() const override
			{
				return { traits::getDataType<A>(), traits::getDataType<B>(), traits::getDataType<C>(), traits::getDataType<D>(), traits::getDataType<E>() };
			}

		protected:
			CLASS& mObject;
			Pointer mPointer;
		};


		template<typename CLASS, typename R, typename A, typename B, typename C, typename D, typename E, typename F>
		class FunctionWrapperM6 : public FunctionWrapperBase<R>
		{
		public:
			typedef R(CLASS::*Pointer)(A, B, C, D, E, F);

		public:
			inline FunctionWrapperM6(CLASS& object, Pointer pointer) : mObject(object), mPointer(pointer) {}

		protected:
			virtual void execute(const NativeFunction::Context context) const override
			{
				// Pop parameters
				const F f = FunctionWrapperBase<R>::template popStack<F>(context);
				const E e = FunctionWrapperBase<R>::template popStack<E>(context);
				const D d = FunctionWrapperBase<R>::template popStack<D>(context);
				const C c = FunctionWrapperBase<R>::template popStack<C>(context);
				const B b = FunctionWrapperBase<R>::template popStack<B>(context);
				const A a = FunctionWrapperBase<R>::template popStack<A>(context);

				// Call the actual function
				ReturnTypeHandlerM6<CLASS, R, A, B, C, D, E, F>().call(mObject, mPointer, context, a, b, c, d, e, f);
			}

			virtual std::vector<const DataTypeDefinition*> getParameterTypes() const override
			{
				return { traits::getDataType<A>(), traits::getDataType<B>(), traits::getDataType<C>(), traits::getDataType<D>(), traits::getDataType<E>(), traits::getDataType<F>() };
			}

		protected:
			CLASS& mObject;
			Pointer mPointer;
		};


		template<typename CLASS, typename R, typename A, typename B, typename C, typename D, typename E, typename F, typename G>
		class FunctionWrapperM7 : public FunctionWrapperBase<R>
		{
		public:
			typedef R(CLASS::*Pointer)(A, B, C, D, E, F, G);

		public:
			inline FunctionWrapperM7(CLASS& object, Pointer pointer) : mObject(object), mPointer(pointer) {}

		protected:
			virtual void execute(const NativeFunction::Context context) const override
			{
				// Pop parameters
				const G g = FunctionWrapperBase<R>::template popStack<G>(context);
				const F f = FunctionWrapperBase<R>::template popStack<F>(context);
				const E e = FunctionWrapperBase<R>::template popStack<E>(context);
				const D d = FunctionWrapperBase<R>::template popStack<D>(context);
				const C c = FunctionWrapperBase<R>::template popStack<C>(context);
				const B b = FunctionWrapperBase<R>::template popStack<B>(context);
				const A a = FunctionWrapperBase<R>::template popStack<A>(context);

				// Call the actual function
				ReturnTypeHandlerM7<CLASS, R, A, B, C, D, E, F, G>().call(mObject, mPointer, context, a, b, c, d, e, f, g);
			}

			virtual std::vector<const DataTypeDefinition*> getParameterTypes() const override
			{
				return { traits::getDataType<A>(), traits::getDataType<B>(), traits::getDataType<C>(), traits::getDataType<D>(), traits::getDataType<E>(), traits::getDataType<F>(), traits::getDataType<G>() };
			}

		protected:
			CLASS& mObject;
			Pointer mPointer;
		};


		template<typename CLASS, typename R, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H>
		class FunctionWrapperM8 : public FunctionWrapperBase<R>
		{
		public:
			typedef R(CLASS::*Pointer)(A, B, C, D, E, F, G, H);

		public:
			inline FunctionWrapperM8(CLASS& object, Pointer pointer) : mObject(object), mPointer(pointer) {}

		protected:
			virtual void execute(const NativeFunction::Context context) const override
			{
				// Pop parameters
				const H h = FunctionWrapperBase<R>::template popStack<H>(context);
				const G g = FunctionWrapperBase<R>::template popStack<G>(context);
				const F f = FunctionWrapperBase<R>::template popStack<F>(context);
				const E e = FunctionWrapperBase<R>::template popStack<E>(context);
				const D d = FunctionWrapperBase<R>::template popStack<D>(context);
				const C c = FunctionWrapperBase<R>::template popStack<C>(context);
				const B b = FunctionWrapperBase<R>::template popStack<B>(context);
				const A a = FunctionWrapperBase<R>::template popStack<A>(context);

				// Call the actual function
				ReturnTypeHandlerM8<CLASS, R, A, B, C, D, E, F, G, H>().call(mObject, mPointer, context, a, b, c, d, e, f, g, h);
			}

			virtual std::vector<const DataTypeDefinition*> getParameterTypes() const override
			{
				return { traits::getDataType<A>(), traits::getDataType<B>(), traits::getDataType<C>(), traits::getDataType<D>(), traits::getDataType<E>(), traits::getDataType<F>(), traits::getDataType<G>(), traits::getDataType<H>() };
			}

		protected:
			CLASS& mObject;
			Pointer mPointer;
		};


		template<typename CLASS, typename R, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I>
		class FunctionWrapperM9 : public FunctionWrapperBase<R>
		{
		public:
			typedef R(CLASS::*Pointer)(A, B, C, D, E, F, G, H, I);

		public:
			inline FunctionWrapperM9(CLASS& object, Pointer pointer) : mObject(object), mPointer(pointer) {}

		protected:
			virtual void execute(const NativeFunction::Context context) const override
			{
				// Pop parameters
				const I i = FunctionWrapperBase<R>::template popStack<I>(context);
				const H h = FunctionWrapperBase<R>::template popStack<H>(context);
				const G g = FunctionWrapperBase<R>::template popStack<G>(context);
				const F f = FunctionWrapperBase<R>::template popStack<F>(context);
				const E e = FunctionWrapperBase<R>::template popStack<E>(context);
				const D d = FunctionWrapperBase<R>::template popStack<D>(context);
				const C c = FunctionWrapperBase<R>::template popStack<C>(context);
				const B b = FunctionWrapperBase<R>::template popStack<B>(context);
				const A a = FunctionWrapperBase<R>::template popStack<A>(context);

				// Call the actual function
				ReturnTypeHandlerM9<CLASS, R, A, B, C, D, E, F, G, H, I>().call(mObject, mPointer, context, a, b, c, d, e, f, g, h, i);
			}

			virtual std::vector<const DataTypeDefinition*> getParameterTypes() const override
			{
				return { traits::getDataType<A>(), traits::getDataType<B>(), traits::getDataType<C>(), traits::getDataType<D>(), traits::getDataType<E>(), traits::getDataType<F>(), traits::getDataType<G>(), traits::getDataType<H>(), traits::getDataType<I>() };
			}

		protected:
			CLASS& mObject;
			Pointer mPointer;
		};


		template<typename CLASS, typename R, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I, typename J>
		class FunctionWrapperM10 : public FunctionWrapperBase<R>
		{
		public:
			typedef R(CLASS::*Pointer)(A, B, C, D, E, F, G, H, I, J);

		public:
			inline FunctionWrapperM10(CLASS& object, Pointer pointer) : mObject(object), mPointer(pointer) {}

		protected:
			virtual void execute(const NativeFunction::Context context) const override
			{
				// Pop parameters
				const J j = FunctionWrapperBase<R>::template popStack<J>(context);
				const I i = FunctionWrapperBase<R>::template popStack<I>(context);
				const H h = FunctionWrapperBase<R>::template popStack<H>(context);
				const G g = FunctionWrapperBase<R>::template popStack<G>(context);
				const F f = FunctionWrapperBase<R>::template popStack<F>(context);
				const E e = FunctionWrapperBase<R>::template popStack<E>(context);
				const D d = FunctionWrapperBase<R>::template popStack<D>(context);
				const C c = FunctionWrapperBase<R>::template popStack<C>(context);
				const B b = FunctionWrapperBase<R>::template popStack<B>(context);
				const A a = FunctionWrapperBase<R>::template popStack<A>(context);

				// Call the actual function
				ReturnTypeHandlerM10<CLASS, R, A, B, C, D, E, F, G, H, I, J>().call(mObject, mPointer, context, a, b, c, d, e, f, g, h, i, j);
			}

			virtual std::vector<const DataTypeDefinition*> getParameterTypes() const override
			{
				return { traits::getDataType<A>(), traits::getDataType<B>(), traits::getDataType<C>(), traits::getDataType<D>(), traits::getDataType<E>(), traits::getDataType<F>(), traits::getDataType<G>(), traits::getDataType<H>(), traits::getDataType<I>(), traits::getDataType<J>() };
			}

		protected:
			CLASS& mObject;
			Pointer mPointer;
		};


		template<typename CLASS, typename R, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I, typename J, typename K>
		class FunctionWrapperM11 : public FunctionWrapperBase<R>
		{
		public:
			typedef R(CLASS::*Pointer)(A, B, C, D, E, F, G, H, I, J, K);

		public:
			inline FunctionWrapperM11(CLASS& object, Pointer pointer) : mObject(object), mPointer(pointer) {}

		protected:
			virtual void execute(const NativeFunction::Context context) const override
			{
				// Pop parameters
				const K k = FunctionWrapperBase<R>::template popStack<K>(context);
				const J j = FunctionWrapperBase<R>::template popStack<J>(context);
				const I i = FunctionWrapperBase<R>::template popStack<I>(context);
				const H h = FunctionWrapperBase<R>::template popStack<H>(context);
				const G g = FunctionWrapperBase<R>::template popStack<G>(context);
				const F f = FunctionWrapperBase<R>::template popStack<F>(context);
				const E e = FunctionWrapperBase<R>::template popStack<E>(context);
				const D d = FunctionWrapperBase<R>::template popStack<D>(context);
				const C c = FunctionWrapperBase<R>::template popStack<C>(context);
				const B b = FunctionWrapperBase<R>::template popStack<B>(context);
				const A a = FunctionWrapperBase<R>::template popStack<A>(context);

				// Call the actual function
				ReturnTypeHandlerM11<CLASS, R, A, B, C, D, E, F, G, H, I, J, K>().call(mObject, mPointer, context, a, b, c, d, e, f, g, h, i, j, k);
			}

			virtual std::vector<const DataTypeDefinition*> getParameterTypes() const override
			{
				return { traits::getDataType<A>(), traits::getDataType<B>(), traits::getDataType<C>(), traits::getDataType<D>(), traits::getDataType<E>(), traits::getDataType<F>(), traits::getDataType<G>(), traits::getDataType<H>(), traits::getDataType<I>(), traits::getDataType<J>(), traits::getDataType<K>() };
			}

		protected:
			CLASS& mObject;
			Pointer mPointer;
		};

	}


	// --- Function wrapper functionality ---

	template<typename R>
	static NativeFunction::FunctionWrapper& wrap(R(*pointer)())
	{
		return *new internal::FunctionWrapper0<R>(pointer);
	}

	template<typename R, typename A>
	static NativeFunction::FunctionWrapper& wrap(R(*pointer)(A))
	{
		return *new internal::FunctionWrapper1<R, A>(pointer);
	}

	template<typename R, typename A, typename B>
	static NativeFunction::FunctionWrapper& wrap(R(*pointer)(A, B))
	{
		return *new internal::FunctionWrapper2<R, A, B>(pointer);
	}

	template<typename R, typename A, typename B, typename C>
	static NativeFunction::FunctionWrapper& wrap(R(*pointer)(A, B, C))
	{
		return *new internal::FunctionWrapper3<R, A, B, C>(pointer);
	}

	template<typename R, typename A, typename B, typename C, typename D>
	static NativeFunction::FunctionWrapper& wrap(R(*pointer)(A, B, C, D))
	{
		return *new internal::FunctionWrapper4<R, A, B, C, D>(pointer);
	}

	template<typename R, typename A, typename B, typename C, typename D, typename E>
	static NativeFunction::FunctionWrapper& wrap(R(*pointer)(A, B, C, D, E))
	{
		return *new internal::FunctionWrapper5<R, A, B, C, D, E>(pointer);
	}

	template<typename R, typename A, typename B, typename C, typename D, typename E, typename F>
	static NativeFunction::FunctionWrapper& wrap(R(*pointer)(A, B, C, D, E, F))
	{
		return *new internal::FunctionWrapper6<R, A, B, C, D, E, F>(pointer);
	}

	template<typename R, typename A, typename B, typename C, typename D, typename E, typename F, typename G>
	static NativeFunction::FunctionWrapper& wrap(R(*pointer)(A, B, C, D, E, F, G))
	{
		return *new internal::FunctionWrapper7<R, A, B, C, D, E, F, G>(pointer);
	}

	template<typename R, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H>
	static NativeFunction::FunctionWrapper& wrap(R(*pointer)(A, B, C, D, E, F, G, H))
	{
		return *new internal::FunctionWrapper8<R, A, B, C, D, E, F, G, H>(pointer);
	}

	template<typename R, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I>
	static NativeFunction::FunctionWrapper& wrap(R(*pointer)(A, B, C, D, E, F, G, H, I))
	{
		return *new internal::FunctionWrapper9<R, A, B, C, D, E, F, G, H, I>(pointer);
	}

	template<typename R, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I, typename J>
	static NativeFunction::FunctionWrapper& wrap(R(*pointer)(A, B, C, D, E, F, G, H, I, J))
	{
		return *new internal::FunctionWrapper10<R, A, B, C, D, E, F, G, H, I, J>(pointer);
	}

	template<typename R, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I, typename J, typename K>
	static NativeFunction::FunctionWrapper& wrap(R(*pointer)(A, B, C, D, E, F, G, H, I, J, K))
	{
		return *new internal::FunctionWrapper11<R, A, B, C, D, E, F, G, H, I, J, K>(pointer);
	}


	template<typename CLASS, typename R>
	static NativeFunction::FunctionWrapper& wrap(CLASS& object, R(CLASS::*pointer)())
	{
		return *new internal::FunctionWrapperM0<CLASS, R>(object, pointer);
	}

	template<typename CLASS, typename R, typename A>
	static NativeFunction::FunctionWrapper& wrap(CLASS& object, R(CLASS::*pointer)(A))
	{
		return *new internal::FunctionWrapperM1<CLASS, R, A>(object, pointer);
	}

	template<typename CLASS, typename R, typename A, typename B>
	static NativeFunction::FunctionWrapper& wrap(CLASS& object, R(CLASS::*pointer)(A, B))
	{
		return *new internal::FunctionWrapperM2<CLASS, R, A, B>(object, pointer);
	}

	template<typename CLASS, typename R, typename A, typename B, typename C>
	static NativeFunction::FunctionWrapper& wrap(CLASS& object, R(CLASS::*pointer)(A, B, C))
	{
		return *new internal::FunctionWrapperM3<CLASS, R, A, B, C>(object, pointer);
	}

	template<typename CLASS, typename R, typename A, typename B, typename C, typename D>
	static NativeFunction::FunctionWrapper& wrap(CLASS& object, R(CLASS::*pointer)(A, B, C, D))
	{
		return *new internal::FunctionWrapperM4<CLASS, R, A, B, C, D>(object, pointer);
	}

	template<typename CLASS, typename R, typename A, typename B, typename C, typename D, typename E>
	static NativeFunction::FunctionWrapper& wrap(CLASS& object, R(CLASS::*pointer)(A, B, C, D, E))
	{
		return *new internal::FunctionWrapperM5<CLASS, R, A, B, C, D, E>(object, pointer);
	}

	template<typename CLASS, typename R, typename A, typename B, typename C, typename D, typename E, typename F>
	static NativeFunction::FunctionWrapper& wrap(CLASS& object, R(CLASS::*pointer)(A, B, C, D, E, F))
	{
		return *new internal::FunctionWrapperM6<CLASS, R, A, B, C, D, E, F>(object, pointer);
	}

	template<typename CLASS, typename R, typename A, typename B, typename C, typename D, typename E, typename F, typename G>
	static NativeFunction::FunctionWrapper& wrap(CLASS& object, R(CLASS::*pointer)(A, B, C, D, E, F, G))
	{
		return *new internal::FunctionWrapperM7<CLASS, R, A, B, C, D, E, F, G>(object, pointer);
	}

	template<typename CLASS, typename R, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H>
	static NativeFunction::FunctionWrapper& wrap(CLASS& object, R(CLASS::*pointer)(A, B, C, D, E, F, G, H))
	{
		return *new internal::FunctionWrapperM8<CLASS, R, A, B, C, D, E, F, G, H>(object, pointer);
	}

	template<typename CLASS, typename R, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I>
	static NativeFunction::FunctionWrapper& wrap(CLASS& object, R(CLASS::*pointer)(A, B, C, D, E, F, G, H, I))
	{
		return *new internal::FunctionWrapperM9<CLASS, R, A, B, C, D, E, F, G, H, I>(object, pointer);
	}

	template<typename CLASS, typename R, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I, typename J>
	static NativeFunction::FunctionWrapper& wrap(CLASS& object, R(CLASS::*pointer)(A, B, C, D, E, F, G, H, I, J))
	{
		return *new internal::FunctionWrapperM10<CLASS, R, A, B, C, D, E, F, G, H, I, J>(object, pointer);
	}

	template<typename CLASS, typename R, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I, typename J, typename K>
	static NativeFunction::FunctionWrapper& wrap(CLASS& object, R(CLASS::*pointer)(A, B, C, D, E, F, G, H, I, J, K))
	{
		return *new internal::FunctionWrapperM11<CLASS, R, A, B, C, D, E, F, G, H, I, J, K>(object, pointer);
	}

}
