/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/compiler/backend/OpcodeOptimization.h"
#include "lemon/program/function/ScriptFunction.h"
#include "lemon/program/Opcode.h"


namespace lemon
{
	void OpcodeOptimization::optimizeOpcodes()
	{
		if (mOpcodes.empty())
			return;

		bool anotherRun = true;
		while (anotherRun)
		{
			anotherRun = false;

			// Build up a list of jump targets
			static std::vector<bool> isOpcodeJumpTarget;
			{
				isOpcodeJumpTarget.clear();
				isOpcodeJumpTarget.resize(mOpcodes.size(), false);

				// Collect jump targets
				for (const Opcode& opcode : mOpcodes)
				{
					if (opcode.mType == Opcode::Type::JUMP || opcode.mType == Opcode::Type::JUMP_CONDITIONAL)
					{
						if ((size_t)opcode.mParameter < isOpcodeJumpTarget.size())
						{
							isOpcodeJumpTarget[(size_t)opcode.mParameter] = true;
						}
					}
				}

				// Collect labels
				for (ScriptFunction::Label& label : mFunction.mLabels)
				{
					if ((size_t)label.mOffset < isOpcodeJumpTarget.size())
					{
						isOpcodeJumpTarget[(size_t)label.mOffset] = true;
					}
				}
			}

			// Look through all pairs of opcodes
			for (size_t i = 0; i < mOpcodes.size() - 1; ++i)
			{
				Opcode& opcode1 = mOpcodes[i];
				Opcode& opcode2 = mOpcodes[i+1];

				// They must be part of the same line
				if (opcode1.mLineNumber != opcode2.mLineNumber)
					continue;

				// Second opcode must not be a jump target
				if (isOpcodeJumpTarget[i+1])
					continue;

				// Cleanup: No need to make a comparison result boolean, it is already
				// TODO: This should not happen any more
				if (opcode1.mType >= Opcode::Type::COMPARE_EQ && opcode1.mType <= Opcode::Type::COMPARE_GE)
				{
					if (opcode2.mType == Opcode::Type::MAKE_BOOL)
					{
						opcode2.mType = Opcode::Type::NOP;
						anotherRun = true;
						continue;
					}
				}

				// Merge: No cast needed after constant
				if (opcode1.mType == Opcode::Type::PUSH_CONSTANT)
				{
					if (opcode2.mType == Opcode::Type::CAST_VALUE)
					{
						// TODO: Support conversions between integer, float, double constants
						//  -> Unless this is done in the compiler frontend already, which actually makes more sense...
						if (BaseTypeHelper::isPureIntegerBaseCast((BaseCastType)opcode2.mParameter))
						{
							switch (opcode2.mParameter & 0x13)
							{
								case 0x00:  opcode1.mParameter =  (uint8)opcode1.mParameter;  break;
								case 0x01:  opcode1.mParameter = (uint16)opcode1.mParameter;  break;
								case 0x02:  opcode1.mParameter = (uint32)opcode1.mParameter;  break;
								case 0x10:  opcode1.mParameter =   (int8)opcode1.mParameter;  break;
								case 0x11:  opcode1.mParameter =  (int16)opcode1.mParameter;  break;
								case 0x12:  opcode1.mParameter =  (int32)opcode1.mParameter;  break;
							}
							opcode2.mType = Opcode::Type::NOP;
							anotherRun = true;
							continue;
						}
					}
				}
			}

			cleanupNOPs();
		}

		// Collapse all chains of jumps
		for (size_t i = 0; i < mOpcodes.size(); ++i)
		{
			Opcode& startOpcode = mOpcodes[i];
			if (startOpcode.mType == Opcode::Type::JUMP || startOpcode.mType == Opcode::Type::JUMP_CONDITIONAL)
			{
				// Check if this (conditional or unconditional) jump leads to another unconditional jump
				size_t nextPosition = (size_t)startOpcode.mParameter;
				if (mOpcodes[nextPosition].mType == Opcode::Type::JUMP)
				{
					// Maybe the chain is longer, get the final target opcode of the chain
					do
					{
						nextPosition = (size_t)mOpcodes[nextPosition].mParameter;
					}
					while (mOpcodes[nextPosition].mType == Opcode::Type::JUMP);

					// Now the opcode at nextPosition is not a jump any more, this is our final target
					const size_t targetPosition = nextPosition;

					// Go through the chain again and let everyone point to the final target
					size_t currentPosition = i;
					do
					{
						nextPosition = (size_t)mOpcodes[currentPosition].mParameter;
						mOpcodes[currentPosition].mParameter = targetPosition;
						currentPosition = nextPosition;
					}
					while (currentPosition != targetPosition);
				}
			}
		}

		// Resolve conditional jumps that have a fixed condition at compile time already
		for (size_t i = 0; i < mOpcodes.size() - 1; ++i)
		{
			Opcode& firstOpcode = mOpcodes[i];
			if (firstOpcode.mType == Opcode::Type::PUSH_CONSTANT)
			{
				bool replace = false;
				size_t condJumpPosition = 0;

				const Opcode& secondOpcode = mOpcodes[i+1];
				if (secondOpcode.mType == Opcode::Type::JUMP_CONDITIONAL)
				{
					replace = true;
					condJumpPosition = i + 1;
				}
				else if (secondOpcode.mType == Opcode::Type::JUMP)
				{
					// Check the jump target, whether it's an unconditional jump
					const size_t jumpTarget = (size_t)secondOpcode.mParameter;
					const Opcode& thirdOpcode = mOpcodes[jumpTarget];
					if (thirdOpcode.mType == Opcode::Type::JUMP_CONDITIONAL)
					{
						replace = true;
						condJumpPosition = jumpTarget;
					}
				}

				if (replace)
				{
					const bool conditionMet = (firstOpcode.mParameter != 0);
					const Opcode& condJumpOpcode = mOpcodes[condJumpPosition];
					uint64 jumpTarget = conditionMet ? ((uint64)condJumpPosition + 1) : condJumpOpcode.mParameter;

					// Check for a shortcut (as this is not ruled out at that point)
					if (mOpcodes[(size_t)jumpTarget].mType == Opcode::Type::JUMP)
					{
						jumpTarget = mOpcodes[(size_t)jumpTarget].mParameter;
					}

					// Replace the first opcode with a jump directly to where the (now not really) conditional jump would lead to
					firstOpcode.mType = Opcode::Type::JUMP;
					firstOpcode.mDataType = BaseType::VOID;
					firstOpcode.mFlags.set(makeBitFlagSet(Opcode::Flag::CTRLFLOW, Opcode::Flag::JUMP, Opcode::Flag::SEQ_BREAK));
					firstOpcode.mParameter = jumpTarget;
					firstOpcode.mLineNumber = condJumpOpcode.mLineNumber;
				}
			}
		}

		// Replace jumps that only lead to a returning opcode
		for (size_t i = 0; i < mOpcodes.size(); ++i)
		{
			Opcode& opcode = mOpcodes[i];
			if (opcode.mType == Opcode::Type::JUMP)
			{
				Opcode& nextOpcode = mOpcodes[(size_t)opcode.mParameter];
				if (nextOpcode.mType == Opcode::Type::RETURN || nextOpcode.mType == Opcode::Type::EXTERNAL_JUMP)
				{
					// Copy the return (or external jump) over the jump
					opcode = nextOpcode;
				}
			}
		}

		// With the optimizations above, there might be opcodes now that are unreachable
		//  -> Replace them with NOPs
		//  -> Check for unnecessary jumps, that would just to skip the NOPs, and NOP them as well
		//  -> And finally, clean up all NOPs once again
		{
			// Mark all opcodes as not visited yet, using the temp flag -- except for the very last return, which must never get removed
			for (size_t i = 0; i < mOpcodes.size() - 1; ++i)
			{
				mOpcodes[i].mFlags.set(Opcode::Flag::TEMP_FLAG);
			}

			static std::vector<size_t> openSeeds;
			openSeeds.clear();
			openSeeds.push_back(0);
			for (const ScriptFunction::Label& label : mFunction.mLabels)
			{
				openSeeds.push_back((size_t)label.mOffset);
			}

			// Trace all reachable opcodes from our seeds
			while (!openSeeds.empty())
			{
				size_t position = openSeeds.back();
				openSeeds.pop_back();

				while (mOpcodes[position].mFlags.isSet(Opcode::Flag::TEMP_FLAG))
				{
					mOpcodes[position].mFlags.clear(Opcode::Flag::TEMP_FLAG);
					switch (mOpcodes[position].mType)
					{
						case Opcode::Type::JUMP:
							position = (size_t)mOpcodes[position].mParameter;
							break;

						case Opcode::Type::JUMP_CONDITIONAL:
							openSeeds.push_back((size_t)mOpcodes[position].mParameter);
							++position;
							break;

						case Opcode::Type::EXTERNAL_JUMP:
						case Opcode::Type::RETURN:
							// Just do nothing, this will exit the while loop automatically
							break;

						default:
							++position;
							break;
					}
				}
			}

			// Remove opcodes that are unreachable
			for (Opcode& opcode : mOpcodes)
			{
				if (opcode.mFlags.isSet(Opcode::Flag::TEMP_FLAG))
				{
					opcode.mType = Opcode::Type::NOP;
				}
			}

			// Remove unnecessary jumps, namely those that only skip a bunch of NOPs (or just lead to the very next opcode)
			if (mOpcodes.size() >= 3)
			{
				for (size_t i = 0; i < mOpcodes.size() - 1; ++i)
				{
					if (mOpcodes[i].mType == Opcode::Type::JUMP || mOpcodes[i].mType == Opcode::Type::JUMP_CONDITIONAL)
					{
						const size_t jumpTarget = (size_t)mOpcodes[i].mParameter;
						size_t position = i + 1;
						if (jumpTarget >= position)
						{
							while (mOpcodes[position].mType == Opcode::Type::NOP)
								++position;

							// Check if the jump target is at the position after the NOPs, or right into one of the NOPs
							if (jumpTarget <= position)
							{
								if (mOpcodes[i].mType == Opcode::Type::JUMP_CONDITIONAL)
								{
									// The conditional jump at the start is not needed any more, but we need to consume one item from the stack (namely the condition)
									mOpcodes[i].mType = Opcode::Type::MOVE_STACK;
									mOpcodes[i].mDataType = BaseType::VOID;			// Because that's what we generally use for MOVE_STACK
									mOpcodes[i].mParameter = -1;
									mOpcodes[i].mFlags.setOnly(Opcode::Flag::NEW_LINE);	// Clear all other flags
								}
								else
								{
									// The jump at the start is not needed any more, NOP it so it gets removed
									mOpcodes[i].mType = Opcode::Type::NOP;
								}
								i = position - 1;
							}
						}
					}
				}
			}

			cleanupNOPs();
		}
	}

