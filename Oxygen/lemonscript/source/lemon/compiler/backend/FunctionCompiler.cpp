/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/compiler/backend/FunctionCompiler.h"
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
		addOpcode(Opcode::Type::MOVE_VAR_STACK, mFunction.mLocalVariablesByID.size());

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
			addOpcode(Opcode::Type::MOVE_STACK, -1);
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
				size_t offset = 0;
				if (!mFunction.getLabel(collectedLabel.mLabelName, offset))
					RMX_ASSERT(false, "Jump target label not found: " << collectedLabel.mLabelName.getString());
				mOpcodes[jumpLocation].mParameter = (uint64)offset;
			}
		}

		// Make sure it ends with a return in any case
		if (mOpcodes.empty() || mOpcodes.back().mType != Opcode::Type::RETURN)
		{
			CHECK_ERROR(mFunction.getReturnType()->getClass() == DataTypeDefinition::Class::VOID, "Function '" << mFunction.getName() << "' must return a " << mFunction.getReturnType()->getName() << " value", blockNode.getLineNumber());
			addOpcode(Opcode::Type::RETURN);
		}
		else
		{
			// There could be jumps leading to the position after the return, correct them
			//  -> E.g. when the function ends with and if-block that itself ends with a return
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
		optimizeOpcodes();

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
		if (dataType->getClass() == DataTypeDefinition::Class::INTEGER && dataType->as<IntegerDataType>().mSemantics == IntegerDataType::Semantics::BOOLEAN)
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
				// TODO: Get correct number of local variables in this scope
				const int numVariables = 0;
				scopeBegin(numVariables);
				buildOpcodesFromNodes(blockNode, context);
				scopeEnd(numVariables);
				break;
			}

			case Node::Type::LABEL:
			{
				mFunction.addLabel(node.as<LabelNode>().mLabel, mOpcodes.size());
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
				addOpcode(Opcode::Type::MOVE_VAR_STACK, -1);	// Consume top of stack if none of the jumps did
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
					CHECK_ERROR(mFunction.getReturnType()->getClass() != DataTypeDefinition::Class::VOID, "Function '" << mFunction.getName() << "' with 'void' return type cannot return a value", node.getLineNumber());
					compileTokenTreeToOpcodes(*returnNode.mStatementToken);
					addCastOpcodeIfNecessary(returnNode.mStatementToken->mDataType, mFunction.getReturnType());
				}
				else
				{
					CHECK_ERROR(mFunction.getReturnType()->getClass() == DataTypeDefinition::Class::VOID, "Function '" << mFunction.getName() << "' must return a " << mFunction.getReturnType()->getName() << " value", node.getLineNumber());
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
			case Token::Type::UNARY_OPERATION:
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
						// TODO: Differentiate between pre- and post-fix!

						compileTokenTreeToOpcodes(*uot.mArgument);
						addOpcode(Opcode::Type::PUSH_CONSTANT, BaseType::INT_CONST, (uot.mOperator == Operator::UNARY_DECREMENT) ? -1 : 1);
						addOpcode(Opcode::Type::ARITHM_ADD, uot.mDataType);
						compileTokenTreeToOpcodes(*uot.mArgument, false, true);
						break;
					}

					default:
						CHECK_ERROR(false, "Unrecognized operator", mLineNumber);
				}

				break;
			}

			case Token::Type::BINARY_OPERATION:
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
					case Operator::ASSIGN:
					{
						// Assignment to variable
						compileTokenTreeToOpcodes(*bot.mRight);
						addCastOpcodeIfNecessary(bot.mRight->mDataType, bot.mLeft->mDataType);
						compileTokenTreeToOpcodes(*bot.mLeft, false, true);
						break;
					}

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

			case Token::Type::PARENTHESIS:
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

			case Token::Type::CONSTANT:
			{
				CHECK_ERROR(!isLValue, "Cannot assign value to a constant", mLineNumber);
				const ConstantToken& ct = token.as<ConstantToken>();
				addOpcode(Opcode::Type::PUSH_CONSTANT, ct.mDataType, ct.mValue.get<uint64>());
				break;
			}

			case Token::Type::VARIABLE:
			{
				const VariableToken& vt = token.as<VariableToken>();
				const Opcode::Type opcodeType = isLValue ? Opcode::Type::SET_VARIABLE_VALUE : Opcode::Type::GET_VARIABLE_VALUE;
				addOpcode(opcodeType, vt.mDataType, vt.mVariable->getID());
				break;
			}

			case Token::Type::FUNCTION:
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

			case Token::Type::MEMORY_ACCESS:
			{
				const MemoryAccessToken& mat = token.as<MemoryAccessToken>();
				compileTokenTreeToOpcodes(*mat.mAddress);

				const Opcode::Type opcodeType = isLValue ? Opcode::Type::WRITE_MEMORY : Opcode::Type::READ_MEMORY;
				addOpcode(opcodeType, mat.mDataType);
				break;
			}

			case Token::Type::VALUE_CAST:
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

		if (consumeResult && token.mDataType->getClass() != DataTypeDefinition::Class::VOID)
		{
			addOpcode(Opcode::Type::MOVE_STACK, -1);	// Pop result of statement
		}
	}

	void FunctionCompiler::compileBinaryAssignmentToOpcodes(const BinaryOperationToken& bot, Opcode::Type opcodeType)
	{
		// Special handling for memory access on left side
		//  -> Memory address calculation must only be done once, especially if it has side effects (e.g. "u8[A0++] += 8")
		if (bot.mLeft->isA<MemoryAccessToken>())
		{
			// Compile memory read
			const MemoryAccessToken& mat = bot.mLeft->as<MemoryAccessToken>();
			compileTokenTreeToOpcodes(*mat.mAddress);

			// Output READ_MEMORY opcode with parameter that tells it to *not* consume its input
			//  -> It would usually do, but in our case here the value is needed again as input for the WRITE_MEMORY below
			//  -> This is an optimization compared to the older approach of having a DUPLICATE opcode (that only existed for this very use-case in the first place)
			addOpcode(Opcode::Type::READ_MEMORY, mat.mDataType, 1);

			// Compile right
			compileTokenTreeToOpcodes(*bot.mRight);

			// Add arithmetic opcode
			addOpcode(opcodeType, bot.mDataType);

			// Output WRITE_MEMORY opcode with parameter that tells it to exchange its inputs
			//  -> Top of stack is value, next is address - but in other cases it's the other way round
			//  -> This is an optimization compared to the older approach of having an EXCHANGE opcode (that only existed for this very use-case in the first place)
			addOpcode(Opcode::Type::WRITE_MEMORY, mat.mDataType, 1);
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

	void FunctionCompiler::scopeBegin(int numVariables)
	{
		if (numVariables > 0)
		{
			addOpcode(Opcode::Type::MOVE_VAR_STACK, numVariables);
		}
	}

	void FunctionCompiler::scopeEnd(int numVariables)
	{
		if (numVariables > 0)
		{
			addOpcode(Opcode::Type::MOVE_VAR_STACK, -numVariables);
		}
	}

	void FunctionCompiler::optimizeOpcodes()
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
						if (DataTypeHelper::isPureIntegerBaseCast((BaseCastType)opcode2.mParameter))
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

	void FunctionCompiler::cleanupNOPs()
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
