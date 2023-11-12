/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/runtime/RuntimeFunction.h"
#include "lemon/runtime/Runtime.h"
#include "lemon/runtime/OpcodeExecUtils.h"
#include "lemon/runtime/OpcodeProcessor.h"
#include "lemon/runtime/provider/DefaultOpcodeProvider.h"
#include "lemon/runtime/provider/OptimizedOpcodeProvider.h"
#include "lemon/runtime/provider/NativizedOpcodeProvider.h"
#include "lemon/program/Program.h"


namespace lemon
{

	RuntimeOpcodeBuffer::~RuntimeOpcodeBuffer()
	{
		if (mSelfManagedBuffer)
			delete[] mBuffer;
	}

	void RuntimeOpcodeBuffer::clear()
	{
		mSize = 0;
		mOpcodePointers.clear();
		// Not touching the reserved memory here
	}

	void RuntimeOpcodeBuffer::reserveForOpcodes(size_t numOpcodes)
	{
		const size_t memoryRequired = numOpcodes * (sizeof(RuntimeOpcode) + 16);	// Estimate for maximum size
		if (memoryRequired > mReserved)
		{
			if (mSelfManagedBuffer)
				delete[] mBuffer;

			mReserved = memoryRequired;
			mBuffer = new uint8[mReserved];
			mSelfManagedBuffer = true;
		}
	}

	RuntimeOpcode& RuntimeOpcodeBuffer::addOpcode(size_t parameterSize)
	{
		const size_t size = sizeof(RuntimeOpcode) + parameterSize;
		RMX_ASSERT(mSize + size <= mReserved, "Exceeding reserved size of runtime opcode buffer");
		RMX_ASSERT(size <= 0xc0, "Got large parameter size of " << parameterSize << " bytes");		// Actual hard limit is 0xff, but everything larger than 0xc0 is suspicious and a hint that the limit might be too low

		uint8* opcodePointer = mBuffer + mSize;
		mSize += size;
		mOpcodePointers.push_back((RuntimeOpcode*)opcodePointer);

		// Setup some default values
		RuntimeOpcode& runtimeOpcode = *(RuntimeOpcode*)opcodePointer;
		runtimeOpcode.mExecFunc = nullptr;
		runtimeOpcode.mOpcodeType = Opcode::Type::NOP;
		runtimeOpcode.mSize = (uint8)size;
		runtimeOpcode.mFlags.clearAll();
		runtimeOpcode.mSuccessiveHandledOpcodes = 1;
		return runtimeOpcode;
	}

	void RuntimeOpcodeBuffer::copyFrom(const RuntimeOpcodeBuffer& other, rmx::OneTimeAllocPool& memoryPool)
	{
		if (mSelfManagedBuffer)
			delete[] mBuffer;

		mBuffer = memoryPool.allocateMemory(other.mSize);
		mSelfManagedBuffer = false;
		mSize = other.mSize;
		mReserved = other.mSize;
		memcpy(mBuffer, other.mBuffer, mSize);

		mOpcodePointers.resize(other.mOpcodePointers.size());
		for (size_t k = 0; k < mOpcodePointers.size(); ++k)
		{
			const size_t offset = (size_t)((uint8*)other.mOpcodePointers[k] - other.mBuffer);
			mOpcodePointers[k] = (RuntimeOpcode*)(mBuffer + offset);
		}
	}


