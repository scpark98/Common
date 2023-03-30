#pragma once
#include "Aspose.Cells.Systems/Object.h"
#include "Aspose.Cells.Systems/Int32.h"
#include "Aspose.Cells.Systems/Double.h"
#include "Aspose.Cells.Systems/Byte.h"

namespace Aspose {
	namespace Cells {
		class IStyle;
		class IStyleFlag;
	}
}
namespace Aspose{
namespace Cells{

	class ASPOSE_CELLS_API IColumn : public Aspose::Cells::Systems::Object
	{
		public:
			/// <summary>
			/// Gets the index of this column.
			/// </summary>
			  virtual Aspose::Cells::Systems::Int32 GetIndex() = 0;
			/// <summary>
			/// Gets the column width in unit of characters.
			/// </summary>
			  virtual Aspose::Cells::Systems::Double GetWidth() = 0;
			/// <summary>
			/// Sets the column width in unit of characters.
			/// </summary>
			  virtual void SetWidth(Aspose::Cells::Systems::Double value) = 0;
			/// <summary>
			/// Gets the group level of the column.
			/// </summary>
			  virtual Aspose::Cells::Systems::Byte GetGroupLevel() = 0;
			/// <summary>
			/// Gets the group level of the column.
			/// </summary>
			 virtual void SetGroupLevel(Aspose::Cells::Systems::Byte value)=0;
			/// <summary>
			/// Indicates whether the column is hidden.
			/// </summary>
			  virtual bool IsHidden() = 0;
			/// <summary>
			/// Indicates whether the column is hidden.
			/// </summary>
			  virtual void SetHidden(bool value) = 0;
			/// <summary>
			/// Indicates whether this column has custom style settings(different from the default one inherited from workbook).
			/// </summary>
			 virtual bool HasCustomStyle()=0;
			/// <summary>
			/// Applies formats for a whole column.
			/// </summary>
			/// <param name="style" >The style object which will be applied.</param>
			/// <param name="flag" >Flags which indicates applied formatting properties.</param>
			 virtual void 		ApplyIStyle(intrusive_ptr<Aspose::Cells::IStyle> style , intrusive_ptr<Aspose::Cells::IStyleFlag> flag)=0;
			/// <summary>
			/// Gets the style of this column.
			/// </summary>
			/// <remarks>
			/// You have to call Column.ApplyIStyle() method to save your changing with the row style,
			/// otherwise it will not effect.
			/// </remarks>
			 virtual intrusive_ptr<Aspose::Cells::IStyle> 		GetIStyle()=0;
			/// <summary>
			/// Sets the style of this column.
			/// </summary>
			/// <param name="style" >the style to be used as the default style for cells in this column.</param>
			 virtual void 		SetIStyle(intrusive_ptr<Aspose::Cells::IStyle> style)=0;
			/// <summary>
			///  whether the column is collapsed
			/// </summary>
			 virtual bool IsCollapsed()=0;
			/// <summary>
			///  whether the column is collapsed
			/// </summary>
			 virtual void SetCollapsed(bool value)=0;

	};
}
}
