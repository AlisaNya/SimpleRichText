// Copyright UnrealEssence. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

/**
 * Details customization for USimpleRichTextBlock:
 * replaces the plain "Text" + "Segments" array with a list of rows
 *   [ text field ][ color ][ gear (font / typeface / size) ][ + ][ - ]
 */
class FSimpleRichTextBlockCustomization : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
};
