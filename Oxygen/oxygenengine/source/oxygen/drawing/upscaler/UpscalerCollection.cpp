/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/drawing/upscaler/UpscalerCollection.h"


void UpscalerCollection::loadUpscalers()
{
	// Internal upscalers, hard-coded for now
	{
		// Simple upscaler (no actual shader used)
		UpscalerDefinition& upscaler = addUpscaler("", "Simple");
		upscaler.addVariant(0, "Simple");
	}

	{
		// Pixel upscaler
		UpscalerDefinition& upscaler = addUpscaler("pixel", "Pixel");
		upscaler.addVariant(0, "Sharp");
		upscaler.addVariant(1, "Soft 1").mFilterLinear = true;
		upscaler.addVariant(2, "Soft 2").mFilterLinear = true;

		upscaler.addOpenGLShader(L"data/shader/upscaler_soft.shader", "Standard");
		upscaler.addOpenGLShader(L"data/shader/upscaler_soft.shader", "Scanlines");
	}

#if !defined(PLATFORM_VITA)
	{
		// xBRZ upscaler
		UpscalerDefinition& upscaler = addUpscaler("xbrz", "xBRZ");
		upscaler.addVariant(0, "xBRZ");

		upscaler.addOpenGLShader(L"data/shader/upscaler_xbrz-freescale-pass0.shader", "Standard");
		upscaler.addOpenGLShader(L"data/shader/upscaler_xbrz-freescale-pass1.shader", "Standard");
	}

	{
		// HQx upscaler
		UpscalerDefinition& upscaler = addUpscaler("hqx", "HQx");
		upscaler.addVariant(0, "HQ2x");
		upscaler.addVariant(1, "HQ3x");
		upscaler.addVariant(2, "HQ4x");

		upscaler.addOpenGLShader(L"data/shader/upscaler_hqx.shader", "Standard_2x");
		upscaler.addOpenGLShader(L"data/shader/upscaler_hqx.shader", "Standard_3x");
		upscaler.addOpenGLShader(L"data/shader/upscaler_hqx.shader", "Standard_4x");

		upscaler.mLookupTextures.push_back(L"data/shader/hq2x.png");
		upscaler.mLookupTextures.push_back(L"data/shader/hq3x.png");
		upscaler.mLookupTextures.push_back(L"data/shader/hq4x.png");
	}
#endif
}

int UpscalerCollection::getUpscalerIndexByName(std::string_view name) const
{
	return getUpscalerIndexByNameHash(rmx::getMurmur2_64(name));
}

int UpscalerCollection::getUpscalerIndexByNameHash(uint64 hash) const
{
	for (size_t index = 0; index < mUpscalers.size(); ++index)
	{
		if (mUpscalers[index].mNameHash == hash)
			return (int)index;
	}
	return 0;
}

void UpscalerCollection::setCurrentConfigUpscalerByIndex(int index)
{
	if (index < 0 || index >= (int)mUpscalers.size())
		index = 0;
	const UpscalerDefinition& upscaler = mUpscalers[index];

	Configuration::ScreenFilter& config = Configuration::instance().mScreenFilter;
	config.mUpscalerNameHash = upscaler.mNameHash;
	config.mUpscalerName = upscaler.mInternalName;
}

const UpscalerDefinition::Variant& UpscalerCollection::getCurrentConfigVariant() const
{
	const Configuration::ScreenFilter& config = Configuration::instance().mScreenFilter;
	const int upscalerIndex = getUpscalerIndexByNameHash(config.mUpscalerNameHash);
	const UpscalerDefinition& upscaler = mUpscalers[upscalerIndex];
	RMX_ASSERT(!upscaler.mVariants.empty(), "Upscaler must have at least one variant");

	int variantIndex = (upscalerIndex == 1) ? config.mPixelVariant : (upscalerIndex == 3) ? config.mHQxVariant : 0;
	if (variantIndex >= 0 && variantIndex < (int)upscaler.mVariants.size())
	{
		return upscaler.mVariants[variantIndex];
	}
	else
	{
		return upscaler.mVariants[0];
	}
}

void UpscalerCollection::changeCurrentConfigVariant(int direction)
{
	Configuration::ScreenFilter& config = Configuration::instance().mScreenFilter;
	int upscalerIndex = getUpscalerIndexByNameHash(config.mUpscalerNameHash);
	const UpscalerDefinition* upscaler = &mUpscalers[upscalerIndex];
	RMX_ASSERT(!upscaler->mVariants.empty(), "Upscaler must have at least one variant");

	int dummy = 0;
	int& variantIndex = (upscalerIndex == 1) ? config.mPixelVariant : (upscalerIndex == 3) ? config.mHQxVariant : dummy;

	if (direction < 0)
	{
		--variantIndex;
		if (variantIndex < 0)
		{
			upscalerIndex = (upscalerIndex + (int)mUpscalers.size() - 1) % (int)mUpscalers.size();
			upscaler = &mUpscalers[upscalerIndex];
			config.mUpscalerNameHash = upscaler->mNameHash;
			config.mUpscalerName = upscaler->mInternalName;
			int& newVariantIndex = (upscalerIndex == 1) ? config.mPixelVariant : (upscalerIndex == 3) ? config.mHQxVariant : dummy;
			newVariantIndex = (int)upscaler->mVariants.size() - 1;
		}
	}
	else
	{
		++variantIndex;
		if (variantIndex >= (int)upscaler->mVariants.size())
		{
			upscalerIndex = (upscalerIndex + 1) % (int)mUpscalers.size();
			upscaler = &mUpscalers[upscalerIndex];
			config.mUpscalerNameHash = upscaler->mNameHash;
			config.mUpscalerName = upscaler->mInternalName;
			int& newVariantIndex = (upscalerIndex == 1) ? config.mPixelVariant : (upscalerIndex == 3) ? config.mHQxVariant : dummy;
			newVariantIndex = 0;
		}
	}
}

UpscalerDefinition& UpscalerCollection::addUpscaler(std::string_view internalName, std::string_view displayName)
{
	UpscalerDefinition& upscaler = vectorAdd(mUpscalers);
	upscaler.mNameHash = rmx::getMurmur2_64(internalName);
	upscaler.mInternalName = internalName;
	upscaler.mDisplayName = displayName;
	return upscaler;
}
