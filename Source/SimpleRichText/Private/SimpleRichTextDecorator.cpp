// Copyright UnrealEssence. All Rights Reserved.

#include "SimpleRichTextDecorator.h"
#include "SimpleRichTextBlock.h"

#include "Framework/Text/IRichTextMarkupParser.h"
#include "Framework/Text/ISlateRun.h"
#include "Framework/Text/SlateTextRun.h"
#include "Styling/SlateTypes.h"
#include "UObject/SoftObjectPath.h"

FSimpleRichTextDecorator::FSimpleRichTextDecorator(const URichTextBlock* InOwner)
	: Owner(InOwner)
{
}

bool FSimpleRichTextDecorator::Supports(const FTextRunParseResults& RunParseResult, const FString& Text) const
{
	return RunParseResult.Name == SimpleRichTextTags::Tag;
}

TSharedRef<ISlateRun> FSimpleRichTextDecorator::Create(
	const TSharedRef<FTextLayout>& TextLayout,
	const FTextRunParseResults& RunParseResult,
	const FString& OriginalText,
	const TSharedRef<FString>& ModelText,
	const ISlateStyle* Style)
{
	// Text of the run -> model string
	FTextRange ModelRange;
	ModelRange.BeginIndex = ModelText->Len();
	const FString Content = OriginalText.Mid(
		RunParseResult.ContentRange.BeginIndex,
		RunParseResult.ContentRange.EndIndex - RunParseResult.ContentRange.BeginIndex);
	*ModelText += UnescapeText(Content);
	ModelRange.EndIndex = ModelText->Len();

	// Attributes
	FRunInfo RunInfo(RunParseResult.Name);
	for (const TPair<FString, FTextRange>& Pair : RunParseResult.MetaData)
	{
		const int32 Begin = Pair.Value.BeginIndex;
		const int32 End = Pair.Value.EndIndex;
		RunInfo.MetaData.Add(Pair.Key, OriginalText.Mid(Begin, End - Begin));
	}

	// Base style = the default style of the owning RichTextBlock
	FTextBlockStyle TextStyle = FTextBlockStyle::GetDefault();
	if (const URichTextBlock* OwnerPtr = Owner.Get())
	{
		TextStyle = USimpleRichTextBlock::GetBaseTextStyle(OwnerPtr);
	}

	// Color
	if (const FString* ColorHex = RunInfo.MetaData.Find(SimpleRichTextTags::AttrColor))
	{
		const FLinearColor Color(FColor::FromHex(*ColorHex)); // sRGB -> linear
		TextStyle.SetColorAndOpacity(FSlateColor(Color));
	}

	// Font / typeface / size
	FSlateFontInfo FontInfo = TextStyle.Font;
	int32 FontSize = static_cast<int32>(FontInfo.Size);
	FName Typeface = FontInfo.TypefaceFontName;
	const UObject* FontObject = FontInfo.FontObject;
	bool bFontChanged = false;

	if (const FString* SizeStr = RunInfo.MetaData.Find(SimpleRichTextTags::AttrSize))
	{
		const int32 NewSize = FCString::Atoi(**SizeStr);
		if (NewSize > 0)
		{
			FontSize = NewSize;
			bFontChanged = true;
		}
	}
	if (const FString* TypefaceStr = RunInfo.MetaData.Find(SimpleRichTextTags::AttrTypeface))
	{
		if (!TypefaceStr->IsEmpty())
		{
			Typeface = FName(**TypefaceStr);
			bFontChanged = true;
		}
	}
	if (const FString* FontPath = RunInfo.MetaData.Find(SimpleRichTextTags::AttrFont))
	{
		if (!FontPath->IsEmpty())
		{
			if (UObject* Loaded = FSoftObjectPath(*FontPath).TryLoad())
			{
				FontObject = Loaded;
				bFontChanged = true;
			}
		}
	}

	if (bFontChanged)
	{
		if (FontObject)
		{
			FontInfo = FSlateFontInfo(FontObject, FontSize, Typeface);
		}
		else
		{
			FontInfo.Size = FontSize;
			FontInfo.TypefaceFontName = Typeface;
		}
		TextStyle.SetFont(FontInfo);
	}

	return FSlateTextRun::Create(RunInfo, ModelText, TextStyle, ModelRange);
}

FString FSimpleRichTextDecorator::EscapeText(const FString& In)
{
	FString Out;
	Out.Reserve(In.Len() + 8);
	for (int32 i = 0; i < In.Len(); ++i)
	{
		const TCHAR C = In[i];
		if (C == TEXT('\\') || C == TEXT('<') || C == TEXT('>'))
		{
			Out.AppendChar(TEXT('\\'));
		}
		Out.AppendChar(C);
	}
	return Out;
}

FString FSimpleRichTextDecorator::UnescapeText(const FString& In)
{
	FString Out;
	Out.Reserve(In.Len());
	for (int32 i = 0; i < In.Len(); ++i)
	{
		if (In[i] == TEXT('\\') && i + 1 < In.Len())
		{
			const TCHAR Next = In[i + 1];
			if (Next == TEXT('\\') || Next == TEXT('<') || Next == TEXT('>'))
			{
				Out.AppendChar(Next);
				++i;
				continue;
			}
		}
		Out.AppendChar(In[i]);
	}
	return Out;
}
