/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/program/DataType.h"
#include "lemon/program/function/FunctionWrapper.h"
#include "lemon/program/Module.h"


namespace lemon
{
	class ModuleBindingsBuilder
	{
	public:
		class NativeFunctionBinding
		{
		public:
			explicit inline NativeFunctionBinding(NativeFunction& function) : mFunction(function) {}

			inline void setParameters(const std::string& param1)
			{
				RMX_CHECK(getNumParams() == 1, "Wrong number of parameters, function expects " << getNumParams(), );
				mFunction.setParameterInfo(0, param1);
			}

			inline void setParameters(const std::string& param1, const std::string& param2)
			{
				RMX_CHECK(getNumParams() == 2, "Wrong number of parameters, function expects " << getNumParams(), );
				mFunction.setParameterInfo(0, param1);
				mFunction.setParameterInfo(1, param2);
			}

			inline void setParameters(const std::string& param1, const std::string& param2, const std::string& param3)
			{
				RMX_CHECK(getNumParams() == 3, "Wrong number of parameters, function expects " << getNumParams(), );
				mFunction.setParameterInfo(0, param1);
				mFunction.setParameterInfo(1, param2);
				mFunction.setParameterInfo(2, param3);
			}

			inline void setParameters(const std::string& param1, const std::string& param2, const std::string& param3, const std::string& param4)
			{
				RMX_CHECK(getNumParams() == 4, "Wrong number of parameters, function expects " << getNumParams(), );
				mFunction.setParameterInfo(0, param1);
				mFunction.setParameterInfo(1, param2);
				mFunction.setParameterInfo(2, param3);
				mFunction.setParameterInfo(3, param4);
			}

			inline void setParameters(const std::string& param1, const std::string& param2, const std::string& param3, const std::string& param4, const std::string& param5)
			{
				RMX_CHECK(getNumParams() == 5, "Wrong number of parameters, function expects " << getNumParams(), );
				mFunction.setParameterInfo(0, param1);
				mFunction.setParameterInfo(1, param2);
				mFunction.setParameterInfo(2, param3);
				mFunction.setParameterInfo(3, param4);
				mFunction.setParameterInfo(4, param5);
			}

			inline void setParameters(const std::string& param1, const std::string& param2, const std::string& param3, const std::string& param4, const std::string& param5, const std::string& param6)
			{
				RMX_CHECK(getNumParams() == 6, "Wrong number of parameters, function expects " << getNumParams(), );
				mFunction.setParameterInfo(0, param1);
				mFunction.setParameterInfo(1, param2);
				mFunction.setParameterInfo(2, param3);
				mFunction.setParameterInfo(3, param4);
				mFunction.setParameterInfo(4, param5);
				mFunction.setParameterInfo(5, param6);
			}

			inline void setParameters(const std::string& param1, const std::string& param2, const std::string& param3, const std::string& param4, const std::string& param5, const std::string& param6, const std::string& param7)
			{
				RMX_CHECK(getNumParams() == 7, "Wrong number of parameters, function expects " << getNumParams(), );
				mFunction.setParameterInfo(0, param1);
				mFunction.setParameterInfo(1, param2);
				mFunction.setParameterInfo(2, param3);
				mFunction.setParameterInfo(3, param4);
				mFunction.setParameterInfo(4, param5);
				mFunction.setParameterInfo(5, param6);
				mFunction.setParameterInfo(6, param7);
			}

			inline void setParameters(const std::string& param1, const std::string& param2, const std::string& param3, const std::string& param4, const std::string& param5, const std::string& param6, const std::string& param7, const std::string& param8)
			{
				RMX_CHECK(getNumParams() == 8, "Wrong number of parameters, function expects " << getNumParams(), );
				mFunction.setParameterInfo(0, param1);
				mFunction.setParameterInfo(1, param2);
				mFunction.setParameterInfo(2, param3);
				mFunction.setParameterInfo(3, param4);
				mFunction.setParameterInfo(4, param5);
				mFunction.setParameterInfo(5, param6);
				mFunction.setParameterInfo(6, param7);
				mFunction.setParameterInfo(7, param8);
			}

