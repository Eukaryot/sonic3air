/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/compiler/backend/FunctionCompiler.h"
#include "lemon/compiler/backend/OpcodeOptimization.h"
#include "lemon/compiler/Node.h"
#include "lemon/compiler/TokenTypes.h"
#include "lemon/compiler/TypeCasting.h"
#include "lemon/compiler/Utility.h"
#include "lemon/program/Program.h"


namespace lemon
{
	namespace
	{
		bool isComparison(const Token& token)
		{
			if (token.isA<BinaryOperationToken>())
			{
				const Operator op = token.as<BinaryOperationToken>().mOperator;
				return (op >= Operator::COMPARE_EQUAL && op <= Operator::COMPARE_GREATER_OR_EQUAL);
			}
			return false;
		}

		bool isCommutative(Operator op)
		{
			return (op == Operator::BINARY_PLUS || op == Operator::BINARY_MULTIPLY ||
					op == Operator::BINARY_AND || op == Operator::BINARY_OR || op == Operator::BINARY_XOR ||
					op == Operator::COMPARE_EQUAL || op == Operator::COMPARE_NOT_EQUAL);
		}
	}


	struct OpcodeBuilder
	{
	public:
		OpcodeBuilder(FunctionCompiler& functionCompiler) :
			mFunctionCompiler(functionCompiler)
		{}

		void beginIf()
		{
			mIfJumpOpcodeIndex = mFunctionCompiler.mOpcodes.size();
			mFunctionCompiler.addOpcode(Opcode::Type::JUMP_CONDITIONAL);	// Target position must be set afterwards when if block is complete
		}

		void beginElse()
		{
			// Conditional jump to the end of the else-part
			mElseJumpOpcodeIndex = mFunctionCompiler.mOpcodes.size();
			mFunctionCompiler.addOpcode(Opcode::Type::JUMP);				// Target position must be set afterwards when else block is complete

			// Correct target position of if-jump
			mFunctionCompiler.mOpcodes[mIfJumpOpcodeIndex].mParameter = mFunctionCompiler.mOpcodes.size();
		}

		void endIf()
		{
			// Correct target position of either if-jump or else-jump
			const bool hadElsePart = (mElseJumpOpcodeIndex != 0);
			if (hadElsePart)
			{
				mFunctionCompiler.mOpcodes[mElseJumpOpcodeIndex].mParameter = mFunctionCompiler.mOpcodes.size();
			}
			else
			{
				mFunctionCompiler.mOpcodes[mIfJumpOpcodeIndex].mParameter = mFunctionCompiler.mOpcodes.size();
			}
		}

	private:
		FunctionCompiler& mFunctionCompiler;
		size_t mIfJumpOpcodeIndex = 0;
		size_t mElseJumpOpcodeIndex = 0;
	};


	FunctionCompiler::FunctionCompiler(ScriptFunction& function, const CompileOptions& compileOptions, const GlobalsLookup& globalsLookup) :
		mFunction(function),
		mCompileOptions(compileOptions),
		mGlobalsLookup(globalsLookup),
		mOpcodes(function.mOpcodes)
	{
	}

	void FunctionCompiler::processParameters()
	{
		// Check if anything to do at all (note that parameters are also local variables)
		if (mFunction.mLocalVariablesByID.empty())
			return;

		mLineNumber = mFunction.mStartLineNumber;

		// Create scope
		RMX_ASSERT((mFunction.mLocalVariablesMemorySize % 8) == 0, "Expected local variables total size to be a multiple of 8 bytes");
		addOpcode(Opcode::Type::MOVE_VAR_STACK, mFunction.mLocalVariablesMemorySize / 8);

		// Go through parameters in reverse order
		for (int index = (int)mFunction.getParameters().size() - 1; index >= 0; --index)
		{
			const Function::Parameter& parameter = mFunction.getParameters()[index];
			const Variable* variable = mFunction.getLocalVariableByIdentifier(parameter.mName.getHash());
			RMX_ASSERT(nullptr != variable, "Variable not found");
			RMX_ASSERT(variable->getDataType() == parameter.mDataType, "Variable has wrong data type");

			// Assume the variable value is on the stack
			addOpcode(Opcode::Type::SET_VARIABLE_VALUE, variable->getDataType(), variable->getID());

			// Pop value from stack (as SET_VARIABLE_VALUE opcode does not consume it)
			const int sizeOnStack = (int)variable->getDataType()->getSizeOnStack();
			RMX_ASSERT(sizeOnStack != 0, "Invalid stack size of type " << variable->getDataType()->getName().getString());
			addMoveStackOpcode(-sizeOnStack);
		}
	}

	void FunctionCompiler::buildOpcodesForFunction(const BlockNode& blockNode)
	{
		// Build opcodes for the block
		NodeContext context;
		buildOpcodesFromNodes(blockNode, context);

		// Process all jumps to labels
		for (const auto& [key, collectedLabel] : mCollectedLabels)
		{
			for (size_t jumpLocation : collectedLabel.mJumpLocations)
			{
				RMX_ASSERT(mOpcodes[jumpLocation].mType == Opcode::Type::JUMP, "Expected JUMP opcode");
				const ScriptFunction::Label* label = mFunction.findLabelByName(collectedLabel.mLabelName);
				if (nullptr == label)
				{
					RMX_ASSERT(false, "Jump target label not found: " << collectedLabel.mLabelName.getString());
					continue;
				}
				mOpcodes[jumpLocation].mParameter = (uint64)label->mOffset;
			}
		}

		// Make sure it ends with a return in any case
		if (mOpcodes.empty() || mOpcodes.back().mType != Opcode::Type::RETURN)
		{
			CHECK_ERROR(mFunction.getReturnType()->isA<VoidDataType>(), "Function '" << mFunction.getName() << "' must return a " << mFunction.getReturnType()->getName() << " value", blockNode.getLineNumber());
			addOpcode(Opcode::Type::RETURN);
		}
		else
		{
			// There could be jumps leading to the position after the return, correct them
			//  -> E.g. when the function ends with an if-block that itself ends with a return
			for (Opcode& opcode : mOpcodes)
			{
				if (opcode.mType == Opcode::Type::JUMP || opcode.mType == Opcode::Type::JUMP_CONDITIONAL)
				{
					if ((size_t)opcode.mParameter >= mOpcodes.size())
					{
						opcode.mParameter = (uint64)mOpcodes.size() - 1;
					}
				}
			}
		}

		// Optimize the whole thing
		OpcodeOptimization opcodeOptimization(mFunction, mOpcodes);
		opcodeOptimization.optimizeOpcodes();

		// Determine opcode flags
		assignOpcodeFlags();
	}

