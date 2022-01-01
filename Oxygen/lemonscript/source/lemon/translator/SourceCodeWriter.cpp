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

	void CppWriter::addDataType(String& line, BaseType dataType)
	{
		switch (dataType)
		{
			case BaseType::VOID:	line << "void";		break;
			case BaseType::INT_8:	line << "int8";		break;
			case BaseType::UINT_8:	line << "uint8";	break;
			case BaseType::INT_16:	line << "int16";	break;
			case BaseType::UINT_16:	line << "uint16";	break;
			case BaseType::INT_32:	line << "int32";	break;
			case BaseType::UINT_32:	line << "uint32";	break;
			case BaseType::INT_64:	line << "int64";	break;
			case BaseType::UINT_64:	line << "uint64";	break;
			default:				line << "<unknown_type>";	break;
		}
	}

	void CppWriter::addDataType(String& line, const DataTypeDefinition* dataType)
	{
		addDataType(line, DataTypeHelper::getBaseType(dataType));
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
