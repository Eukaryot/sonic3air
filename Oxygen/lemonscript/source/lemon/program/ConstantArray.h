/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/utility/AnyBaseValue.h"
#include "lemon/utility/FlyweightString.h"


namespace lemon
{
	struct DataTypeDefinition;

	class API_EXPORT ConstantArray
	{
	friend class Module;

	public:
		inline FlyweightString getName() const  { return mName; }
		inline const DataTypeDefinition* getElementDataType() const  { return mElementDataType; }
		inline uint32 getID() const  { return mID; }

		inline size_t getSize() const  { return mData.size(); }
		void setSize(size_t size);

		void setContent(const AnyBaseValue* values, size_t size);

		const AnyBaseValue& getElement(size_t index) const;
		void setElement(size_t index, const AnyBaseValue& value);

		void serializeData(VectorBinarySerializer& serializer);

	private:
		FlyweightString mName;
		const DataTypeDefinition* mElementDataType = nullptr;
		uint32 mID = 0;
		std::vector<AnyBaseValue> mData;
	};
}
