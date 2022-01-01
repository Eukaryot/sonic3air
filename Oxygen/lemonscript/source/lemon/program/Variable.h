/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/program/DataType.h"
#include <functional>


namespace lemon
{

	class API_EXPORT Variable
	{
	friend class Module;
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
		virtual int64 getValue() const = 0;
		virtual void setValue(int64 value) = 0;

		inline Type getType() const  { return mType; }
		inline const std::string& getName() const  { return mName; }
		inline uint64 getNameHash() const  { return mNameHash; }
		inline uint32 getId() const  { return mId; }
		inline const DataTypeDefinition* getDataType() const  { return mDataType; }

	protected:
		inline Variable(Type type) : mType(type) {}

	private:
		Type mType;
		std::string mName;
		uint64 mNameHash = 0;
		uint32 mId = 0;
		const DataTypeDefinition* mDataType = nullptr;
	};


	class API_EXPORT LocalVariable : public Variable
	{
	public:
		inline LocalVariable() : Variable(Type::LOCAL) {}

		// Do not use these for variables, instead look it up in the current state's variables stack
		int64 getValue() const override		 { return 0; }
		void setValue(int64 value) override  {}
	};


	class API_EXPORT GlobalVariable : public Variable
	{
	public:
		inline GlobalVariable() : Variable(Type::GLOBAL) {}

		// Do not use these for variables, instead look it up in the runtime's global variables list
		int64 getValue() const override		 { return 0; }
		void setValue(int64 value) override  {}

	public:
		int64 mInitialValue = 0;
	};


	class API_EXPORT UserDefinedVariable : public Variable
	{
	public:
		inline UserDefinedVariable() : Variable(Type::USER) {}

		int64 getValue() const override		 { return (mGetter) ? mGetter() : 0; }
		void setValue(int64 value) override  { if (mSetter) mSetter(value); }

	public:
		std::function<int64()> mGetter;
		std::function<void(int64)> mSetter;
	};


	class API_EXPORT ExternalVariable : public Variable
	{
	public:
		inline ExternalVariable() : Variable(Type::EXTERNAL) {}

		// Do not use these for variables, instead directly access the pointer with the right data type
		int64 getValue() const override		 { return 0; }
		void setValue(int64 value) override  {}

	public:
		void* mPointer = nullptr;
	};

}
