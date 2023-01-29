/*
*	rmx Library
*	Copyright (C) 2008-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#define RMX_LIB
#include "../source/rmxmedia.h"


class App : public GuiBase
{
public:
	virtual void initialize() override
	{
		Bitmap bitmap;
		bitmap.create(400, 224, 0xff602020);
		mTexture.load(bitmap);

		mFont.setSize(10.0f);
	}

	virtual void update(float timeElapsed) override
	{
	}

	virtual void render() override
	{
		FTX::Painter->drawRect(Rectf(0, 0, 1200, 672), mTexture);
		FTX::Painter->print(mFont, Rectf(100, 100, 0, 0), "Hello World");
	}

private:
	Texture mTexture;
	Font mFont;
};


int main(int argc, char** argv)
{
	INIT_RMX;

	FTX::System->initialize();
	FTX::Video->initialize(rmx::VideoConfig(false, 1200, 672, "rmx_test"));
	FTX::Video->setPixelView();

	FTX::System->run<App>();

	FTX::System->exit();

	return 0;
}
