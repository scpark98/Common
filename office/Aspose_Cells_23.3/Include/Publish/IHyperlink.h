#pragma once
#include "Aspose.Cells.Systems/Object.h"
#include "Aspose.Cells.Systems/String.h"
#include "TargetModeType.h"

namespace Aspose {
	namespace Cells {
		class ICellArea;
		enum TargetModeType;
	}
}
namespace Aspose{
namespace Cells{
			/// <summary>
			/// Encapsulates the object that represents a hyperlink.
			/// </summary>
			/// <example><code>
			/// 
			/// [C++]
			/// 
			/// //Instantiating a Workbook object
			/// intrusive_ptr<Aspose::Cells::IWorkbook> workbook = Factory::CreateIWorkbook();
			/// //Adding a new worksheet to the Workbook object
			/// workbook->GetIWorksheets()->Add();
			/// //Obtaining the reference of the newly added worksheet by passing its sheet index
			/// intrusive_ptr<IWorksheet> worksheet = workbook->GetIWorksheets()->GetObjectByIndex(0);
			/// //Adding a hyperlink to a URL at "A1" cell
			/// worksheet->GetIHyperlinks()->Add(new String("A1"), 1, 1, new String("http://www.aspose.com"));
			/// //Saving the Excel file
			/// workbook->Save(new String("C:\\IHyperlinkTest.xls"));
			/// 
			/// </code>
			/// </example>
	class ASPOSE_CELLS_API IHyperlink : public Aspose::Cells::Systems::Object
	{
		public:
			/// <summary>
			/// Represents the address of a hyperlink.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Systems::String> GetAddress()=0;
			/// <summary>
			/// Represents the address of a hyperlink.
			/// </summary>
			 virtual void SetAddress(intrusive_ptr<Aspose::Cells::Systems::String> value)=0;
			/// <summary>
			/// Represents the text to be displayed for the specified hyperlink. The default value is the address of the hyperlink. 
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Systems::String> GetTextToDisplay()=0;
			/// <summary>
			/// Represents the text to be displayed for the specified hyperlink. The default value is the address of the hyperlink. 
			/// </summary>
			 virtual void SetTextToDisplay(intrusive_ptr<Aspose::Cells::Systems::String> value)=0;
			/// <summary>
			/// Gets the range of hyperlink.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::ICellArea> 		GetIArea()=0;
			/// <summary>
			/// Gets the screen tip text for the specified hyperlink.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Systems::String> GetScreenTip()=0;
			/// <summary>
			/// Sets the screen tip text for the specified hyperlink.
			/// </summary>
			 virtual void SetScreenTip(intrusive_ptr<Aspose::Cells::Systems::String> value)=0;
			/// <summary>
			/// Gets the link type.
			/// </summary>
			 virtual Aspose::Cells::TargetModeType GetLinkType()=0;
			/// <summary>
			/// Deletes this hyperlink
			/// </summary>
			 virtual void Delete()=0;

	};
}
}
