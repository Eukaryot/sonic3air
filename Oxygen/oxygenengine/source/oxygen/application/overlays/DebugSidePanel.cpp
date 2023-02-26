/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/overlays/DebugSidePanel.h"
#include "oxygen/application/overlays/DebugSidePanelCategory.h"
#include "oxygen/application/mainview/GameView.h"
#include "oxygen/application/Application.h"
#include "oxygen/application/Configuration.h"
#include "oxygen/application/EngineMain.h"
#include "oxygen/application/video/VideoOut.h"
#include "oxygen/rendering/parts/RenderParts.h"
#include "oxygen/simulation/CodeExec.h"
#include "oxygen/simulation/EmulatorInterface.h"
#include "oxygen/simulation/LemonScriptProgram.h"
#include "oxygen/simulation/LogDisplay.h"
#include "oxygen/simulation/Simulation.h"

#include <lemon/program/Function.h>


namespace
{
	enum CategoryIdentifier
	{
		CATEGORY_NONE = 0,
		CATEGORY_CALL_FRAMES,
		CATEGORY_UNKNOWN_ADDRESSES,
		CATEGORY_WATCHES,
		CATEGORY_VARIABLES,
		CATEGORY_RENDERED_GEOMETRIES,
		CATEGORY_VRAM_WRITES,
		CATEGORY_LOG,
		_NUM_INTERNAL_CATEGORIES
	};
}


DebugSidePanel::Builder::TextLine& DebugSidePanel::Builder::addLine(const String& text, const Color& color, int intend, uint64 key, int lineSpacing)
{
	TextLine& textLine = vectorAdd(mTextLines);
	textLine.mText = text;
	textLine.mColor = color;
	textLine.mIntend = intend;
	textLine.mKey = key;
	textLine.mLineSpacing = lineSpacing;
	return textLine;
}

DebugSidePanel::Builder::TextLine& DebugSidePanel::Builder::addOption(const String& text, bool value, const Color& color, int intend, uint64 key, int lineSpacing)
{
	TextLine& textLine = vectorAdd(mTextLines);
	textLine.mText = text;
	textLine.mColor = color;
	textLine.mIntend = intend;
	textLine.mKey = key;
	textLine.mLineSpacing = lineSpacing;
	textLine.mOptionValue = value ? 1 : 0;
	return textLine;
}

void DebugSidePanel::Builder::addSpacing(int lineSpacing)
{
	TextLine& textLine = vectorAdd(mTextLines);
	textLine.mLineSpacing = lineSpacing;
}


DebugSidePanel::DebugSidePanel()
{
	addCategory(CATEGORY_NONE, "");		// We need a dummy for this
	addCategory(CATEGORY_CALL_FRAMES, "CALL FRAMES");
	addCategory(CATEGORY_UNKNOWN_ADDRESSES, "UNKNOWN ADDRESSES");
	addCategory(CATEGORY_WATCHES, "WATCHES");
	addCategory(CATEGORY_VARIABLES, "GLOBAL DEFINES");
	addCategory(CATEGORY_RENDERED_GEOMETRIES, "RENDERED GEOMETRIES");
	addCategory(CATEGORY_VRAM_WRITES, "VRAM WRITES");
	addCategory(CATEGORY_LOG, "LOG");
}

DebugSidePanel::~DebugSidePanel()
{
	for (DebugSidePanelCategory* category : mCategories)
	{
		delete category;
	}
}

void DebugSidePanel::initialize()
{
	mSmallFont.loadFromFile("data/font/freefont_sampled.json");
	mSmallFont.addFontProcessor(std::make_shared<ShadowFontProcessor>(Vec2i(1, 1), 1.0f));
}

void DebugSidePanel::deinitialize()
{
}

void DebugSidePanel::keyboard(const rmx::KeyboardEvent& ev)
{
	GuiBase::keyboard(ev);

	if (ev.state)
	{
		DebugSidePanelCategory& category = *mCategories[mActiveCategoryIndex];
		switch (ev.key)
		{
			case SDLK_HOME:
				mActiveCategoryIndex = (mActiveCategoryIndex + mCategories.size() - 1) % mCategories.size();
				break;
			case SDLK_END:
				mActiveCategoryIndex = (mActiveCategoryIndex + 1) % mCategories.size();
				break;

			case SDLK_PAGEUP:
				category.mScrollPosition = clamp(category.mScrollPosition - 100, 0, category.mScrollSize);
				break;
			case SDLK_PAGEDOWN:
				category.mScrollPosition = clamp(category.mScrollPosition + 100, 0, category.mScrollSize);
				break;
		}
	}
}

