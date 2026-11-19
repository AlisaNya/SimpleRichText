// Copyright UnrealEssence. All Rights Reserved.

#include "SimpleRichTextBlockCustomization.h"
#include "SimpleRichTextBlock.h"

#include "Components/RichTextBlock.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailCustomNodeBuilder.h"
#include "IPropertyUtilities.h"
#include "PropertyHandle.h"
#include "Styling/AppStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SimpleRichTextCustomization"

/** Builds one editable row per segment. */
class FSimpleRichTextSegmentsBuilder : public IDetailCustomNodeBuilder, public TSharedFromThis<FSimpleRichTextSegmentsBuilder>
{
public:
	FSimpleRichTextSegmentsBuilder(TSharedRef<IPropertyHandle> InSegmentsHandle, TSharedPtr<IPropertyHandle> InTextHandle)
		: SegmentsHandle(InSegmentsHandle)
		, TextHandle(InTextHandle)
		, ArrayHandle(InSegmentsHandle->AsArray())
	{
	}

	/** Must be called right after MakeShared (needs a valid shared pointer to bind delegates). */
	void Init()
	{
		if (ArrayHandle.IsValid())
		{
			ArrayHandle->SetOnNumElementsChanged(FSimpleDelegate::CreateSP(this, &FSimpleRichTextSegmentsBuilder::HandleNumElementsChanged));
		}
		SegmentsHandle->SetOnChildPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FSimpleRichTextSegmentsBuilder::HandleSegmentsEdited));
	}

	//~ IDetailCustomNodeBuilder
	virtual void SetOnRebuildChildren(FSimpleDelegate InOnRegenerateChildren) override { OnRebuildChildren = InOnRegenerateChildren; }
	virtual bool RequiresTick() const override { return false; }
	virtual bool InitiallyCollapsed() const override { return false; }
	virtual FName GetName() const override { return TEXT("SimpleRichTextSegments"); }

	virtual void GenerateHeaderRowContent(FDetailWidgetRow& NodeRow) override
	{
		NodeRow
		.NameContent()
		[
			SNew(STextBlock)
			.Text(IsSingleField() ? LOCTEXT("FormatsHeader", "Segment Formats (by piece order)") : LOCTEXT("SegmentsHeader", "Text Segments"))
			.Font(IDetailLayoutBuilder::GetDetailFontBold())
		]
		.ValueContent()
		.HAlign(HAlign_Left)
		[
			SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "SimpleButton")
			.Visibility(IsSingleField() ? EVisibility::Collapsed : EVisibility::Visible)
			.ToolTipText(LOCTEXT("AddFirstTip", "Add a text segment at the end"))
			.OnClicked(this, &FSimpleRichTextSegmentsBuilder::OnAddAtEnd)
			.ContentPadding(FMargin(4.f, 2.f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 4, 0)
				[
					SNew(SImage).Image(FAppStyle::GetBrush("Icons.Plus"))
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(LOCTEXT("AddSegment", "Add segment"))
				]
			]
		];
	}

	virtual void GenerateChildContent(IDetailChildrenBuilder& ChildrenBuilder) override
	{
		if (!ArrayHandle.IsValid())
		{
			return;
		}

		uint32 NumElements = 0;
		ArrayHandle->GetNumElements(NumElements);

		for (uint32 i = 0; i < NumElements; ++i)
		{
			const int32 Index = static_cast<int32>(i);
			TSharedRef<IPropertyHandle> Element = ArrayHandle->GetElement(Index);

			const bool bSingleField = IsSingleField();
			TSharedPtr<IPropertyHandle> SegTextHandle = Element->GetChildHandle(GET_MEMBER_NAME_CHECKED(FSimpleRichTextSegment, Text));
			TSharedPtr<IPropertyHandle> ColorHandle = Element->GetChildHandle(GET_MEMBER_NAME_CHECKED(FSimpleRichTextSegment, Color));
			if (!SegTextHandle.IsValid() || !ColorHandle.IsValid())
			{
				continue;
			}

			ChildrenBuilder.AddCustomRow(FText::GetEmpty())
			.WholeRowContent()
			[
				SNew(SHorizontalBox)

				// Text
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(0, 1, 4, 1)
				[
					bSingleField ? MakePreviewWidget(Index, SegTextHandle.ToSharedRef()) : SegTextHandle->CreatePropertyValueWidget()
				]

				// Color
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 4, 0)
				[
					SNew(SBox)
					.WidthOverride(56.f)
					[
						ColorHandle->CreatePropertyValueWidget()
					]
				]

				// Font / typeface / size popup
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 2, 0)
				[
					SNew(SComboButton)
					.HasDownArrow(false)
					.ToolTipText(LOCTEXT("FormatTip", "Font, typeface and size of this segment"))
					.ButtonContent()
					[
						SNew(SImage).Image(FAppStyle::GetBrush("Icons.Settings"))
					]
					.OnGetMenuContent(FOnGetContent::CreateSP(this, &FSimpleRichTextSegmentsBuilder::BuildFormatMenu, Element))
				]

				// Add after this one (copies the formatting, clears the text)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "SimpleButton")
					.Visibility(bSingleField ? EVisibility::Collapsed : EVisibility::Visible)
					.ToolTipText(LOCTEXT("AddAfterTip", "Add a segment after this one (inherits color and formatting)"))
					.OnClicked(this, &FSimpleRichTextSegmentsBuilder::OnAddAfter, Index)
					[
						SNew(SImage).Image(FAppStyle::GetBrush("Icons.Plus"))
					]
				]

				// Remove
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "SimpleButton")
					.Visibility(bSingleField ? EVisibility::Collapsed : EVisibility::Visible)
					.ToolTipText(LOCTEXT("RemoveTip", "Remove this segment"))
					.OnClicked(this, &FSimpleRichTextSegmentsBuilder::OnRemove, Index)
					[
						SNew(SImage).Image(FAppStyle::GetBrush("Icons.Minus"))
					]
				]
			];
		}
	}