	Opcode& FunctionCompiler::addOpcode(Opcode::Type type, int64 parameter)
	{
		return addOpcode(type, BaseType::VOID, parameter);
	}

	Opcode& FunctionCompiler::addOpcode(Opcode::Type type, BaseType dataType, int64 parameter)
	{
		// Remove signed flag for certain opcodes where it makes no difference
		if (type == Opcode::Type::GET_VARIABLE_VALUE || type == Opcode::Type::SET_VARIABLE_VALUE ||
			type == Opcode::Type::READ_MEMORY || type == Opcode::Type::WRITE_MEMORY ||
			type == Opcode::Type::ARITHM_ADD || type == Opcode::Type::ARITHM_SUB || type == Opcode::Type::ARITHM_AND || type == Opcode::Type::ARITHM_OR || type == Opcode::Type::ARITHM_XOR ||
			type == Opcode::Type::ARITHM_SHL || type == Opcode::Type::ARITHM_NEG || type == Opcode::Type::ARITHM_NOT || type == Opcode::Type::ARITHM_BITNOT ||
			type == Opcode::Type::COMPARE_EQ || type == Opcode::Type::COMPARE_NEQ)
		{
			// This won't make any changes floating point types
			dataType = BaseTypeHelper::makeIntegerUnsigned(dataType);
		}

		Opcode& opcode = vectorAdd(mOpcodes);
		opcode.mType = type;
		opcode.mDataType = dataType;
		opcode.mParameter = parameter;
		opcode.mLineNumber = mLineNumber;
		return opcode;
	}

	Opcode& FunctionCompiler::addOpcode(Opcode::Type type, const DataTypeDefinition* dataType, int64 parameter)
	{
		BaseType baseType = dataType->getBaseType();
		if (dataType->isA<IntegerDataType>() && dataType->as<IntegerDataType>().mSemantics == IntegerDataType::Semantics::BOOLEAN)
		{
			baseType = BaseType::BOOL;
		}
		return addOpcode(type, baseType, parameter);
	}

	void FunctionCompiler::addCastOpcodeIfNecessary(const DataTypeDefinition* sourceType, const DataTypeDefinition* targetType)
	{
		CHECK_ERROR(nullptr != sourceType, "Internal error: Got an invalid source type for cast", mLineNumber);
		CHECK_ERROR(nullptr != targetType, "Internal error: Got an invalid target type for cast", mLineNumber);

		const TypeCasting::CastHandling castHandling = TypeCasting(mCompileOptions).getCastHandling(sourceType, targetType, false);
		switch (castHandling.mResult)
		{
			case TypeCasting::CastHandling::Result::NO_CAST:
			{
				// No cast needed
				break;
			}

			case TypeCasting::CastHandling::Result::BASE_CAST:
			{
				// It's a base cast type, we have an opcode for this
				addOpcode(Opcode::Type::CAST_VALUE, (int64)castHandling.mBaseCastType);
				break;
			}

			case TypeCasting::CastHandling::Result::ANY_CAST:
			{
				// Cast to "any" by adding explicit information about the type
				addOpcode(Opcode::Type::PUSH_CONSTANT, BaseType::INT_CONST, sourceType->getID());
				break;
			}

			default:
				CHECK_ERROR(false, "Cannot cast from " << sourceType->getName() << " to " << targetType->getName(), mLineNumber);
				break;
		}
	}

	void FunctionCompiler::addMoveStackOpcode(int stackChange)
	{
		// If there was a move stack opcode added last, then modify or remove it
		if (!mOpcodes.empty() && mOpcodes.back().mType == Opcode::Type::MOVE_STACK)
		{
			mOpcodes.back().mParameter += stackChange;
			if (mOpcodes.back().mParameter == 0)
				mOpcodes.pop_back();
		}
		else
		{
			addOpcode(Opcode::Type::MOVE_STACK, stackChange);
		}
	}

	Opcode& FunctionCompiler::addJumpToLabel(Opcode::Type type, const LabelToken& labelToken)
	{
		CollectedLabel* collectedLabel = mapFind(mCollectedLabels, labelToken.mName.getHash());
		CHECK_ERROR(nullptr != collectedLabel, "Jump target label not found: " << labelToken.mName.getString(), mLineNumber);
		collectedLabel->mJumpLocations.push_back((uint32)mOpcodes.size());

		return addOpcode(type);	// Target position will be set afterwards
	}

	void FunctionCompiler::buildOpcodesFromNodes(const BlockNode& blockNode, NodeContext& context)
	{
		// First collect all the labels
		for (size_t i = 0; i < blockNode.mNodes.size(); ++i)
		{
			const Node& node = blockNode.mNodes[i];
			if (node.isA<LabelNode>())
			{
				const FlyweightString& labelName = node.as<LabelNode>().mLabel;
				CollectedLabel& collectedLabel = mCollectedLabels[labelName.getHash()];
				CHECK_ERROR(!collectedLabel.mLabelName.isValid(), "Label is defined more than once: " << labelName.getString(), node.getLineNumber());
				collectedLabel.mLabelName = labelName;
			}
		}

		// Cycle through all nodes
		for (size_t i = 0; i < blockNode.mNodes.size(); ++i)
		{
			const Node& node = blockNode.mNodes[i];
			buildOpcodesForNode(node, context);
		}
	}