void DebugSidePanel::mouse(const rmx::MouseEvent& ev)
{
	if (ev.state && ev.button == rmx::MouseButton::Left)
	{
		if (mMouseOverTab > 0)
		{
			mActiveCategoryIndex = mMouseOverTab;
		}
		else if (mMouseOverKey != INVALID_KEY)
		{
			DebugSidePanelCategory& category = *mCategories[mActiveCategoryIndex];
			auto& openKeys = category.mOpenKeys;
			auto it = openKeys.find(mMouseOverKey);
			if (it == openKeys.end())
			{
				openKeys.insert(mMouseOverKey);
			}
			else
			{
				openKeys.erase(it);
			}
			category.mChangedKey = mMouseOverKey;
		}
	}
}

void DebugSidePanel::update(float timeElapsed)
{
	if (FTX::mouseWheel() != 0)
	{
		DebugSidePanelCategory& category = *mCategories[mActiveCategoryIndex];
		category.mScrollPosition -= FTX::mouseWheel() * 75;
		category.mScrollPosition = clamp(category.mScrollPosition, 0, category.mScrollSize);
	}
}

void DebugSidePanel::render()
{
	mRect = FTX::screenRect();
	const Vec2i screenSize = mRect.getSize();

	GuiBase::render();

	mBuilder.mTextLines.clear();
	if (mActiveCategoryIndex == 0)
	{
		mChangingSidePanelWidth = false;
		return;
	}

	Drawer& drawer = EngineMain::instance().getDrawer();
	DebugSidePanelCategory& category = *mCategories[mActiveCategoryIndex];

	// Draw background
	Recti mainRect = getRect();
	mainRect.x = mainRect.width - mSidePanelWidth;
	mainRect.width = mSidePanelWidth;
	drawer.drawRect(mainRect, Color(0.0f, 0.0f, 0.0f, 0.5f));

	Recti rect = mainRect;
	rect.y += 10 - category.mScrollPosition;

	// Draw the tabs
	{
		Recti r(rect.x + 25, rect.y, 20, 20);
		DebugSidePanelCategory::Type lastType = DebugSidePanelCategory::Type::INTERNAL;
		mMouseOverTab = false;
		for (size_t i = 1; i < mCategories.size(); ++i)
		{
			if (mCategories[i]->mType != lastType)		// Add a gap between panel tabs of different types
			{
				r.x += 10;
				lastType = mCategories[i]->mType;
			}

			const bool mouseInRect = FTX::mouseIn(r);
			if (mouseInRect)
			{
				drawer.drawRect(r, Color(1.0f, 1.0f, 0.0f, 0.5f));
				mMouseOverTab = i;
			}

			const char buffer[2] = { mCategories[i]->mShortCharacter, 0 };
			Color color = Color::WHITE;
			switch (mCategories[i]->mType)
			{
				case DebugSidePanelCategory::Type::INTERNAL:	color = ((i == mActiveCategoryIndex) ? Color::YELLOW : Color(0.6f, 0.6f, 0.5f, 0.75f));	 break;
				case DebugSidePanelCategory::Type::CUSTOM:		color = ((i == mActiveCategoryIndex) ? Color::GREEN : Color(0.5f, 0.7f, 0.5f, 0.75f));	 break;
				case DebugSidePanelCategory::Type::GAME:		color = ((i == mActiveCategoryIndex) ? Color::GREEN : Color(0.5f, 0.7f, 0.5f, 0.75f));	 break;
			}
			drawer.printText(mSmallFont, r, buffer, 5, color);

			r.x += 20;
		}
	}
	rect.y += 35;

	// Draw the header
	rect.x += 18;
	drawer.printText(mSmallFont, rect, category.mHeader, 1, Color::WHITE);
	rect.y += 16;

	// Build the content
	switch (category.mType)
	{
		case DebugSidePanelCategory::Type::INTERNAL:
		{
			buildInternalCategoryContent(category, mBuilder, drawer);
			break;
		}

		case DebugSidePanelCategory::Type::CUSTOM:
		{
			static_cast<CustomDebugSidePanelCategory&>(category).buildCategoryContent(mBuilder, drawer, mMouseOverKey);
			break;
		}

		case DebugSidePanelCategory::Type::GAME:
		{
			if (category.mCallback)
			{
				category.mCallback(category, mBuilder, mMouseOverKey);
			}
			break;
		}
	}

	// Reset changed key
	category.mChangedKey = INVALID_KEY;

	// Update mouse over
	{
		const Recti backup = rect;
		mMouseOverKey = INVALID_KEY;
		for (const Builder::TextLine& line : mBuilder.mTextLines)
		{
			rect.y += line.mLineSpacing;
			if (!line.mText.empty())
			{
				Recti selectionRect = rect;
				selectionRect.height = 12;

				// Check if mouse cursor is inside
				if (line.mKey != INVALID_KEY && FTX::mouseIn(selectionRect))
				{
					mMouseOverKey = line.mKey;
				}
			}
		}
		rect = backup;
	}

	// Draw mouse-over highlight for width change
	{
		const Recti sensorRect(mainRect.x - 6, mainRect.y, 12, mainRect.height);
		const bool hovered = FTX::mouseIn(sensorRect);
		if (hovered || mChangingSidePanelWidth)
		{
			drawer.drawRect(sensorRect, mChangingSidePanelWidth ? Color(0.1f, 0.1f, 0.1f) : Color(0.0f, 0.0f, 0.0));
		}

		if (mChangingSidePanelWidth)
		{
			if (!FTX::mouseState(rmx::MouseButton::Left))
			{
				mChangingSidePanelWidth = false;
			}
		}
		else
		{
			if (hovered && FTX::mouseChange(rmx::MouseButton::Left) && FTX::mouseState(rmx::MouseButton::Left))
			{
				mChangingSidePanelWidth = true;
			}
		}

		if (mChangingSidePanelWidth)
		{
			mSidePanelWidth = (int)mRect.width - FTX::mousePos().x;
		}
	}

	// Now draw the texts
	for (const Builder::TextLine& line : mBuilder.mTextLines)
	{
		rect.y += line.mLineSpacing;
		if (rect.y < -12 || rect.y >= screenSize.y)	// The 12 is just a guess for the maximum visible line height
			continue;
		if (line.mText.empty())
			continue;

		Recti selectionRect = rect;
		selectionRect.x -= 8;
		selectionRect.height = 12;

		Recti textRect = rect;
		textRect.x += line.mIntend;
		textRect.height = 12;

		if (line.mKey == mMouseOverKey && mMouseOverKey != INVALID_KEY)
		{
			drawer.drawRect(selectionRect, Color(1.0f, 1.0f, 0.0f, 0.5f));
		}

		if (line.mOptionValue >= 0)
		{
			drawer.printText(mSmallFont, textRect, (line.mOptionValue != 0) ? "[x" : "[ ", 1, line.mColor);
			textRect.x += 10;
			drawer.printText(mSmallFont, textRect, "]", 1, line.mColor);
			textRect.x += 10;
			textRect.width -= 20;
		}

		drawer.printText(mSmallFont, textRect, line.mText, 1, line.mColor);
	}

	category.mScrollSize = std::max(category.mScrollPosition + rect.y - screenSize.y / 5, 0);

	if (category.mScrollSize > 0)
	{
		const int totalHeight = screenSize.y - 48;
		const float scrollBarSize = (float)totalHeight / (float)(category.mScrollSize + screenSize.y);
		const float scrollBarPos = (float)category.mScrollPosition / (float)category.mScrollSize * (1.0f - scrollBarSize);

		drawer.drawRect(Recti(FTX::screenWidth() - 6, 42 + roundToInt(scrollBarPos * totalHeight), 2, roundToInt(scrollBarSize * totalHeight)), Color(1.0f, 1.0f, 1.0f, 0.6f));
	}

	drawer.performRendering();
}