private:
	void HandleNumElementsChanged()
	{
		OnRebuildChildren.ExecuteIfBound();
		HandleSegmentsEdited();
	}

	bool IsSingleField() const
	{
		TArray<UObject*> Objects;
		SegmentsHandle->GetOuterObjects(Objects);
		for (const UObject* Object : Objects)
		{
			if (const USimpleRichTextBlock* Block = Cast<USimpleRichTextBlock>(Object))
			{
				return Block->InputMode == ESimpleRichTextInputMode::SingleField;
			}
		}
		return false;
	}

	/** Read-only preview of the piece of text that belongs to this segment (Single Field mode). */
	TSharedRef<SWidget> MakePreviewWidget(int32 Index, TSharedRef<IPropertyHandle> SegmentTextHandle)
	{
		return SNew(STextBlock)
			.Text_Lambda([SegmentTextHandle, Index]()
			{
				FText Piece;
				SegmentTextHandle->GetValueAsDisplayText(Piece);
				const FText Shown = Piece.IsEmpty() ? LOCTEXT("EmptyPiece", "(empty)") : Piece;
				return FText::Format(LOCTEXT("PieceFmt", "{0}: {1}"), FText::AsNumber(Index + 1), Shown);
			})
			.OverflowPolicy(ETextOverflowPolicy::Ellipsis);
	}

	/** Pushes the generated markup into the standard Text field through its property handle,
	 *  so the Text row in Details updates and the change goes through the normal property system. */
	void HandleSegmentsEdited()
	{
		if (!TextHandle.IsValid())
		{
			return;
		}

		TArray<UObject*> Objects;
		SegmentsHandle->GetOuterObjects(Objects);
		if (Objects.Num() != 1)
		{
			return;
		}

		const USimpleRichTextBlock* Block = Cast<USimpleRichTextBlock>(Objects[0]);
		if (!Block)
		{
			return;
		}

		const FString Markup = Block->BuildMarkup();
		if (!Markup.IsEmpty())
		{
			TextHandle->SetValue(FText::FromString(Markup));
		}
	}

	FReply OnAddAtEnd()
	{
		if (ArrayHandle.IsValid())
		{
			uint32 NumElements = 0;
			ArrayHandle->GetNumElements(NumElements);
			if (NumElements > 0)
			{
				// Duplicate the last one so that the color/format is inherited.
				return OnAddAfter(static_cast<int32>(NumElements) - 1);
			}
			ArrayHandle->AddItem();
		}
		return FReply::Handled();
	}

	FReply OnAddAfter(int32 Index)
	{
		if (ArrayHandle.IsValid())
		{
			ArrayHandle->DuplicateItem(Index);

			// The duplicate keeps color/font/size, only the text is cleared.
			TSharedRef<IPropertyHandle> NewElement = ArrayHandle->GetElement(Index + 1);
			if (TSharedPtr<IPropertyHandle> NewTextHandle = NewElement->GetChildHandle(GET_MEMBER_NAME_CHECKED(FSimpleRichTextSegment, Text)))
			{
				NewTextHandle->SetValue(FText::GetEmpty());
			}
		}
		return FReply::Handled();
	}

	FReply OnRemove(int32 Index)
	{
		if (ArrayHandle.IsValid())
		{
			ArrayHandle->DeleteItem(Index);
		}
		return FReply::Handled();
	}

	TSharedRef<SWidget> BuildFormatMenu(TSharedRef<IPropertyHandle> Element)
	{
		TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);

		auto AddRow = [&Box, &Element](FName MemberName)
		{
			TSharedPtr<IPropertyHandle> Child = Element->GetChildHandle(MemberName);
			if (!Child.IsValid())
			{
				return;
			}
			Box->AddSlot()
			.AutoHeight()
			.Padding(0, 2)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)
				[
					SNew(SBox).WidthOverride(70.f)
					[
						Child->CreatePropertyNameWidget()
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
				[
					Child->CreatePropertyValueWidget()
				]
			];
		};

		AddRow(GET_MEMBER_NAME_CHECKED(FSimpleRichTextSegment, Font));
		AddRow(GET_MEMBER_NAME_CHECKED(FSimpleRichTextSegment, Typeface));
		AddRow(GET_MEMBER_NAME_CHECKED(FSimpleRichTextSegment, Size));

		return SNew(SBox)
			.MinDesiredWidth(300.f)
			.Padding(8.f)
			[
				Box
			];
	}

