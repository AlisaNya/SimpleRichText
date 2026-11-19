// Copyright UnrealEssence. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/RichTextBlock.h"
#include "SimpleRichTextBlock.generated.h"

/** How the segments are entered in the Details panel. */
UENUM(BlueprintType)
enum class ESimpleRichTextInputMode : uint8
{
	/** Every segment has its own text field. */
	PerSegment UMETA(DisplayName = "Per Segment Fields"),
	/** One field with the whole text; segments are the pieces between separators, formats apply in order. */
	SingleField UMETA(DisplayName = "Single Field + Separator")
};

/**
 * One piece of text with its own formatting.
 * Edited in the Details panel via a custom row (see SimpleRichTextEditor module).
 */
USTRUCT(BlueprintType)
struct SIMPLERICHTEXT_API FSimpleRichTextSegment
{
	GENERATED_BODY()

	/** The text of this segment. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Segment", meta = (MultiLine = true))
	FText Text;

	/** Color of this segment. A newly added segment inherits the color of the previous one. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Segment")
	FLinearColor Color = FLinearColor::White;

	/** Font asset (UFont / UFontFace). Empty = default font of the RichTextBlock. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Segment", meta = (AllowedClasses = "/Script/Engine.Font,/Script/Engine.FontFace"))
	TSoftObjectPtr<UObject> Font;

	/** Typeface name inside the font (e.g. Regular, Bold, Italic). None = default. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Segment")
	FName Typeface;

	/** Font size. 0 = default size of the RichTextBlock. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Segment", meta = (ClampMin = "0", UIMin = "0", UIMax = "128"))
	int32 Size = 0;
};

/**
 * RichTextBlock with a segment list instead of hand-written tags.
 * Segments are converted to <seg ...>text</> markup automatically,
 * everything else (style set, decorators, wrapping, ...) is the normal RichTextBlock.
 */
UCLASS(meta = (DisplayName = "Simple Rich Text Block"))
class SIMPLERICHTEXT_API USimpleRichTextBlock : public URichTextBlock
{
	GENERATED_BODY()

public:
	/**
	 * Formatted pieces of text, displayed one after another as a single text.
	 * Editing the segments writes the generated markup into the standard Text field.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Content")
	TArray<FSimpleRichTextSegment> Segments;

	/**
	 * Off (default): Text is a normal RichTextBlock field. Segments only write markup into it when you edit them,
	 * so you can still type tags by hand or call SetText as usual.
	 * On: Text is regenerated from Segments on every load / widget rebuild, so manual changes to Text are overwritten.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Content")
	bool bAlwaysRebuildFromSegments = false;

	/** How segments are edited in Details. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Content")
	ESimpleRichTextInputMode InputMode = ESimpleRichTextInputMode::PerSegment;

	/**
	 * Single Field mode: the whole text. It is split by SegmentSeparator, piece N gets the format of segment N.
	 * If there are more pieces than segments, new segments copy the format of the last one.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Content", meta = (MultiLine = true, EditCondition = "InputMode == ESimpleRichTextInputMode::SingleField", EditConditionHides))
	FText CombinedText;

	/** Single Field mode: the string that separates pieces inside CombinedText. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Content", meta = (EditCondition = "InputMode == ESimpleRichTextInputMode::SingleField", EditConditionHides))
	FString SegmentSeparator = TEXT("|");

	/** Sets CombinedText, splits it into segments and refreshes Text. */
	UFUNCTION(BlueprintCallable, Category = "SimpleRichText")
	void SetCombinedText(const FText& NewCombinedText);

	/**
	 * Sets the whole text and the formats in one call.
	 * NewCombinedText is split by SegmentSeparator; piece N gets Formats[N] (the Text field inside Formats is ignored).
	 * If there are more pieces than formats, the extra pieces copy the last format.
	 */
	UFUNCTION(BlueprintCallable, Category = "SimpleRichText")
	void SetCombinedTextWithFormats(const FText& NewCombinedText, const TArray<FSimpleRichTextSegment>& Formats);

	/** Splits CombinedText by SegmentSeparator into Segments (texts only, formats stay) and refreshes Text. */
	void ApplyCombinedText();

