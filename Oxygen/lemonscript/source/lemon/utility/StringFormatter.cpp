/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/utility/StringFormatter.h"
#include "lemon/utility/FastStringStream.h"
#include "lemon/program/FunctionWrapper.h"


namespace lemon
{
	namespace
	{
		void addFormattedString(detail::FastStringStream& output, const AnyTypeWrapper& arg, std::string_view options)
		{
			// TODO: Support options

			switch (arg.mType->getClass())
			{
				case lemon::DataTypeDefinition::Class::INTEGER:
				{
					if (arg.mType == &lemon::PredefinedDataTypes::INT_8)
					{
						output.addDecimal(arg.mValue.get<int8>());
					}
					else if (arg.mType == &lemon::PredefinedDataTypes::UINT_8)
					{
						output.addDecimal(arg.mValue.get<uint8>());
					}
					else if (arg.mType == &lemon::PredefinedDataTypes::INT_16)
					{
						output.addDecimal(arg.mValue.get<int16>());
					}
					else if (arg.mType == &lemon::PredefinedDataTypes::UINT_16)
					{
						output.addDecimal(arg.mValue.get<uint16>());
					}
					else if (arg.mType == &lemon::PredefinedDataTypes::INT_32)
					{
						output.addDecimal(arg.mValue.get<int32>());
					}
					else if (arg.mType == &lemon::PredefinedDataTypes::UINT_32)
					{
						output.addDecimal(arg.mValue.get<uint32>());
					}
					else if (arg.mType == &lemon::PredefinedDataTypes::INT_64)
					{
						output.addDecimal(arg.mValue.get<int64>());
					}
					else
					{
						output.addDecimal(arg.mValue.get<uint64>());
					}
					break;
				}

				case lemon::DataTypeDefinition::Class::FLOAT:
				{
					if (arg.mType->getBytes() == 4)
					{
						output.addFloat(arg.mValue.get<float>());
					}
					else
					{
						output.addDouble(arg.mValue.get<double>());
					}
					break;
				}

				case lemon::DataTypeDefinition::Class::STRING:
				{
					lemon::Runtime* runtime = lemon::Runtime::getActiveRuntime();
					if (nullptr != runtime)
					{
						const lemon::FlyweightString* str = runtime->resolveStringByKey(arg.mValue.get<uint64>());
						if (nullptr != str)
						{
							output.addString(str->getString());
						}
					}
					break;
				}

				default:
					break;
			}
		}

		void addFormattedString(detail::FastStringStream& output, std::string_view expression, size_t numArguments, const AnyTypeWrapper* args, size_t counter)
		{
			if (expression.empty())
			{
				if (counter < numArguments)
				{
					addFormattedString(output, args[counter], "");
				}
				return;
			}

			// Find the colon, if there is one
			const size_t colonPosition = expression.find_first_of(':', 0);
			const std::string_view beforeColon = (colonPosition == std::string_view::npos) ? expression : expression.substr(0, colonPosition);
			const std::string_view options = (colonPosition == std::string_view::npos) ? std::string_view() : expression.substr(colonPosition + 1);

			size_t index = 0;
			for (char ch : beforeColon)
			{
				if (ch >= '0' && ch < '9')
				{
					index = index * 10 + (ch - '0');
				}
				else
				{
					RMX_ERROR("Invalid expression {" << expression << "} in 'string.build': Before the colon, only a single number is supported", break);
				}
			}

			if (index < numArguments)
			{
				addFormattedString(output, args[index], options);
			}
		}
	}



	void StringFormatter::buildFormattedString(detail::FastStringStream& output, std::string_view formatString, size_t numArguments, const AnyTypeWrapper* args)
	{
		const char* fmtPtr = formatString.data();
		const char* fmtEnd = fmtPtr + formatString.length();
		size_t expressionCounter = 0;

		for (; fmtPtr < fmtEnd; ++fmtPtr)
		{
			// Continue until getting a '{' character
			{
				const char* fmtStart = fmtPtr;
				while (fmtPtr != fmtEnd && *fmtPtr != '{' && *fmtPtr != '}')
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

			// Was this the last character?
			const int remaining = (int)(fmtEnd - fmtPtr);
			if (remaining < 2)
			{
				output.addChar(*fmtPtr);
				break;
			}

			// Handling of '}' outside of a format expression
			if (*fmtPtr == '}')
			{
				output.addChar(*fmtPtr);

				// Interpret "}}" as '}'
				if (fmtPtr[1] == '}')
					++fmtPtr;
				continue;
			}

			// Evaluate the expression
			{
				++fmtPtr;
				const char* fmtStart = fmtPtr;

				// Check for "{{", i.e. escaped '{'
				if (*fmtPtr == '{')
				{
					output.addChar('{');
					continue;
				}

				// Search for the ending '}'
				while (fmtPtr != fmtEnd && *fmtPtr != '}')
				{
					++fmtPtr;
				}
				if (fmtPtr == fmtEnd)
				{
					output.addString(fmtStart, (int)(fmtPtr - fmtStart));
					break;
				}

				const std::string_view innerExpression(fmtStart, (int)(fmtPtr - fmtStart));
				addFormattedString(output, innerExpression, numArguments, args, expressionCounter);
				++expressionCounter;
			}
		}
	}
}