private:
	TSharedRef<IPropertyHandle> SegmentsHandle;
	TSharedPtr<IPropertyHandle> TextHandle;
	TSharedPtr<IPropertyHandleArray> ArrayHandle;
	FSimpleDelegate OnRebuildChildren;
};

TSharedRef<IDetailCustomization> FSimpleRichTextBlockCustomization::MakeInstance()
{
	return MakeShared<FSimpleRichTextBlockCustomization>();
}

void FSimpleRichTextBlockCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TSharedRef<IPropertyHandle> SegmentsHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(USimpleRichTextBlock, Segments));

	// The standard Text field stays visible and fully usable; the segment editor writes into it.
	TSharedPtr<IPropertyHandle> TextHandle = DetailBuilder.GetProperty(FName("Text"), URichTextBlock::StaticClass());

	// Changing the mode / combined text / separator changes the number of segments behind the property
	// system's back, so the whole panel is rebuilt (deferred, after the value is committed).
	{
		TWeakPtr<IPropertyUtilities> WeakUtils = DetailBuilder.GetPropertyUtilities();
		const FSimpleDelegate RefreshDelegate = FSimpleDelegate::CreateLambda([WeakUtils]()
		{
			if (TSharedPtr<IPropertyUtilities> Utils = WeakUtils.Pin())
			{
				Utils->RequestRefresh();
			}
		});

		DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(USimpleRichTextBlock, InputMode))->SetOnPropertyValueChanged(RefreshDelegate);
		DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(USimpleRichTextBlock, CombinedText))->SetOnPropertyValueChanged(RefreshDelegate);
		DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(USimpleRichTextBlock, SegmentSeparator))->SetOnPropertyValueChanged(RefreshDelegate);
	}

	// Only the raw array is replaced by the custom rows.
	DetailBuilder.HideProperty(SegmentsHandle);

	IDetailCategoryBuilder& ContentCategory = DetailBuilder.EditCategory("Content");

	TSharedRef<FSimpleRichTextSegmentsBuilder> Builder = MakeShared<FSimpleRichTextSegmentsBuilder>(SegmentsHandle, TextHandle);
	Builder->Init();
	ContentCategory.AddCustomBuilder(Builder);
}

#undef LOCTEXT_NAMESPACE


// TODO : v2 

