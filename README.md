SimpleRichText

English | Русский

An Unreal Engine 5 plugin that adds SimpleRichTextBlock, a drop-in extension of the standard RichTextBlock.

It keeps everything RichTextBlock does (style sets, decorators, wrapping, SetText, ...) and adds a convenient segment editor to the Details panel: instead of typing tags by hand, you type text, pick a color (and optionally font, typeface and size) per segment, and the plugin writes the rich text markup into the regular Text field for you.

Developed and built against Unreal Engine 5.7. Other 5.x versions are untested; some engine API names used by the plugin (for example URichTextBlock::GetDefaultTextStyle) may differ between versions.

Features
Subclass of URichTextBlock, so all standard functionality keeps working.
The standard Text field stays visible and editable. Segments only write into it.
Custom Details rows: [text][color][⚙ font / typeface / size][+][-] per segment.
The + button inserts a new segment after the current one that inherits its color and formatting.
Two input modes:
Per Segment Fields: every segment has its own text field.
Single Field + Separator: the whole text lives in one field, pieces are split by a separator and formats are applied by order.
Per-segment color, font (UFont / UFontFace), typeface name and font size.
A set of Blueprint / C++ functions for building and changing segments at runtime.
Optional mode that always regenerates Text from the segments.
Installation
Copy the plugin folder into your project:
YourProject/Plugins/SimpleRichText/
Regenerate project files and build the project.
Enable the plugin in Edit → Plugins → UI → Simple Rich Text if it is not enabled yet and restart the editor.
Plugin layout
SimpleRichText/
  SimpleRichText.uplugin
  Source/
    SimpleRichText/                 (Runtime module)
      SimpleRichText.Build.cs
      Public/
        SimpleRichTextModule.h
        SimpleRichTextBlock.h
        SimpleRichTextDecorator.h
      Private/
        SimpleRichTextModule.cpp
        SimpleRichTextBlock.cpp
        SimpleRichTextDecorator.cpp
    SimpleRichTextEditor/           (Editor module)
      SimpleRichTextEditor.Build.cs
      Public/
        SimpleRichTextEditorModule.h
        SimpleRichTextBlockCustomization.h
      Private/
        SimpleRichTextEditorModule.cpp
        SimpleRichTextBlockCustomization.cpp
SimpleRichText (Runtime): USimpleRichTextBlock, the segment struct and the text decorator that understands the generated tag.
SimpleRichTextEditor (Editor): the IDetailCustomization that builds the segment rows.
Quick start
Open a Widget Blueprint.
In the palette, category Simple, drag Simple Rich Text Block into the hierarchy.
In Details → Content click Add segment.
Type text, pick a color. Click + on a row to add the next piece of text with the same color, then change the color there.
The generated markup appears in the standard Text field, and the preview updates.
Details panel

All controls are in the Content category.

