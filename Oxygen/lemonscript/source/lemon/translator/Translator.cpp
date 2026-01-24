/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/translator/Translator.h"
#include "lemon/translator/SourceCodeWriter.h"
#include "lemon/program/Function.h"


namespace lemon
{

	class CppTranslator
	{
	public:
		void translate(String& output, const BlockNode& rootNode)
		{
			CppWriter writer(output);
			translateBlockNodeInternal(writer, rootNode, false);
		}

	private:
		void translateBlockNodeInternal(CppWriter& writer, const BlockNode& blockNode, bool addBraces)
		{
			if (addBraces)
			{
				writer.beginBlock();
			}

			for (size_t nodeIndex = 0; nodeIndex < blockNode.mNodes.size(); ++nodeIndex)
			{
				translateNodeInternal(writer, blockNode.mNodes[nodeIndex]);
			}

			if (addBraces)
			{
				writer.endBlock();
			}
		}

		void translateNodeInternal(CppWriter& writer, const Node& node, bool withExtraIndentation = false)
		{
			if (node.isA<BlockNode>())
			{
				withExtraIndentation = false;		// Block does its own intendation
			}

			if (withExtraIndentation)
			{
				writer.increaseIndentation();
			}

			switch (node.getType())
			{
				case Node::Type::BLOCK:
				{
					translateBlockNodeInternal(writer, node.as<BlockNode>(), true);
					break;
				}

				case Node::Type::PRAGMA:
				{
					const PragmaNode& pn = node.as<PragmaNode>();
					writer.writeLine("//# " + pn.mContent);
					break;
				}

				case Node::Type::FUNCTION:
				{
					const FunctionNode& fn = node.as<FunctionNode>();
					writer.writeFunctionHeader(*fn.mFunction);

					translateBlockNodeInternal(writer, *fn.mContent, true);
					writer.writeEmptyLine();
					writer.writeEmptyLine();
					break;
				}

				case Node::Type::LABEL:
				{
					const LabelNode& labelNode = node.as<LabelNode>();
					writer.decreaseIndentation();
					writer.writeLine(std::string(labelNode.mLabel.getString()) + ":");
					writer.increaseIndentation();
					break;
				}

				case Node::Type::JUMP:
				{
					const JumpNode& jumpNode = node.as<JumpNode>();
					writer.writeLine("goto " + std::string(jumpNode.mLabelToken->mName.getString()) + ";");
					break;
				}

				case Node::Type::BREAK:
				{
					writer.writeLine("break;");
					break;
				}

				case Node::Type::CONTINUE:
				{
					writer.writeLine("continue;");
					break;
				}

				case Node::Type::RETURN:
				{
					const ReturnNode& rn = node.as<ReturnNode>();
					if (rn.mStatementToken.valid())
					{
						String line;
						line << "return ";
						translateTokenInternal(line, *rn.mStatementToken);
						line << ";";
						writer.writeLine(line);
					}
					else
					{
						writer.writeLine("return;");
					}
					break;
				}

				case Node::Type::EXTERNAL:
				{
					const ExternalNode& externalNode = node.as<ExternalNode>();
					String line;
					line << ((externalNode.mSubType == ExternalNode::SubType::EXTERNAL_CALL) ? "call " : "jump ");
					translateTokenInternal(line, externalNode.mStatementToken);
					line << ";";
					writer.writeLine(line);
					break;
				}

				case Node::Type::STATEMENT:
				{
					const StatementNode& sn = node.as<StatementNode>();
					String line;
					translateTokenInternal(line, *sn.mStatementToken);
					line << ";";
					writer.writeLine(line);
					break;
				}

				case Node::Type::IF_STATEMENT:
				{
					writeIfStatement(writer, node.as<IfStatementNode>());
					break;
				}

				case Node::Type::WHILE_STATEMENT:
				{
					const WhileStatementNode& wsn = node.as<WhileStatementNode>();
					String line;
					line << "while (";
					translateTokenInternal(line, *wsn.mConditionToken);
					line << ")";
					writer.writeLine(line);

					translateNodeInternal(writer, *wsn.mContent, true);
					break;
				}

				case Node::Type::FOR_STATEMENT:
				{
					const ForStatementNode& fsn = node.as<ForStatementNode>();
					String line;
					line << "for (";
					translateTokenInternal(line, fsn.mInitialToken);
					line << "; ";
					translateTokenInternal(line, fsn.mConditionToken);
					line << "; ";
					translateTokenInternal(line, fsn.mIterationToken);
					line << ")";
					writer.writeLine(line);

					translateNodeInternal(writer, *fsn.mContent, true);
					break;
				}

				default:
				{
					writer.writeLine("<unknown_node_" + std::to_string((int)node.getType()) + ">");
					break;
				}
			}

			if (withExtraIndentation)
			{
				writer.decreaseIndentation();
			}
		}

