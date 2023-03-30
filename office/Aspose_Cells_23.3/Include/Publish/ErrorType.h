#pragma once
namespace Aspose {
namespace Cells {
			/// <summary>
			/// Summary description for ErrorType.
			/// </summary>
	enum ErrorType 

	{

			/// <summary>
			/// Division by zero.
			/// </summary>
			ErrorType_Div = 0x07 ,
			/// <summary>
			/// Cell values doesn't exist.
			/// </summary>
			ErrorType_NA = 0x2A ,
			/// <summary>
			/// Text cannot be identified.
			/// </summary>
			ErrorType_Name = 0x1D ,
			/// <summary>
			/// Null error..
			/// </summary>
			ErrorType_Null = 0 ,
			/// <summary>
			/// Invalid number.
			/// </summary>
			ErrorType_Number = 0x24 ,
			/// <summary>
			/// Invalid reference.
			/// </summary>
			ErrorType_Ref = 0x17 ,
			/// <summary>
			/// Recursive reference.
			/// </summary>
			ErrorType_Recursive = 0xFE ,
			/// <summary>
			/// Invalid parameter or operand type.
			/// </summary>
			ErrorType_Value = 0x0F ,
	};


}

}