Property	Description
Text	The standard RichTextBlock text field. Fully usable by hand.
Text Segments	The segment editor (rows described below).
Always Rebuild From Segments	Off (default): Text is only written when you edit segments. On: Text is regenerated from the segments on load and widget rebuild, so manual changes to Text are overwritten.
Input Mode	Per Segment Fields or Single Field + Separator.
Combined Text	(Single Field mode only) The whole text.
Segment Separator	(Single Field mode only) String that separates pieces in Combined Text. Default `
Segment row
[ text field ] [ color ] [ ⚙ ] [ + ] [ - ]
text field: the text of this segment (multi-line).
color: color of this segment. New segments created with + copy the color of the segment they were created from.
⚙: popup with
Font: a UFont or UFontFace asset. Empty = default font of the block.
Typeface: typeface name inside the font (for example Bold, Italic). Currently typed in by hand. Empty = default.
Size: font size. 0 = default size.
+: add a new segment after this one (inherits color and formatting, text is empty).
-: remove this segment.
Input modes
Per Segment Fields (default)

Each segment has its own text field. Editing any segment rewrites Text with the generated markup.

To avoid wiping a hand-written Text, nothing is written while the generated markup is empty (for example right after adding the first, still empty, segment).

Single Field + Separator

Type the whole text into Combined Text and put the separator between the parts that should be formatted differently:

Hello |beautiful |world|!

With the separator | this is split into four pieces. Piece N receives the format (color, font, typeface, size) of segment N. The segment rows then show only the color and the ⚙ popup next to a read-only preview of the piece (1: Hello, 2: beautiful, ...).

Rules:

The number of segments always equals the number of pieces.
If there are more pieces than segments, new segments copy the format of the last one.
If there are fewer pieces than segments, the trailing segments (and their formats) are removed.
Inserting a separator in the middle shifts formats by one piece, because formats are matched strictly by order.
When you switch an existing block to this mode, the current segments are joined with the separator into Combined Text.
After Combined Text, the separator or the mode changes, the Details panel rebuilds itself.

If your texts can contain the separator, choose a rarer one (for example || or ¦). A segment text that contains the separator will be split into two at the next split.

Generated markup

Segments are converted into one tag per segment. The closing tag is the standard </>.

<seg c="FF0000FF" f="/Game/Fonts/Roboto.Roboto" tf="Bold" s="32">Hello</>
Attribute	Meaning
c	Color as RRGGBBAA hex (sRGB). Always present.
f	Soft object path of the font asset. Only if a font is set.
tf	Typeface name. Only if set.
s	Font size. Only if greater than 0.

Notes:

Segments with empty text are skipped.
<, > and \ inside segment text are escaped with a backslash.
The tag is handled by FSimpleRichTextDecorator, which is added automatically to the block. Starting from the default text style of the block, it overrides color, font, typeface and size.
Because it is plain rich text markup, you can mix <seg> tags with your own style-set tags in the Text field.
Segment struct: FSimpleRichTextSegment
Field	Type	Description
Text	FText	Text of the segment.
Color	FLinearColor	Color. Default white.
Font	TSoftObjectPtr<UObject>	UFont / UFontFace asset. Empty = default.
Typeface	FName	Typeface inside the font. None = default.
Size	int32	Font size. 0 = default.
Blueprint / C++ API

All functions are BlueprintCallable (or BlueprintPure for getters) in the category SimpleRichText. Every function that changes segments also updates Text and the widget.

Whole content
Function	Description
SetSegments(Segments)	Replaces the whole segment array. Takes everything from the structs literally (no inheritance).
SetCombinedText(Text)	Sets CombinedText, splits it by SegmentSeparator into segments. Existing formats stay.
SetCombinedTextWithFormats(Text, Formats)	Sets the whole text and the formats at once. Piece N gets Formats[N] (the Text inside the structs is ignored). Extra pieces copy the last format.
ClearSegments()	Removes all segments.
Formats only
Function	Description
SetFormats(Formats)	Applies formats to the existing segments by order, texts stay. Formats[N] goes to segment N, segments beyond the array get the last format. Empty array does nothing.
SetSegmentFormat(Index, Format)	Replaces the full format of one segment, its text stays.
SetSegmentColor(Index, Color)	Sets the color of one segment.
SetAllSegmentsColor(Color)	Sets the color of every segment.
SetSegmentSize(Index, Size)	Sets the font size of one segment (0 = default).
Structure
Function	Description
AddSegment(NewSegment)	Adds a segment at the end, returns its index.
InsertSegment(Index, NewSegment)	Inserts a segment at Index.
RemoveSegment(Index)	Removes a segment.
SetSegmentText(Index, Text)	Changes the text of one segment.

Inheritance rules for AddSegment and InsertSegment:

Text and Color are taken from NewSegment as they are.
If Font is empty, Font is inherited from the previous segment (the last one for AddSegment, the one before Index for InsertSegment; when inserting at 0, the segment that is pushed to position 1). Typeface is inherited together with the font, but only if it is not set.
If Size is 0, the size is inherited from the same segment.
Reading
Function	Description
GetSegmentCount()	Number of segments.
GetSegmentText(Index)	Text of one segment.
GetPlainText()	All segment texts joined, without any tags.
BuildMarkup()	The markup generated from the segments.
SetSegments vs SetFormats

Both take an array of FSimpleRichTextSegment, but they do different things:

SetSegments replaces the content completely: texts, formats and the number of segments come from the array.
SetFormats only changes formats of the segments that already exist: the number of segments and their texts stay, and the Text field inside the structs is ignored.
Single Field mode and the API

In Single Field mode, functions that change segment texts or the segment count (SetSegments, SetSegmentText, AddSegment, InsertSegment, RemoveSegment, ClearSegments) keep CombinedText in sync automatically by joining the segment texts with SegmentSeparator.

C++ example
cpp
#include "SimpleRichTextBlock.h"

void UMyWidget::ShowDamage(USimpleRichTextBlock* Block, int32 Damage)
{
	FSimpleRichTextSegment Label;
	Label.Text = FText::FromString(TEXT("Damage: "));
	Label.Color = FLinearColor::White;
	Label.Size = 24;

	FSimpleRichTextSegment Value;
	Value.Text = FText::AsNumber(Damage);
	Value.Color = FLinearColor::Red;   // font and size are inherited from the previous segment

	Block->ClearSegments();
	Block->AddSegment(Label);
	Block->AddSegment(Value);
}

Single field variant:

cpp
FSimpleRichTextSegment White; White.Color = FLinearColor::White;
FSimpleRichTextSegment Red;   Red.Color   = FLinearColor::Red;

Block->SetCombinedTextWithFormats(FText::FromString(TEXT("Damage: |42")), { White, Red });
Notes and limitations
Text and segments are not linked in both directions. Segments write into Text, but the markup in Text is not parsed back into segments. If you edit Text by hand, the segments become stale until you edit a segment again, which rewrites Text.
Calling SetText yourself works as usual. With Always Rebuild From Segments enabled, Text will be overwritten from the segments on the next rebuild.
Typeface has to be typed in manually.
Segment color defaults to white and is always written to the tag.
The Details customization updates Text from the first selected object only when several widgets are selected at once.
Requirements
Unreal Engine 5.x (built with 5.7)
Modules used: Core, CoreUObject, Engine, Slate, SlateCore, UMG (runtime); additionally InputCore, PropertyEditor (editor)
License

MIT

Русская версия

English | Русский

SimpleRichText

Плагин для Unreal Engine 5, который добавляет SimpleRichTextBlock — расширение стандартного RichTextBlock.

Он сохраняет всё, что умеет RichTextBlock (наборы стилей, декораторы, перенос строк, SetText и т. д.), и добавляет удобный редактор сегментов в панель Details: вместо ручного ввода тегов вы вводите текст, выбираете цвет (а при необходимости шрифт, typeface и размер) для каждого сегмента, а плагин сам записывает разметку rich text в обычное поле Text.

Разработан и собран под Unreal Engine 5.7.4 Другие версии 5.x не проверялись; некоторые имена функций движка, которые использует плагин (например, URichTextBlock::GetDefaultTextStyle), могут отличаться в разных версиях.

Возможности
Наследник URichTextBlock, поэтому вся стандартная функциональность продолжает работать.
Стандартное поле Text остаётся видимым и доступным для редактирования. Сегменты только записывают в него.
Кастомные строки в Details: [текст][цвет][⚙ шрифт / typeface / размер][+][-] для каждого сегмента.
Кнопка + добавляет новый сегмент после текущего, который наследует его цвет и форматирование.
Два режима ввода:
Per Segment Fields: у каждого сегмента своё поле текста.
Single Field + Separator: весь текст в одном поле, части разделяются разделителем, а форматы применяются по порядку.
Для каждого сегмента: цвет, шрифт (UFont / UFontFace), имя typeface и размер шрифта.
Набор Blueprint / C++ функций для создания и изменения сегментов во время выполнения.
Опциональный режим, в котором Text всегда пересобирается из сегментов.
Установка
Скопируйте папку плагина в проект:
YourProject/Plugins/SimpleRichText/
Перегенерируйте файлы проекта и соберите проект.
Если плагин ещё не включён, включите его в Edit → Plugins → UI → Simple Rich Text и перезапустите редактор.
Структура плагина
SimpleRichText/
  SimpleRichText.uplugin
  Source/
    SimpleRichText/                 (Runtime-модуль)
      SimpleRichText.Build.cs
      Public/
        SimpleRichTextModule.h
        SimpleRichTextBlock.h
        SimpleRichTextDecorator.h
      Private/
        SimpleRichTextModule.cpp
        SimpleRichTextBlock.cpp
        SimpleRichTextDecorator.cpp
    SimpleRichTextEditor/           (Editor-модуль)
      SimpleRichTextEditor.Build.cs
      Public/
        SimpleRichTextEditorModule.h
        SimpleRichTextBlockCustomization.h
      Private/
        SimpleRichTextEditorModule.cpp
        SimpleRichTextBlockCustomization.cpp
SimpleRichText (Runtime): USimpleRichTextBlock, структура сегмента и декоратор текста, понимающий генерируемый тег.
SimpleRichTextEditor (Editor): IDetailCustomization, который строит строки сегментов.
Быстрый старт
Откройте Widget Blueprint.
В палитре, категория Simple, перетащите Simple Rich Text Block в иерархию.
В Details → Content нажмите Add segment.
Введите текст, выберите цвет. Нажмите + в строке, чтобы добавить следующий кусок текста с тем же цветом, и поменяйте цвет там.
Сгенерированная разметка появится в стандартном поле Text, превью обновится.
Панель Details

Все элементы находятся в категории Content.

Свойство	Описание
Text	Стандартное поле текста RichTextBlock. Полностью доступно для ручного ввода.
Text Segments	Редактор сегментов (строки описаны ниже).
Always Rebuild From Segments	Выключено (по умолчанию): Text записывается только когда вы редактируете сегменты. Включено: Text пересобирается из сегментов при загрузке и пересоздании виджета, поэтому ручные изменения Text будут перезаписаны.
Input Mode	Per Segment Fields или Single Field + Separator.
Combined Text	(только режим Single Field) Весь текст целиком.
Segment Separator	(только режим Single Field) Строка, разделяющая части в Combined Text. По умолчанию `
Строка сегмента
[ поле текста ] [ цвет ] [ ⚙ ] [ + ] [ - ]
поле текста: текст этого сегмента (многострочный).
цвет: цвет этого сегмента. Новые сегменты, созданные кнопкой +, копируют цвет сегмента, от которого были созданы.
⚙: всплывающее окно с
Font: ассет UFont или UFontFace. Пусто = шрифт блока по умолчанию.
Typeface: имя typeface внутри шрифта (например, Bold, Italic). Сейчас вводится вручную. Пусто = по умолчанию.
Size: размер шрифта. 0 = размер по умолчанию.
+: добавить новый сегмент после этого (наследует цвет и форматирование, текст пустой).
-: удалить этот сегмент.
Режимы ввода
Per Segment Fields (по умолчанию)

