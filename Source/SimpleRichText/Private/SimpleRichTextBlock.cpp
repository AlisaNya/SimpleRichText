// Copyright UnrealEssence. All Rights Reserved.

#include "SimpleRichTextBlock.h"
#include "SimpleRichTextDecorator.h"

#define LOCTEXT_NAMESPACE "SimpleRichText"

void USimpleRichTextBlock::SetSegments(const TArray<FSimpleRichTextSegment>& InSegments)
{
	Segments = InSegments;
	NotifySegmentsChanged();
}

void USimpleRichTextBlock::SetSegmentText(int32 Index, const FText& NewText)
{
	if (Segments.IsValidIndex(Index))
	{
		Segments[Index].Text = NewText;
		NotifySegmentsChanged();
	}
}

void USimpleRichTextBlock::SetSegmentColor(int32 Index, FLinearColor NewColor)
{
	if (Segments.IsValidIndex(Index))
	{
		Segments[Index].Color = NewColor;
		RefreshText();
	}
}

void USimpleRichTextBlock::InheritMissingFormat(FSimpleRichTextSegment& Target, const FSimpleRichTextSegment& Source)
{
	// No font given: inherit font together with its typeface (a typeface name only makes sense for its own font).
	if (Target.Font.IsNull())
	{
		Target.Font = Source.Font;
		if (Target.Typeface.IsNone())
		{
			Target.Typeface = Source.Typeface;
		}
	}
	if (Target.Size <= 0)
	{
		Target.Size = Source.Size;
	}
}

void USimpleRichTextBlock::NotifySegmentsChanged()
{
	if (InputMode == ESimpleRichTextInputMode::SingleField)
	{
		CombinedText = FText::FromString(BuildCombinedFromSegments());
	}
	RefreshText();
}

void USimpleRichTextBlock::SetAllSegmentsColor(FLinearColor NewColor)
{
	for (FSimpleRichTextSegment& Segment : Segments)
	{
		Segment.Color = NewColor;
	}
	RefreshText();
}

void USimpleRichTextBlock::SetSegmentSize(int32 Index, int32 NewSize)
{
	if (Segments.IsValidIndex(Index))
	{
		Segments[Index].Size = FMath::Max(0, NewSize);
		RefreshText();
	}
}

void USimpleRichTextBlock::SetSegmentFormat(int32 Index, const FSimpleRichTextSegment& Format)
{
	if (Segments.IsValidIndex(Index))
	{
		const FText KeepText = Segments[Index].Text;
		Segments[Index] = Format;
		Segments[Index].Text = KeepText;
		RefreshText();
	}
}

void USimpleRichTextBlock::SetFormats(const TArray<FSimpleRichTextSegment>& Formats)
{
	if (Formats.Num() == 0)
	{
		return;
	}

	for (int32 i = 0; i < Segments.Num(); ++i)
	{
		const FText KeepText = Segments[i].Text;
		Segments[i] = Formats[FMath::Min(i, Formats.Num() - 1)];
		Segments[i].Text = KeepText;
	}
	RefreshText();
}

int32 USimpleRichTextBlock::AddSegment(const FSimpleRichTextSegment& NewSegment)
{
	FSimpleRichTextSegment Result = NewSegment;

	if (Segments.Num() > 0)
	{
		InheritMissingFormat(Result, Segments.Last());
	}

	const int32 NewIndex = Segments.Add(Result);
	NotifySegmentsChanged();
	return NewIndex;
}

void USimpleRichTextBlock::InsertSegment(int32 Index, const FSimpleRichTextSegment& NewSegment)
{
	Index = FMath::Clamp(Index, 0, Segments.Num());

	FSimpleRichTextSegment Result = NewSegment;
	if (Index > 0)
	{
		InheritMissingFormat(Result, Segments[Index - 1]);
	}
	else if (Segments.Num() > 0)
	{
		InheritMissingFormat(Result, Segments[0]);
	}

	Segments.Insert(Result, Index);
	NotifySegmentsChanged();
}

void USimpleRichTextBlock::RemoveSegment(int32 Index)
{
	if (Segments.IsValidIndex(Index))
	{
		Segments.RemoveAt(Index);
		NotifySegmentsChanged();
	}
}

void USimpleRichTextBlock::ClearSegments()
{
	Segments.Reset();
	NotifySegmentsChanged();
}

int32 USimpleRichTextBlock::GetSegmentCount() const
{
	return Segments.Num();
}

FText USimpleRichTextBlock::GetSegmentText(int32 Index) const
{
	return Segments.IsValidIndex(Index) ? Segments[Index].Text : FText::GetEmpty();
}

FString USimpleRichTextBlock::GetPlainText() const
{
	FString Plain;
	for (const FSimpleRichTextSegment& Segment : Segments)
	{
		Plain += Segment.Text.ToString();
	}
	return Plain;
}

void USimpleRichTextBlock::SetCombinedText(const FText& NewCombinedText)
{
	CombinedText = NewCombinedText;
	ApplyCombinedText();
}

void USimpleRichTextBlock::SetCombinedTextWithFormats(const FText& NewCombinedText, const TArray<FSimpleRichTextSegment>& Formats)
{
	Segments = Formats;
	CombinedText = NewCombinedText;
	ApplyCombinedText();
}

