#pragma once
namespace Aspose {
namespace Cells {
			/// <summary>
			/// Enumerates displaying text type when the text width is larger than cell width.
			/// </summary>
	enum TextCrossType 

	{

			/// <summary>
			/// Display text like in Microsoft Excel.
			/// </summary>
			TextCrossType_Default = 1 ,
			/// <summary>
			/// Display all the text by crossing other cells and keep text of crossed cells.
			/// </summary>
			TextCrossType_CrossKeep = 2 ,
			/// <summary>
			/// Display all the text by crossing other cells and override text of crossed cells.
			/// </summary>
			TextCrossType_CrossOverride = 3 ,
			/// <summary>
			/// Only display the text within the width of cell.
			/// </summary>
			TextCrossType_StrictInCell = 4 ,
	};


}

}