DebugSidePanelCategory& DebugSidePanel::createGameCategory(size_t identifier, const std::string& header, char shortCharacter, const std::function<void(DebugSidePanelCategory&,Builder&,uint64)>& callback)
{
	DebugSidePanelCategory& category = addCategory(identifier, header, shortCharacter);
	category.mType = DebugSidePanelCategory::Type::GAME;
	category.mCallback = callback;
	return category;
}

bool DebugSidePanel::setupCustomCategory(std::string_view header, char shortCharacter)
{
	// Search for the category
	mSetupCustomCategory = nullptr;
	int index = -1;
	for (int i = _NUM_INTERNAL_CATEGORIES; i < (int)mCategories.size(); ++i)
	{
		if (mCategories[i]->mType == DebugSidePanelCategory::Type::CUSTOM && mCategories[i]->mHeader == header)
		{
			mSetupCustomCategory = static_cast<CustomDebugSidePanelCategory*>(mCategories[i]);
			index = i;
			break;
		}
	}

	if (nullptr == mSetupCustomCategory)
	{
		// Create a new one
		if (mCategories.size() >= 20)
			return false;

		index = (int)mCategories.size();
		mSetupCustomCategory = new CustomDebugSidePanelCategory();

		mSetupCustomCategory->mHeader = header;
		mSetupCustomCategory->mShortCharacter = shortCharacter;
		mCategories.push_back(mSetupCustomCategory);
	}

	mSetupCustomCategory->onSetup();

	return ((size_t)index == mActiveCategoryIndex);
}