void USimpleRichTextBlock::ApplyCombinedText()
{
	const FString Whole = CombinedText.ToString();

	TArray<FString> Pieces;
	if (SegmentSeparator.IsEmpty())
	{
		Pieces.Add(Whole);
	}
	else
	{
		Whole.ParseIntoArray(Pieces, *SegmentSeparator, /*InCullEmpty=*/false);
	}
	if (Pieces.Num() == 0)
	{
		Pieces.Add(FString());
	}

	// Segments count == pieces count. New segments inherit the format of the last existing one.
	while (Segments.Num() < Pieces.Num())
	{
		FSimpleRichTextSegment NewSegment = Segments.Num() > 0 ? Segments.Last() : FSimpleRichTextSegment();
		Segments.Add(NewSegment);
	}
	if (Segments.Num() > Pieces.Num())
	{
		Segments.SetNum(Pieces.Num());
	}

	for (int32 i = 0; i < Pieces.Num(); ++i)
	{
		Segments[i].Text = FText::FromString(Pieces[i]);
	}

	RefreshText();
}

FString USimpleRichTextBlock::BuildCombinedFromSegments() const
{
	FString Joined;
	for (int32 i = 0; i < Segments.Num(); ++i)
	{
		if (i > 0)
		{
			Joined += SegmentSeparator;
		}
		Joined += Segments[i].Text.ToString();
	}
	return Joined;
}

FString USimpleRichTextBlock::BuildMarkup() const
{
	FString Out;
	for (const FSimpleRichTextSegment& Segment : Segments)
	{
		const FString Raw = Segment.Text.ToString();
		if (Raw.IsEmpty())
		{
			continue;
		}

		Out += FString::Printf(TEXT("<%s %s=\"%s\""),
			SimpleRichTextTags::Tag,
			SimpleRichTextTags::AttrColor,
			*Segment.Color.ToFColor(true).ToHex());

		if (!Segment.Font.IsNull())
		{
			Out += FString::Printf(TEXT(" %s=\"%s\""),
				SimpleRichTextTags::AttrFont,
				*Segment.Font.ToSoftObjectPath().ToString());
		}
		if (!Segment.Typeface.IsNone())
		{
			Out += FString::Printf(TEXT(" %s=\"%s\""),
				SimpleRichTextTags::AttrTypeface,
				*Segment.Typeface.ToString());
		}
		if (Segment.Size > 0)
		{
			Out += FString::Printf(TEXT(" %s=\"%d\""),
				SimpleRichTextTags::AttrSize,
				Segment.Size);
		}

		Out += TEXT(">");
		Out += FSimpleRichTextDecorator::EscapeText(Raw);
		Out += TEXT("</>");
	}
	return Out;
}

void USimpleRichTextBlock::RefreshText()
{
	SetText(FText::FromString(BuildMarkup()));
}

const FTextBlockStyle& USimpleRichTextBlock::GetBaseTextStyle(const URichTextBlock* RichText)
{
	// NOTE: if this name differs in your engine version
	// (e.g. GetCurrentDefaultTextStyle), this is the only place to change.
	return RichText->GetDefaultTextStyle();
}

void USimpleRichTextBlock::SynchronizeProperties()
{
	if (bAlwaysRebuildFromSegments)
	{
		RefreshText();
	}
	Super::SynchronizeProperties();
}

void USimpleRichTextBlock::PostLoad()
{
	Super::PostLoad();
	if (bAlwaysRebuildFromSegments)
	{
		RefreshText();
	}
}

TSharedRef<SWidget> USimpleRichTextBlock::RebuildWidget()
{
	// The Slate widget takes its initial text from Text, so it must be up to date before Super.
	if (bAlwaysRebuildFromSegments)
	{
		RefreshText();
	}
	return Super::RebuildWidget();
}

void USimpleRichTextBlock::CreateDecorators(TArray<TSharedRef<ITextDecorator>>& OutDecorators)
{
	Super::CreateDecorators(OutDecorators);
	OutDecorators.Add(MakeShared<FSimpleRichTextDecorator>(this));
}

#if WITH_EDITOR
void USimpleRichTextBlock::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName MemberName = PropertyChangedEvent.MemberProperty ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;
	const bool bSegmentsChanged = (MemberName == GET_MEMBER_NAME_CHECKED(USimpleRichTextBlock, Segments));

	const bool bSingleField = (InputMode == ESimpleRichTextInputMode::SingleField);
	const bool bCombinedChanged = (MemberName == GET_MEMBER_NAME_CHECKED(USimpleRichTextBlock, CombinedText))
		|| (MemberName == GET_MEMBER_NAME_CHECKED(USimpleRichTextBlock, SegmentSeparator));
	const bool bModeChanged = (MemberName == GET_MEMBER_NAME_CHECKED(USimpleRichTextBlock, InputMode));

	if (bSingleField && bCombinedChanged)
	{
		ApplyCombinedText();
	}
	else if (bSingleField && bModeChanged)
	{
		if (CombinedText.IsEmpty() && Segments.Num() > 0)
		{
			// Switching over: show the existing segments as one text.
			CombinedText = FText::FromString(BuildCombinedFromSegments());
		}
		else if (!CombinedText.IsEmpty())
		{
			ApplyCombinedText();
		}
	}
	else if (bAlwaysRebuildFromSegments)
	{
		RefreshText();
	}
	else if (bSegmentsChanged && !BuildMarkup().IsEmpty())
	{
		// Don't wipe a hand-written Text just because an (still empty) segment was added.
		RefreshText();
	}
}

const FText USimpleRichTextBlock::GetPaletteCategory()
{
	return LOCTEXT("PaletteCategory", "Simple");
}
#endif

#undef LOCTEXT_NAMESPACE
