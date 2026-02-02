/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/runtime/Runtime.h"
#include "lemon/runtime/RuntimeOpcode.h"
#include "lemon/utility/AnyBaseValue.h"


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
		FORCE_INLINE T readLocalVariable(size_t offset) const
		{
			return mControlFlow->readLocalVariable<T>(offset);
		}

		template<typename T>
		FORCE_INLINE void writeLocalVariable(size_t offset, T value) const
		{
			return mControlFlow->writeLocalVariable(offset, value);
		}
	};
}
