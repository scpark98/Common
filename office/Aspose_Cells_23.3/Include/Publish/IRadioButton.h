#pragma once
#include "Aspose.Cells.Systems/Object.h"
#include "IShape.h"

namespace Aspose {
	namespace Cells {
		namespace Drawing {
			class IGroupBox;
		}
	}
}
namespace Aspose{
namespace Cells{
namespace Drawing{
			/// <summary>
			/// Represents a radio button.
			/// </summary>
	class ASPOSE_CELLS_API IRadioButton : virtual public Aspose::Cells::Drawing::IShape
	{
		public:
			/// <summary>
			/// Indicates if the radiobutton is checked or not.
			/// </summary>
			 virtual bool IsChecked()=0;
			/// <summary>
			/// Indicates if the radiobutton is checked or not.
			/// </summary>
			 virtual void SetChecked(bool value)=0;
			/// <summary>
			/// Indicates whether the combobox has 3-D shading.
			/// </summary>
			 virtual bool GetShadow()=0;
			/// <summary>
			/// Indicates whether the combobox has 3-D shading.
			/// </summary>
			 virtual void SetShadow(bool value)=0;
			/// <summary>
			/// Gets the GroupBox that contains this RadioButton.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Drawing::IGroupBox> 		GetIGroupBox()=0;

	};
}
}
}