// #include "SimpleRichTextBlockCustomization.h"
// #include "SimpleRichTextBlock.h"
//
// #include "Components/RichTextBlock.h"
// #include "DetailCategoryBuilder.h"
// #include "DetailLayoutBuilder.h"
// #include "DetailWidgetRow.h"
// #include "IDetailChildrenBuilder.h"
// #include "IDetailCustomNodeBuilder.h"
// #include "PropertyHandle.h"
// #include "Styling/AppStyle.h"
// #include "Widgets/Images/SImage.h"
// #include "Widgets/Input/SButton.h"
// #include "Widgets/Input/SComboButton.h"
// #include "Widgets/Layout/SBox.h"
// #include "Widgets/SBoxPanel.h"
// #include "Widgets/Text/STextBlock.h"
//
// #define LOCTEXT_NAMESPACE "SimpleRichTextCustomization"
//
// /** Builds one editable row per segment. */
// class FSimpleRichTextSegmentsBuilder : public IDetailCustomNodeBuilder, public TSharedFromThis<FSimpleRichTextSegmentsBuilder>
// {
// public:
// 	FSimpleRichTextSegmentsBuilder(TSharedRef<IPropertyHandle> InSegmentsHandle, TSharedPtr<IPropertyHandle> InTextHandle)
// 		: SegmentsHandle(InSegmentsHandle)
// 		, TextHandle(InTextHandle)
// 		, ArrayHandle(InSegmentsHandle->AsArray())
// 	{
// 	}
//
// 	/** Must be called right after MakeShared (needs a valid shared pointer to bind delegates). */
// 	void Init()
// 	{
// 		if (ArrayHandle.IsValid())
// 		{
// 			ArrayHandle->SetOnNumElementsChanged(FSimpleDelegate::CreateSP(this, &FSimpleRichTextSegmentsBuilder::HandleNumElementsChanged));
// 		}
// 		SegmentsHandle->SetOnChildPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FSimpleRichTextSegmentsBuilder::HandleSegmentsEdited));
// 	}
//
// 	//~ IDetailCustomNodeBuilder
// 	virtual void SetOnRebuildChildren(FSimpleDelegate InOnRegenerateChildren) override { OnRebuildChildren = InOnRegenerateChildren; }
// 	virtual bool RequiresTick() const override { return false; }
// 	virtual bool InitiallyCollapsed() const override { return false; }
// 	virtual FName GetName() const override { return TEXT("SimpleRichTextSegments"); }
//
// 	virtual void GenerateHeaderRowContent(FDetailWidgetRow& NodeRow) override
// 	{
// 		NodeRow
// 		.NameContent()
// 		[
// 			SNew(STextBlock)
// 			.Text(LOCTEXT("SegmentsHeader", "Text Segments"))
// 			.Font(IDetailLayoutBuilder::GetDetailFontBold())
// 		]
// 		.ValueContent()
// 		.HAlign(HAlign_Left)
// 		[
// 			SNew(SButton)
// 			.ButtonStyle(FAppStyle::Get(), "SimpleButton")
// 			.ToolTipText(LOCTEXT("AddFirstTip", "Add a text segment at the end"))
// 			.OnClicked(this, &FSimpleRichTextSegmentsBuilder::OnAddAtEnd)
// 			.ContentPadding(FMargin(4.f, 2.f))
// 			[
// 				SNew(SHorizontalBox)
// 				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 4, 0)
// 				[
// 					SNew(SImage).Image(FAppStyle::GetBrush("Icons.Plus"))
// 				]
// 				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
// 				[
// 					SNew(STextBlock).Text(LOCTEXT("AddSegment", "Add segment"))
// 				]
// 			]
// 		];
// 	}
//
// 	virtual void GenerateChildContent(IDetailChildrenBuilder& ChildrenBuilder) override
// 	{
// 		if (!ArrayHandle.IsValid())
// 		{
// 			return;
// 		}
//
// 		uint32 NumElements = 0;
// 		ArrayHandle->GetNumElements(NumElements);
//
// 		for (uint32 i = 0; i < NumElements; ++i)
// 		{
// 			const int32 Index = static_cast<int32>(i);
// 			TSharedRef<IPropertyHandle> Element = ArrayHandle->GetElement(Index);
//
// 			TSharedPtr<IPropertyHandle> SegTextHandle = Element->GetChildHandle(GET_MEMBER_NAME_CHECKED(FSimpleRichTextSegment, Text));
// 			TSharedPtr<IPropertyHandle> ColorHandle = Element->GetChildHandle(GET_MEMBER_NAME_CHECKED(FSimpleRichTextSegment, Color));
// 			if (!SegTextHandle.IsValid() || !ColorHandle.IsValid())
// 			{
// 				continue;
// 			}
//
// 			ChildrenBuilder.AddCustomRow(FText::GetEmpty())
// 			.WholeRowContent()
// 			[
// 				SNew(SHorizontalBox)
//
// 				// Text
// 				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(0, 1, 4, 1)
// 				[
// 					SegTextHandle->CreatePropertyValueWidget()
// 				]
//
// 				// Color
// 				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 4, 0)
// 				[
// 					SNew(SBox)
// 					.WidthOverride(56.f)
// 					[
// 						ColorHandle->CreatePropertyValueWidget()
// 					]
// 				]
//
// 				// Font / typeface / size popup
// 				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 2, 0)
// 				[
// 					SNew(SComboButton)
// 					.HasDownArrow(false)
// 					.ToolTipText(LOCTEXT("FormatTip", "Font, typeface and size of this segment"))
// 					.ButtonContent()
// 					[
// 						SNew(SImage).Image(FAppStyle::GetBrush("Icons.Settings"))
// 					]
// 					.OnGetMenuContent(FOnGetContent::CreateSP(this, &FSimpleRichTextSegmentsBuilder::BuildFormatMenu, Element))
// 				]
//
// 				// Add after this one (copies the formatting, clears the text)
// 				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
// 				[
// 					SNew(SButton)
// 					.ButtonStyle(FAppStyle::Get(), "SimpleButton")
// 					.ToolTipText(LOCTEXT("AddAfterTip", "Add a segment after this one (inherits color and formatting)"))
// 					.OnClicked(this, &FSimpleRichTextSegmentsBuilder::OnAddAfter, Index)
// 					[
// 						SNew(SImage).Image(FAppStyle::GetBrush("Icons.Plus"))
// 					]
// 				]
//
// 				// Remove
// 				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
// 				[
// 					SNew(SButton)
// 					.ButtonStyle(FAppStyle::Get(), "SimpleButton")
// 					.ToolTipText(LOCTEXT("RemoveTip", "Remove this segment"))
// 					.OnClicked(this, &FSimpleRichTextSegmentsBuilder::OnRemove, Index)
// 					[
// 						SNew(SImage).Image(FAppStyle::GetBrush("Icons.Minus"))
// 					]
// 				]
// 			];
// 		}
// 	}
//
// private:
// 	void HandleNumElementsChanged()
// 	{
// 		OnRebuildChildren.ExecuteIfBound();
// 		HandleSegmentsEdited();
// 	}
//
// 	/** Pushes the generated markup into the standard Text field through its property handle,
// 	 *  so the Text row in Details updates and the change goes through the normal property system. */
// 	void HandleSegmentsEdited()
// 	{
// 		if (!TextHandle.IsValid())
// 		{
// 			return;
// 		}
//
// 		TArray<UObject*> Objects;
// 		SegmentsHandle->GetOuterObjects(Objects);
// 		if (Objects.Num() != 1)
// 		{
// 			return;
// 		}
//
// 		const USimpleRichTextBlock* Block = Cast<USimpleRichTextBlock>(Objects[0]);
// 		if (!Block)
// 		{
// 			return;
// 		}
//
// 		const FString Markup = Block->BuildMarkup();
// 		if (!Markup.IsEmpty())
// 		{
// 			TextHandle->SetValue(FText::FromString(Markup));
// 		}
// 	}
//
// 	FReply OnAddAtEnd()
// 	{
// 		if (ArrayHandle.IsValid())
// 		{
// 			uint32 NumElements = 0;
// 			ArrayHandle->GetNumElements(NumElements);
// 			if (NumElements > 0)
// 			{
// 				// Duplicate the last one so that the color/format is inherited.
// 				return OnAddAfter(static_cast<int32>(NumElements) - 1);
// 			}
// 			ArrayHandle->AddItem();
// 		}
// 		return FReply::Handled();
// 	}
//
// 	FReply OnAddAfter(int32 Index)
// 	{
// 		if (ArrayHandle.IsValid())
// 		{
// 			ArrayHandle->DuplicateItem(Index);
//
// 			// The duplicate keeps color/font/size, only the text is cleared.
// 			TSharedRef<IPropertyHandle> NewElement = ArrayHandle->GetElement(Index + 1);
// 			if (TSharedPtr<IPropertyHandle> NewTextHandle = NewElement->GetChildHandle(GET_MEMBER_NAME_CHECKED(FSimpleRichTextSegment, Text)))
// 			{
// 				NewTextHandle->SetValue(FText::GetEmpty());
// 			}
// 		}
// 		return FReply::Handled();
// 	}
//
// 	FReply OnRemove(int32 Index)
// 	{
// 		if (ArrayHandle.IsValid())
// 		{
// 			ArrayHandle->DeleteItem(Index);
// 		}
// 		return FReply::Handled();
// 	}
//
// 	TSharedRef<SWidget> BuildFormatMenu(TSharedRef<IPropertyHandle> Element)
// 	{
// 		TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);
//
// 		auto AddRow = [&Box, &Element](FName MemberName)
// 		{
// 			TSharedPtr<IPropertyHandle> Child = Element->GetChildHandle(MemberName);
// 			if (!Child.IsValid())
// 			{
// 				return;
// 			}
// 			Box->AddSlot()
// 			.AutoHeight()
// 			.Padding(0, 2)
// 			[
// 				SNew(SHorizontalBox)
// 				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)
// 				[
// 					SNew(SBox).WidthOverride(70.f)
// 					[
// 						Child->CreatePropertyNameWidget()
// 					]
// 				]
// 				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
// 				[
// 					Child->CreatePropertyValueWidget()
// 				]
// 			];
// 		};
//
// 		AddRow(GET_MEMBER_NAME_CHECKED(FSimpleRichTextSegment, Font));
// 		AddRow(GET_MEMBER_NAME_CHECKED(FSimpleRichTextSegment, Typeface));
// 		AddRow(GET_MEMBER_NAME_CHECKED(FSimpleRichTextSegment, Size));
//
// 		return SNew(SBox)
// 			.MinDesiredWidth(300.f)
// 			.Padding(8.f)
// 			[
// 				Box
// 			];
// 	}
//
// private:
// 	TSharedRef<IPropertyHandle> SegmentsHandle;
// 	TSharedPtr<IPropertyHandle> TextHandle;
// 	TSharedPtr<IPropertyHandleArray> ArrayHandle;
// 	FSimpleDelegate OnRebuildChildren;
// };
//
// TSharedRef<IDetailCustomization> FSimpleRichTextBlockCustomization::MakeInstance()
// {
// 	return MakeShared<FSimpleRichTextBlockCustomization>();
// }
//
// void FSimpleRichTextBlockCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
// {
// 	TSharedRef<IPropertyHandle> SegmentsHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(USimpleRichTextBlock, Segments));
//
// 	// The standard Text field stays visible and fully usable; the segment editor writes into it.
// 	TSharedPtr<IPropertyHandle> NewTextHandle = DetailBuilder.GetProperty(FName("Text"), URichTextBlock::StaticClass());
//
// 	// Only the raw array is replaced by the custom rows.
// 	DetailBuilder.HideProperty(SegmentsHandle);
//
// 	IDetailCategoryBuilder& ContentCategory = DetailBuilder.EditCategory("Content");
//
// 	TSharedRef<FSimpleRichTextSegmentsBuilder> Builder = MakeShared<FSimpleRichTextSegmentsBuilder>(SegmentsHandle, NewTextHandle);
// 	Builder->Init();
// 	ContentCategory.AddCustomBuilder(Builder);
// }
//
// #undef LOCTEXT_NAMESPACE


