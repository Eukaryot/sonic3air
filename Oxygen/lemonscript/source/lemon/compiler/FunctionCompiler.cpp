/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/compiler/FunctionCompiler.h"
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
			if (token.getType() != Token::Type::BINARY_OPERATION)
				return false;

			const Operator op = token.as<BinaryOperationToken>().mOperator;
			return (op >= Operator::COMPARE_EQUAL && op <= Operator::COMPARE_GREATER_OR_EQUAL);
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

		Opcode& addOpcode(Opcode::Type type, BaseType dataType = BaseType::VOID, int64 parameter = 0)
		{
			Opcode& opcode = vectorAdd(mFunctionCompiler.mOpcodes);
			opcode.mType = type;
			opcode.mDataType = dataType;
			opcode.mParameter = parameter;
			opcode.mLineNumber = mFunctionCompiler.mLineNumber;
			return opcode;
		}

		void beginIf()
		{
			mIfJumpOpcodeIndex = mFunctionCompiler.mOpcodes.size();
			addOpcode(Opcode::Type::JUMP_CONDITIONAL, BaseType::UINT_32);	// Target position must be set afterwards when if block is complete
		}

		void beginElse()
		{
			// Conditional jump to the end of the else-part
			mElseJumpOpcodeIndex = mFunctionCompiler.mOpcodes.size();
			addOpcode(Opcode::Type::JUMP, BaseType::UINT_32);				// Target position must be set afterwards when else block is complete

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


	FunctionCompiler::FunctionCompiler(ScriptFunction& function, const Configuration& config) :
		mFunction(function),
		mConfig(config),
		mOpcodes(function.mOpcodes)
	{
	}

	void FunctionCompiler::processParameters()
	{
		// Check if anything to do at all (note that parameters are also local variables)
		if (mFunction.mLocalVariablesById.empty())
			return;

		// Create scope
		addOpcode(Opcode::Type::MOVE_VAR_STACK, BaseType::VOID, mFunction.mLocalVariablesById.size());

		// Go through parameters in reverse order
		for (int index = (int)mFunction.getParameters().size() - 1; index >= 0; --index)
		{
			const Function::Parameter& parameter = mFunction.getParameters()[index];
			const Variable* variable = mFunction.getLocalVariableByIdentifier(parameter.mIdentifier);
			RMX_ASSERT(nullptr != variable, "Variable not found");
			RMX_ASSERT(variable->getDataType() == parameter.mType, "Variable has wrong type");

			// Assume the variable value is on the stack
			addOpcode(Opcode::Type::SET_VARIABLE_VALUE, variable->getDataType(), variable->getId());

			// Pop value from stack (as SET_VARIABLE_VALUE opcode does not consume it)
			addOpcode(Opcode::Type::MOVE_STACK, BaseType::VOID, -1);
		}
	}

	void FunctionCompiler::buildOpcodesForFunction(const BlockNode& blockNode)
	{
		// Build opcodes for the block
		NodeContext context;
		buildOpcodesFromNodes(blockNode, context);

		// Make sure it ends with a return in any case
		if (mOpcodes.empty() || mOpcodes.back().mType != Opcode::Type::RETURN)
		{
			CHECK_ERROR(mFunction.getReturnType()->mClass == DataTypeDefinition::Class::VOID, "Function must return a value", blockNode.getLineNumber());
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

	Opcode& FunctionCompiler::addOpcode(Opcode::Type type, BaseType dataType, int64 parameter)
	{
		Opcode& opcode = vectorAdd(mOpcodes);
		opcode.mType = type;
		opcode.mDataType = dataType;
		opcode.mParameter = parameter;
		opcode.mLineNumber = mLineNumber;
		return opcode;
	}

	Opcode& FunctionCompiler::addOpcode(Opcode::Type type, const DataTypeDefinition* dataType, int64 parameter)
	{
		Opcode& opcode = vectorAdd(mOpcodes);
		opcode.mType = type;
		opcode.mDataType = DataTypeHelper::getBaseType(dataType);
		opcode.mParameter = parameter;
		opcode.mLineNumber = mLineNumber;
		return opcode;
	}

	void FunctionCompiler::addCastOpcodeIfNecessary(const DataTypeDefinition* sourceType, const DataTypeDefinition* targetType)
	{
		const int castType = TypeCasting::getCastType(sourceType, targetType);
		if (castType >= 0)
		{
			addOpcode(Opcode::Type::CAST_VALUE, BaseType::VOID, castType);
		}
		else
		{
			CHECK_ERROR(castType != -2, "Cannot cast from " << sourceType->toString() << " to " << targetType->toString(), mLineNumber);
		}
	}

	void FunctionCompiler::buildOpcodesFromNodes(const BlockNode& blockNode, NodeContext& context)
	{
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

				size_t offset = 0xffffffff;
				if (!mFunction.getLabel(jumpNode.mLabelToken->mName, offset))
					CHECK_ERROR(false, "Jump target label not found: " + jumpNode.mLabelToken->mName, node.getLineNumber());

				addOpcode(Opcode::Type::JUMP, BaseType::UINT_32, offset);
				break;
			}

			case Node::Type::BREAK:
			{
				CHECK_ERROR(context.mIsLoopBlock, "Keyword 'break' is only allowed inside a while or for loop", node.getLineNumber());

				context.mBreakLocations.push_back((uint32)mOpcodes.size());
				addOpcode(Opcode::Type::JUMP, BaseType::UINT_32);	// Target position must be set afterwards when while block is complete
				break;
			}

			case Node::Type::CONTINUE:
			{
				CHECK_ERROR(context.mIsLoopBlock, "Keyword 'continue' is only allowed inside a while or for loop", node.getLineNumber());

				context.mContinueLocations.push_back((uint32)mOpcodes.size());
				addOpcode(Opcode::Type::JUMP, BaseType::UINT_32);	// Target position must be set afterwards when while block is complete
				break;
			}

			case Node::Type::RETURN:
			{
				const ReturnNode& returnNode = node.as<ReturnNode>();
				if (returnNode.mStatementToken.valid())
				{
					CHECK_ERROR(mFunction.getReturnType()->mClass != DataTypeDefinition::Class::VOID, "Function '" << mFunction.getName() << "' with 'void' return type cannot return a value", node.getLineNumber());
					compileTokenTreeToOpcodes(*returnNode.mStatementToken);
					addCastOpcodeIfNecessary(returnNode.mStatementToken->mDataType, mFunction.getReturnType());
				}
				else
				{
					CHECK_ERROR(mFunction.getReturnType()->mClass == DataTypeDefinition::Class::VOID, "Function '" << mFunction.getName() << "' must return a " << mFunction.getReturnType()->toString() << " value", node.getLineNumber());
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
					addCastOpcodeIfNecessary(externalNode.mStatementToken->mDataType, mConfig.mExternalAddressType);
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
				const IfStatementNode& isn = node.as<IfStatementNode>();

				// First evaluate the condition
				compileTokenTreeToOpcodes(*isn.mConditionToken);

				OpcodeBuilder builder(*this);
				builder.beginIf();
				{
					// Compile if-block content
					buildOpcodesForNode(*isn.mContentIf, context);
				}

				// Optional here is an else-statement
				if (isn.mContentElse.valid())
				{
					builder.beginElse();

					// Compile else-block content
					buildOpcodesForNode(*isn.mContentElse, context);
				}
				builder.endIf();
				break;
			}

			case Node::Type::WHILE_STATEMENT:
			{
				const WhileStatementNode& wsn = node.as<WhileStatementNode>();
				const size_t startPosition = mOpcodes.size();

				// First evaluate the condition
				compileTokenTreeToOpcodes(*wsn.mConditionToken);

				const size_t ifJumpOpcodeIndex = mOpcodes.size();
				addOpcode(Opcode::Type::JUMP_CONDITIONAL, BaseType::UINT_32);	// Target position must be set afterwards when if block is complete

				NodeContext innerContext = context;
				innerContext.mIsLoopBlock = true;

				buildOpcodesForNode(*wsn.mContent, innerContext);

				// Jump back to condition evaluation
				addOpcode(Opcode::Type::JUMP, BaseType::UINT_32, startPosition);

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
					addOpcode(Opcode::Type::JUMP_CONDITIONAL, BaseType::UINT_32);	// Target position must be set afterwards when if block is complete
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
				addOpcode(Opcode::Type::JUMP, BaseType::UINT_32, startPosition);

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
						addOpcode(Opcode::Type::PUSH_CONSTANT, BaseType::INT_8, (uot.mOperator == Operator::UNARY_DECREMENT) ? -1 : 1);
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
				const BinaryOperationToken& bot = token.as<BinaryOperationToken>();
				switch (bot.mOperator)
				{
					case Operator::ASSIGN:
					{
						// Assignment to variable
						compileTokenTreeToOpcodes(*bot.mRight);
						compileTokenTreeToOpcodes(*bot.mLeft, false, true);
						break;
					}

					case Operator::ASSIGN_PLUS:				 compileBinaryAssigmentToOpcodes(bot, Opcode::Type::ARITHM_ADD);	break;
					case Operator::ASSIGN_MINUS:			 compileBinaryAssigmentToOpcodes(bot, Opcode::Type::ARITHM_SUB);	break;
					case Operator::ASSIGN_MULTIPLY:			 compileBinaryAssigmentToOpcodes(bot, Opcode::Type::ARITHM_MUL);	break;
					case Operator::ASSIGN_DIVIDE:			 compileBinaryAssigmentToOpcodes(bot, Opcode::Type::ARITHM_DIV);	break;
					case Operator::ASSIGN_MODULO:			 compileBinaryAssigmentToOpcodes(bot, Opcode::Type::ARITHM_MOD);	break;
					case Operator::ASSIGN_AND:				 compileBinaryAssigmentToOpcodes(bot, Opcode::Type::ARITHM_AND);	break;
					case Operator::ASSIGN_OR:				 compileBinaryAssigmentToOpcodes(bot, Opcode::Type::ARITHM_OR);		break;
					case Operator::ASSIGN_XOR:				 compileBinaryAssigmentToOpcodes(bot, Opcode::Type::ARITHM_XOR);	break;
					case Operator::ASSIGN_SHIFT_LEFT:		 compileBinaryAssigmentToOpcodes(bot, Opcode::Type::ARITHM_SHL);	break;
					case Operator::ASSIGN_SHIFT_RIGHT:		 compileBinaryAssigmentToOpcodes(bot, Opcode::Type::ARITHM_SHR);	break;

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
					case Operator::COMPARE_EQUAL:			 compileBinaryOperationToOpcodes(bot, Opcode::Type::COMPARE_EQ);	break;
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
							addOpcode(Opcode::Type::PUSH_CONSTANT, BaseType::BOOL, 0);
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
							addOpcode(Opcode::Type::PUSH_CONSTANT, BaseType::BOOL, 1);
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
						CHECK_ERROR(bot.mRight->getType() == Token::Type::BINARY_OPERATION, "Expected : after ? operator, but no binary operation found at all", mLineNumber);
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
				const ConstantToken& ct = token.as<ConstantToken>();
				addOpcode(Opcode::Type::PUSH_CONSTANT, ct.mDataType, ct.mValue);
				break;
			}

			case Token::Type::VARIABLE:
			{
				const VariableToken& vt = token.as<VariableToken>();
				const Opcode::Type opcodeType = isLValue ? Opcode::Type::SET_VARIABLE_VALUE : Opcode::Type::GET_VARIABLE_VALUE;
				addOpcode(opcodeType, vt.mDataType, vt.mVariable->getId());
				break;
			}

			case Token::Type::FUNCTION:
			{
				const FunctionToken& ft = token.as<FunctionToken>();
				const TokenList& content = ft.mParenthesis->mContent;

				if (!content.empty())
				{
					// TODO: Check parameters vs. function signature
					CHECK_ERROR(content.size() == 1, "More than one token left in function parameters", mLineNumber);
					if (content[0].getType() == Token::Type::COMMA_SEPARATED)
					{
						for (const TokenList& tokens : content[0].as<CommaSeparatedListToken>().mContent)
						{
							CHECK_ERROR(tokens.size() == 1, "More than one token left between commas", mLineNumber);
							CHECK_ERROR(tokens[0].isStatement(), "Expected a statement as function parameter", mLineNumber);
							compileTokenTreeToOpcodes(tokens[0].as<StatementToken>());
						}
					}
					else
					{
						CHECK_ERROR(content[0].isStatement(), "Expected a statement as function parameter", mLineNumber);
						compileTokenTreeToOpcodes(content[0].as<StatementToken>());
					}
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
				const ValueCastToken& vct = token.as<ValueCastToken>();
				compileTokenTreeToOpcodes(*vct.mArgument);

				addCastOpcodeIfNecessary(vct.mArgument->mDataType, vct.mDataType);
				break;
			}

			default:
				CHECK_ERROR(false, "Token type should be eliminated by now", mLineNumber);
		}

		if (consumeResult && token.mDataType->mClass != DataTypeDefinition::Class::VOID)
		{
			addOpcode(Opcode::Type::MOVE_STACK, BaseType::VOID, -1);	// Pop result of statement
		}
	}

	void FunctionCompiler::compileBinaryAssigmentToOpcodes(const BinaryOperationToken& bot, Opcode::Type opcodeType)
	{
		// Special handling for memory access on left side
		//  -> Memory address calculation must only be done once, especially if it has side effects (e.g. "u8[A0++] += 8")
		if (bot.mLeft->getType() == Token::Type::MEMORY_ACCESS)
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
		if (leftToken->getType() == Token::Type::CONSTANT && rightToken->getType() != Token::Type::CONSTANT && isCommutative(bot.mOperator))
		{
			rightToken = bot.mLeft.get();
			leftToken = bot.mRight.get();
		}

		compileTokenTreeToOpcodes(*leftToken);
		if (usesBoolTypes && !isComparison(*leftToken))
		{
			addOpcode(Opcode::Type::MAKE_BOOL, leftToken->mDataType);
		}

		compileTokenTreeToOpcodes(*rightToken);
		if (usesBoolTypes && !isComparison(*rightToken))
		{
			addOpcode(Opcode::Type::MAKE_BOOL, rightToken->mDataType);
		}

		// Not the token's own data type, that does not work for comparisons
		//  -> TODO: This is potentially dangerous for u8 * s8 multiplications!
		addOpcode(opcodeType, leftToken->mDataType);
	}

	void FunctionCompiler::scopeBegin(int numVariables)
	{
		if (numVariables > 0)
		{
			addOpcode(Opcode::Type::MOVE_VAR_STACK, BaseType::VOID, numVariables);
		}
	}

	void FunctionCompiler::scopeEnd(int numVariables)
	{
		if (numVariables > 0)
		{
			addOpcode(Opcode::Type::MOVE_VAR_STACK, BaseType::VOID, -numVariables);
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
				for (auto& pair : mFunction.mLabels)
				{
					if ((size_t)pair.second < isOpcodeJumpTarget.size())
					{
						isOpcodeJumpTarget[(size_t)pair.second] = true;
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
						switch (opcode2.mParameter & 0x83)
						{
							case 0x00:  opcode1.mParameter =  (uint8)opcode1.mParameter;  break;
							case 0x01:  opcode1.mParameter = (uint16)opcode1.mParameter;  break;
							case 0x02:  opcode1.mParameter = (uint32)opcode1.mParameter;  break;
							case 0x80:  opcode1.mParameter =   (int8)opcode1.mParameter;  break;
							case 0x81:  opcode1.mParameter =  (int16)opcode1.mParameter;  break;
							case 0x82:  opcode1.mParameter =  (int32)opcode1.mParameter;  break;
						}
						opcode2.mType = Opcode::Type::NOP;
						anotherRun = true;
						continue;
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
					uint64 jumpTarget = conditionMet ? (uint64)(condJumpPosition + 1) : condJumpOpcode.mParameter;

					// Check for a shortcut (as this is not ruled out at that point)
					if (mOpcodes[(size_t)jumpTarget].mType == Opcode::Type::JUMP)
					{
						jumpTarget = mOpcodes[(size_t)jumpTarget].mParameter;
					}

					// Replace the first opcode with a jump directly to where the (now not really) conditional jump would lead to
					firstOpcode.mType = Opcode::Type::JUMP;
					firstOpcode.mDataType = BaseType::UINT_32;
					firstOpcode.mFlags = Opcode::Flag::CTRLFLOW | Opcode::Flag::JUMP | Opcode::Flag::SEQ_BREAK;
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
				mOpcodes[i].mFlags |= Opcode::Flag::TEMP_FLAG;
			}

			static std::vector<size_t> openSeeds;
			openSeeds.clear();
			openSeeds.push_back(0);
			for (const auto& pair : mFunction.mLabels)
			{
				openSeeds.push_back(pair.second);
			}

			// Trace all reachable opcodes from our seeds
			while (!openSeeds.empty())
			{
				size_t position = openSeeds.back();
				openSeeds.pop_back();

				while (mOpcodes[position].mFlags & Opcode::Flag::TEMP_FLAG)
				{
					mOpcodes[position].mFlags &= ~Opcode::Flag::TEMP_FLAG;
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
				if (opcode.mFlags & Opcode::Flag::TEMP_FLAG)
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
									mOpcodes[i].mParameter = -1;
									mOpcodes[i].mFlags &= Opcode::Flag::NEW_LINE;	// Clear all other flags
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
			for (auto& pair : mFunction.mLabels)
			{
				pair.second = ((size_t)pair.second < indexRemap.size()) ? indexRemap[(size_t)pair.second] : lastOpcode;
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
					opcode.mFlags |= Opcode::Flag::CTRLFLOW | Opcode::Flag::JUMP;
					break;

				case Opcode::Type::CALL:
				case Opcode::Type::RETURN:
				case Opcode::Type::EXTERNAL_CALL:
				case Opcode::Type::EXTERNAL_JUMP:
					opcode.mFlags |= Opcode::Flag::CTRLFLOW;
					break;

				default:
					break;
			}

			if (lastLineNumber != opcode.mLineNumber)
			{
				opcode.mFlags |= Opcode::Flag::NEW_LINE;
				lastLineNumber = opcode.mLineNumber;
			}
		}

		// Add label targets
		for (const auto& pair : mFunction.mLabels)
		{
			mOpcodes[pair.second].mFlags |= Opcode::Flag::LABEL;
		}

		// Add jump targets
		for (size_t i = 0; i < numOpcodes; ++i)
		{
			const Opcode& opcode = mOpcodes[i];
			if (opcode.mFlags & Opcode::Flag::JUMP)
			{
				const size_t jumpTarget = std::min((size_t)mOpcodes[i].mParameter, mOpcodes.size() - 1);
				mOpcodes[jumpTarget].mFlags |= Opcode::Flag::JUMP_TARGET;
			}
		}

		// Add sequence break flags
		for (size_t i = 0; i < numOpcodes; ++i)
		{
			if ((mOpcodes[i].mFlags & Opcode::Flag::CTRLFLOW) != 0)
			{
				mOpcodes[i].mFlags |= Opcode::Flag::SEQ_BREAK;
			}
			else if (i+1 < numOpcodes && (mOpcodes[i+1].mFlags & (Opcode::Flag::LABEL | Opcode::Flag::JUMP_TARGET | Opcode::Flag::NEW_LINE | Opcode::Flag::CTRLFLOW)) != 0)
			{
				mOpcodes[i].mFlags |= Opcode::Flag::SEQ_BREAK;
			}
		}
	}

}
