/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


struct InputConfig
{
public:
	enum class DeviceType
	{
		KEYBOARD,
		GAMEPAD
	};

	struct Assignment
	{
		enum class Type
		{
			BUTTON,		// Gamepad button or keyboard key, using internal index starting with 0
			AXIS,		// Gamepad axis, using internal index times 2, and added 1 for negative axis
			POV			// Gamepad pov / hat, using bitmask in lower 8 bits of index, and the hat index (usually 0) in upper bits
		};

		Type mType = Type::BUTTON;
		uint32 mIndex = 0;

		inline Assignment()  {}
		inline Assignment(Type type, uint32 index) : mType(type), mIndex(index)  {}
		inline bool operator==(const Assignment& other) const  { return (mType == other.mType && mIndex == other.mIndex); }

		void getMappingString(String& outString, DeviceType deviceType) const;
		static bool setFromMappingString(Assignment& output, const String& mappingString, DeviceType deviceType);
	};

	struct ControlMapping	// This represents the mapping of a single control, which may have multiple assignments
	{
		std::vector<Assignment> mAssignments;
		size_t mNumFixedAssignments = 0;		// For keyboard 1, some controls have fixed assignments that can't be changed
	};

	struct DeviceDefinition
	{
		enum class Button
		{
			UP = 0,
			DOWN,
			LEFT,
			RIGHT,
			A,
			B,
			X,
			Y,
			START,
			BACK,
			L,
			R,
			_NUM
		};

		static const size_t NUM_BUTTONS = (size_t)Button::_NUM;
		static const std::string BUTTON_NAME[NUM_BUTTONS];

		DeviceType mDeviceType = DeviceType::KEYBOARD;
		std::string mIdentifier;
		std::map<uint64, std::string> mDeviceNames;		// Uses string hash as key
		ControlMapping mMappings[NUM_BUTTONS];
	};

public:
	static void setupDefaultDeviceDefinitions(std::vector<DeviceDefinition>& outDeviceDefinitions);
	static void setupDefaultKeyboardMappings(DeviceDefinition& outDeviceDefinition, int keyboardIndex);

	static void clearAssignments(DeviceDefinition& deviceDefinition, size_t buttonIndex);
	static void addAssignment(DeviceDefinition& deviceDefinition, size_t buttonIndex, const Assignment& newAssignment, bool removeDuplicates);
	static void setAssignments(DeviceDefinition& deviceDefinition, size_t buttonIndex, const std::vector<Assignment>& assignments, bool removeDuplicates);

private:
	static ControlMapping& getMapping(DeviceDefinition& deviceDefinition, size_t buttonIndex);
};
