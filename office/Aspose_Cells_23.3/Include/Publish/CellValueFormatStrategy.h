#pragma once
namespace Aspose {
namespace Cells {
			/// <summary>
			/// Specifies how to apply style for the value of the cell.
			/// </summary>
	enum CellValueFormatStrategy 

	{

			/// <summary>
			/// Not formatted.
			/// </summary>
			CellValueFormatStrategy_None ,
			/// <summary>
			/// Only formatted with the cell's original style.
			/// </summary>
			CellValueFormatStrategy_CellStyle ,
			/// <summary>
			/// Formatted with the cell's displayed style.
			/// </summary>
			CellValueFormatStrategy_DisplayStyle ,
			/// <summary>
			/// Gets the displayed string shown in ms excel.
			/// The main difference from <see cref="DisplayStyle" />
			///  is this option also considers the effect of column width.
			/// If the column width is too small to show the formatted string completely,
			/// "#" may be shown, just like what ms excel does.
			/// </summary>
			CellValueFormatStrategy_DisplayString ,
	};


}

}
