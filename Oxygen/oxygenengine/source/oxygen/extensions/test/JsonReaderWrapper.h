/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <lemon/program/function/FunctionWrapper.h>

class JsonReader;


struct JsonReaderWrapper
{
	uint64 mHandle = 0;
	static inline const lemon::CustomDataType* mDataType = nullptr;

	JsonReaderWrapper() = default;
	JsonReaderWrapper(uint64 handle) : mHandle(handle) {}

	JsonReader* getJsonReader();
};


namespace lemon
{
	namespace traits
	{
		template<> inline const DataTypeDefinition* getDataType<JsonReaderWrapper>()  { return JsonReaderWrapper::mDataType; }
	}

	namespace internal
	{
		template<>
		struct StackHandler<JsonReaderWrapper>
		{
			static void pushStack(JsonReaderWrapper value, const NativeFunction::Context context)
			{
				context.mControlFlow.pushValueStack(value.mHandle);
			}

			static JsonReaderWrapper popStack(const NativeFunction::Context context)
			{
				return JsonReaderWrapper { context.mControlFlow.popValueStack<uint64>() };
			}
		};
	}
}
