/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/program/function/NativeFunction.h"
#include "lemon/runtime/Runtime.h"
#include "lemon/utility/AnyBaseValue.h"


namespace lemon
{

	struct AnyTypeWrapper
	{
		AnyBaseValue mValue;
		const DataTypeDefinition* mType = nullptr;

		inline AnyTypeWrapper() {}
		explicit AnyTypeWrapper(uint64 value);

		void pushToStack(ControlFlow& controlFlow) const;
		void popFromStack(ControlFlow& controlFlow);
		void readFromStack(ControlFlow& controlFlow);
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
		template<> const DataTypeDefinition* getDataType<ArrayBaseWrapper>();
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



		// Template specializations for ArrayBaseWrapper, representing all array types in script

		template<>
		void pushStackGeneric<ArrayBaseWrapper>(ArrayBaseWrapper value, const NativeFunction::Context context);

		template<>
		ArrayBaseWrapper popStackGeneric(const NativeFunction::Context context);



		// Template specializations for AnyTypeWrapper, representing the "any" type in script

		template<>
		void pushStackGeneric<AnyTypeWrapper>(AnyTypeWrapper value, const NativeFunction::Context context);

		template<>
		AnyTypeWrapper popStackGeneric(const NativeFunction::Context context);



		// Parameter builder helper class

		template<typename CLASS, bool WITH_CONTEXT, typename... Tuple>
		struct ParameterBuilder
		{
			using ClassPart   = std::conditional_t<std::is_void_v<CLASS>, std::tuple<>, std::tuple<CLASS*>>;
			using ContextPart = std::conditional_t<WITH_CONTEXT, std::tuple<const NativeFunction::Context*>, std::tuple<>>;
			using FullTuple   = decltype(std::tuple_cat(std::declval<ClassPart>(), std::declval<ContextPart>(), std::declval<std::tuple<Tuple...>>()));

			FullTuple mTuple;
			const NativeFunction::Context& mContext;

			ParameterBuilder(const NativeFunction::Context& context) : mContext(context)
			{
				if constexpr (WITH_CONTEXT)
				{
					std::get<0>(mTuple) = &context;
				}
			}

			ParameterBuilder(const NativeFunction::Context& context, CLASS* object) : mContext(context)
			{
				if constexpr (std::is_void_v<CLASS>)
				{
					if constexpr (WITH_CONTEXT)
						std::get<0>(mTuple) = &context;
				}
				else
				{
					std::get<0>(mTuple) = object;
					if constexpr (WITH_CONTEXT)
						std::get<1>(mTuple) = &context;
				}
			}

			template<typename T, typename... Args>
			void popStackInReverseOrder()
			{
				// Recursive call right at the start, so we get the reverse order
				if constexpr (sizeof...(Args) > 0)
				{
					popStackInReverseOrder<Args...>();
				}

				// Pop from stack
				constexpr size_t numFixedEntries = (std::is_void_v<CLASS> ? 0 : 1) + (WITH_CONTEXT ? 1 : 0);
				constexpr size_t index = sizeof...(Tuple) - sizeof...(Args) - 1 + numFixedEntries;
				std::get<index>(mTuple) = popStackGeneric<T>(mContext);
			}
		};



		// Function wrappers

		template<bool WITH_CONTEXT, typename R, typename... Args>
		class FunctionWrapper : public NativeFunction::FunctionWrapper
		{
		public:
			using Pointer = std::conditional_t<WITH_CONTEXT,
											   R(*)(const NativeFunction::Context*, Args...),
											   R(*)(Args...) >;

			explicit FunctionWrapper(Pointer pointer) : mPointer(pointer) {}

		protected:
			void execute(const NativeFunction::Context context) const override
			{
				ParameterBuilder<void, WITH_CONTEXT, Args...> parameters(context);
				if constexpr (sizeof...(Args) > 0)
				{
					parameters.template popStackInReverseOrder<Args...>();
				}

				if constexpr (std::is_void_v<R>)
				{
					std::apply(mPointer, parameters.mTuple);
				}
				else
				{
					pushStackGeneric(std::apply(mPointer, parameters.mTuple), context);
				}
			}