			inline void setParameters(const std::string& param1, const std::string& param2, const std::string& param3, const std::string& param4, const std::string& param5, const std::string& param6, const std::string& param7, const std::string& param8, const std::string& param9)
			{
				RMX_CHECK(getNumParams() == 9, "Wrong number of parameters, function expects " << getNumParams(), );
				mFunction.setParameterInfo(0, param1);
				mFunction.setParameterInfo(1, param2);
				mFunction.setParameterInfo(2, param3);
				mFunction.setParameterInfo(3, param4);
				mFunction.setParameterInfo(4, param5);
				mFunction.setParameterInfo(5, param6);
				mFunction.setParameterInfo(6, param7);
				mFunction.setParameterInfo(7, param8);
				mFunction.setParameterInfo(8, param9);
			}

			inline void setParameters(const std::string& param1, const std::string& param2, const std::string& param3, const std::string& param4, const std::string& param5, const std::string& param6, const std::string& param7, const std::string& param8, const std::string& param9, const std::string& param10)
			{
				RMX_CHECK(getNumParams() == 10, "Wrong number of parameters, function expects " << getNumParams(), );
				mFunction.setParameterInfo(0, param1);
				mFunction.setParameterInfo(1, param2);
				mFunction.setParameterInfo(2, param3);
				mFunction.setParameterInfo(3, param4);
				mFunction.setParameterInfo(4, param5);
				mFunction.setParameterInfo(5, param6);
				mFunction.setParameterInfo(6, param7);
				mFunction.setParameterInfo(7, param8);
				mFunction.setParameterInfo(8, param9);
				mFunction.setParameterInfo(9, param10);
			}

			inline void setParameters(const std::string& param1, const std::string& param2, const std::string& param3, const std::string& param4, const std::string& param5, const std::string& param6, const std::string& param7, const std::string& param8, const std::string& param9, const std::string& param10, const std::string& param11)
			{
				RMX_CHECK(getNumParams() == 11, "Wrong number of parameters, function expects " << getNumParams(), );
				mFunction.setParameterInfo(0, param1);
				mFunction.setParameterInfo(1, param2);
				mFunction.setParameterInfo(2, param3);
				mFunction.setParameterInfo(3, param4);
				mFunction.setParameterInfo(4, param5);
				mFunction.setParameterInfo(5, param6);
				mFunction.setParameterInfo(6, param7);
				mFunction.setParameterInfo(7, param8);
				mFunction.setParameterInfo(8, param9);
				mFunction.setParameterInfo(9, param10);
				mFunction.setParameterInfo(10, param11);
			}

		private:
			NativeFunction& mFunction;

		private:
			inline size_t getNumParams() const  { return mFunction.getParameters().size(); }
		};

	public:
		explicit inline ModuleBindingsBuilder(Module& module) : mModule(module) {}

		template<typename T, typename S>
		inline void addConstant(FlyweightString name, S value)
		{
			mModule.addConstant(name, traits::getDataType<T>(), AnyBaseValue((T)value));
		}

		inline NativeFunctionBinding addNativeFunction(FlyweightString name, const NativeFunction::FunctionWrapper& functionWrapper, BitFlagSet<Function::Flag> flags = BitFlagSet<Function::Flag>())
		{
			NativeFunction& function = mModule.addNativeFunction(name, functionWrapper, flags);
			return NativeFunctionBinding(function);
		}

		inline NativeFunctionBinding addNativeMethod(FlyweightString context, FlyweightString name, const NativeFunction::FunctionWrapper& functionWrapper, BitFlagSet<Function::Flag> flags = BitFlagSet<Function::Flag>())
		{
			NativeFunction& function = mModule.addNativeMethod(context, name, functionWrapper, flags);
			return NativeFunctionBinding(function);
		}

	private:
		Module& mModule;
	};
}
