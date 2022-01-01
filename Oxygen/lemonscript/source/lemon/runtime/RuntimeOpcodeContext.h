/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/runtime/Runtime.h"
#include "lemon/runtime/RuntimeOpcode.h"


namespace lemon
{
	struct API_EXPORT RuntimeOpcodeContext
	{
		ControlFlow* mControlFlow = nullptr;
		const RuntimeOpcode* mOpcode = nullptr;

		template<typename T>
		FORCE_INLINE T getParameter() const
		{
			return mOpcode->getParameter<T>();
		}

		template<typename T>
		FORCE_INLINE T getParameter(size_t offset) const
		{
			return mOpcode->getParameter<T>(offset);
		}

		template<typename T>
		FORCE_INLINE T readValueStack(int offset) const
		{
			return mControlFlow->readValueStack<T>(offset);
		}

		template<typename T>
		FORCE_INLINE void writeValueStack(int offset, T value) const
		{
			mControlFlow->writeValueStack<T>(offset, value);
		}

		FORCE_INLINE void moveValueStack(int change) const
		{
			mControlFlow->moveValueStack(change);
		}

		template<typename T>
		FORCE_INLINE T readLocalVariable(size_t index) const
		{
			//return *(T*)&mControlFlow->mCurrentLocalVariables[index];
			return (T)mControlFlow->mCurrentLocalVariables[index];
		}

		template<typename T>
		FORCE_INLINE void writeLocalVariable(size_t index, T value) const
		{
			//*(T*)&mControlFlow->mCurrentLocalVariables[index] = value;	// TODO: Try this instead
			mControlFlow->mCurrentLocalVariables[index] = (T)value;
		}
	};
}
