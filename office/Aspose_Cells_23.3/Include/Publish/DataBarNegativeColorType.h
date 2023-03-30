#pragma once
namespace Aspose {
namespace Cells {
			/// <summary>
			/// Specifies whether to use the same border and fill color as positive data bars.
			/// </summary>
	enum DataBarNegativeColorType 

	{

			/// <summary>
			/// Use the color specified in the Negative Value and Axis Setting dialog box 
			/// or by using the ColorType and BorderColorType properties of the NegativeBarFormat object.
			/// </summary>
			DataBarNegativeColorType_Color = 0 ,
			/// <summary>
			/// Use the same color as positive data bars.
			/// </summary>
			DataBarNegativeColorType_SameAsPositive = 1 ,
	};


}

}