У каждого сегмента своё поле текста. Редактирование любого сегмента перезаписывает Text сгенерированной разметкой.

Чтобы не стереть вручную написанный Text, запись не выполняется, пока сгенерированная разметка пуста (например, сразу после добавления первого, ещё пустого сегмента).

Single Field + Separator

Введите весь текст в Combined Text и поставьте разделитель между частями, которые должны быть оформлены по-разному:

Привет |красивый |мир|!

С разделителем | это разбивается на четыре части. Часть N получает формат (цвет, шрифт, typeface, размер) сегмента N. Строки сегментов при этом показывают только цвет и окно ⚙ рядом с превью части, доступным только для чтения (1: Привет, 2: красивый, ...).

Правила:

Количество сегментов всегда равно количеству частей.
Если частей больше, чем сегментов, новые сегменты копируют формат последнего.
Если частей меньше, чем сегментов, лишние сегменты в конце (вместе с их форматами) удаляются.
Вставка разделителя в середину сдвигает форматы на одну часть, потому что форматы сопоставляются строго по порядку.
При переключении существующего блока в этот режим текущие сегменты склеиваются через разделитель в Combined Text.
После изменения Combined Text, разделителя или режима панель Details перестраивается.

Если в ваших текстах может встречаться разделитель, выберите более редкий (например, || или ¦). Текст сегмента, содержащий разделитель, при следующем разбиении будет разрезан на два.

