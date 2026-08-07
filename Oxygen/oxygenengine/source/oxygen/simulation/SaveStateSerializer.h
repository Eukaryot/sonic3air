/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>

class CodeExec;
class RenderParts;
class Simulation;


class SaveStateSerializer
{
public:
	enum class StateType : uint8
	{
		INVALID	= 0,
		OXYGEN	= 1,
		GENSX	= 2
	};

public:
	static inline uint32 mLastReadPC = 0;		// Only relevant after reading a Gensx emulator save state

public:
	SaveStateSerializer(Simulation& simulation, RenderParts& renderParts);

	bool loadState(const std::vector<uint8>& input, StateType* outStateType = nullptr);
	bool loadState(const std::wstring& filename, StateType* outStateType = nullptr);

	bool saveState(std::vector<uint8>& output);
	bool saveState(const std::wstring& filename);

private:
	bool serializeState(VectorBinarySerializer& serializer, StateType& stateType);
	bool readGensxState(VectorBinarySerializer& serializer);

private:
	Simulation& mSimulation;
	CodeExec& mCodeExec;
	RenderParts& mRenderParts;
};
