/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/program/DataType.h"


namespace lemon
{
	class Function;

	class SourceCodeWriter
	{
	public:
		inline SourceCodeWriter(String& output) : mOutput(output) {}

		inline void increaseIndentation()	{ ++mIndentation; }
		inline void decreaseIndentation()	{ --mIndentation; }

		void writeLine(const String& line);
		void writeEmptyLine();

		void beginBlock(const char* lineContent = "{");
		void endBlock(const char* lineContent = "}");

	protected:
		String& mOutput;
		int mIndentation = 0;
	};


	class CppWriter : public SourceCodeWriter
	{
	public:
		inline CppWriter(String& output) : SourceCodeWriter(output) {}

		void writeFunctionHeader(const Function& function);

	public:
		static void addDataType(String& line, BaseType dataType);
		static void addDataType(String& line, const DataTypeDefinition* dataType);
		static void addIdentifier(String& line, const std::string& identifier);
	};

}