	void FunctionCompiler::buildOpcodesForNode(const Node& node, NodeContext& context)
	{
		mLineNumber = node.getLineNumber();
		switch (node.getType())
		{
			case Node::Type::BLOCK:
			{
				const BlockNode& blockNode = node.as<BlockNode>();
				// TODO: Get correct size of local variables in this scope
				const int memorySize = 0;
				scopeBegin(memorySize);
				buildOpcodesFromNodes(blockNode, context);
				scopeEnd(memorySize);
				break;
			}

			case Node::Type::LABEL:
			{
				const LabelNode& labelNode = node.as<LabelNode>();
				mFunction.addLabel(labelNode.mLabel, mOpcodes.size(), labelNode.mAddressHooks);
				break;
			}

			case Node::Type::STATEMENT:
			{
				const StatementToken& token = *node.as<StatementNode>().mStatementToken;
				compileTokenTreeToOpcodes(token, true);
				break;
			}

			case Node::Type::JUMP:
			{
				const JumpNode& jumpNode = node.as<JumpNode>();
				CHECK_ERROR(jumpNode.mLabelToken.valid(), "Jump node must have a label", node.getLineNumber());

				addJumpToLabel(Opcode::Type::JUMP, *jumpNode.mLabelToken);
				break;
			}

			case Node::Type::JUMP_INDIRECT:
			{
				const JumpIndirectNode& jumpNode = node.as<JumpIndirectNode>();
				CHECK_ERROR(!jumpNode.mLabelTokens.empty(), "Indirect jump node must have at least one label", node.getLineNumber());

				compileTokenTreeToOpcodes(*jumpNode.mIndexToken);

				for (const TokenPtr<LabelToken>& labelToken : jumpNode.mLabelTokens)
				{
					addJumpToLabel(Opcode::Type::JUMP_SWITCH, *labelToken);
				}
				addOpcode(Opcode::Type::MOVE_VAR_STACK, -1);	// Consume top of stack if none of the jumps did -- TODO: Isn't this meant to be MOVE_STACK (via "addMoveStackOpcode") instead?
				break;
			}

			case Node::Type::BREAK:
			{
				CHECK_ERROR(context.mIsLoopBlock, "Keyword 'break' is only allowed inside a while or for loop", node.getLineNumber());

				context.mBreakLocations.push_back((uint32)mOpcodes.size());
				addOpcode(Opcode::Type::JUMP);	// Target position must be set afterwards when while block is complete
				break;
			}

			case Node::Type::CONTINUE:
			{
				CHECK_ERROR(context.mIsLoopBlock, "Keyword 'continue' is only allowed inside a while or for loop", node.getLineNumber());

				context.mContinueLocations.push_back((uint32)mOpcodes.size());
				addOpcode(Opcode::Type::JUMP);	// Target position must be set afterwards when while block is complete
				break;
			}

			case Node::Type::RETURN:
			{
				const ReturnNode& returnNode = node.as<ReturnNode>();
				if (returnNode.mStatementToken.valid())
				{
					CHECK_ERROR(!mFunction.getReturnType()->isA<VoidDataType>(), "Function '" << mFunction.getName() << "' with 'void' return type cannot return a value", node.getLineNumber());
					compileTokenTreeToOpcodes(*returnNode.mStatementToken);
					addCastOpcodeIfNecessary(returnNode.mStatementToken->mDataType, mFunction.getReturnType());
				}
				else
				{
					CHECK_ERROR(mFunction.getReturnType()->isA<VoidDataType>(), "Function '" << mFunction.getName() << "' must return a " << mFunction.getReturnType()->getName() << " value", node.getLineNumber());
				}
				addOpcode(Opcode::Type::RETURN);
				break;
			}

			case Node::Type::EXTERNAL:
			{
				const ExternalNode& externalNode = node.as<ExternalNode>();
				if (externalNode.mStatementToken.valid())
				{
					compileTokenTreeToOpcodes(*externalNode.mStatementToken);
					addCastOpcodeIfNecessary(externalNode.mStatementToken->mDataType, mCompileOptions.mExternalAddressType);
				}
				else
				{
					CHECK_ERROR(false, "Call/jump must have an integer argument", node.getLineNumber());
				}
				addOpcode((externalNode.mSubType == ExternalNode::SubType::EXTERNAL_CALL) ? Opcode::Type::EXTERNAL_CALL : Opcode::Type::EXTERNAL_JUMP);
				break;
			}

			case Node::Type::IF_STATEMENT:
			{
				std::vector<OpcodeBuilder> openOpcodeBuilders;
				const IfStatementNode* currentIfNode = &node.as<IfStatementNode>();
				while (true)
				{
					const IfStatementNode& isn = *currentIfNode;

					// First evaluate the condition
					compileTokenTreeToOpcodes(*isn.mConditionToken);

					OpcodeBuilder& builder = openOpcodeBuilders.emplace_back(*this);
					builder.beginIf();
					{
						// Compile if-block content
						buildOpcodesForNode(*isn.mContentIf, context);
					}

					// Optional here is an else-statement
					if (isn.mContentElse.valid())
					{
						builder.beginElse();

						mLineNumber = node.getLineNumber();
						if (isn.mContentElse->isA<IfStatementNode>())
						{
							// Start another cycle in the loop
							currentIfNode = &isn.mContentElse->as<IfStatementNode>();
							continue;
						}
						else
						{
							// Compile else-block content
							buildOpcodesForNode(*isn.mContentElse, context);
						}
					}
					break;
				}

				for (auto it = openOpcodeBuilders.rbegin(); it != openOpcodeBuilders.rend(); ++it)
				{
					it->endIf();
				}
				break;
			}

			case Node::Type::WHILE_STATEMENT:
			{
				const WhileStatementNode& wsn = node.as<WhileStatementNode>();
				const size_t startPosition = mOpcodes.size();

				// First evaluate the condition
				compileTokenTreeToOpcodes(*wsn.mConditionToken);

				const size_t ifJumpOpcodeIndex = mOpcodes.size();
				addOpcode(Opcode::Type::JUMP_CONDITIONAL);	// Target position must be set afterwards when if block is complete

				NodeContext innerContext = context;
				innerContext.mIsLoopBlock = true;

				buildOpcodesForNode(*wsn.mContent, innerContext);

				// Jump back to condition evaluation
				addOpcode(Opcode::Type::JUMP, startPosition);

				// This is where the conditional jump leads to
				mOpcodes[ifJumpOpcodeIndex].mParameter = mOpcodes.size();

				// Update all break jump targets
				for (uint32 location : innerContext.mBreakLocations)
				{
					mOpcodes[location].mParameter = mOpcodes.size();
				}

				// Update all continue jump targets
				for (uint32 location : innerContext.mContinueLocations)
				{
					mOpcodes[location].mParameter = startPosition;
				}
				break;
			}

			case Node::Type::FOR_STATEMENT:
			{
				const ForStatementNode& fsn = node.as<ForStatementNode>();

				if (fsn.mInitialToken.valid())
				{
					// Add code for initial statement
					compileTokenTreeToOpcodes(*fsn.mInitialToken, true);
				}

				const size_t startPosition = mOpcodes.size();
				size_t ifJumpOpcodeIndex = 0;

				if (fsn.mConditionToken.valid())
				{
					// Evaluate the condition
					compileTokenTreeToOpcodes(*fsn.mConditionToken);

					ifJumpOpcodeIndex = mOpcodes.size();
					addOpcode(Opcode::Type::JUMP_CONDITIONAL);	// Target position must be set afterwards when if block is complete
				}

				NodeContext innerContext = context;
				innerContext.mIsLoopBlock = true;

				buildOpcodesForNode(*fsn.mContent, innerContext);

				const size_t continuePosition = mOpcodes.size();

				if (fsn.mIterationToken.valid())
				{
					// Add code for iteration statement
					compileTokenTreeToOpcodes(*fsn.mIterationToken, true);
				}

				// Jump back to condition evaluation
				addOpcode(Opcode::Type::JUMP, startPosition);

				if (fsn.mConditionToken.valid())
				{
					// This is where the conditional jump leads to
					mOpcodes[ifJumpOpcodeIndex].mParameter = mOpcodes.size();
				}

				// Update all break jump targets
				for (uint32 location : innerContext.mBreakLocations)
				{
					mOpcodes[location].mParameter = mOpcodes.size();
				}

				// Update all continue jump targets
				for (uint32 location : innerContext.mContinueLocations)
				{
					mOpcodes[location].mParameter = continuePosition;
				}
				break;
			}

			default:
				break;
		}
	}

