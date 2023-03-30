#pragma once
namespace Aspose {
namespace Cells {
namespace Rendering {
			/// <summary>
			/// Specifies the way <see cref="CustomDocumentPropertyCollection" />
			///  are exported to PDF file.
			/// </summary>
	enum PdfCustomPropertiesExport 

	{

			/// <summary>
			/// No custom properties are exported.
			/// </summary>
			PdfCustomPropertiesExport_None ,
			/// <summary>
			/// Custom properties are exported as entries in Info dictionary.
			/// </summary>
			/// <remarks>
			/// Custom properties with the following names are not exported:
			/// "Title", "Author", "Subject", "Keywords", "Creator", "Producer", "CreationDate", "ModDate", "Trapped".
			/// </remarks>
			PdfCustomPropertiesExport_Standard ,
	};


}

}

}
