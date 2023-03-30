#pragma once
namespace Aspose {
namespace Cells {
			/// <summary>
			/// Represents the auto fill type.
			/// </summary>
	enum AutoFillType 

	{

			/// <summary>
			/// Copies the value and format of the source area to the target area 
			/// </summary>
			AutoFillType_Copy = 1 ,
			/// <summary>
			/// Automatically fills the target area with the value and format.
			/// </summary>
			AutoFillType_Default = 0 ,
			/// <summary>
			/// Copies only the format of the source area to the target area,
			/// </summary>
			AutoFillType_Formats = 3 ,
			/// <summary>
			/// Extend the value in the source area to the target area in the form of a series and copy format to the target area.
			/// </summary>
			AutoFillType_Series = 2 ,
			/// <summary>
			/// Copies only the value of the source area to the target area,
			/// </summary>
			AutoFillType_Values = 4 ,
	};


}

}