	void FunctionCompiler::compileTokenTreeToOpcodes(const StatementToken& token, bool consumeResult, bool isLValue)
	{
		switch (token.getType())
		{
			case UnaryOperationToken::TYPE:
			{
				CHECK_ERROR(!isLValue, "Cannot assign value to a unary operation", mLineNumber);
				const UnaryOperationToken& uot = token.as<UnaryOperationToken>();
				switch (uot.mOperator)
				{
					case Operator::BINARY_MINUS:
					{
						compileTokenTreeToOpcodes(*uot.mArgument);
						addOpcode(Opcode::Type::ARITHM_NEG, uot.mDataType);
						break;
					}

					case Operator::UNARY_NOT:
					{
						compileTokenTreeToOpcodes(*uot.mArgument);
						addOpcode(Opcode::Type::ARITHM_NOT, uot.mDataType);
						break;
					}

					case Operator::UNARY_BITNOT:
					{
						compileTokenTreeToOpcodes(*uot.mArgument);
						addOpcode(Opcode::Type::ARITHM_BITNOT, uot.mDataType);
						break;
					}

					case Operator::UNARY_DECREMENT:
					case Operator::UNARY_INCREMENT:
					{
						compileUnaryDecIncToOpcodes(uot);
						break;
					}

					default:
						CHECK_ERROR(false, "Unrecognized operator", mLineNumber);
				}

				break;
			}

			case BinaryOperationToken::TYPE:
			{
				CHECK_ERROR(!isLValue, "Cannot assign value to a binary operation", mLineNumber);
				const BinaryOperationToken& bot = token.as<BinaryOperationToken>();
				if (nullptr != bot.mFunction)
				{
					// Treat this just like a function call
					compileTokenTreeToOpcodes(*bot.mLeft);
					compileTokenTreeToOpcodes(*bot.mRight);
					// TODO: Do we need implicit casts here?
					addOpcode(Opcode::Type::CALL, (BaseType)0, bot.mFunction->getNameAndSignatureHash());
					break;
				}

				switch (bot.mOperator)
				{
					case Operator::ASSIGN:					 compileAssignmentToOpcodes(bot);  break;

					case Operator::ASSIGN_PLUS:				 compileBinaryAssignmentToOpcodes(bot, Opcode::Type::ARITHM_ADD);	break;
					case Operator::ASSIGN_MINUS:			 compileBinaryAssignmentToOpcodes(bot, Opcode::Type::ARITHM_SUB);	break;
					case Operator::ASSIGN_MULTIPLY:			 compileBinaryAssignmentToOpcodes(bot, Opcode::Type::ARITHM_MUL);	break;
					case Operator::ASSIGN_DIVIDE:			 compileBinaryAssignmentToOpcodes(bot, Opcode::Type::ARITHM_DIV);	break;
					case Operator::ASSIGN_MODULO:			 compileBinaryAssignmentToOpcodes(bot, Opcode::Type::ARITHM_MOD);	break;
					case Operator::ASSIGN_AND:				 compileBinaryAssignmentToOpcodes(bot, Opcode::Type::ARITHM_AND);	break;
					case Operator::ASSIGN_OR:				 compileBinaryAssignmentToOpcodes(bot, Opcode::Type::ARITHM_OR);	break;
					case Operator::ASSIGN_XOR:				 compileBinaryAssignmentToOpcodes(bot, Opcode::Type::ARITHM_XOR);	break;
					case Operator::ASSIGN_SHIFT_LEFT:		 compileBinaryAssignmentToOpcodes(bot, Opcode::Type::ARITHM_SHL);	break;
					case Operator::ASSIGN_SHIFT_RIGHT:		 compileBinaryAssignmentToOpcodes(bot, Opcode::Type::ARITHM_SHR);	break;

					case Operator::BINARY_PLUS:				 compileBinaryOperationToOpcodes(bot, Opcode::Type::ARITHM_ADD);	break;
					case Operator::BINARY_MINUS:			 compileBinaryOperationToOpcodes(bot, Opcode::Type::ARITHM_SUB);	break;
					case Operator::BINARY_MULTIPLY:			 compileBinaryOperationToOpcodes(bot, Opcode::Type::ARITHM_MUL);	break;
					case Operator::BINARY_DIVIDE:			 compileBinaryOperationToOpcodes(bot, Opcode::Type::ARITHM_DIV);	break;
					case Operator::BINARY_MODULO:			 compileBinaryOperationToOpcodes(bot, Opcode::Type::ARITHM_MOD);	break;
					case Operator::BINARY_SHIFT_LEFT:		 compileBinaryOperationToOpcodes(bot, Opcode::Type::ARITHM_SHL);	break;
					case Operator::BINARY_SHIFT_RIGHT:		 compileBinaryOperationToOpcodes(bot, Opcode::Type::ARITHM_SHR);	break;
					case Operator::BINARY_AND:				 compileBinaryOperationToOpcodes(bot, Opcode::Type::ARITHM_AND);	break;
					case Operator::BINARY_OR:				 compileBinaryOperationToOpcodes(bot, Opcode::Type::ARITHM_OR);		break;
					case Operator::BINARY_XOR:				 compileBinaryOperationToOpcodes(bot, Opcode::Type::ARITHM_XOR);	break;

					case Operator::COMPARE_EQUAL:
						compileBinaryOperationToOpcodes(bot, Opcode::Type::COMPARE_EQ);
						if (consumeResult && mCompileOptions.mScriptFeatureLevel >= 2)
							CHECK_ERROR(false, "Result of comparison is not used, this is certainly a mistake in the script", mLineNumber);
						break;

					case Operator::COMPARE_NOT_EQUAL:		 compileBinaryOperationToOpcodes(bot, Opcode::Type::COMPARE_NEQ);	break;
					case Operator::COMPARE_LESS:			 compileBinaryOperationToOpcodes(bot, Opcode::Type::COMPARE_LT);	break;
					case Operator::COMPARE_LESS_OR_EQUAL:	 compileBinaryOperationToOpcodes(bot, Opcode::Type::COMPARE_LE);	break;
					case Operator::COMPARE_GREATER:			 compileBinaryOperationToOpcodes(bot, Opcode::Type::COMPARE_GT);	break;
					case Operator::COMPARE_GREATER_OR_EQUAL: compileBinaryOperationToOpcodes(bot, Opcode::Type::COMPARE_GE);	break;

					case Operator::LOGICAL_AND:
					{
						// Short circuit evaluation: An expression like "A && B" is treated as "A ? B : false"

						// First evaluate the left side
						compileTokenTreeToOpcodes(*bot.mLeft);

						OpcodeBuilder builder(*this);
						builder.beginIf();
						{
							// Evaluate the right side
							compileTokenTreeToOpcodes(*bot.mRight);
						}
						builder.beginElse();
						{
							// Push "false"
							addOpcode(Opcode::Type::PUSH_CONSTANT, BaseType::INT_CONST, 0);
						}
						builder.endIf();
						break;
					}

					case Operator::LOGICAL_OR:
					{
						// Short circuit evaluation: An expression like "A || B" is treated as "A ? true : B"

						// First evaluate the left side
						compileTokenTreeToOpcodes(*bot.mLeft);

						OpcodeBuilder builder(*this);
						builder.beginIf();
						{
							// Push "true"
							addOpcode(Opcode::Type::PUSH_CONSTANT, BaseType::INT_CONST, 1);
						}
						builder.beginElse();
						{
							// Evaluate the right side
							compileTokenTreeToOpcodes(*bot.mRight);
						}
						builder.endIf();
						break;
					}

					case Operator::QUESTIONMARK:
					{
						// Trinary operation handling
						CHECK_ERROR(bot.mRight->isA<BinaryOperationToken>(), "Expected : after ? operator, but no binary operation found at all", mLineNumber);
						const BinaryOperationToken& colonToken = *bot.mRight.as<BinaryOperationToken>();
						CHECK_ERROR(colonToken.mOperator == Operator::COLON, "Expected : after ? operator, but found wrong binary operation there", mLineNumber);

						// First evaluate the condition
						compileTokenTreeToOpcodes(*bot.mLeft);

						OpcodeBuilder builder(*this);
						builder.beginIf();
						{
							// Evaluate first alternative
							compileTokenTreeToOpcodes(*colonToken.mLeft);
						}
						builder.beginElse();
						{
							// Evaluate second alternative
							compileTokenTreeToOpcodes(*colonToken.mRight);
						}
						builder.endIf();
						break;
					}

					case Operator::COLON:
						CHECK_ERROR(false, "Found : without outer ? operator", mLineNumber);
						break;

					default:
						CHECK_ERROR(false, "Unrecognized operator", mLineNumber);
						break;
				}

				break;
			}

			case ParenthesisToken::TYPE:
			{
				CHECK_ERROR(!isLValue, "Cannot assign value to an expression in parentheses", mLineNumber);
				const ParenthesisToken& pt = token.as<ParenthesisToken>();
				if (!pt.mContent.empty())
				{
					CHECK_ERROR(pt.mContent.size() == 1, "Too many tokens left inside parenthesis", mLineNumber);
					CHECK_ERROR(pt.mContent[0].isStatement(), "Parenthesis content is not a statement", mLineNumber);

					compileTokenTreeToOpcodes(pt.mContent[0].as<StatementToken>());
				}
				break;
			}

			case ConstantToken::TYPE:
			{
				CHECK_ERROR(!isLValue, "Cannot assign value to a constant", mLineNumber);
				const ConstantToken& ct = token.as<ConstantToken>();
				addOpcode(Opcode::Type::PUSH_CONSTANT, ct.mDataType, ct.mValue.get<uint64>());
				break;
			}

			case VariableToken::TYPE:
			{
				const VariableToken& vt = token.as<VariableToken>();
				if (vt.mDataType->isA<ArrayDataType>())
				{
					// Push variable ID
					addOpcode(Opcode::Type::PUSH_CONSTANT, vt.mVariable->getID());
				}
				else
				{
					const Opcode::Type opcodeType = isLValue ? Opcode::Type::SET_VARIABLE_VALUE : Opcode::Type::GET_VARIABLE_VALUE;
					addOpcode(opcodeType, vt.mDataType, vt.mVariable->getID());
				}
				break;
			}

			case FunctionToken::TYPE:
			{
				CHECK_ERROR(!isLValue, "Cannot assign value to a function call", mLineNumber);
				const FunctionToken& ft = token.as<FunctionToken>();

				for (size_t i = 0; i < ft.mParameters.size(); ++i)
				{
					const TokenPtr<StatementToken>& statementToken = ft.mParameters[i];
					compileTokenTreeToOpcodes(*statementToken);
					addCastOpcodeIfNecessary(statementToken->mDataType, ft.mFunction->getParameters()[i].mDataType);
				}

				// Using the data type parameter here to encode whether or not this is a base function call
				addOpcode(Opcode::Type::CALL, (BaseType)(ft.mIsBaseCall ? 1 : 0), ft.mFunction->getNameAndSignatureHash());
				break;
			}

			case MemoryAccessToken::TYPE:
			{
				// For writing to a memory access, this implementation should not be reached any more, instead "compileAssignmentToOpcodes" or a similar method is used
				CHECK_ERROR(!isLValue, "Internal error: Memory write should have been resolved differently", mLineNumber);
				const MemoryAccessToken& mat = token.as<MemoryAccessToken>();

				// Compile memory address calculation
				compileTokenTreeToOpcodes(*mat.mAddress);

				// Opcode to read from memory address
				addOpcode(Opcode::Type::READ_MEMORY, mat.mDataType);
				break;
			}

			case BracketAccessToken::TYPE:
			{
				const BracketAccessToken& bat = token.as<BracketAccessToken>();
				const DataTypeDefinition::BracketOperator& bracket = bat.mVariable->getDataType()->getBracketOperator();

				// Choose the right function depending on isLValue
				if (isLValue)
				{
					// Note that this is partially moved to "compileAssignmentToOpcodes"
					//  -> TODO: Also implement support in "compileUnaryDecIncToOpcodes" and "compileBinaryAssignmentToOpcodes"
					//  -> Afterwards, this block can be removed and turned into a "CHECK_ERROR(!isLValue, ...)" - similar to MemoryAccessToken, see case above

					CHECK_ERROR(nullptr != bracket.mSetter, "Write access is not possible for bracket operator [] for type " << bat.mVariable->getDataType()->getName(), mLineNumber);
					// TODO: Also check the setter signature

					// Note that the value to assign was already pushed, so that will be the first parameter for the setter

					// Second parameter is the variable ID
					addOpcode(Opcode::Type::PUSH_CONSTANT, BaseType::INT_CONST, bat.mVariable->getID());

					// Third parameter is the parameter inside the brackets
					compileTokenTreeToOpcodes(*bat.mParameter);
					addCastOpcodeIfNecessary(bat.mParameter->mDataType, bracket.mParameterType);

					addOpcode(Opcode::Type::CALL, bracket.mSetter->getNameAndSignatureHash());

					// Differentiate on whether the setter returns a value or void
					if (bracket.mSetter->getReturnType()->isVoid())
					{
						addMoveStackOpcode(1);	// Push a dummy value onto stack
					}
				}
				else
				{
					CHECK_ERROR(nullptr != bracket.mGetter, "Write access is not possible for bracket operator [] for type " << bat.mVariable->getDataType()->getName(), mLineNumber);
					// TODO: Also check the getter signature

					// First parameter is the variable ID
					addOpcode(Opcode::Type::PUSH_CONSTANT, BaseType::INT_CONST, bat.mVariable->getID());

					// Second parameter is the parameter inside the brackets
					compileTokenTreeToOpcodes(*bat.mParameter);
					addCastOpcodeIfNecessary(bat.mParameter->mDataType, bracket.mParameterType);

					addOpcode(Opcode::Type::CALL, bracket.mGetter->getNameAndSignatureHash());
				}
				break;
			}

			case ValueCastToken::TYPE:
			{
				CHECK_ERROR(!isLValue, "Cannot assign value to a type cast", mLineNumber);
				const ValueCastToken& vct = token.as<ValueCastToken>();
				compileTokenTreeToOpcodes(*vct.mArgument);

				addCastOpcodeIfNecessary(vct.mArgument->mDataType, vct.mDataType);
				break;
			}

			default:
				CHECK_ERROR(false, "Token type should be eliminated by now", mLineNumber);
		}

		if (consumeResult && !token.mDataType->isA<VoidDataType>())
		{
			const int sizeOnStack = (int)token.mDataType->getSizeOnStack();
			RMX_ASSERT(sizeOnStack != 0, "Invalid stack size of type " << token.mDataType->getName().getString());
			addMoveStackOpcode(-sizeOnStack);	// Pop result of statement
		}
	}

