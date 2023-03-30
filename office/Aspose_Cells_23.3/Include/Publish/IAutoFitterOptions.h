#pragma once
#include "Aspose.Cells.Systems/Object.h"
#include "DefaultEditLanguage.h"
#include "AutoFitMergedCellsType.h"
#include "Aspose.Cells.Systems/Double.h"
#include "AutoFitWrappedTextType.h"
#include "CellValueFormatStrategy.h"

namespace Aspose {
	namespace Cells {
		enum DefaultEditLanguage;
		enum AutoFitMergedCellsType;
		enum AutoFitWrappedTextType;
		enum CellValueFormatStrategy;
	}
}
namespace Aspose{
namespace Cells{
			/// <summary>
			/// Represents all auto fitter options.
			/// </summary>
	class ASPOSE_CELLS_API IAutoFitterOptions : public Aspose::Cells::Systems::Object
	{
		public:
			/// <summary>
			/// Gets default edit language.
			/// </summary>
			/// <remarks>
			/// It may display/render different layouts for text paragraph when different edit languages is set.
			/// Default is Auto <see cref="DefaultEditLanguage" />
			/// .
			/// </remarks>
			 virtual Aspose::Cells::DefaultEditLanguage GetDefaultEditLanguage()=0;
			/// <summary>
			/// Sets default edit language.
			/// </summary>
			/// <remarks>
			/// It may display/render different layouts for text paragraph when different edit languages is set.
			/// Default is Auto <see cref="DefaultEditLanguage" />
			/// .
			/// </remarks>
			 virtual void SetDefaultEditLanguage(Aspose::Cells::DefaultEditLanguage value)=0;
			/// <summary>
			/// Indicates whether auto fit row height when the cells is merged in a row.
			/// The default value is false.
			/// </summary>
			/// <remarks>
			/// If the cells in merged in a row and auto fit the rows in MS Excel,
			/// MS Excel do not expand the row height.
			/// If this option is true, Aspose.Cells will expand the row height to fit the cells' data.
			/// </remarks>
			 virtual bool GetAutoFitMergedCells()=0;
			/// <summary>
			/// Indicates whether auto fit row height when the cells is merged in a row.
			/// The default value is false.
			/// </summary>
			/// <remarks>
			/// If the cells in merged in a row and auto fit the rows in MS Excel,
			/// MS Excel do not expand the row height.
			/// If this option is true, Aspose.Cells will expand the row height to fit the cells' data.
			/// </remarks>
			 virtual void SetAutoFitMergedCells(bool value)=0;
			/// <summary>
			/// Gets and set the type of auto fitting row height of merged cells.
			/// </summary>
			/// <remarks>
			/// Excel defaults to ignore merged cells when fitting the row height, so Aspose.Cells works as MS Excel default.
			/// Please set this type to change the way of auto fitting row height of merged cells.
			/// </remarks>
			 virtual Aspose::Cells::AutoFitMergedCellsType GetAutoFitMergedCellsType()=0;
			/// <summary>
			/// Gets and set the type of auto fitting row height of merged cells.
			/// </summary>
			/// <remarks>
			/// Excel defaults to ignore merged cells when fitting the row height, so Aspose.Cells works as MS Excel default.
			/// Please set this type to change the way of auto fitting row height of merged cells.
			/// </remarks>
			 virtual void SetAutoFitMergedCellsType(Aspose::Cells::AutoFitMergedCellsType value)=0;
			/// <summary>
			/// Indicates whether only fit the rows which height are not customed.
			/// </summary>
			 virtual bool GetOnlyAuto()=0;
			/// <summary>
			/// Indicates whether only fit the rows which height are not customed.
			/// </summary>
			 virtual void SetOnlyAuto(bool value)=0;
			/// <summary>
			/// Ingores the hidden rows/columns.
			/// </summary>
			 virtual bool GetIgnoreHidden()=0;
			/// <summary>
			/// Ingores the hidden rows/columns.
			/// </summary>
			 virtual void SetIgnoreHidden(bool value)=0;
			/// <summary>
			/// Gets the max row height(in unit of Point) when autofitting rows.
			/// </summary>
			 virtual Aspose::Cells::Systems::Double GetMaxRowHeight()=0;
			/// <summary>
			/// Sets the max row height(in unit of Point) when autofitting rows.
			/// </summary>
			 virtual void SetMaxRowHeight(Aspose::Cells::Systems::Double value)=0;
			/// <summary>
			/// Gets the type of auto fitting wrapped text.
			/// </summary>
			 virtual Aspose::Cells::AutoFitWrappedTextType GetAutoFitWrappedTextType()=0;
			/// <summary>
			/// Sets the type of auto fitting wrapped text.
			/// </summary>
			 virtual void SetAutoFitWrappedTextType(Aspose::Cells::AutoFitWrappedTextType value)=0;
			/// <summary>
			/// Gets the formatted strategy.
			/// </summary>
			/// <remarks>
			/// The default value is CellStyle for performance.
			/// </remarks>
			 virtual Aspose::Cells::CellValueFormatStrategy GetFormatStrategy()=0;
			/// <summary>
			/// Sets the formatted strategy.
			/// </summary>
			/// <remarks>
			/// The default value is CellStyle for performance.
			/// </remarks>
			 virtual void SetFormatStrategy(Aspose::Cells::CellValueFormatStrategy value)=0;

	};
}
}
