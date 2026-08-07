/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/program/function/Function.h"


namespace lemon
{
	class ControlFlow;

	class NativeFunction : public Function
	{
	public:
		static const Type TYPE = Type::NATIVE;

	public:
		struct Context
		{
			inline explicit Context(ControlFlow& controlFlow) : mControlFlow(controlFlow) {}
			ControlFlow& mControlFlow;
		};

		class FunctionWrapper
		{
		public:
			virtual ~FunctionWrapper() {}
			virtual void execute(const Context context) const = 0;
			virtual const DataTypeDefinition* getReturnType() const = 0;
			virtual std::vector<const DataTypeDefinition*> getParameterTypes() const = 0;
		};

	public:
		inline NativeFunction() : Function(Type::NATIVE) {}
		inline virtual ~NativeFunction()  { delete mFunctionWrapper; }

		void setFunction(const FunctionWrapper& functionWrapper);
		NativeFunction& setParameterInfo(size_t index, const std::string& identifier);

		void execute(const Context context) const;

	public:
		const FunctionWrapper* mFunctionWrapper = nullptr;
	};

}
