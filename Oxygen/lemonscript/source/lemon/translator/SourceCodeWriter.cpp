/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/translator/SourceCodeWriter.h"
#include "lemon/program/Function.h"


namespace lemon
{

	void SourceCodeWriter::writeLine(const String& line)
	{
		for (int i = 0; i < mIndentation; ++i)
			mOutput << "\t";
		mOutput << line << "\r\n";
	}

	void SourceCodeWriter::writeEmptyLine()
	{
		mOutput << "\r\n";
	}

	void SourceCodeWriter::beginBlock(const char* lineContent)
	{
		writeLine(lineContent);
		increaseIndentation();
	}

	void SourceCodeWriter::endBlock(const char* lineContent)
	{
		decreaseIndentation();
		writeLine(lineContent);
	}



	void CppWriter::writeFunctionHeader(const Function& function)
	{
		String line;
		addDataType(line, function.getReturnType());
		line << " ";
		addIdentifier(line, function.getName());
		line << "(";
		for (size_t i = 0; i < function.getParameters().size(); ++i)
		{
			if (i > 0)
				line << ", ";
			addDataType(line, function.getParameters()[i].mType);
			line << " ";
			addIdentifier(line, function.getParameters()[i].mIdentifier);
		}
		line << ")";
		writeLine(line);
	}

	void CppWriter::addDataType(String& line, const DataTypeDefinition* dataType)
	{
		if (dataType->mClass == DataTypeDefinition::Class::VOID)
		{
			line << "void";
		}
		else if (dataType->mClass == DataTypeDefinition::Class::INTEGER)
		{
			const IntegerDataType& integerType = dataType->as<IntegerDataType>();
			if (integerType.mSemantics == IntegerDataType::Semantics::BOOLEAN)
			{
				line << "bool";
			}
			else
			{
				switch (integerType.mBytes)
				{
					case 1:  line << (integerType.mIsSigned ? "int8"  : "uint8" );  break;
					case 2:  line << (integerType.mIsSigned ? "int16" : "uint16");  break;
					case 4:  line << (integerType.mIsSigned ? "int32" : "uint32");  break;
					case 8:  line << (integerType.mIsSigned ? "int64" : "uint64");  break;
				}
			}
		}
	}

	void CppWriter::addIdentifier(String& line, const std::string& identifier)
	{
		for (size_t i = 0; i < identifier.length(); ++i)
		{
			if (identifier[i] == '.')
				line << '_';
			else
				line << identifier[i];
		}
	}

}