	void FunctionCompiler::compileUnaryDecIncToOpcodes(const UnaryOperationToken& uot)
	{
		// TODO: Differentiate between pre- and post-fix!

		// Special handling for memory access on left side
		//  -> Memory address calculation must only be done once, especially if it has side effects (e.g. "u8[A0++] += 8")
		if (uot.mArgument->isA<MemoryAccessToken>())
		{
			const MemoryAccessToken& mat = uot.mArgument->as<MemoryAccessToken>();

			// Compile memory address calculation
			compileTokenTreeToOpcodes(*mat.mAddress);

			// Output READ_MEMORY opcode with parameter that tells it to *not* consume its input
			//  -> It would usually do, but in our case here the value is needed again as input for the WRITE_MEMORY below
			addOpcode(Opcode::Type::READ_MEMORY, mat.mDataType, 1);

			// Add arithmetic add opcode with constant -1 or +1
			addOpcode(Opcode::Type::PUSH_CONSTANT, BaseType::INT_CONST, (uot.mOperator == Operator::UNARY_DECREMENT) ? -1 : 1);
			addOpcode(Opcode::Type::ARITHM_ADD, uot.mDataType);

			// Output WRITE_MEMORY opcode
			addOpcode(Opcode::Type::WRITE_MEMORY, mat.mDataType);
		}
		else if (uot.mArgument->isA<BracketAccessToken>())
		{
			const BracketAccessToken& bat = uot.mArgument->as<BracketAccessToken>();
			const DataTypeDefinition::BracketOperator& bracket = bat.mVariable->getDataType()->getBracketOperator();

			CHECK_ERROR(nullptr != bracket.mSetter, "Write access is not possible for bracket operator [] for type " << bat.mVariable->getDataType()->getName(), mLineNumber);
			// TODO: Also check the setter signature

			// First parameter is the variable ID
			addOpcode(Opcode::Type::PUSH_CONSTANT, BaseType::INT_CONST, bat.mVariable->getID());

			// Second parameter is the parameter inside the brackets
			compileTokenTreeToOpcodes(*bat.mParameter);
			addCastOpcodeIfNecessary(bat.mParameter->mDataType, bracket.mParameterType);

			// Both parameters so far will be needed by the setter again, are but consumed by the getter - so we need to copy them
			addOpcode(Opcode::Type::DUPLICATE, 2);

			// Call the getter
			addOpcode(Opcode::Type::CALL, bracket.mGetter->getNameAndSignatureHash());

			// Add arithmetic add opcode with constant -1 or +1
			addOpcode(Opcode::Type::PUSH_CONSTANT, BaseType::INT_CONST, (uot.mOperator == Operator::UNARY_DECREMENT) ? -1 : 1);
			addOpcode(Opcode::Type::ARITHM_ADD, uot.mDataType);

			// Call the setter
			addOpcode(Opcode::Type::CALL, bracket.mSetter->getNameAndSignatureHash());

			// Differentiate on whether the setter returns a value or void
			if (bracket.mSetter->getReturnType()->isVoid())
			{
				addMoveStackOpcode(1);	// Push a dummy value onto stack
			}
		}
		else
		{
			// Compile argument
			compileTokenTreeToOpcodes(*uot.mArgument);

			// Add arithmetic add opcode with constant -1 or +1
			addOpcode(Opcode::Type::PUSH_CONSTANT, BaseType::INT_CONST, (uot.mOperator == Operator::UNARY_DECREMENT) ? -1 : 1);
			addOpcode(Opcode::Type::ARITHM_ADD, uot.mDataType);

			// Compile argument again for assignment
			compileTokenTreeToOpcodes(*uot.mArgument, false, true);
		}
	}

