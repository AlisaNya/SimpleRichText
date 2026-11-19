// Copyright UnrealEssence. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/RichTextBlockDecorator.h"
#include "Framework/Text/ITextDecorator.h"
#include "UObject/WeakObjectPtrTemplates.h"

class URichTextBlock;

/** Names of the tag and its attributes that SimpleRichText generates, e.g.
 *  <seg c="FF0000FF" f="/Game/Fonts/Roboto.Roboto" tf="Bold" s="32">text</>
 */
namespace SimpleRichTextTags
{
	inline constexpr const TCHAR* Tag = TEXT("seg");
	inline constexpr const TCHAR* AttrColor = TEXT("c");      // RRGGBBAA hex (sRGB)
	inline constexpr const TCHAR* AttrFont = TEXT("f");       // soft object path of UFont / UFontFace
	inline constexpr const TCHAR* AttrTypeface = TEXT("tf");  // typeface name inside the font
	inline constexpr const TCHAR* AttrSize = TEXT("s");       // font size
}

/** Decorator that turns <seg ...>text</> into a text run with overridden color / font / size. */
class SIMPLERICHTEXT_API FSimpleRichTextDecorator : public ITextDecorator //, URichTextBlockDecorator
{
public:
	explicit FSimpleRichTextDecorator(const URichTextBlock* InOwner);

	virtual bool Supports(const FTextRunParseResults& RunParseResult, const FString& Text) const override;

	virtual TSharedRef<ISlateRun> Create(
		const TSharedRef<FTextLayout>& TextLayout,
		const FTextRunParseResults& RunParseResult,
		const FString& OriginalText,
		const TSharedRef<FString>& ModelText,
		const ISlateStyle* Style) override;

	/** Escapes characters that would break the markup (\ < >). */
	static FString EscapeText(const FString& In);
	/** Reverse of EscapeText. */
	static FString UnescapeText(const FString& In);

private:
	TWeakObjectPtr<const URichTextBlock> Owner;
};