Генерируемая разметка

Сегменты превращаются в один тег на сегмент. Закрывающий тег стандартный: </>.

<seg c="FF0000FF" f="/Game/Fonts/Roboto.Roboto" tf="Bold" s="32">Привет</>
Атрибут	Значение
c	Цвет в hex-формате RRGGBBAA (sRGB). Присутствует всегда.
f	Soft object path ассета шрифта. Только если шрифт задан.
tf	Имя typeface. Только если задано.
s	Размер шрифта. Только если больше 0.

Примечания:

Сегменты с пустым текстом пропускаются.
Символы <, > и \ внутри текста сегмента экранируются обратной косой чертой.
Тег обрабатывается FSimpleRichTextDecorator, который автоматически добавляется в блок. Отталкиваясь от стиля текста блока по умолчанию, он переопределяет цвет, шрифт, typeface и размер.
Так как это обычная разметка rich text, в поле Text можно смешивать теги <seg> с вашими собственными тегами из набора стилей.
Структура сегмента: FSimpleRichTextSegment
Поле	Тип	Описание
Text	FText	Текст сегмента.
Color	FLinearColor	Цвет. По умолчанию белый.
Font	TSoftObjectPtr<UObject>	Ассет UFont / UFontFace. Пусто = по умолчанию.
Typeface	FName	Typeface внутри шрифта. None = по умолчанию.
Size	int32	Размер шрифта. 0 = по умолчанию.
Blueprint / C++ API

Все функции — BlueprintCallable (или BlueprintPure для геттеров) в категории SimpleRichText. Каждая функция, меняющая сегменты, также обновляет Text и виджет.

