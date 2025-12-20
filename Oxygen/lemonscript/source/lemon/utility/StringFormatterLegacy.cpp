/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/utility/StringFormatterLegacy.h"
#include "lemon/utility/FastStringStream.h"
#include "lemon/program/FunctionWrapper.h"


namespace lemon
{
	namespace
	{
		void addFormattedInt(detail::FastStringStream& output, char formatCharacter, const AnyTypeWrapper& arg, int minDigits)
		{
			int64 value = 0;
			switch (arg.mType->getClass())
			{
				case DataTypeDefinition::Class::INTEGER:
					value = arg.mValue.get<int64>();
					break;

				case DataTypeDefinition::Class::FLOAT:
					if (arg.mType->getBytes() == 4)
						value = roundToInt(arg.mValue.get<float>());
					else
						value = roundToInt(arg.mValue.get<double>());
					break;

				default:
					return;
			}

			if (formatCharacter == 'd')
			{
				output.addDecimal(value, minDigits);
			}
			else if (formatCharacter == 'b')
			{
				output.addBinary(value, minDigits);
			}
			else if (formatCharacter == 'x')
			{
				output.addHex(value, minDigits);
			}
		}

		void addFormattedFloat(detail::FastStringStream& output, const AnyTypeWrapper& arg)
		{
			switch (arg.mType->getClass())
			{
				case DataTypeDefinition::Class::INTEGER:
					output.addDouble((double)arg.mValue.get<int64>());
					break;

				case DataTypeDefinition::Class::FLOAT:
					if (arg.mType->getBytes() == 4)
						output.addFloat(arg.mValue.get<float>());
					else
						output.addDouble(arg.mValue.get<double>());
					break;

				default:
					return;
			}
		}
	}


	void StringFormatterLegacy::buildFormattedString(detail::FastStringStream& output, std::string_view formatString, size_t numArguments, const AnyTypeWrapper* args)
	{
		const char* fmtPtr = formatString.data();
		const char* fmtEnd = fmtPtr + formatString.length();

		for (; fmtPtr < fmtEnd; ++fmtPtr)
		{
			if (numArguments <= 0)
			{
				// Warning: This means that additional '%' characters won't be processed at all, which also means that escaped ones won't be reduces to a single one
				//  -> There's scripts that rely on this exact behavior, so don't ever change that!
				output.addString(fmtPtr, (int)(fmtEnd - fmtPtr));
				break;
			}

			// Continue until getting a '%' character
			{
				const char* fmtStart = fmtPtr;
				while (fmtPtr != fmtEnd && *fmtPtr != '%')
				{
					++fmtPtr;
				}
				if (fmtPtr != fmtStart)
				{
					output.addString(fmtStart, (int)(fmtPtr - fmtStart));
				}
				if (fmtPtr == fmtEnd)
					break;
			}

			const int remaining = (int)(fmtEnd - fmtPtr);
			if (remaining >= 2)
			{
				int charsRead = 0;

				if (fmtPtr[1] == '%')
				{
					output.addChar('%');
					charsRead = 1;
				}
				else if (fmtPtr[1] == 's')
				{
					// String argument
					const FlyweightString* argStoredString = nullptr;
					if (args[0].mType->getClass() == DataTypeDefinition::Class::STRING || args[0].mType->getClass() == DataTypeDefinition::Class::INTEGER)
					{
						lemon::Runtime* runtime = lemon::Runtime::getActiveRuntime();
						argStoredString = runtime->resolveStringByKey(args[0].mValue.get<uint64>());
					}

					if (nullptr == argStoredString)
						output.addString("<?>", 3);
					else
						output.addString(argStoredString->getString());

					++args;
					--numArguments;
					charsRead = 1;
				}
				else if (fmtPtr[1] == 'd' || fmtPtr[1] == 'b' || fmtPtr[1] == 'x')
				{
					// Integer argument
					addFormattedInt(output, fmtPtr[1], args[0], 0);

					++args;
					--numArguments;
					charsRead = 1;
				}
				else if (remaining >= 4 && fmtPtr[1] == '0' && (fmtPtr[2] >= '1' && fmtPtr[2] <= '9') && (fmtPtr[3] == 'd' || fmtPtr[3] == 'b' || fmtPtr[3] == 'x'))
				{
					// Integer argument with minimum number of digits (9 or less)
					const int minDigits = (int)(fmtPtr[2] - '0');
					addFormattedInt(output, fmtPtr[3], args[0], minDigits);

					++args;
					--numArguments;
					charsRead = 3;
				}
				else if (remaining >= 5 && fmtPtr[1] == '0' && (fmtPtr[2] >= '1' && fmtPtr[2] <= '9') && (fmtPtr[3] >= '0' && fmtPtr[3] <= '9') && (fmtPtr[4] == 'd' || fmtPtr[4] == 'b' || fmtPtr[4] == 'x'))
				{
					// Integer argument with minimum number of digits (10 or more)
					const int minDigits = (int)(fmtPtr[2] - '0') * 10 + (int)(fmtPtr[3] - '0');
					addFormattedInt(output, fmtPtr[4], args[0], minDigits);

					++args;
					--numArguments;
					charsRead = 4;
				}
				else if (fmtPtr[1] == 'f')
				{
					// Float argument
					// TODO: Support formats like "0.2f" as well
					addFormattedFloat(output, args[0]);

					++args;
					--numArguments;
					charsRead = 1;
				}
				else
				{
					output.addChar('%');
				}

				fmtPtr += charsRead;
			}
			else
			{
				output.addChar('%');
			}
		}
	}
}