	/** Joins the texts of all segments with SegmentSeparator. */
	FString BuildCombinedFromSegments() const;

	/** Replace all segments and refresh the text. */
	UFUNCTION(BlueprintCallable, Category = "SimpleRichText")
	void SetSegments(const TArray<FSimpleRichTextSegment>& InSegments);

	UFUNCTION(BlueprintCallable, Category = "SimpleRichText")
	void SetSegmentText(int32 Index, const FText& NewText);

	UFUNCTION(BlueprintCallable, Category = "SimpleRichText")
	void SetSegmentColor(int32 Index, FLinearColor NewColor);

	/** Sets the color of every segment. */
	UFUNCTION(BlueprintCallable, Category = "SimpleRichText")
	void SetAllSegmentsColor(FLinearColor NewColor);

	/** Sets font size of one segment (0 = default size). */
	UFUNCTION(BlueprintCallable, Category = "SimpleRichText")
	void SetSegmentSize(int32 Index, int32 NewSize);

	/** Replaces the whole format (color, font, typeface, size) of one segment, its text stays. */
	UFUNCTION(BlueprintCallable, Category = "SimpleRichText")
	void SetSegmentFormat(int32 Index, const FSimpleRichTextSegment& Format);

	/**
	 * Applies formats to the existing segments by order, the texts stay.
	 * Formats[N] goes to segment N; segments beyond the end of the array get the last format.
	 */
	UFUNCTION(BlueprintCallable, Category = "SimpleRichText")
	void SetFormats(const TArray<FSimpleRichTextSegment>& Formats);

	/**
	 * Adds a segment at the end and returns its index.
	 * Text and Color are taken from NewSegment as they are. If Font is empty, Font and Typeface are inherited
	 * from the previous last segment; if Size is 0, the size is inherited from it as well.
	 */
	UFUNCTION(BlueprintCallable, Category = "SimpleRichText")
	int32 AddSegment(const FSimpleRichTextSegment& NewSegment);

	/**
	 * Inserts a segment at Index. Same rules as AddSegment: Text and Color come from NewSegment,
	 * an empty Font (with its Typeface) and a Size of 0 are inherited from the segment before it
	 * (when inserting at 0, from the segment that gets pushed to position 1).
	 */
	UFUNCTION(BlueprintCallable, Category = "SimpleRichText")
	void InsertSegment(int32 Index, const FSimpleRichTextSegment& NewSegment);

	UFUNCTION(BlueprintCallable, Category = "SimpleRichText")
	void RemoveSegment(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "SimpleRichText")
	void ClearSegments();

	UFUNCTION(BlueprintPure, Category = "SimpleRichText")
	int32 GetSegmentCount() const;

	UFUNCTION(BlueprintPure, Category = "SimpleRichText")
	FText GetSegmentText(int32 Index) const;

	/** All segment texts joined without any tags. */
	UFUNCTION(BlueprintPure, Category = "SimpleRichText")
	FString GetPlainText() const;

	/** Returns the generated markup (what actually gets fed to the RichTextBlock). */
	UFUNCTION(BlueprintPure, Category = "SimpleRichText")
	FString BuildMarkup() const;

	/** Writes the markup generated from Segments into Text and pushes it to the widget. */
	void RefreshText();

	/** Default text style of the given RichTextBlock (used by the decorator as base style). */
	static const FTextBlockStyle& GetBaseTextStyle(const URichTextBlock* RichText);

	//~ Begin UWidget / UObject interface
	virtual void SynchronizeProperties() override;
	virtual void PostLoad() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual const FText GetPaletteCategory() override;
#endif
	//~ End UWidget / UObject interface

protected:
	/** Fills in what Target leaves unset (no font, size 0) from Source. Used by AddSegment / InsertSegment. */
	static void InheritMissingFormat(FSimpleRichTextSegment& Target, const FSimpleRichTextSegment& Source);

	/** After a change of segment texts / count: keeps CombinedText in sync (Single Field mode) and refreshes Text. */
	void NotifySegmentsChanged();

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void CreateDecorators(TArray<TSharedRef<class ITextDecorator>>& OutDecorators) override;
};

