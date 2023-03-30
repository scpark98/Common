#pragma once
#include "Aspose.Cells.Systems/Object.h"

namespace Aspose{
namespace Cells{
namespace Charts{
			/// <summary>
			/// Represents a complex type that specifies the pivot controls that appear on the chart
			/// </summary>
	class ASPOSE_CELLS_API IPivotOptions : public Aspose::Cells::Systems::Object
	{
		public:
			/// <summary>
			/// Specifies whether a control for each PivotTable field on the PivotTable page axis
			///  of the source PivotTable appears on the chart when dropZonesVisible is set to true.
			/// </summary>
			 virtual bool GetDropZoneFilter()=0;
			/// <summary>
			/// Specifies whether a control for each PivotTable field on the PivotTable page axis
			///  of the source PivotTable appears on the chart when dropZonesVisible is set to true.
			/// </summary>
			 virtual void SetDropZoneFilter(bool value)=0;
			/// <summary>
			/// Specifies whether a control for each PivotTable field on the PivotTable row axis
			///  of the source PivotTable appears on the chart when dropZonesVisible is set to true.
			/// </summary>
			 virtual bool GetDropZoneCategories()=0;
			/// <summary>
			/// Specifies whether a control for each PivotTable field on the PivotTable row axis
			///  of the source PivotTable appears on the chart when dropZonesVisible is set to true.
			/// </summary>
			 virtual void SetDropZoneCategories(bool value)=0;
			/// <summary>
			/// Specifies whether a control for each PivotTable field on the PivotTable data axis
			///  of the source PivotTable appears on the chart when dropZonesVisible is set to true.
			/// </summary>
			 virtual bool GetDropZoneData()=0;
			/// <summary>
			/// Specifies whether a control for each PivotTable field on the PivotTable data axis
			///  of the source PivotTable appears on the chart when dropZonesVisible is set to true.
			/// </summary>
			 virtual void SetDropZoneData(bool value)=0;
			/// <summary>
			/// Specifies whether a control for each PivotTable field on the PivotTable column axis
			///  of the source PivotTable appears on the chart when dropZonesVisible is set to true.
			/// </summary>
			 virtual bool GetDropZoneSeries()=0;
			/// <summary>
			/// Specifies whether a control for each PivotTable field on the PivotTable column axis
			///  of the source PivotTable appears on the chart when dropZonesVisible is set to true.
			/// </summary>
			 virtual void SetDropZoneSeries(bool value)=0;
			/// <summary>
			/// Specifies whether any pivot controls can appear on the pivot chart.
			/// </summary>
			 virtual bool GetDropZonesVisible()=0;
			/// <summary>
			/// Specifies whether any pivot controls can appear on the pivot chart.
			/// </summary>
			 virtual void SetDropZonesVisible(bool value)=0;

	};
}
}
}