	void OpcodeOptimization::cleanupNOPs()
	{
		// Remove all NOPs and update all jump targets etc. appropriately
		static std::vector<int> indexRemap;
		indexRemap.clear();
		indexRemap.resize(mOpcodes.size());
		size_t newSize = 0;
		for (size_t i = 0; i < mOpcodes.size(); ++i)
		{
			indexRemap[i] = (int)newSize;
			if (mOpcodes[i].mType != Opcode::Type::NOP)
			{
				++newSize;
			}
		}

		if (newSize < mOpcodes.size())
		{
			const int lastOpcode = (int)newSize - 1;	// Point to last opcode, which is a return in any case

			// Move opcodes
			for (size_t i = 0; i < mOpcodes.size(); ++i)
			{
				if ((int)i != indexRemap[i] && mOpcodes[i].mType != Opcode::Type::NOP)
				{
					mOpcodes[indexRemap[i]] = mOpcodes[i];
				}
			}

			// Update jump targets
			for (size_t i = 0; i < newSize; ++i)
			{
				Opcode& opcode = mOpcodes[i];
				if (opcode.mType == Opcode::Type::JUMP || opcode.mType == Opcode::Type::JUMP_CONDITIONAL)
				{
					opcode.mParameter = ((size_t)opcode.mParameter < indexRemap.size()) ? indexRemap[(size_t)opcode.mParameter] : lastOpcode;
					RMX_ASSERT(opcode.mParameter != -1, "Invalid opcode parameter");
				}
			}

			// Update labels
			for (ScriptFunction::Label& label : mFunction.mLabels)
			{
				label.mOffset = ((size_t)label.mOffset < indexRemap.size()) ? (uint32)indexRemap[(size_t)label.mOffset] : (uint32)lastOpcode;
			}

			mOpcodes.resize(newSize);
		}
	}
}
