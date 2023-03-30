#pragma once
#include "Aspose.Cells.Systems/Object.h"
#include "Aspose.Cells.Systems/Int32.h"
#include "SortOrder.h"
#include "Aspose.Cells.Systems/String.h"
#include "SortOnType.h"
#include "Aspose.Cells.Systems/Array1D.h"

namespace Aspose {
	namespace Cells {
		enum SortOrder;
		enum SortOnType;
		class ICells;
		class ICellArea;
	}
}
namespace Aspose{
namespace Cells{
			/// <summary>
			/// Summary description for DataSorter.
			/// </summary>
			/// <example><code>
			/// 
			/// [C++]
			/// //You need to create a right Workbook to test.
			/// intrusive_ptr<IWorkbook> workbook = Factory::CreateIWorkbook(new String("D:\\Book1.xls"));
			/// //Get the workbook datasorter object.
			/// intrusive_ptr<IDataSorter> sorter = workbook->GetIDataSorter();
			/// //Set the first order for datasorter object.
			/// sorter->SetOrder1(SortOrder_Descending);
			/// //Define the first key.
			/// sorter->SetKey1(0);
			/// //Set the second order for datasorter object.
			/// sorter->SetOrder2(SortOrder_Ascending);
			/// //Define the second key.
			/// sorter->SetKey2(1);
			/// //Create a cells area (range).
			/// intrusive_ptr<ICellArea> ca = Factory::CreateICellArea();
			/// //Specify the start row index.
			/// ca->SetstartRow(0);
			/// //Specify the start column index.
			/// ca->SetstartColumn(0);
			/// //Specify the last row index.
			/// ca->SetendRow(13);
			/// //Specify the last column index.
			/// ca->SetendColumn(1);
			/// //Sort data in the specified data range (A1:B14)
			/// sorter->Sort(workbook->GetIWorksheets()->GetObjectByIndex(0)->GetICells(), ca);
			/// //Save the excel file.
			/// workbook->Save(new String("D:\\outBook.xls"));
			///
			/// </code>
			/// </example>
	class ASPOSE_CELLS_API IDataSorter : public Aspose::Cells::Systems::Object
	{
		public:
			/// <summary>
			/// Clear all settings.
			/// </summary>
			 virtual void Clear()=0;
			/// <summary>
			/// Represents whether the range has headers.
			/// </summary>
			 virtual bool HasHeaders()=0;
			/// <summary>
			/// Represents whether the range has headers.
			/// </summary>
			 virtual void SetHasHeaders(bool value)=0;
			/// <summary>
			/// Adds sorted column index and sort order.
			/// </summary>
			/// <param name="key" >The sorted column index(absolute position, column A is 0, B is 1, ...)</param>
			/// <param name="order" >The sort order</param>
			 virtual void AddKey(Aspose::Cells::Systems::Int32 key , Aspose::Cells::SortOrder order)=0;
			/// <summary>
			/// Adds sorted column index and sort order with custom sort list.
			/// </summary>
			/// <param name="key" >The sorted column index(absolute position, column A is 0, B is 1, ...)</param>
			/// <param name="order" >The sort order.</param>
			/// <param name="customList" >The custom sort list.</param>
			 virtual void AddKey(Aspose::Cells::Systems::Int32 key , Aspose::Cells::SortOrder order , intrusive_ptr<Aspose::Cells::Systems::String> customList)=0;
			/// <summary>
			/// Adds sorted column index and sort order with custom sort list.
			/// </summary>
			/// <param name="key" >The sorted column index(absolute position, column A is 0, B is 1, ...)</param>
			/// <param name="type" >The sorted value type.</param>
			/// <param name="order" >The sort order.</param>
			/// <param name="customList" >The custom sort list.</param>
			/// <remarks>
			/// If type is SortOnType.CellColor or SortOnType.FontColor, the customList is Color.
			/// </remarks>
			 virtual void AddKey(Aspose::Cells::Systems::Int32 key , Aspose::Cells::SortOnType type , Aspose::Cells::SortOrder order , intrusive_ptr<Aspose::Cells::Systems::Object> customList)=0;
			/// <summary>
			/// Adds sorted column index and sort order with custom sort list.
			/// </summary>
			/// <param name="key" >The sorted column index(absolute position, column A is 0, B is 1, ...)</param>
			/// <param name="order" >The sort order.</param>
			/// <param name="customList" >The custom sort list.</param>
			 virtual void AddKey(Aspose::Cells::Systems::Int32 key , Aspose::Cells::SortOrder order , intrusive_ptr<Aspose::Cells::Systems::Array1D<Aspose::Cells::Systems::String*>> customList)=0;
			/// <summary>
			/// Represents first sorted column index(absolute position, column A is 0, B is 1, ...). 
			/// </summary>
			 virtual Aspose::Cells::Systems::Int32 GetKey1()=0;
			/// <summary>
			/// Represents first sorted column index(absolute position, column A is 0, B is 1, ...). 
			/// </summary>
			 virtual void SetKey1(Aspose::Cells::Systems::Int32 value)=0;
			/// <summary>
			/// Represents sort order of the first key.
			/// </summary>
			 virtual Aspose::Cells::SortOrder GetOrder1()=0;
			/// <summary>
			/// Represents sort order of the first key.
			/// </summary>
			 virtual void SetOrder1(Aspose::Cells::SortOrder value)=0;
			/// <summary>
			/// Represents second sorted column index(absolute position, column A is 0, B is 1, ...).
			/// </summary>
			 virtual Aspose::Cells::Systems::Int32 GetKey2()=0;
			/// <summary>
			/// Represents second sorted column index(absolute position, column A is 0, B is 1, ...).
			/// </summary>
			 virtual void SetKey2(Aspose::Cells::Systems::Int32 value)=0;
			/// <summary>
			/// Represents sort order of the second key.
			/// </summary>
			 virtual Aspose::Cells::SortOrder GetOrder2()=0;
			/// <summary>
			/// Represents sort order of the second key.
			/// </summary>
			 virtual void SetOrder2(Aspose::Cells::SortOrder value)=0;
			/// <summary>
			/// Represents third sorted column index(absolute position, column A is 0, B is 1, ...).
			/// </summary>
			 virtual Aspose::Cells::Systems::Int32 GetKey3()=0;
			/// <summary>
			/// Represents third sorted column index(absolute position, column A is 0, B is 1, ...).
			/// </summary>
			 virtual void SetKey3(Aspose::Cells::Systems::Int32 value)=0;
			/// <summary>
			/// Represents sort order of the third key.
			/// </summary>
			 virtual Aspose::Cells::SortOrder GetOrder3()=0;
			/// <summary>
			/// Represents sort order of the third key.
			/// </summary>
			 virtual void SetOrder3(Aspose::Cells::SortOrder value)=0;
			/// <summary>
			/// True means that sorting orientation is from left to right.
			/// False means that sorting orientation is from top to bottom.
			/// The default value is false.
			/// </summary>
			 virtual bool GetSortLeftToRight()=0;
			/// <summary>
			/// True means that sorting orientation is from left to right.
			/// False means that sorting orientation is from top to bottom.
			/// The default value is false.
			/// </summary>
			 virtual void SetSortLeftToRight(bool value)=0;
			/// <summary>
			/// Gets and sets whether case sensitive when comparing string.
			/// </summary>
			 virtual bool GetCaseSensitive()=0;
			/// <summary>
			/// Gets and sets whether case sensitive when comparing string.
			/// </summary>
			 virtual void SetCaseSensitive(bool value)=0;
			/// <summary>
			/// Indicates whether sorting anything that looks like a number.
			/// </summary>
			 virtual bool GetSortAsNumber()=0;
			/// <summary>
			/// Indicates whether sorting anything that looks like a number.
			/// </summary>
			 virtual void SetSortAsNumber(bool value)=0;
			/// <summary>
			/// Sorts the data of the area.
			/// </summary>
			/// <param name="cells" >The cells contains the data area.</param>
			/// <param name="startRow" >The start row of the area.</param>
			/// <param name="startColumn" >The start column of the area.</param>
			/// <param name="endRow" >The end row of the area.</param>
			/// <param name="endColumn" >The end column of the area.</param>
			/// <returns>the original indices(absolute position, for example, column A is 0, B is 1, ...) of the sorted rows/columns.
			/// If no rows/columns needs to be moved by this sorting operation, null will be returned.</returns>
			 virtual intrusive_ptr<Aspose::Cells::Systems::Array1D<Aspose::Cells::Systems::Int32>> 		Sort(intrusive_ptr<Aspose::Cells::ICells> cells , Aspose::Cells::Systems::Int32 startRow , Aspose::Cells::Systems::Int32 startColumn , Aspose::Cells::Systems::Int32 endRow , Aspose::Cells::Systems::Int32 endColumn)=0;
			/// <summary>
			/// Sort the data of the area.
			/// </summary>
			/// <param name="cells" >The cells contains the data area.</param>
			/// <param name="area" >The area needed to sort</param>
			/// <returns>the original indices(absolute position, for example, column A is 0, B is 1, ...) of the sorted rows/columns.
			/// If no rows/columns needs to be moved by this sorting operation, null will be returned.</returns>
			 virtual intrusive_ptr<Aspose::Cells::Systems::Array1D<Aspose::Cells::Systems::Int32>> 		Sort(intrusive_ptr<Aspose::Cells::ICells> cells , intrusive_ptr<Aspose::Cells::ICellArea> area)=0;
			/// <summary>
			/// Sort the data in the range.
			/// </summary>
			/// <returns>the original indices(absolute position, for example, column A is 0, B is 1, ...) of the sorted rows/columns.
			/// If no rows/columns needs to be moved by this sorting operation, null will be returned.</returns>
			 virtual intrusive_ptr<Aspose::Cells::Systems::Array1D<Aspose::Cells::Systems::Int32>> Sort()=0;

	};
}
}
