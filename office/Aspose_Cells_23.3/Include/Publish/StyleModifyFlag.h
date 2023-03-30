#pragma once
namespace Aspose {
namespace Cells {
			/// <summary>
			/// The style modified flags.
			/// </summary>
	enum StyleModifyFlag 

	{

			/// <summary>
			/// Indicates whether left border has been modified for the style.
			/// </summary>
			StyleModifyFlag_LeftBorder = 0x00000100 ,
			/// <summary>
			/// Indicates whether right border has been modified for the style.
			/// </summary>
			StyleModifyFlag_RightBorder = 0x00000200 ,
			/// <summary>
			/// Indicates whether top border has been modified for the style.
			/// </summary>
			StyleModifyFlag_TopBorder = 0x00000400 ,
			/// <summary>
			/// Indicates whether bottom border has been modified for the style.
			/// </summary>
			StyleModifyFlag_BottomBorder = 0x00000800 ,
			/// <summary>
			/// Indicates whether diagonal-down border has been modified for the style.
			/// </summary>
			StyleModifyFlag_DiagonalDownBorder = 0x00001000 ,
			/// <summary>
			/// Indicates whether diagonal-up border has been modified for the style.
			/// </summary>
			StyleModifyFlag_DiagonalUpBorder = 0x00002000 ,
			/// <summary>
			/// Indicates whether one or more diagonal borders(<see cref="DiagonalDownBorder" />
			/// ,
			/// <see cref="DiagonalUpBorder" />
			/// ) have been modified for the style.
			/// </summary>
			StyleModifyFlag_Diagonal = Aspose::Cells::StyleModifyFlag_DiagonalDownBorder|Aspose::Cells::StyleModifyFlag_DiagonalUpBorder ,
			/// <summary>
			/// Indicates whether horizontal border has been modified for the style.
			/// Only for dynamic style, such as conditional formatting.
			/// </summary>
			StyleModifyFlag_HorizontalBorder = 0x0000020 ,
			/// <summary>
			/// Indicates whether vertical border has been modified for the style.
			/// Only for dynamic style, such as conditional formatting.
			/// </summary>
			StyleModifyFlag_VerticalBorder = 0x0000040 ,
			/// <summary>
			/// Indicates whether one or more borders(<see cref="LeftBorder" />
			/// ,
			/// <see cref="RightBorder" />
			/// , <see cref="TopBorder" />
			/// , <see cref="BottomBorder" />
			/// ,
			/// <see cref="Diagonal" />
			/// , <see cref="HorizontalBorder" />
			/// , <see cref="VerticalBorder" />
			/// )
			/// have been modified for the style.
			/// </summary>
			StyleModifyFlag_Borders = Aspose::Cells::StyleModifyFlag_LeftBorder|Aspose::Cells::StyleModifyFlag_RightBorder|Aspose::Cells::StyleModifyFlag_TopBorder|Aspose::Cells::StyleModifyFlag_BottomBorder|Aspose::Cells::StyleModifyFlag_Diagonal|Aspose::Cells::StyleModifyFlag_HorizontalBorder|Aspose::Cells::StyleModifyFlag_VerticalBorder ,
			/// <summary>
			/// Indicates whether numberformat has been modified.
			/// </summary>
			StyleModifyFlag_NumberFormat = 0x00004000 ,
			/// <summary>
			/// Indicates whether horizontal alignment has been modified.
			/// </summary>
			StyleModifyFlag_HorizontalAlignment = 0x00008000 ,
			/// <summary>
			/// Indicates whether vertical alignment has been modified.
			/// </summary>
			StyleModifyFlag_VerticalAlignment = 0x00010000 ,
			/// <summary>
			/// Indicates whether indent property has been modified.
			/// </summary>
			StyleModifyFlag_Indent = 0x00020000 ,
			/// <summary>
			/// Indicates whether rotation property has been modified.
			/// </summary>
			StyleModifyFlag_Rotation = 0x00040000 ,
			/// <summary>
			/// Indicates whether wrap text property has been modified.
			/// </summary>
			StyleModifyFlag_WrapText = 0x00080000 ,
			/// <summary>
			/// Indicates whether shrink to fit property has been modified.
			/// </summary>
			StyleModifyFlag_ShrinkToFit = 0x00100000 ,
			/// <summary>
			/// Indicates whether text direction property has been modified.
			/// </summary>
			StyleModifyFlag_TextDirection = 0x00200000 ,
			/// <summary>
			/// Indicates whether relative indent property has been modified for the style.
			/// Only for dynamic style, such as conditional formatting.
			/// </summary>
			StyleModifyFlag_RelativeIndent = 0x0000080 ,
			/// <summary>
			/// Indicates whether one or more alignment-related properties(<see cref="HorizontalAlignment" />
			/// ,
			/// <see cref="VerticalAlignment" />
			/// , <see cref="Rotation" />
			/// , <see cref="WrapText" />
			/// ,
			/// <see cref="WrapText" />
			/// , <see cref="Indent" />
			/// , <see cref="ShrinkToFit" />
			/// , <see cref="TextDirection" />
			/// ,
			/// <see cref="RelativeIndent" />
			/// ) have been modified.
			/// </summary>
			StyleModifyFlag_AlignmentSettings = Aspose::Cells::StyleModifyFlag_HorizontalAlignment|Aspose::Cells::StyleModifyFlag_VerticalAlignment|Aspose::Cells::StyleModifyFlag_Rotation|Aspose::Cells::StyleModifyFlag_WrapText|Aspose::Cells::StyleModifyFlag_Indent|Aspose::Cells::StyleModifyFlag_ShrinkToFit|Aspose::Cells::StyleModifyFlag_TextDirection|Aspose::Cells::StyleModifyFlag_RelativeIndent ,
			/// <summary>
			/// Indicates whether pattern of the shading has been modified.
			/// </summary>
			StyleModifyFlag_Pattern = 0x00400000 ,
			/// <summary>
			/// Indicates whether foreground color has been modified.
			/// </summary>
			StyleModifyFlag_ForegroundColor = 0x00800000 ,
			/// <summary>
			/// Indicates whether background color has been modified.
			/// </summary>
			StyleModifyFlag_BackgroundColor = 0x01000000 ,
			/// <summary>
			/// Indicates whether one or more shading-related properties(<see cref="Pattern" />
			/// ,
			/// <see cref="ForegroundColor" />
			/// , <see cref="BackgroundColor" />
			/// ) have been modified.
			/// </summary>
			StyleModifyFlag_CellShading = Aspose::Cells::StyleModifyFlag_Pattern|Aspose::Cells::StyleModifyFlag_ForegroundColor|Aspose::Cells::StyleModifyFlag_BackgroundColor ,
			/// <summary>
			/// Indicates whether locked property has been modified.
			/// </summary>
			StyleModifyFlag_Locked = 0x04000000 ,
			/// <summary>
			/// Indicates whether hide formula has been modified.
			/// </summary>
			StyleModifyFlag_HideFormula = 0x08000000 ,
			/// <summary>
			/// Indicates whether one or more protection-related properties(<see cref="Locked" />
			/// ,
			/// <see cref="HideFormula" />
			/// ) have been modified.
			/// </summary>
			StyleModifyFlag_ProtectionSettings = Aspose::Cells::StyleModifyFlag_Locked|Aspose::Cells::StyleModifyFlag_HideFormula ,
			/// <summary>
			/// Indicates whether font size has been modified.
			/// </summary>
			StyleModifyFlag_FontSize = 0x00000001 ,
			/// <summary>
			/// Indicates whether font name has been modified.
			/// </summary>
			StyleModifyFlag_FontName = 0x00000002 ,
			/// <summary>
			/// Indicates whether font color has been modified.
			/// </summary>
			StyleModifyFlag_FontColor = 0x00000003 ,
			/// <summary>
			/// Indicates whether font weight has been modified.
			/// </summary>
			StyleModifyFlag_FontWeight = 0x00000004 ,
			/// <summary>
			/// Indicates whether italic property of font has been modified.
			/// </summary>
			StyleModifyFlag_FontItalic = 0x00000005 ,
			/// <summary>
			/// Indicates whether underline property of font has been modified.
			/// </summary>
			StyleModifyFlag_FontUnderline = 0x00000006 ,
			/// <summary>
			/// Indicates whether strike property font has been modified.
			/// </summary>
			StyleModifyFlag_FontStrike = 0x00000007 ,
			/// <summary>
			/// Indicates whether subscript or superscript property of font has been modified.
			/// </summary>
			StyleModifyFlag_FontScript = 0x00000008 ,
			/// <summary>
			/// Indicates whether font family has been modified.
			/// </summary>
			StyleModifyFlag_FontFamily = 0x00000009 ,
			/// <summary>
			/// Indicates whether charset of the font has been modified.
			/// </summary>
			StyleModifyFlag_FontCharset = 0x0000000A ,
			/// <summary>
			/// unused.
			/// </summary>
			StyleModifyFlag_FontScheme = 0x0000000B ,
			/// <summary>
			/// unused.
			/// </summary>
			StyleModifyFlag_FontDirty = 0x0000000C ,
			/// <summary>
			/// unused.
			/// </summary>
			StyleModifyFlag_FontSpellingError = 0x0000000D ,
			/// <summary>
			/// unused.
			/// </summary>
			StyleModifyFlag_FontUFillTx = 0x0000000E ,
			/// <summary>
			/// unused.
			/// </summary>
			StyleModifyFlag_FontSpacing = 0x0000000F ,
			/// <summary>
			/// unused.
			/// </summary>
			StyleModifyFlag_FontKerning = 0x00000010 ,
			/// <summary>
			/// unused.
			/// </summary>
			StyleModifyFlag_FontEqualize = 0x00000011 ,
			/// <summary>
			/// unused.
			/// </summary>
			StyleModifyFlag_FontCap = 0x00000012 ,
			/// <summary>
			/// 
			/// </summary>
			StyleModifyFlag_FontVerticalText = 0x00000013 ,
			/// <summary>
			/// Indicates whether one or more properties have been modified for the font of the style.
			/// </summary>
			StyleModifyFlag_Font = 0x000000014 ,
			/// <summary>
			/// Indicates whether one or more properties have been modified for the style.
			/// </summary>
			StyleModifyFlag_All = Aspose::Cells::StyleModifyFlag_Borders|Aspose::Cells::StyleModifyFlag_NumberFormat|Aspose::Cells::StyleModifyFlag_AlignmentSettings|Aspose::Cells::StyleModifyFlag_CellShading|Aspose::Cells::StyleModifyFlag_ProtectionSettings|Aspose::Cells::StyleModifyFlag_Font ,
	};


}

}