	void RuntimeFunction::build(Runtime& runtime)
	{
		// First check if it is built already
		if (!mRuntimeOpcodeBuffer.empty() || mFunction->mOpcodes.empty())
			return;

		// Create the runtime opcodes
		{
			// Initialize runtime opcodes now that they are needed
			const std::vector<Opcode>& opcodes = mFunction->mOpcodes;
			const size_t numOpcodes = opcodes.size();

			// Preparation: Build some useful information about opcodes
			static std::vector<OpcodeProcessor::OpcodeData> opcodeData;
			OpcodeProcessor::buildOpcodeData(opcodeData, *mFunction);

			// Using a static buffer as temporary buffer before knowing the final size
			static RuntimeOpcodeBuffer tempBuffer;
			tempBuffer.clear();
			tempBuffer.reserveForOpcodes(numOpcodes);

			mProgramCounterByOpcodeIndex.resize(numOpcodes, 0xffffffff);

			// Let the opcode providers create runtime opcodes
			//  -> They may choose to merge more than one opcode into a runtime opcode, where that's feasible
			for (size_t i = 0; i < numOpcodes; )
			{
				const size_t start = tempBuffer.size();

				int numOpcodesConsumed = 1;
				createRuntimeOpcode(tempBuffer, &opcodes[i], opcodeData[i].mRemainingSequenceLength, (int)i, numOpcodesConsumed, runtime);
				for (int k = 0; k < numOpcodesConsumed; ++k)
				{
					mProgramCounterByOpcodeIndex[k + i] = start;
				}
				i += numOpcodesConsumed;
			}

			// Copy the runtime opcodes over into the actual opcode buffer for this function
			mRuntimeOpcodeBuffer.copyFrom(tempBuffer, runtime.mRuntimeOpcodesPool);
		}

		// Post-processing
		{
			// Translation of jumps
			const std::vector<RuntimeOpcode*>& runtimeOpcodePointers = mRuntimeOpcodeBuffer.getOpcodePointers();
			for (size_t i = 0; i < runtimeOpcodePointers.size(); ++i)
			{
				RuntimeOpcode& runtimeOpcode = *runtimeOpcodePointers[i];
				if (runtimeOpcode.mOpcodeType == Opcode::Type::JUMP || runtimeOpcode.mOpcodeType == Opcode::Type::JUMP_SWITCH)
				{
					runtimeOpcode.setParameter(translateJumpTarget(runtimeOpcode.getParameter<uint32>()));
				}
				else if (runtimeOpcode.mOpcodeType == Opcode::Type::JUMP_CONDITIONAL)
				{
					runtimeOpcode.setParameter(translateJumpTarget(runtimeOpcode.getParameter<uint32>(0)), 0);
				#ifdef USE_JUMP_CONDITIONAL_RUNTIME_EXEC
					runtimeOpcode.setParameter(translateJumpTarget(runtimeOpcode.getParameter<uint32>(8)), 8);
				#endif
				}
			}

			// Update successive handled opcode counts
			uint8 sequenceLength = 0;
			for (int i = (int)runtimeOpcodePointers.size()-1; i >= 0; --i)
			{
				if (runtimeOpcodePointers[i]->mSuccessiveHandledOpcodes == 0)
				{
					sequenceLength = 0;
				}
				else if (runtimeOpcodePointers[i]->mOpcodeType == Opcode::Type::JUMP_CONDITIONAL)
				{
					sequenceLength = 1;		// Sequence needs to stop after executing the conditional jump
				}
				else
				{
					if (sequenceLength < 0xff)
						++sequenceLength;
				}
				runtimeOpcodePointers[i]->mSuccessiveHandledOpcodes = sequenceLength;
			}

			// Fill in the pointers to the next opcode
			for (size_t i = 0; i < runtimeOpcodePointers.size() - 1; ++i)
			{
				RuntimeOpcode& runtimeOpcode = *runtimeOpcodePointers[i];
				runtimeOpcode.mNext = (RuntimeOpcode*)((uint8*)&runtimeOpcode + (size_t)runtimeOpcode.mSize);

				for (int runs = 0; runs < 5; ++runs)
				{
					if (runtimeOpcode.mNext->mOpcodeType != Opcode::Type::JUMP)
						break;

					// Take a shortcut by skipping the jump opcode and directly pointing to its target as next opcode
					//  -> But only do that for jumps forward, otherwise it's possible that script execution can get stuck in an infinite loop
					//  -> That's because counted steps are only checked in actually executed jumps, but not in those that we optimize away here
					RuntimeOpcode* targetPointer = reinterpret_cast<RuntimeOpcode*>(runtimeOpcode.mNext->getParameter<uint64>());
					RuntimeOpcode* ownPointer = &runtimeOpcode;
					if (targetPointer <= ownPointer)
						break;

					runtimeOpcode.mNext = targetPointer;
					// Continue the for-loop, in case mNext is yet another jump that can be resolved by a shortcut
				}
			}
		}
	}