			virtual const DataTypeDefinition* getReturnType() const override
			{
				return traits::getDataType<R>();
			}

			virtual std::vector<const DataTypeDefinition*> getParameterTypes() const override
			{
				return { traits::getDataType<Args>()... };
			}

		private:
			Pointer mPointer;
		};



		// Class method wrapper

		template<bool WITH_CONTEXT, typename CLASS, typename R, typename... Args>
		class MethodWrapper : public NativeFunction::FunctionWrapper
		{
		public:
			using Pointer = std::conditional_t<WITH_CONTEXT,
											   R(CLASS::*)(const NativeFunction::Context*, Args...),
											   R(CLASS::*)(Args...) >;

			explicit MethodWrapper(CLASS& object, Pointer pointer) : mObject(object), mPointer(pointer) {}

		protected:
			void execute(const NativeFunction::Context context) const override
			{
				ParameterBuilder<CLASS, WITH_CONTEXT, Args...> parameters(context, &mObject);
				if constexpr (sizeof...(Args) > 0)
				{
					parameters.template popStackInReverseOrder<Args...>();
				}

				if constexpr (std::is_void_v<R>)
				{
					std::apply(mPointer, parameters.mTuple);
				}
				else
				{
					pushStackGeneric(std::apply(mPointer, parameters.mTuple), context);
				}
			}

			virtual const DataTypeDefinition* getReturnType() const override
			{
				return traits::getDataType<R>();
			}

			virtual std::vector<const DataTypeDefinition*> getParameterTypes() const override
			{
				return { traits::getDataType<Args>()... };
			}

		private:
			CLASS& mObject;
			Pointer mPointer;
		};

	}


	// --- Function wrapper functionality ---

	template<typename R>
	static NativeFunction::FunctionWrapper& wrap(R(*pointer)())
	{
		return *new internal::FunctionWrapper<false, R>(pointer);
	}

	template<typename R>
	static NativeFunction::FunctionWrapper& wrap(R(*pointer)(const NativeFunction::Context*))
	{
		return *new internal::FunctionWrapper<true, R>(pointer);
	}

	template<typename R, typename... Args>
	static NativeFunction::FunctionWrapper& wrap(R(*pointer)(Args...))
	{
		return *new internal::FunctionWrapper<false, R, Args...>(pointer);
	}

	template<typename R, typename... Args>
	static NativeFunction::FunctionWrapper& wrap(R(*pointer)(const NativeFunction::Context*, Args...))
	{
		return *new internal::FunctionWrapper<true, R, Args...>(pointer);
	}


	template<typename CLASS, typename R>
	static NativeFunction::FunctionWrapper& wrap(CLASS& object, R(CLASS::*pointer)())
	{
		return *new internal::MethodWrapper<false, CLASS, R>(object, pointer);
	}

	template<typename CLASS, typename R>
	static NativeFunction::FunctionWrapper& wrap(CLASS& object, R(CLASS::*pointer)(const NativeFunction::Context*))
	{
		return *new internal::MethodWrapper<true, CLASS, R>(object, pointer);
	}

	template<typename CLASS, typename R, typename... Args>
	static NativeFunction::FunctionWrapper& wrap(CLASS& object, R(CLASS::*pointer)(Args...))
	{
		return *new internal::MethodWrapper<false, CLASS, R, Args...>(object, pointer);
	}

	template<typename CLASS, typename R, typename... Args>
	static NativeFunction::FunctionWrapper& wrap(CLASS& object, R(CLASS::*pointer)(const NativeFunction::Context*, Args...))
	{
		return *new internal::MethodWrapper<true, CLASS, R, Args...>(object, pointer);
	}

}