bool DebugSidePanel::addOption(std::string_view text, bool defaultValue)
{
	if (nullptr == mSetupCustomCategory)
		return false;

	return mSetupCustomCategory->addOption(text, defaultValue);
}

void DebugSidePanel::addEntry(uint64 key)
{
	if (nullptr == mSetupCustomCategory)
		return;

	return mSetupCustomCategory->addEntry(key);
}

void DebugSidePanel::addLine(std::string_view text, int indent, const Color& color)
{
	if (nullptr == mSetupCustomCategory)
		return;

	return mSetupCustomCategory->addLine(text, indent, color);
}

bool DebugSidePanel::isEntryHovered(uint64 key)
{
	if (nullptr == mSetupCustomCategory)
		return false;

	return mSetupCustomCategory->isEntryHovered(key);
}

DebugSidePanelCategory& DebugSidePanel::addCategory(size_t identifier, std::string_view header, char shortCharacter)
{
	DebugSidePanelCategory* category = new DebugSidePanelCategory();
	category->mIdentifier = identifier;
	category->mHeader = header;
	category->mShortCharacter = shortCharacter ? shortCharacter : header.empty() ? 0 : header[0];

	mCategories.push_back(category);
	return *category;
}

void DebugSidePanel::buildInternalCategoryContent(DebugSidePanelCategory& category, Builder& builder, Drawer& drawer)
{
	CodeExec& codeExec = Application::instance().getSimulation().getCodeExec();
	EmulatorInterface& emulatorInterface = codeExec.getEmulatorInterface();

	switch (category.mIdentifier)
	{
		case CATEGORY_CALL_FRAMES:
		{
			codeExec.processCallFrames();
			const auto& callFrames = codeExec.getCallFrames();

			const bool showAllHitFunctions  = (category.mOpenKeys.count(0x1000000000000001) > 0);
			const bool visualizationSorting = (category.mOpenKeys.count(0x1000000000000002) > 0);
			const bool showOpcodesExecuted  = (category.mOpenKeys.count(0x1000000000000003) > 0);

			builder.addOption("Show all hit functions", showAllHitFunctions, Color::CYAN, 0, 0x1000000000000001);
			builder.addSpacing(12);

			if (showAllHitFunctions)
			{
				builder.addOption("Sort by filename", visualizationSorting, Color::CYAN, 0, 0x1000000000000002);
				builder.addSpacing(12);

				std::map<uint32, const lemon::Function*> functions;
				for (const CodeExec::CallFrame& callFrame : callFrames)
				{
					if (nullptr != callFrame.mFunction)
					{
						const uint32 key = (callFrame.mAddress != 0xffffffff) ? callFrame.mAddress : (0x80000000 + callFrame.mFunction->getID());
						functions.emplace(key, callFrame.mFunction);
					}
				}

				std::vector<std::pair<uint32, const lemon::Function*>> sortedFunctions;
				for (const auto& pair : functions)
				{
					sortedFunctions.emplace_back(pair);
				}
				std::sort(sortedFunctions.begin(), sortedFunctions.end(),
					[visualizationSorting](const std::pair<uint32, const lemon::Function*>& a, const std::pair<uint32, const lemon::Function*>& b)
					{
						if (visualizationSorting)
						{
							const std::wstring& filenameA = (a.second->getType() == lemon::Function::Type::SCRIPT) ? static_cast<const lemon::ScriptFunction*>(a.second)->mSourceFileInfo->mFilename : L"";
							const std::wstring& filenameB = (b.second->getType() == lemon::Function::Type::SCRIPT) ? static_cast<const lemon::ScriptFunction*>(b.second)->mSourceFileInfo->mFilename : L"";
							if (filenameA != filenameB)
							{
								return (filenameA < filenameB);
							}
						}
						if ((a.first & 0x80000000) && (b.first & 0x80000000))
						{
							return a.second->getName().getString() < b.second->getName().getString();
						}
						else
						{
							return a.first < b.first;
						}
					}
				);

				for (const auto& pair : sortedFunctions)
				{
					const String filename = (pair.second->getType() == lemon::Function::Type::SCRIPT) ? WString(static_cast<const lemon::ScriptFunction*>(pair.second)->mSourceFileInfo->mFilename).toString() : "";
					String line;
					if (visualizationSorting && !filename.empty())
						line << filename << " | ";
					line << ((pair.first < 0x80000000) ? String(0, "0x%06x: ", pair.first) : "");
					line << pair.second->getName().getString();
					if (!visualizationSorting && !filename.empty())
						line << " | " << filename;
					builder.addLine(line, Color::WHITE, 10);
				}
			}
			else
			{
				builder.addOption("Show profiling samples", showOpcodesExecuted, Color::CYAN, 0, 0x1000000000000003);
				builder.addSpacing(12);

				int ignoreDepth = 0;
				for (const CodeExec::CallFrame& callFrame : callFrames)
				{
					// Ignore debugging stuff
					if (nullptr != callFrame.mFunction && rmx::startsWith(callFrame.mFunction->getName().getString(), "debug"))
						continue;

					if (ignoreDepth != 0)
					{
						if (callFrame.mDepth > ignoreDepth)
							continue;
						else
							ignoreDepth = 0;
					}

					const uint64 key = (nullptr == callFrame.mFunction) ? INVALID_KEY : callFrame.mFunction->getID();
					const bool isOpen = (callFrame.mAnyChildFailed || callFrame.mDepth < 1 || category.mOpenKeys.count(key) != 0);

					std::string postfix;
					if (!isOpen)
					{
						ignoreDepth = callFrame.mDepth;
						postfix = " (+)";
					}

					const int indent = callFrame.mDepth * 16;
					if (callFrame.mType == CodeExec::CallFrame::Type::FAILED_HOOK)
					{
						builder.addLine(String(0, "%06x", callFrame.mAddress) + postfix, Color::RED, indent);
					}
					else
					{
						Color color = Color::WHITE;
						if (showOpcodesExecuted)
						{
							postfix += " <" + std::to_string(callFrame.mSteps) + ">";
							const float log = log10f((float)clamp((int)callFrame.mSteps, 100, 1000000));
							color.setHSL(Vec3f(0.75f - log / 6.0f, 1.0f, 0.5f));
						}
						else
						{
							color = (callFrame.mType == CodeExec::CallFrame::Type::SCRIPT_STACK) ? Color::fromABGR32(0xffa0a0a0) :
									(callFrame.mType == CodeExec::CallFrame::Type::SCRIPT_DIRECT) ? Color::WHITE : Color::YELLOW;
						}
						RMX_ASSERT(nullptr != callFrame.mFunction, "Invalid function pointer");
						builder.addLine(std::string(callFrame.mFunction->getName().getString()) + postfix, color, indent, key);
					}
				}
				if (callFrames.size() == CodeExec::CALL_FRAMES_LIMIT)
				{
					builder.addLine("[!] Reached call frames limit", Color::RED, 0);
				}
			}
			break;
		}

		case CATEGORY_UNKNOWN_ADDRESSES:
		{
			const auto& unknownAddresses = codeExec.getUnknownAddresses();
			for (uint32 address : unknownAddresses)
			{
				builder.addLine(String(0, "0x%06x", address), Color::RED);
			}
			break;
		}

		case CATEGORY_WATCHES:
		{
			const std::vector<CodeExec::Watch*>& watches = codeExec.getWatches();
			for (const CodeExec::Watch* watch : watches)
			{
				// Display 0xffff???? instead of 0x00ff????
				uint32 displayAddress = watch->mAddress;
				if ((displayAddress & 0x00ff0000) == 0x00ff0000)
					displayAddress |= 0xff000000;

				if (watch->mHits.empty())
				{
					builder.addLine(String(0, "Watch 0x%08x (0x%02x bytes)", displayAddress, watch->mBytes), Color::fromABGR32(0xffa0a0a0));
					if (watch->mBytes <= 4)
						builder.addLine(String(0, "= %s at %s", rmx::hexString(watch->mInitialValue, watch->mBytes * 2).c_str(), watch->mLastHitLocation.toString(codeExec).c_str()), Color::fromABGR32(0xffa0a0a0), 8);
				}
				else
				{
					builder.addLine(String(0, "Watch 0x%08x (0x%02x bytes)", displayAddress, watch->mBytes), Color::WHITE);
					if (watch->mBytes <= 4)
						builder.addLine(String(0, "= %s initially", rmx::hexString(watch->mInitialValue, watch->mBytes * 2).c_str()), Color::WHITE, 8);

					for (size_t hitIndex = 0; hitIndex < watch->mHits.size(); ++hitIndex)
					{
						const auto& hit = *watch->mHits[hitIndex];
						const uint64 key = ((uint64)watch->mAddress << 32) + hitIndex;
						Builder::TextLine* textLine;
						if (watch->mBytes <= 4)
							textLine = &builder.addLine(String(0, "= %s at %s", rmx::hexString(hit.mWrittenValue, watch->mBytes * 2).c_str(), hit.mLocation.toString(codeExec).c_str()), Color::WHITE, 8, key);
						else
							textLine = &builder.addLine(String(0, "u%d[0xffff%04x] = %s at %s", hit.mBytes * 8, hit.mAddress, rmx::hexString(hit.mWrittenValue, std::min(hit.mBytes * 2, 8)).c_str(), hit.mLocation.toString(codeExec).c_str()), Color::WHITE, 8, key);

						// Just a test
					#if 0
						if (key == category.mChangedKey)
						{
							std::string scriptFilename;
							uint32 lineNumber;
							codeExec.getLemonScriptProgram().resolveLocation(*hit.mLocation.mFunction, (uint32)hit.mLocation.mProgramCounter, scriptFilename, lineNumber);
							textLine->mCodeLocation = "\"" + scriptFilename + "\":" + std::to_string(lineNumber);

							// TODO: The script file name needs to contains the full file path for this to work, not just the file name itself
							//  -> Maybe store a list of source files in the module?
							//  -> Also, this only makes sense if the sources are available, i.e. not for modules loaded from a serialized binary file
						#if defined(PLATFORM_WINDOWS)
							::system(("code -r -g " + textLine->mCodeLocation).c_str());
						#endif
						}
					#endif

						if (category.mOpenKeys.count(key) != 0)
						{
							std::vector<uint64> callStack;
							codeExec.getCallStackFromCallFrameIndex(callStack, hit.mCallFrameIndex);
							for (uint64 functionNameHash : callStack)
							{
								const std::string_view name = codeExec.getLemonScriptProgram().getFunctionNameByHash(functionNameHash);
								builder.addLine(String(name), Color::fromABGR32(0xffc0c0c0), 32);
							}
						}
					}
				}
				builder.addSpacing(20);
			}
			break;
		}

		case CATEGORY_VARIABLES:
		{
			const std::vector<LemonScriptProgram::GlobalDefine>& globalDefines = codeExec.getLemonScriptProgram().getGlobalDefines();
			uint64 lastCategoryHash = 1;	// This 1 is a magic number to tell us that it's the first global define (it certainly won't appear as an actual hash)
			for (const LemonScriptProgram::GlobalDefine& globalDefine : globalDefines)
			{
				const uint64 key = globalDefine.mAddress + ((uint64)globalDefine.mBytes << 32);
				const bool active = (category.mOpenKeys.count(key) != 0);
				const int indent = active ? 0 : 16;
				const Color color = active ? Color::WHITE : Color::fromABGR32(0xffc0c0c0);

				if (lastCategoryHash != globalDefine.mCategoryHash || lastCategoryHash == 1)
				{
					lastCategoryHash = globalDefine.mCategoryHash;
					builder.addSpacing(8);
				}
				else if (active)
				{
					builder.addSpacing(4);
				}

				const std::string_view& name = globalDefine.mName.getString();
				switch (globalDefine.mBytes)
				{
					case 1:
					{
						const uint8 value = emulatorInterface.readMemory8(globalDefine.mAddress);
						builder.addLine(String(0, "%.*s   = 0x%02x", name.length(), name.data(), value), color, indent, key);
						break;
					}

					case 2:
					{
						const uint16 value = emulatorInterface.readMemory16(globalDefine.mAddress);
						builder.addLine(String(0, "%.*s   = 0x%04x", name.length(), name.data(), value), color, indent, key);
						break;
					}

					case 4:
					{
						const uint32 value = emulatorInterface.readMemory32(globalDefine.mAddress);
						builder.addLine(String(0, "%.*s   = 0x%08x", name.length(), name.data(), value), color, indent, key);
						break;
					}
				}

				if (active)
				{
					if ((globalDefine.mAddress & 0xff0000) == 0xff0000 || (globalDefine.mAddress & 0xf00000) == 0x800000)
					{
						const uint64 key2 = key + (1ULL << 32);
						const bool watched = (category.mOpenKeys.count(key2) != 0);
						if (key2 == category.mChangedKey)
						{
							// Add or remove watch
							if (watched)
							{
								codeExec.addWatch(globalDefine.mAddress, globalDefine.mBytes, true);
							}
							else
							{
								codeExec.removeWatch(globalDefine.mAddress, globalDefine.mBytes);
							}
						}
						builder.addOption("Watched", watched, color, indent + 20, key2);
					}
					builder.addSpacing(5);
				}
			}
			break;
		}

		case CATEGORY_RENDERED_GEOMETRIES:
		{
			const auto& geometries = VideoOut::instance().getGeometries();
			uint64 key = 1;
			for (const Geometry* geometry : geometries)
			{
				bool ignore = false;
				switch (geometry->getType())
				{
					case Geometry::Type::PLANE:
					{
						const PlaneGeometry& pg = *static_cast<const PlaneGeometry*>(geometry);
						builder.addLine(String(0, "0x%04x:   Plane:  %d%s", pg.mRenderQueue, pg.mPlaneIndex, pg.mPriorityFlag ? " -- prio" : ""), Color::WHITE);
						break;
					}

					case Geometry::Type::SPRITE:
					{
						const SpriteManager::SpriteInfo& info = static_cast<const SpriteGeometry*>(geometry)->mSpriteInfo;
						const char* spriteType = nullptr;
						Color color = Color::fromABGR32(0xffa0a0a0);
						Recti objectRect(info.mPosition.x, info.mPosition.y, 32, 32);
						switch (info.getType())
						{
							case SpriteManager::SpriteInfo::Type::VDP:
							{
								spriteType = "VDP sprite";
								color.setABGR32(0xffffffc0);
								SpriteManager::VdpSpriteInfo& vsi = (SpriteManager::VdpSpriteInfo&)info;
								objectRect.width = vsi.mSize.x * 8;
								objectRect.height = vsi.mSize.y * 8;
								break;
							}
							case SpriteManager::SpriteInfo::Type::PALETTE:
							{
								spriteType = "Palette sprite";
								color.setABGR32(0xffffc0ff);
								SpriteManager::PaletteSpriteInfo& psi = (SpriteManager::PaletteSpriteInfo&)info;
								objectRect.setPos(objectRect.getPos() + psi.mPivotOffset);
								objectRect.width = psi.mSize.x;
								objectRect.height = psi.mSize.y;
								break;
							}
							case SpriteManager::SpriteInfo::Type::COMPONENT:
							{
								spriteType = "Component sprite";
								color.setABGR32(0xffffc0e0);
								SpriteManager::ComponentSpriteInfo& csi = (SpriteManager::ComponentSpriteInfo&)info;
								objectRect.setPos(objectRect.getPos() + csi.mPivotOffset);
								objectRect.width = csi.mSize.x;
								objectRect.height = csi.mSize.y;
								break;
							}
							case SpriteManager::SpriteInfo::Type::MASK:
							{
								spriteType = "Sprite mask";
								color.setABGR32(0xffc0ffff);
								SpriteManager::SpriteMaskInfo& smi = (SpriteManager::SpriteMaskInfo&)info;
								objectRect.width = smi.mSize.x;
								objectRect.height = smi.mSize.y;
								break;
							}
							default: break;
						}

						if (nullptr != spriteType)
						{
							if (mMouseOverKey == key)
							{
								Rectf translatedRect;
								Application::instance().getGameView().translateRectIntoScreenCoords(translatedRect, objectRect);
								drawer.drawRect(translatedRect, Color(0.0f, 1.0f, 0.5f, 0.75f));
							}
							builder.addLine(String(0, "0x%04x:   %s:  (%d, %d)%s", info.mRenderQueue, spriteType, info.mPosition.x, info.mPosition.y, info.mPriorityFlag ? " -- prio" : ""), color, 0, key);
						}
						break;
					}

					case Geometry::Type::RECT:
					{
						// Ignore this type, it's only for debugging anyways
						ignore = true;
						break;
					}

					case Geometry::Type::EFFECT_BLUR:
					{
						builder.addLine(String(0, "0x%04x:   Blur effect", geometry->mRenderQueue), Color::fromABGR32(0xffc0ffff));
						break;
					}

					case Geometry::Type::VIEWPORT:
					{
						const ViewportGeometry& vg = *static_cast<const ViewportGeometry*>(geometry);
						builder.addLine(String(0, "0x%04x:   Viewport:  (%d, %d, %d, %d)", geometry->mRenderQueue, vg.mRect.x, vg.mRect.y, vg.mRect.width, vg.mRect.height), Color::fromABGR32(0xffc0ffff), 0, key);
						break;
					}

					default:
					{
						builder.addLine("Unknown geometry type", Color::fromABGR32(0xffa0a0a0));
						break;
					}
				}

				if (!ignore)
				{
					builder.addSpacing(2);
					++key;
				}
			}
			break;
		}

		case CATEGORY_VRAM_WRITES:
		{
			const PlaneManager& planeManager = VideoOut::instance().getRenderParts().getPlaneManager();
			const uint16 startAddressPlaneA = planeManager.getPlaneBaseVRAMAddress(PlaneManager::PLANE_A);
			const uint16 startAddressPlaneB = planeManager.getPlaneBaseVRAMAddress(PlaneManager::PLANE_B);
			const uint16 endAddressPlaneA = startAddressPlaneA + (uint16)planeManager.getPlaneSizeInVRAM(PlaneManager::PLANE_A);
			const uint16 endAddressPlaneB = startAddressPlaneB + (uint16)planeManager.getPlaneSizeInVRAM(PlaneManager::PLANE_B);
			const ScrollOffsetsManager& scrollOffsetsManager = VideoOut::instance().getRenderParts().getScrollOffsetsManager();
			const uint16 startAddressScrollOffsets = scrollOffsetsManager.getHorizontalScrollTableBase();
			const uint16 endAddressScrollOffsets = startAddressScrollOffsets + 0x200;

			// Create a sorted list
			std::vector<CodeExec::VRAMWrite*> writes = codeExec.getVRAMWrites();
			std::sort(writes.begin(), writes.end(), [](const CodeExec::VRAMWrite* a, const CodeExec::VRAMWrite* b) { return a->mAddress < b->mAddress; } );

			const bool showPlaneA = (category.mOpenKeys.count(0x1000000000000001) == 0);
			const bool showPlaneB = (category.mOpenKeys.count(0x1000000000000002) == 0);
			const bool showScroll = (category.mOpenKeys.count(0x1000000000000003) == 0);
			const bool showOthers = (category.mOpenKeys.count(0x1000000000000004) == 0);

			builder.addOption("Plane A", showPlaneA, Color::CYAN, 0, 0x1000000000000001);
			builder.addOption("Plane B", showPlaneB, Color::CYAN, 0, 0x1000000000000002);
			builder.addOption("Scroll Offsets", showScroll, Color::CYAN, 0, 0x1000000000000003);
			builder.addOption("Patterns and others", showOthers, Color::CYAN, 0, 0x1000000000000004);
			builder.addSpacing(12);

			for (const CodeExec::VRAMWrite* write : writes)
			{
				const uint64 key = ((uint64)write->mAddress << 32) + write->mSize;
				String line(0, "0x%04x (0x%02x bytes) at %s", write->mAddress, write->mSize, write->mLocation.toString(codeExec).c_str());
				Color color = Color::WHITE;
				if (write->mAddress >= startAddressPlaneA && write->mAddress < endAddressPlaneA)
				{
					if (!showPlaneA)
						continue;
					line = String("[Plane A] ") + line;
					color = Color::fromABGR32(0xffc0ffff);
				}
				else if (write->mAddress >= startAddressPlaneB && write->mAddress < endAddressPlaneB)
				{
					if (!showPlaneB)
						continue;
					line = String("[Plane B] ") + line;
					color = Color::fromABGR32(0xffffc0ff);
				}
				else if (write->mAddress >= startAddressScrollOffsets && write->mAddress < endAddressScrollOffsets)
				{
					if (!showScroll)
						continue;
					line = String("[Scroll Offsets] ") + line;
					color = Color::fromABGR32(0xffffffc0);
				}
				else
				{
					if (!showOthers)
						continue;
				}
				builder.addLine(line, color, 0, key);

				if (category.mOpenKeys.count(key) != 0)
				{
					std::vector<uint64> callStack;
					codeExec.getCallStackFromCallFrameIndex(callStack, write->mCallFrameIndex);
					for (uint64 functionNameHash : callStack)
					{
						const std::string_view name = codeExec.getLemonScriptProgram().getFunctionNameByHash(functionNameHash);
						builder.addLine(String(name), Color::fromABGR32(0xffc0c0c0), 32);
					}
				}
			}

			if (writes.size() == codeExec.getVRAMWrites().capacity())
			{
				builder.addLine("[Reached the limit]", Color::WHITE, 4);
			}
			break;
		}

		case CATEGORY_LOG:
		{
			const auto& entries = LogDisplay::instance().getScriptLogEntries();
			for (const auto& pair : entries)
			{
				const LogDisplay::ScriptLogEntry& entry = pair.second;
				if (!entry.mEntries.empty())
				{
					const bool hasCurrent = (entry.mLastUpdate >= Application::instance().getSimulation().getFrameNumber() - 1);

					const float brightness = hasCurrent ? 1.0f : 0.6f;
					const Color color(brightness, brightness, brightness);

					builder.addLine(String(0, "%s:", pair.first.c_str()), color, 8);
					builder.addSpacing(-12);
					for (size_t i = 0; i < entry.mEntries.size(); ++i)
					{
						const LogDisplay::ScriptLogSingleEntry& singleEntry = entry.mEntries[i];
						const uint64 key = (((uint64)entry.mEntries.size() << 16) + ((uint64)i << 32)) ^ rmx::getMurmur2_64(String(singleEntry.mValue));
						builder.addLine(singleEntry.mValue, color, 56, key);

						if (category.mOpenKeys.count(key) != 0)
						{
							std::vector<uint64> callStack;
							codeExec.getCallStackFromCallFrameIndex(callStack, singleEntry.mCallFrameIndex);
							for (uint64 functionNameHash : callStack)
							{
								const std::string_view name = codeExec.getLemonScriptProgram().getFunctionNameByHash(functionNameHash);
								builder.addLine(String(name), Color::fromABGR32(0xffc0c0c0), 32);
							}
						}
					}
					builder.addSpacing(3);
				}
			}
			break;
		}
	}
}
