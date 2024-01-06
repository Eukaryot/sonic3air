/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>
#include <functional>
#include <optional>


class CustomDebugSidePanelCategory;
class DebugSidePanelCategory;
class DebugTracking;
class Drawer;

class DebugSidePanel : public GuiBase
{
public:
	static const uint64 INVALID_KEY = 0xffffffffffffffffULL;

	class Builder
	{
	friend class DebugSidePanel;

	public:
		struct TextLine
		{
			String mText;
			Color mColor = Color::WHITE;
			int mIntend = 0;
			uint64 mKey = INVALID_KEY;
			int mLineSpacing = 12;
			int mOptionValue = -1;
			std::string mCodeLocation;
		};

	public:
		TextLine& addLine(std::string_view text, const Color& color = Color::WHITE, int intend = 0, uint64 key = INVALID_KEY, int lineSpacing = 12);
		TextLine& addOption(std::string_view text, bool value, const Color& color = Color::WHITE, int intend = 0, uint64 key = INVALID_KEY, int lineSpacing = 12);
		void addSpacing(int lineSpacing);
		void addCallStack(DebugTracking& debugTracking, int callFrameIndex, std::optional<size_t> firstProgramCounter);

	private:
		std::vector<TextLine> mTextLines;
	};

public:
	DebugSidePanel();
	~DebugSidePanel();

	virtual void initialize() override;
	virtual void deinitialize() override;
	virtual void keyboard(const rmx::KeyboardEvent& ev) override;
	virtual void mouse(const rmx::MouseEvent& ev) override;
	virtual void update(float timeElapsed) override;
	virtual void render() override;

	DebugSidePanelCategory& createGameCategory(size_t identifier, const std::string& header, char shortCharacter, const std::function<void(DebugSidePanelCategory&,Builder&,uint64)>& callback);

	bool setupCustomCategory(std::string_view header, char shortCharacter);
	bool addOption(std::string_view text, bool defaultValue);
	void addEntry(uint64 key);
	void addLine(std::string_view text, int indent, const Color& color);
	bool isEntryHovered(uint64 key);

private:
	DebugSidePanelCategory& addCategory(size_t identifier, std::string_view header, char shortCharacter = 0);
	void buildInternalCategoryContent(DebugSidePanelCategory& category, Builder& builder, Drawer& drawer);

private:
	Font mSmallFont;

	std::vector<DebugSidePanelCategory*> mCategories;
	size_t mActiveCategoryIndex = 0;

	CustomDebugSidePanelCategory* mSetupCustomCategory = nullptr;

	Builder mBuilder;
	uint64 mMouseOverKey = INVALID_KEY;
	size_t mMouseOverTab = 0;

	int mSidePanelWidth = 360;
	bool mChangingSidePanelWidth = false;
};
