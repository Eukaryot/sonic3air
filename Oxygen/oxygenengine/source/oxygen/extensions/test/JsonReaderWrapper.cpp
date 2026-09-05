/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/extensions/test/JsonReaderWrapper.h"
#include "oxygen/extensions/test/JsonReader.h"
#include "oxygen/extensions/test/TestExtension.h"


JsonReader* JsonReaderWrapper::getJsonReader()
{
	// TODO: This basic implementation is pretty much hard-coded for TestExtension
	switch (mHandle)
	{
		case 1:   return &TestExtension::instance().getMainJsonReader();
		default:  return nullptr;
	}
}