// TODO : v1 

// #include "SimpleRichTextBlockCustomization.h"
// #include "SimpleRichTextBlock.h"
//
// #include "Components/RichTextBlock.h"
// #include "DetailCategoryBuilder.h"
// #include "DetailLayoutBuilder.h"
// #include "DetailWidgetRow.h"
// #include "IDetailChildrenBuilder.h"
// #include "IDetailCustomNodeBuilder.h"
// #include "PropertyHandle.h"
// #include "Styling/AppStyle.h"
// #include "Widgets/Images/SImage.h"
// #include "Widgets/Input/SButton.h"
// #include "Widgets/Input/SComboButton.h"
// #include "Widgets/Layout/SBox.h"
// #include "Widgets/SBoxPanel.h"
// #include "Widgets/Text/STextBlock.h"
//
// #define LOCTEXT_NAMESPACE "SimpleRichTextCustomization"
//
// /** Builds one editable row per segment. */
// class FSimpleRichTextSegmentsBuilder : public IDetailCustomNodeBuilder, public TSharedFromThis<FSimpleRichTextSegmentsBuilder>
// {
// public:
// 	explicit FSimpleRichTextSegmentsBuilder(TSharedRef<IPropertyHandle> InSegmentsHandle)
// 		: SegmentsHandle(InSegmentsHandle)
// 		, ArrayHandle(InSegmentsHandle->AsArray())
// 	{
// 	}
//
// 	/** Must be called right after MakeShared (needs a valid shared pointer to bind delegates). */
// 	void Init()
// 	{
// 		if (ArrayHandle.IsValid())
// 		{
// 			ArrayHandle->SetOnNumElementsChanged(FSimpleDelegate::CreateSP(this, &FSimpleRichTextSegmentsBuilder::HandleNumElementsChanged));
// 		}
// 	}
//
// 	//~ IDetailCustomNodeBuilder
// 	virtual void SetOnRebuildChildren(FSimpleDelegate InOnRegenerateChildren) override { OnRebuildChildren = InOnRegenerateChildren; }
// 	virtual bool RequiresTick() const override { return false; }
// 	virtual bool InitiallyCollapsed() const override { return false; }
// 	virtual FName GetName() const override { return TEXT("SimpleRichTextSegments"); }
//
// 	virtual void GenerateHeaderRowContent(FDetailWidgetRow& NodeRow) override
// 	{
// 		NodeRow
// 		.NameContent()
// 		[
// 			SNew(STextBlock)
// 			.Text(LOCTEXT("SegmentsHeader", "Text Segments"))
// 			.Font(IDetailLayoutBuilder::GetDetailFontBold())
// 		]
// 		.ValueContent()
// 		.HAlign(HAlign_Left)
// 		[
// 			SNew(SButton)
// 			.ButtonStyle(FAppStyle::Get(), "SimpleButton")
// 			.ToolTipText(LOCTEXT("AddFirstTip", "Add a text segment at the end"))
// 			.OnClicked(this, &FSimpleRichTextSegmentsBuilder::OnAddAtEnd)
// 			.ContentPadding(FMargin(4.f, 2.f))
// 			[
// 				SNew(SHorizontalBox)
// 				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 4, 0)
// 				[
// 					SNew(SImage).Image(FAppStyle::GetBrush("Icons.Plus"))
// 				]
// 				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
// 				[
// 					SNew(STextBlock).Text(LOCTEXT("AddSegment", "Add segment"))
// 				]
// 			]
// 		];
// 	}
//
// 	virtual void GenerateChildContent(IDetailChildrenBuilder& ChildrenBuilder) override
// 	{
// 		if (!ArrayHandle.IsValid())
// 		{
// 			return;
// 		}
//
// 		uint32 NumElements = 0;
// 		ArrayHandle->GetNumElements(NumElements);
//
// 		for (uint32 i = 0; i < NumElements; ++i)
// 		{
// 			const int32 Index = static_cast<int32>(i);
// 			TSharedRef<IPropertyHandle> Element = ArrayHandle->GetElement(Index);
//
// 			TSharedPtr<IPropertyHandle> TextHandle = Element->GetChildHandle(GET_MEMBER_NAME_CHECKED(FSimpleRichTextSegment, Text));
// 			TSharedPtr<IPropertyHandle> ColorHandle = Element->GetChildHandle(GET_MEMBER_NAME_CHECKED(FSimpleRichTextSegment, Color));
// 			if (!TextHandle.IsValid() || !ColorHandle.IsValid())
// 			{
// 				continue;
// 			}
//
// 			ChildrenBuilder.AddCustomRow(FText::GetEmpty())
// 			.WholeRowContent()
// 			[
// 				SNew(SHorizontalBox)
//
// 				// Text
// 				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(0, 1, 4, 1)
// 				[
// 					TextHandle->CreatePropertyValueWidget()
// 				]
//
// 				// Color
// 				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 4, 0)
// 				[
// 					SNew(SBox)
// 					.WidthOverride(56.f)
// 					[
// 						ColorHandle->CreatePropertyValueWidget()
// 					]
// 				]
//
// 				// Font / typeface / size popup
// 				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 2, 0)
// 				[
// 					SNew(SComboButton)
// 					.HasDownArrow(false)
// 					.ToolTipText(LOCTEXT("FormatTip", "Font, typeface and size of this segment"))
// 					.ButtonContent()
// 					[
// 						SNew(SImage).Image(FAppStyle::GetBrush("Icons.Settings"))
// 					]
// 					.OnGetMenuContent(FOnGetContent::CreateSP(this, &FSimpleRichTextSegmentsBuilder::BuildFormatMenu, Element))
// 				]
//
// 				// Add after this one (copies the formatting, clears the text)
// 				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
// 				[
// 					SNew(SButton)
// 					.ButtonStyle(FAppStyle::Get(), "SimpleButton")
// 					.ToolTipText(LOCTEXT("AddAfterTip", "Add a segment after this one (inherits color and formatting)"))
// 					.OnClicked(this, &FSimpleRichTextSegmentsBuilder::OnAddAfter, Index)
// 					[
// 						SNew(SImage).Image(FAppStyle::GetBrush("Icons.Plus"))
// 					]
// 				]
//
// 				// Remove
// 				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
// 				[
// 					SNew(SButton)
// 					.ButtonStyle(FAppStyle::Get(), "SimpleButton")
// 					.ToolTipText(LOCTEXT("RemoveTip", "Remove this segment"))
// 					.OnClicked(this, &FSimpleRichTextSegmentsBuilder::OnRemove, Index)
// 					[
// 						SNew(SImage).Image(FAppStyle::GetBrush("Icons.Minus"))
// 					]
// 				]
// 			];
// 		}
// 	}
//
// private:
// 	void HandleNumElementsChanged()
// 	{
// 		OnRebuildChildren.ExecuteIfBound();
// 	}
//
// 	FReply OnAddAtEnd()
// 	{
// 		if (ArrayHandle.IsValid())
// 		{
// 			uint32 NumElements = 0;
// 			ArrayHandle->GetNumElements(NumElements);
// 			if (NumElements > 0)
// 			{
// 				// Duplicate the last one so that the color/format is inherited.
// 				return OnAddAfter(static_cast<int32>(NumElements) - 1);
// 			}
// 			ArrayHandle->AddItem();
// 		}
// 		return FReply::Handled();
// 	}
//
// 	FReply OnAddAfter(int32 Index)
// 	{
// 		if (ArrayHandle.IsValid())
// 		{
// 			ArrayHandle->DuplicateItem(Index);
//
// 			// The duplicate keeps color/font/size, only the text is cleared.
// 			TSharedRef<IPropertyHandle> NewElement = ArrayHandle->GetElement(Index + 1);
// 			if (TSharedPtr<IPropertyHandle> TextHandle = NewElement->GetChildHandle(GET_MEMBER_NAME_CHECKED(FSimpleRichTextSegment, Text)))
// 			{
// 				TextHandle->SetValue(FText::GetEmpty());
// 			}
// 		}
// 		return FReply::Handled();
// 	}
//
// 	FReply OnRemove(int32 Index)
// 	{
// 		if (ArrayHandle.IsValid())
// 		{
// 			ArrayHandle->DeleteItem(Index);
// 		}
// 		return FReply::Handled();
// 	}
//
// 	TSharedRef<SWidget> BuildFormatMenu(TSharedRef<IPropertyHandle> Element)
// 	{
// 		TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);
//
// 		auto AddRow = [&Box, &Element](FName MemberName)
// 		{
// 			TSharedPtr<IPropertyHandle> Child = Element->GetChildHandle(MemberName);
// 			if (!Child.IsValid())
// 			{
// 				return;
// 			}
// 			Box->AddSlot()
// 			.AutoHeight()
// 			.Padding(0, 2)
// 			[
// 				SNew(SHorizontalBox)
// 				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)
// 				[
// 					SNew(SBox).WidthOverride(70.f)
// 					[
// 						Child->CreatePropertyNameWidget()
// 					]
// 				]
// 				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
// 				[
// 					Child->CreatePropertyValueWidget()
// 				]
// 			];
// 		};
//
// 		AddRow(GET_MEMBER_NAME_CHECKED(FSimpleRichTextSegment, Font));
// 		AddRow(GET_MEMBER_NAME_CHECKED(FSimpleRichTextSegment, Typeface));
// 		AddRow(GET_MEMBER_NAME_CHECKED(FSimpleRichTextSegment, Size));
//
// 		return SNew(SBox)
// 			.MinDesiredWidth(300.f)
// 			.Padding(8.f)
// 			[
// 				Box
// 			];
// 	}
//
// private:
// 	TSharedRef<IPropertyHandle> SegmentsHandle;
// 	TSharedPtr<IPropertyHandleArray> ArrayHandle;
// 	FSimpleDelegate OnRebuildChildren;
// };
//
// TSharedRef<IDetailCustomization> FSimpleRichTextBlockCustomization::MakeInstance()
// {
// 	return MakeShared<FSimpleRichTextBlockCustomization>();
// }
//
// void FSimpleRichTextBlockCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
// {
// 	TSharedRef<IPropertyHandle> SegmentsHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(USimpleRichTextBlock, Segments));
//
// 	// The text is generated from the segments, so the plain Text field is hidden.
// 	TSharedPtr<IPropertyHandle> TextHandle = DetailBuilder.GetProperty(FName("Text"), URichTextBlock::StaticClass());
// 	if (TextHandle.IsValid())
// 	{
// 		DetailBuilder.HideProperty(TextHandle);
// 	}
// 	DetailBuilder.HideProperty(SegmentsHandle);
//
// 	IDetailCategoryBuilder& ContentCategory = DetailBuilder.EditCategory("Content");
//
// 	TSharedRef<FSimpleRichTextSegmentsBuilder> Builder = MakeShared<FSimpleRichTextSegmentsBuilder>(SegmentsHandle);
// 	Builder->Init();
// 	ContentCategory.AddCustomBuilder(Builder);
// }
//
// #undef LOCTEXT_NAMESPACE
