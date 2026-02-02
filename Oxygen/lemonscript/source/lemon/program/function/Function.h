/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/program/function/FunctionReference.h"
#include "lemon/program/DataType.h"
#include "lemon/utility/FlyweightString.h"


namespace lemon
{

	class API_EXPORT Function
	{
	friend class Module;
	friend class ModuleSerializer;

	public:
		enum class Type : uint8
		{
			SCRIPT,
			NATIVE
		};

		struct Parameter
		{
			const DataTypeDefinition* mDataType = nullptr;
			FlyweightString mName;
		};
		typedef std::vector<Parameter> ParameterList;

		struct SignatureBuilder
		{
			void clear(const DataTypeDefinition& returnType);
			void addParameterType(const DataTypeDefinition& dataType);
			uint32 getSignatureHash();

			std::vector<uint32> mData;
		};

		struct AliasName
		{
			FlyweightString mName;
			bool mIsDeprecated = false;
		};

		enum class Flag
		{
			ALLOW_INLINE_EXECUTION	 = 0x01,	// Native only: Function can be called directly inside the opcode run loop and does not interfere with control flow
			COMPILE_TIME_CONSTANT	 = 0x02,	// Native only: Function only does calculation on the parameters, does not read from run-time sources and has no side-effects
			DEPRECATED				 = 0x04,	// Function is marked as deprecated
			EXCLUDE_FROM_DEFINITIONS = 0x80,	// Don't show in function listings produced by "Module::dumpDefinitionsToScriptFile"
		};

	public:
		static uint32 getVoidSignatureHash();

	public:
		inline Type getType() const  { return mType; }
		inline uint32 getID() const  { return mID; }

		template<typename T> bool isA() const		{ return getType() == T::TYPE; }
		template<typename T> T& as()				{ return static_cast<T&>(*this); }
		template<typename T> const T& as() const	{ return static_cast<const T&>(*this); }
		template<typename T> T* cast()				{ return isA<T>() ? static_cast<T*>(*this) : nullptr; }
		template<typename T> const T* cast() const	{ return isA<T>() ? static_cast<const T*>(*this) : nullptr; }

		inline BitFlagSet<Flag> getFlags() const  { return mFlags; }
		inline bool hasFlag(Flag flag) const	  { return mFlags.isSet(flag); }

		inline FlyweightString getContext() const { return mContext; }
		inline FlyweightString getName() const    { return mName; }
		inline uint64 getNameAndSignatureHash() const { return mNameAndSignatureHash; }
		inline const std::vector<AliasName>& getAliasNames() const { return mAliasNames; }

		const DataTypeDefinition* getReturnType() const  { return mReturnType; }
		const ParameterList& getParameters() const  { return mParameters; }

		uint32 getSignatureHash() const;

	protected:
		inline Function(Type type) : mType(type) {}
		inline virtual ~Function() {}

		void setParametersByTypes(const std::vector<const DataTypeDefinition*>& parameterTypes);

	protected:
		Type mType;
		uint32 mID = 0;
		BitFlagSet<Flag> mFlags;

		// Metadata
		FlyweightString mContext;		// Name of the type if this is a method-like function
		FlyweightString mName;
		uint64 mNameAndSignatureHash = 0;
		std::vector<AliasName> mAliasNames;

		// Signature
		const DataTypeDefinition* mReturnType = &PredefinedDataTypes::VOID;
		ParameterList mParameters;
		mutable uint32 mSignatureHash = 0;
	};

}