Всё содержимое
Функция	Описание
SetSegments(Segments)	Заменяет весь массив сегментов. Берёт всё из структур буквально (без наследования).
SetCombinedText(Text)	Устанавливает CombinedText и разбивает его по SegmentSeparator на сегменты. Существующие форматы сохраняются.
SetCombinedTextWithFormats(Text, Formats)	Задаёт весь текст и форматы за один вызов. Часть N получает Formats[N] (поле Text внутри структур игнорируется). Лишние части копируют последний формат.
ClearSegments()	Удаляет все сегменты.
Только форматы
Функция	Описание
SetFormats(Formats)	Применяет форматы к существующим сегментам по порядку, тексты остаются. Formats[N] получает сегмент N, сегменты за пределами массива получают последний формат. Пустой массив ничего не делает.
SetSegmentFormat(Index, Format)	Заменяет весь формат одного сегмента, его текст остаётся.
SetSegmentColor(Index, Color)	Задаёт цвет одного сегмента.
SetAllSegmentsColor(Color)	Задаёт цвет всех сегментов.
SetSegmentSize(Index, Size)	Задаёт размер шрифта одного сегмента (0 = по умолчанию).
Структура
Функция	Описание
AddSegment(NewSegment)	Добавляет сегмент в конец, возвращает его индекс.
InsertSegment(Index, NewSegment)	Вставляет сегмент на позицию Index.
RemoveSegment(Index)	Удаляет сегмент.
SetSegmentText(Index, Text)	Меняет текст одного сегмента.

Правила наследования для AddSegment и InsertSegment:

Text и Color берутся из NewSegment как есть.
Если Font пустой, он наследуется от предыдущего сегмента (для AddSegment — последнего, для InsertSegment — стоящего перед Index; при вставке в позицию 0 — от сегмента, который сдвигается на позицию 1). Typeface наследуется вместе со шрифтом, но только если он не задан.
Если Size равен 0, размер наследуется от того же сегмента.
Чтение
Функция	Описание
GetSegmentCount()	Количество сегментов.
GetSegmentText(Index)	Текст одного сегмента.
GetPlainText()	Тексты всех сегментов, склеенные без тегов.
BuildMarkup()	Разметка, сгенерированная из сегментов.
SetSegments и SetFormats

Обе принимают массив FSimpleRichTextSegment, но делают разное:

SetSegments полностью заменяет содержимое: тексты, форматы и количество сегментов берутся из массива.
SetFormats меняет только форматы уже существующих сегментов: количество сегментов и их тексты остаются, а поле Text внутри структур игнорируется.
Режим Single Field и API

В режиме Single Field функции, меняющие тексты сегментов или их количество (SetSegments, SetSegmentText, AddSegment, InsertSegment, RemoveSegment, ClearSegments), автоматически поддерживают CombinedText в актуальном состоянии, склеивая тексты сегментов через SegmentSeparator.

Пример на C++
cpp
#include "SimpleRichTextBlock.h"

void UMyWidget::ShowDamage(USimpleRichTextBlock* Block, int32 Damage)
{
	FSimpleRichTextSegment Label;
	Label.Text = FText::FromString(TEXT("Урон: "));
	Label.Color = FLinearColor::White;
	Label.Size = 24;

	FSimpleRichTextSegment Value;
	Value.Text = FText::AsNumber(Damage);
	Value.Color = FLinearColor::Red;   // шрифт и размер наследуются от предыдущего сегмента

	Block->ClearSegments();
	Block->AddSegment(Label);
	Block->AddSegment(Value);
}

Вариант с одним полем:

cpp
FSimpleRichTextSegment White; White.Color = FLinearColor::White;
FSimpleRichTextSegment Red;   Red.Color   = FLinearColor::Red;

Block->SetCombinedTextWithFormats(FText::FromString(TEXT("Урон: |42")), { White, Red });
Примечания и ограничения
Текст и сегменты не связаны в обе стороны. Сегменты записывают в Text, но разметка в Text не разбирается обратно в сегменты. Если отредактировать Text вручную, сегменты устареют, пока вы снова не отредактируете сегмент, что перезапишет Text.
Вызов SetText напрямую работает как обычно. При включённом Always Rebuild From Segments Text будет перезаписан из сегментов при следующей пересборке.
Typeface нужно вводить вручную.
Цвет сегмента по умолчанию белый и всегда записывается в тег.
Если в редакторе выбрано сразу несколько виджетов, Details-кастомизация обновляет Text только по первому выбранному объекту.
Требования
Unreal Engine 5.x (собрано на 5.7)
Используемые модули: Core, CoreUObject, Engine, Slate, SlateCore, UMG (runtime); дополнительно InputCore, PropertyEditor (editor)
Лицензия

MIT