	size_t RuntimeFunction::translateFromRuntimeProgramCounter(const uint8* runtimeProgramCounter) const
	{
		const int result = translateFromRuntimeProgramCounterOptional(runtimeProgramCounter);
		RMX_ASSERT(result >= 0, "Program counter couldn't be translated");
		return (size_t)std::max(result, 0);
	}

	int RuntimeFunction::translateFromRuntimeProgramCounterOptional(const uint8* runtimeProgramCounter) const
	{
		// Binary search
		if (mProgramCounterByOpcodeIndex.empty())
			return -1;

		const size_t programCounter = (size_t)(runtimeProgramCounter - getFirstRuntimeOpcode());
		size_t minimum = 0;
		size_t maximum = mProgramCounterByOpcodeIndex.size() - 1;
		while (minimum <= maximum)
		{
			const size_t median = (minimum + maximum) / 2;
			if (programCounter < mProgramCounterByOpcodeIndex[median])
			{
				maximum = median - 1;
			}
			else if (programCounter > mProgramCounterByOpcodeIndex[median])
			{
				minimum = median + 1;
			}
			else
			{
				return (int)median;
			}
		}
		return -1;
	}

	const uint8* RuntimeFunction::translateToRuntimeProgramCounter(size_t originalProgramCounter) const
	{
		const size_t index = (originalProgramCounter < mProgramCounterByOpcodeIndex.size()) ? mProgramCounterByOpcodeIndex[originalProgramCounter] : 0;
		return &mRuntimeOpcodeBuffer[index];
	}

	void RuntimeFunction::createRuntimeOpcode(RuntimeOpcodeBuffer& buffer, const Opcode* opcodes, int numOpcodesAvailable, int firstOpcodeIndex, int& outNumOpcodesConsumed, const Runtime& runtime)
	{
		const Program& program = runtime.getProgram();
		if (program.getOptimizationLevel() >= 2 && nullptr != program.mNativizedOpcodeProvider)
		{
			const bool success = program.mNativizedOpcodeProvider->buildRuntimeOpcode(buffer, opcodes, numOpcodesAvailable, firstOpcodeIndex, outNumOpcodesConsumed, runtime);
			if (success)
				return;
		}

		// Runtime opcode generation by merging multiple opcodes where possible
		if (program.getOptimizationLevel() >= 1)
		{
			const bool success = OptimizedOpcodeProvider::buildRuntimeOpcodeStatic(buffer, opcodes, numOpcodesAvailable, firstOpcodeIndex, outNumOpcodesConsumed, runtime);
			if (success)
				return;
		}

		// Fallback: Direct translation of one opcode to the respective runtime opcode
		DefaultOpcodeProvider::buildRuntimeOpcodeStatic(buffer, opcodes, numOpcodesAvailable, firstOpcodeIndex, outNumOpcodesConsumed, runtime);
	}

	const uint8* RuntimeFunction::translateJumpTarget(uint32 targetOpcodeIndex) const
	{
		const size_t oldJumpTarget = (size_t)targetOpcodeIndex;
		if (oldJumpTarget < mProgramCounterByOpcodeIndex.size())
		{
			return mRuntimeOpcodeBuffer.getStart() + mProgramCounterByOpcodeIndex[oldJumpTarget];
		}
		else
		{
			RMX_ASSERT(mRuntimeOpcodeBuffer.getOpcodePointers().back()->mOpcodeType == Opcode::Type::RETURN, "Functions must end with a return in all cases");
			return (const uint8*)mRuntimeOpcodeBuffer.getOpcodePointers().back();
		}
	}

}
