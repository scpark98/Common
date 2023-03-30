#pragma once
namespace Aspose {
namespace Cells {
			/// <summary>
			/// WaringType
			/// </summary>
	enum WarningType 

	{

			/// <summary>
			/// Font substitution warning type
			/// when a font has not been found, this warning type can be get.
			/// </summary>
			WarningType_FontSubstitution = 0 ,
			/// <summary>
			/// Duplicate defined name is found in the file.
			/// </summary>
			WarningType_DuplicateDefinedName = 1 ,
			/// <summary>
			/// Unsupported file format.
			/// </summary>
			WarningType_UnsupportedFileFormat = 2 ,
			/// <summary>
			/// Invalid text of the defined name.
			/// </summary>
			WarningType_InvalidTextOfDefinedName = 3 ,
			/// <summary>
			/// Invalid the font name.
			/// </summary>
			WarningType_InvalidFontName = 4 ,
			/// <summary>
			/// Invalid autofilter range.
			/// </summary>
			WarningType_InvalidAutoFilterRange = 5 ,
			/// <summary>
			/// The file is corrupted.
			/// </summary>
			WarningType_IO = 6 ,
			/// <summary>
			/// Out of MS Excel limitation error.
			/// </summary>
			WarningType_Limitation = 7 ,
			/// <summary>
			/// Invalid data.
			/// </summary>
			WarningType_InvalidData = 8 ,
			/// <summary>
			/// Invalid formula.
			/// </summary>
			WarningType_Formula = 9 ,
			/// <summary>
			/// Invalid operator.
			/// </summary>
			WarningType_InvalidOperator = 10 ,
	};


}

}
