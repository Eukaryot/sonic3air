/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/program/DataType.h"
#include "lemon/utility/FlyweightString.h"

#include <functional>


namespace lemon
{
	class ControlFlow;
	class Environment;


	class API_EXPORT Variable
	{
	friend class Module;
	friend class Runtime;
	friend class ScriptFunction;

	public:
		enum class Type : uint8
		{
			LOCAL    = 0,		// Variable IDs 0x00000000...0x0fffffff
			GLOBAL   = 1,		// Variable IDs 0x10000000...0x1fffffff
			USER     = 2,		// Variable IDs 0x20000000...0x2fffffff
			EXTERNAL = 3		// Variable IDs 0x30000000...0x3fffffff
		};

	public:
		inline Type getType() const							 { return mType; }
		inline FlyweightString getName() const				 { return mName; }
		inline uint32 getID() const							 { return mID; }
		inline const DataTypeDefinition* getDataType() const { return mDataType; }
		inline size_t getStaticMemoryOffset() const			 { return mStaticMemoryOffset; }
		inline size_t getStaticMemorySize() const			 { return mStaticMemorySize; }

	protected:
		inline Variable(Type type) : mType(type) {}
		virtual ~Variable() {}

	private:
		Type mType;
		FlyweightString mName;
		uint32 mID = 0;
		const DataTypeDefinition* mDataType = nullptr;
		size_t mStaticMemoryOffset = 0;
		size_t mStaticMemorySize = 0;
	};


	class API_EXPORT LocalVariable : public Variable
	{
	public:
		inline LocalVariable() : Variable(Type::LOCAL) {}

		// Local variables get accessed via the current state's variables stack
	};


	class API_EXPORT GlobalVariable : public Variable
	{
	public:
		inline GlobalVariable() : Variable(Type::GLOBAL) {}

		// Global variables get accessed via the runtime's global variables list

	public:
		int64 mInitialValue = 0;
	};


	class API_EXPORT UserDefinedVariable : public Variable
	{
	public:
		inline UserDefinedVariable() : Variable(Type::USER) {}

	public:
		// User defined variables get accessed using their getter and setter methods
		//  -> Note that these need to read from / write to the value stack, using the given runtime opcode context
		std::function<void(ControlFlow&)> mGetter;
		std::function<void(ControlFlow&)> mSetter;
	};


	class API_EXPORT ExternalVariable : public Variable
	{
	public:
		inline ExternalVariable() : Variable(Type::EXTERNAL) {}

	public:
		// External variables get accessed directly via the accessor the pointer, using the right data type
		std::function<int64*()> mAccessor;
	};

}