	void FunctionCompiler::compileAssignmentToOpcodes(const BinaryOperationToken& bot)
	{
		// Special handling for memory access on left side
		//  -> Just to ensure the memory address gets pushed first, before the right side
		if (bot.mLeft->isA<MemoryAccessToken>())
		{
			const MemoryAccessToken& mat = bot.mLeft->as<MemoryAccessToken>();

			// Compile memory address calculation
			compileTokenTreeToOpcodes(*mat.mAddress);

			// Compile right, and cast if necessary
			compileTokenTreeToOpcodes(*bot.mRight);
			addCastOpcodeIfNecessary(bot.mRight->mDataType, bot.mLeft->mDataType);

			// Output WRITE_MEMORY opcode
			addOpcode(Opcode::Type::WRITE_MEMORY, mat.mDataType);
		}
		else if (bot.mLeft->isA<BracketAccessToken>())
		{
			const BracketAccessToken& bat = bot.mLeft->as<BracketAccessToken>();
			const DataTypeDefinition::BracketOperator& bracket = bat.mVariable->getDataType()->getBracketOperator();

			CHECK_ERROR(nullptr != bracket.mSetter, "Write access is not possible for bracket operator [] for type " << bat.mVariable->getDataType()->getName(), mLineNumber);
			// TODO: Also check the setter signature

			// First parameter is the variable ID
			addOpcode(Opcode::Type::PUSH_CONSTANT, BaseType::INT_CONST, bat.mVariable->getID());

			// Second parameter is the parameter inside the brackets
			compileTokenTreeToOpcodes(*bat.mParameter);
			addCastOpcodeIfNecessary(bat.mParameter->mDataType, bracket.mParameterType);

			// Third parameter is the value to assign, i.e. the right side of the assignment
			compileTokenTreeToOpcodes(*bot.mRight);
			addCastOpcodeIfNecessary(bot.mRight->mDataType, bot.mLeft->mDataType);

			addOpcode(Opcode::Type::CALL, bracket.mSetter->getNameAndSignatureHash());

			// TODO: For operators like +=, this likely requires a special case inside "compileBinaryAssignmentToOpcodes" just like with memory access
			//  -> Otherwise the parameter inside the brackets is evaluated twice, which might have unintended side-effects

			// Differentiate on whether the setter returns a value or void
			if (bracket.mSetter->getReturnType()->isVoid())
			{
				addMoveStackOpcode(1);	// Push a dummy value onto stack
			}
		}
		else
		{
			// Compile right, and cast if necessary
			compileTokenTreeToOpcodes(*bot.mRight);
			addCastOpcodeIfNecessary(bot.mRight->mDataType, bot.mLeft->mDataType);

			// Compile left for assignment
			compileTokenTreeToOpcodes(*bot.mLeft, false, true);
		}
	}

