#pragma once
namespace Aspose {
namespace Cells {
			/// <summary>
			/// Represents five types of html cross string.
			/// </summary>
	enum HtmlCrossType 

	{

			/// <summary>
			/// Display like MS Excel,depends on the next cell. 
			/// If the next cell is null,the string will cross,or it will be truncated
			/// </summary>
			HtmlCrossType_Default ,
			/// <summary>
			/// Display the string like MS Excel exporting html.
			/// </summary>
			HtmlCrossType_MSExport ,
			/// <summary>
			/// Display HTML cross string, this performance for creating large html files will be more than ten times faster than setting the value to Default or FitToCell.
			/// </summary>
			HtmlCrossType_Cross ,
			/// <summary>
			/// Display HTML cross string and hide the right string when the texts overlap.
			/// </summary>
			HtmlCrossType_CrossHideRight ,
			/// <summary>
			/// Only displaying the string within the width of cell.
			/// </summary>
			HtmlCrossType_FitToCell ,
	};


}

}