		void writeIfStatement(CppWriter& writer, const IfStatementNode& isn, bool startWithElseIf = false)
		{
			String line;
			line << (startWithElseIf ? "else if (" : "if (");
			translateTokenInternal(line, *isn.mConditionToken);
			line << ")";
			writer.writeLine(line);

			translateNodeInternal(writer, *isn.mContentIf, true);

			if (isn.mContentElse.valid())
			{
				if (isn.mContentElse->isA<IfStatementNode>())
				{
					writeIfStatement(writer, isn.mContentElse->as<IfStatementNode>(), true);
				}
				else
				{
					writer.writeLine("else");
					translateNodeInternal(writer, *isn.mContentElse, true);
				}
			}
		}

		void translateTokenInternal(String& line, const lemon::TokenPtr<StatementToken>& tokenPtr)
		{
			if (tokenPtr.valid())
			{
				translateTokenInternal(line, *tokenPtr);
			}
		}

		void translateTokenInternal(String& line, const StatementToken& token)
		{
			switch (token.getType())
			{
				case ConstantToken::TYPE:
				{
					const ConstantToken& ct = token.as<ConstantToken>();
					line << rmx::hexString(ct.mValue.get<uint64>());	// TODO: Support float and double here as well
					break;
				}

				case ParenthesisToken::TYPE:
				{
					const ParenthesisToken& pt = token.as<ParenthesisToken>();
					switch (pt.mParenthesisType)
					{
						case ParenthesisType::PARENTHESIS:	line << "(";  break;
						case ParenthesisType::BRACKET:		line << "[";  break;
					}
					for (size_t k = 0; k < pt.mContent.size(); ++k)
					{
						if (k > 0)
							line << ", ";
						translateTokenInternal(line, pt.mContent[k].as<StatementToken>());
					}
					switch (pt.mParenthesisType)
					{
						case ParenthesisType::PARENTHESIS:	line << ")";  break;
						case ParenthesisType::BRACKET:		line << "]";  break;
					}
					break;
				}

				case CommaSeparatedListToken::TYPE:
				{
					const CommaSeparatedListToken& cslt = token.as<CommaSeparatedListToken>();
					for (size_t k = 0; k < cslt.mContent.size(); ++k)
					{
						if (k > 0)
							line << ", ";
						translateTokenInternal(line, cslt.mContent[k][0].as<StatementToken>());
					}
					break;
				}

				case UnaryOperationToken::TYPE:
				{
					const UnaryOperationToken& uot = token.as<UnaryOperationToken>();
					switch (uot.mOperator)
					{
						case Operator::BINARY_MINUS:	line << "-";		break;
						case Operator::UNARY_NOT:		line << "!";		break;
						case Operator::UNARY_BITNOT:	line << "~";		break;
						case Operator::UNARY_INCREMENT:	line << "++";		break;
						case Operator::UNARY_DECREMENT:	line << "--";		break;
						default:						line << " <unknown_operator> ";	break;
					}

					translateTokenInternal(line, *uot.mArgument);
					break;
				}

				case BinaryOperationToken::TYPE:
				{
					const BinaryOperationToken& bot = token.as<BinaryOperationToken>();
					translateTokenInternal(line, *bot.mLeft);

					switch (bot.mOperator)
					{
						case Operator::ASSIGN:					line << " = ";		break;
						case Operator::ASSIGN_PLUS:				line << " += ";		break;
						case Operator::ASSIGN_MINUS:			line << " -= ";		break;
						case Operator::ASSIGN_MULTIPLY:			line << " *= ";		break;
						case Operator::ASSIGN_DIVIDE:			line << " /= ";		break;
						case Operator::ASSIGN_MODULO:			line << " %= ";		break;
						case Operator::ASSIGN_SHIFT_LEFT:		line << " <<= ";	break;
						case Operator::ASSIGN_SHIFT_RIGHT:		line << " >>= ";	break;
						case Operator::ASSIGN_AND:				line << " &= ";		break;
						case Operator::ASSIGN_OR:				line << " |= ";		break;
						case Operator::ASSIGN_XOR:				line << " ^= ";		break;
						case Operator::BINARY_PLUS:				line << " + ";		break;
						case Operator::BINARY_MINUS:			line << " - ";		break;
						case Operator::BINARY_MULTIPLY:			line << " * ";		break;
						case Operator::BINARY_DIVIDE:			line << " / ";		break;
						case Operator::BINARY_MODULO:			line << " % ";		break;
						case Operator::BINARY_SHIFT_LEFT:		line << " << ";		break;
						case Operator::BINARY_SHIFT_RIGHT:		line << " >> ";		break;
						case Operator::BINARY_AND:				line << " & ";		break;
						case Operator::BINARY_OR:				line << " | ";		break;
						case Operator::BINARY_XOR:				line << " ^ ";		break;
						case Operator::LOGICAL_AND:				line << " && ";		break;
						case Operator::LOGICAL_OR:				line << " || ";		break;
						case Operator::COMPARE_EQUAL:			line << " == ";		break;
						case Operator::COMPARE_NOT_EQUAL:		line << " != ";		break;
						case Operator::COMPARE_LESS:			line << " < ";		break;
						case Operator::COMPARE_LESS_OR_EQUAL:	line << " <= ";		break;
						case Operator::COMPARE_GREATER:			line << " > ";		break;
						case Operator::COMPARE_GREATER_OR_EQUAL:line << " >= ";		break;
						case Operator::QUESTIONMARK:			line << " ? ";		break;
						case Operator::COLON:					line << " : ";		break;
						default:								line << " <unknown_operator> ";	break;
					}

					translateTokenInternal(line, *bot.mRight);
					break;
				}

				case VariableToken::TYPE:
				{
					const VariableToken& vt = token.as<VariableToken>();
					CppWriter::addIdentifier(line, vt.mVariable->getName().getString());
					break;
				}

				case FunctionToken::TYPE:
				{
					const FunctionToken& ft = token.as<FunctionToken>();
					CppWriter::addIdentifier(line, ft.mFunction->getName().getString());
					line << "(";

					for (size_t k = 0; k < ft.mParameters.size(); ++k)
					{
						if (k > 0)
							line << ", ";
						translateTokenInternal(line, ft.mParameters[k]);
					}

					line << ")";
					break;
				}

				case MemoryAccessToken::TYPE:
				{
					const MemoryAccessToken& mat = token.as<MemoryAccessToken>();
					line << "accessMemory_";
					CppWriter::addDataType(line, mat.mDataType);
					line << "(";
					translateTokenInternal(line, *mat.mAddress);
					line << ")";
					break;
				}

				case ValueCastToken::TYPE:
				{
					const ValueCastToken& vct = token.as<ValueCastToken>();
					line << "(";
					CppWriter::addDataType(line, vct.mDataType);
					line << ")(";
					translateTokenInternal(line, *vct.mArgument);
					line << ")";
					break;
				}

				default:
				{
					line << "<unknown_token>";
					break;
				}
			}
		}
	};



	void Translator::translateToCpp(String& output, const BlockNode& rootNode)
	{
		CppTranslator cppTranslator;
		cppTranslator.translate(output, rootNode);
	}

	void Translator::translateToCppAndSave(std::wstring_view filename, const BlockNode& rootNode)
	{
		String output;
		translateToCpp(output, rootNode);
		output.saveFile(filename);
	}

}