	void FunctionCompiler::compileBinaryAssignmentToOpcodes(const BinaryOperationToken& bot, Opcode::Type opcodeType)
	{
		// Special handling for memory access on left side
		//  -> Memory address calculation must only be done once, especially if it has side effects (e.g. "u8[A0++] += 8")
		if (bot.mLeft->isA<MemoryAccessToken>())
		{
			const MemoryAccessToken& mat = bot.mLeft->as<MemoryAccessToken>();

			// Compile memory address calculation
			compileTokenTreeToOpcodes(*mat.mAddress);

			// Output READ_MEMORY opcode with parameter that tells it to *not* consume its input, i.e. the memory address
			//  -> It would usually do, but in our case here the address is needed again as input for the WRITE_MEMORY below
			addOpcode(Opcode::Type::READ_MEMORY, mat.mDataType, 1);

			// Compile right
			compileTokenTreeToOpcodes(*bot.mRight);

			// Add arithmetic opcode
			addOpcode(opcodeType, bot.mDataType);

			// Output WRITE_MEMORY opcode
			addOpcode(Opcode::Type::WRITE_MEMORY, mat.mDataType);
		}
		else if (bot.mLeft->isA<BracketAccessToken>())
		{
			const BracketAccessToken& bat = bot.mLeft->as<BracketAccessToken>();
			const DataTypeDefinition::BracketOperator& bracket = bat.mVariable->getDataType()->getBracketOperator();

			CHECK_ERROR(nullptr != bracket.mSetter, "Write access is not possible for bracket operator [] for type " << bat.mVariable->getDataType()->getName(), mLineNumber);
			// TODO: Also check the setter signature

			// First parameter is the variable ID
			addOpcode(Opcode::Type::PUSH_CONSTANT, BaseType::INT_CONST, bat.mVariable->getID());

			// Second parameter is the parameter inside the brackets
			compileTokenTreeToOpcodes(*bat.mParameter);
			addCastOpcodeIfNecessary(bat.mParameter->mDataType, bracket.mParameterType);

			// Both parameters so far will be needed by the setter again, are but consumed by the getter - so we need to copy them
			addOpcode(Opcode::Type::DUPLICATE, 2);

			// Call the getter
			addOpcode(Opcode::Type::CALL, bracket.mGetter->getNameAndSignatureHash());

			// Compile right
			compileTokenTreeToOpcodes(*bot.mRight);

			// Add arithmetic opcode
			addOpcode(opcodeType, bot.mDataType);

			// Call the setter
			addOpcode(Opcode::Type::CALL, bracket.mSetter->getNameAndSignatureHash());

			// Differentiate on whether the setter returns a value or void
			if (bracket.mSetter->getReturnType()->isVoid())
			{
				addMoveStackOpcode(1);	// Push a dummy value onto stack
			}
		}
		else
		{
			// Compile left
			compileTokenTreeToOpcodes(*bot.mLeft);

			// Compile right
			compileTokenTreeToOpcodes(*bot.mRight);

			// Add arithmetic opcode
			addOpcode(opcodeType, bot.mDataType);

			// Compile left again for assignment
			compileTokenTreeToOpcodes(*bot.mLeft, false, true);
		}
	}

	void FunctionCompiler::compileBinaryOperationToOpcodes(const BinaryOperationToken& bot, Opcode::Type opcodeType)
	{
		const bool usesBoolTypes = (bot.mOperator == Operator::LOGICAL_AND || bot.mOperator == Operator::LOGICAL_OR);

		// Move constant to the right for easier optimization later on
		StatementToken* leftToken = bot.mLeft.get();
		StatementToken* rightToken = bot.mRight.get();
		if (leftToken->isA<ConstantToken>() && !rightToken->isA<ConstantToken>() && isCommutative(bot.mOperator))
		{
			rightToken = bot.mLeft.get();
			leftToken = bot.mRight.get();
		}

		compileTokenTreeToOpcodes(*leftToken);
		if (usesBoolTypes && !isComparison(*leftToken))
		{
			addOpcode(Opcode::Type::MAKE_BOOL, leftToken->mDataType);
		}
		// TODO: Add cast opcode if necessary - but this requires knowing the binary operation signature (similar to function parameters)

		compileTokenTreeToOpcodes(*rightToken);
		if (usesBoolTypes && !isComparison(*rightToken))
		{
			addOpcode(Opcode::Type::MAKE_BOOL, rightToken->mDataType);
		}
		// TODO: Add cast opcode if necessary - but this requires knowing the binary operation signature (similar to function parameters)

		// Not the token's own data type, that does not work for comparisons
		//  -> TODO: This is potentially dangerous for u8 * s8 multiplications!
		addOpcode(opcodeType, leftToken->mDataType);
	}

	void FunctionCompiler::scopeBegin(int memoryToReserve)
	{
		if (memoryToReserve > 0)
		{
			addOpcode(Opcode::Type::MOVE_VAR_STACK, memoryToReserve);
		}
	}

	void FunctionCompiler::scopeEnd(int memoryToFree)
	{
		if (memoryToFree > 0)
		{
			addOpcode(Opcode::Type::MOVE_VAR_STACK, -memoryToFree);
		}
	}

	void FunctionCompiler::assignOpcodeFlags()
	{
		const size_t numOpcodes = mOpcodes.size();

		// Add flags by opcodes
		uint32 lastLineNumber = 0xffffffff;
		for (size_t i = 0; i < numOpcodes; ++i)
		{
			Opcode& opcode = mOpcodes[i];
			switch (opcode.mType)
			{
				case Opcode::Type::JUMP:
				case Opcode::Type::JUMP_CONDITIONAL:
					opcode.mFlags.set(Opcode::Flag::CTRLFLOW);
					opcode.mFlags.set(Opcode::Flag::JUMP);
					break;

				case Opcode::Type::CALL:
				case Opcode::Type::RETURN:
				case Opcode::Type::EXTERNAL_CALL:
				case Opcode::Type::EXTERNAL_JUMP:
					opcode.mFlags.set(Opcode::Flag::CTRLFLOW);
					break;

				default:
					break;
			}

			if (lastLineNumber != opcode.mLineNumber)
			{
				opcode.mFlags.set(Opcode::Flag::NEW_LINE);
				lastLineNumber = opcode.mLineNumber;
			}
		}

		// Add label targets
		for (const ScriptFunction::Label& label : mFunction.mLabels)
		{
			mOpcodes[label.mOffset].mFlags.set(Opcode::Flag::LABEL);
		}

		// Add jump targets
		for (size_t i = 0; i < numOpcodes; ++i)
		{
			const Opcode& opcode = mOpcodes[i];
			if (opcode.mFlags.isSet(Opcode::Flag::JUMP))
			{
				const size_t jumpTarget = std::min((size_t)mOpcodes[i].mParameter, mOpcodes.size() - 1);
				mOpcodes[jumpTarget].mFlags.set(Opcode::Flag::JUMP_TARGET);
			}
		}

		// Add sequence break flags
		for (size_t i = 0; i < numOpcodes; ++i)
		{
			if (mOpcodes[i].mFlags.isSet(Opcode::Flag::CTRLFLOW))
			{
				mOpcodes[i].mFlags.set(Opcode::Flag::SEQ_BREAK);
			}
			else if (i+1 < numOpcodes && mOpcodes[i+1].mFlags.anySet(makeBitFlagSet(Opcode::Flag::LABEL, Opcode::Flag::JUMP_TARGET, Opcode::Flag::NEW_LINE, Opcode::Flag::CTRLFLOW)))
			{
				mOpcodes[i].mFlags.set(Opcode::Flag::SEQ_BREAK);
			}
		}
	}

}
