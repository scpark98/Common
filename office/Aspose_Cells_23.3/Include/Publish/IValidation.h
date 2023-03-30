#pragma once
#include "Aspose.Cells.Systems/Object.h"
#include "OperatorType.h"
#include "ValidationAlertType.h"
#include "ValidationType.h"
#include "Aspose.Cells.Systems/String.h"
#include "Aspose.Cells.Systems/Int32.h"
#include "Aspose.Cells.Systems/Array1D.h"

namespace Aspose {
	namespace Cells {
		enum OperatorType;
		enum ValidationAlertType;
		enum ValidationType;
		class ICellArea;
		class IValidation;
		class ICopyOptions;
	}
}
namespace Aspose{
namespace Cells{
			/// <summary>
			///  Represents data validation settings.
			///  </summary>
			/// <example>
			///  <code>
			///  [C++]
			///  intrusive_ptr<IWorkbook> workbook = Factory::CreateIWorkbook();
			///  intrusive_ptr<IValidationCollection> validations = workbook->GetIWorksheets()->GetObjectByIndex(0)->GetIValidations();
			///  intrusive_ptr<IValidation> validation = validations->GetObjectByIndex(validations->Add());
			///  validation->SetType(ValidationType_WholeNumber);
			///  validation->SetOperator(OperatorType_Between);
			///  validation->SetFormula1(new String("3"));
			///  validation->SetFormula2(new String("1234"));
			///  
			/// 
			///  </code>
			/// 
			///  </example>
	class ASPOSE_CELLS_API IValidation : public Aspose::Cells::Systems::Object
	{
		public:
			/// <summary>
			/// Represents the operator for the data validation.
			/// </summary>
			 virtual Aspose::Cells::OperatorType GetOperator()=0;
			/// <summary>
			/// Represents the operator for the data validation.
			/// </summary>
			 virtual void SetOperator(Aspose::Cells::OperatorType value)=0;
			/// <summary>
			/// Represents the validation alert style.
			/// </summary>
			 virtual Aspose::Cells::ValidationAlertType GetAlertStyle()=0;
			/// <summary>
			/// Represents the validation alert style.
			/// </summary>
			 virtual void SetAlertStyle(Aspose::Cells::ValidationAlertType value)=0;
			/// <summary>
			/// Represents the data validation type.
			/// </summary>
			 virtual Aspose::Cells::ValidationType GetType()=0;
			/// <summary>
			/// Represents the data validation type.
			/// </summary>
			 virtual void SetType(Aspose::Cells::ValidationType value)=0;
			/// <summary>
			/// Represents the data validation input message.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Systems::String> GetInputMessage()=0;
			/// <summary>
			/// Represents the data validation input message.
			/// </summary>
			 virtual void SetInputMessage(intrusive_ptr<Aspose::Cells::Systems::String> value)=0;
			/// <summary>
			/// Represents the title of the data-validation input dialog box.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Systems::String> GetInputTitle()=0;
			/// <summary>
			/// Represents the title of the data-validation input dialog box.
			/// </summary>
			 virtual void SetInputTitle(intrusive_ptr<Aspose::Cells::Systems::String> value)=0;
			/// <summary>
			/// Represents the data validation error message.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Systems::String> GetErrorMessage()=0;
			/// <summary>
			/// Represents the data validation error message.
			/// </summary>
			 virtual void SetErrorMessage(intrusive_ptr<Aspose::Cells::Systems::String> value)=0;
			/// <summary>
			/// Represents the title of the data-validation error dialog box. 
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Systems::String> GetErrorTitle()=0;
			/// <summary>
			/// Represents the title of the data-validation error dialog box. 
			/// </summary>
			 virtual void SetErrorTitle(intrusive_ptr<Aspose::Cells::Systems::String> value)=0;
			/// <summary>
			/// Indicates whether the data validation input message will be displayed whenever the user selects a cell in the data validation range.
			/// </summary>
			 virtual bool GetShowInput()=0;
			/// <summary>
			/// Indicates whether the data validation input message will be displayed whenever the user selects a cell in the data validation range.
			/// </summary>
			 virtual void SetShowInput(bool value)=0;
			/// <summary>
			/// Indicates whether the data validation error message will be displayed whenever the user enters invalid data. 
			/// </summary>
			 virtual bool GetShowError()=0;
			/// <summary>
			/// Indicates whether the data validation error message will be displayed whenever the user enters invalid data. 
			/// </summary>
			 virtual void SetShowError(bool value)=0;
			/// <summary>
			/// Indicates whether blank values are permitted by the range data validation.
			/// </summary>
			 virtual bool GetIgnoreBlank()=0;
			/// <summary>
			/// Indicates whether blank values are permitted by the range data validation.
			/// </summary>
			 virtual void SetIgnoreBlank(bool value)=0;
			/// <summary>
			/// Gets the value or expression associated with this validation.
			/// </summary>
			/// <param name="isR1C1" >Whether the formula needs to be formatted as R1C1.</param>
			/// <param name="isLocal" >Whether the formula needs to be formatted by locale.</param>
			/// <returns>The value or expression associated with this validation.</returns>
			 virtual intrusive_ptr<Aspose::Cells::Systems::String> GetFormula1(bool isR1C1 , bool isLocal)=0;
			/// <summary>
			/// Gets the value or expression associated with this validation.
			/// </summary>
			/// <param name="isR1C1" >Whether the formula needs to be formatted as R1C1.</param>
			/// <param name="isLocal" >Whether the formula needs to be formatted by locale.</param>
			/// <returns>The value or expression associated with this validation.</returns>
			 virtual intrusive_ptr<Aspose::Cells::Systems::String> GetFormula2(bool isR1C1 , bool isLocal)=0;
			/// <summary>
			/// Gets the value or expression associated with this validation for specific cell.
			/// </summary>
			/// <param name="isR1C1" >Whether the formula needs to be formatted as R1C1.</param>
			/// <param name="isLocal" >Whether the formula needs to be formatted by locale.</param>
			/// <param name="row" >The row index.</param>
			/// <param name="column" >The column index.</param>
			/// <returns>The value or expression associated with this validation.</returns>
			 virtual intrusive_ptr<Aspose::Cells::Systems::String> GetFormula1(bool isR1C1 , bool isLocal , Aspose::Cells::Systems::Int32 row , Aspose::Cells::Systems::Int32 column)=0;
			/// <summary>
			/// Gets the value or expression associated with this validation for specific cell.
			/// </summary>
			/// <param name="isR1C1" >Whether the formula needs to be formatted as R1C1.</param>
			/// <param name="isLocal" >Whether the formula needs to be formatted by locale.</param>
			/// <param name="row" >The row index.</param>
			/// <param name="column" >The column index.</param>
			/// <returns>The value or expression associated with this validation.</returns>
			 virtual intrusive_ptr<Aspose::Cells::Systems::String> GetFormula2(bool isR1C1 , bool isLocal , Aspose::Cells::Systems::Int32 row , Aspose::Cells::Systems::Int32 column)=0;
			/// <summary>
			/// Sets the value or expression associated with this validation.
			/// </summary>
			/// <param name="formula" >The value or expression associated with this format condition.</param>
			/// <param name="isR1C1" >Whether the formula is R1C1 formula.</param>
			/// <param name="isLocal" >Whether the formula is locale formatted.</param>
			 virtual void SetFormula1(intrusive_ptr<Aspose::Cells::Systems::String> formula , bool isR1C1 , bool isLocal)=0;
			/// <summary>
			/// Sets the value or expression associated with this validation.
			/// </summary>
			/// <param name="formula" >The value or expression associated with this format condition.</param>
			/// <param name="isR1C1" >Whether the formula is R1C1 formula.</param>
			/// <param name="isLocal" >Whether the formula is locale formatted.</param>
			 virtual void SetFormula2(intrusive_ptr<Aspose::Cells::Systems::String> formula , bool isR1C1 , bool isLocal)=0;
			/// <summary>
			/// Represents the value or expression associated with the data validation.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Systems::String> GetFormula1()=0;
			/// <summary>
			/// Represents the value or expression associated with the data validation.
			/// </summary>
			 virtual void SetFormula1(intrusive_ptr<Aspose::Cells::Systems::String> value)=0;
			/// <summary>
			/// Represents the value or expression associated with the data validation.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Systems::String> GetFormula2()=0;
			/// <summary>
			/// Represents the value or expression associated with the data validation.
			/// </summary>
			 virtual void SetFormula2(intrusive_ptr<Aspose::Cells::Systems::String> value)=0;
			/// <summary>
			/// Get the value for list of the validation for the specified cell.
			/// </summary>
			/// <param name="row" >The row index.</param>
			/// <param name="column" >The column index.</param>
			/// <returns>The value to produce the list of this validation for the specified cell.
			/// If the list references to a range, then the returned value will be a <see cref="IReferredArea" />
			///  object;
			/// Otherwise the returned value may be null, object[], or simple object.</returns>
			/// <remarks>Only for validation whose type is List and has been applied to given cell,
			/// otherwise null will be returned.</remarks>
			 virtual intrusive_ptr<Aspose::Cells::Systems::Object> GetListValue(Aspose::Cells::Systems::Int32 row , Aspose::Cells::Systems::Int32 column)=0;
			/// <summary>
			/// Represents the first value associated with the data validation.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Systems::Object> GetValue1()=0;
			/// <summary>
			/// Represents the first value associated with the data validation.
			/// </summary>
			 virtual void SetValue1(intrusive_ptr<Aspose::Cells::Systems::Object> value)=0;
			/// <summary>
			/// Represents the second value associated with the data validation.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Systems::Object> GetValue2()=0;
			/// <summary>
			/// Represents the second value associated with the data validation.
			/// </summary>
			 virtual void SetValue2(intrusive_ptr<Aspose::Cells::Systems::Object> value)=0;
			/// <summary>
			/// Indicates whether data validation displays a drop-down list that contains acceptable values.
			/// </summary>
			 virtual bool GetInCellDropDown()=0;
			/// <summary>
			/// Indicates whether data validation displays a drop-down list that contains acceptable values.
			/// </summary>
			 virtual void SetInCellDropDown(bool value)=0;
			/// <summary>
			/// Gets all <see cref="ICellArea" />
			///  which contain the data validation settings.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Systems::Array1D<Aspose::Cells::ICellArea*>> 		GetICellAreas()=0;
			/// <summary>
			/// Applies the validation to the area.
			/// </summary>
			/// <param name="cellArea" >The area.</param>
			/// <remarks>
			/// It is equivalent to use <see cref="AddArea(Aspose.Cells.CellArea,bool,bool)" />
			/// 
			/// with checking intersection and edge.
			/// </remarks>
			 virtual void 		AddArea(intrusive_ptr<Aspose::Cells::ICellArea> cellArea)=0;
			/// <summary>
			/// Applies the validation to the area.
			/// </summary>
			/// <param name="cellArea" >The area.</param>
			/// <param name="checkIntersection" >Whether check the intersection of given area with existing validations' areas.
			/// If one validation has been applied in given area(or part of it),
			/// then the existing validation should be removed at first from given area.
			/// Otherwise corruption may be caused for the generated Validations.
			/// If user is sure that the added area does not intersect with any existing area,
			/// this parameter can be set as false for performance consideration.</param>
			/// <param name="checkEdge" >Whether check the edge of this validation's applied areas.
			/// Validation's internal settings depend on the top-left one of its applied ranges,
			/// so if given area will become the new top-left one of the applied ranges,
			/// the internal settings should be changed and rebuilt, otherwise unexpected result may be caused.
			/// If user is sure that the added area is not the top-left one,
			/// this parameter can be set as false for performance consideration.</param>
			/// <remarks>
			/// In this method, we will remove all old validations in given area.
			/// For the top-left one of Validation's applied ranges, firstly its StartRow is smallest,
			/// secondly its StartColumn is the smallest one of those areas who have the same smallest StartRow.
			/// </remarks>
			 virtual void 		AddArea(intrusive_ptr<Aspose::Cells::ICellArea> cellArea , bool checkIntersection , bool checkEdge)=0;
			/// <summary>
			/// Applies the validation to given areas.
			/// </summary>
			/// <param name="areas" >The areas.</param>
			/// <param name="checkIntersection" >Whether check the intersection of given area with existing validations' areas.
			/// If one validation has been applied in given area(or part of it),
			/// then the existing validation should be removed at first from given area.
			/// Otherwise corruption may be caused for the generated Validations.
			/// If user is sure that all the added areas do not intersect with any existing area,
			/// this parameter can be set as false for performance consideration.</param>
			/// <param name="checkEdge" >Whether check the edge of this validation's applied areas.
			/// Validation's internal settings depend on the top-left one of its applied ranges,
			/// so if one of given areas will become the new top-left one of the applied ranges,
			/// the internal settings should be changed and rebuilt, otherwise unexpected result may be caused.
			/// If user is sure that no one of those added areas is the top-left,
			/// this parameter can be set as false for performance consideration.</param>
			/// <remarks>
			/// In this method, we will remove all old validations in given area.
			/// For the top-left one of Validation's applied ranges, firstly its StartRow is smallest,
			/// secondly its StartColumn is the smallest one of those areas who have the same smallest StartRow.
			/// </remarks>
			 virtual void 		AddAreas(intrusive_ptr<Aspose::Cells::Systems::Array1D<Aspose::Cells::ICellArea*>> areas , bool checkIntersection , bool checkEdge)=0;
			/// <summary>
			/// Remove the validation settings in the range.
			/// </summary>
			/// <param name="cellArea" >the areas where this validation settings should be removed.</param>
			 virtual void 		RemoveArea(intrusive_ptr<Aspose::Cells::ICellArea> cellArea)=0;
			/// <summary>
			/// Removes this validation from given areas.
			/// </summary>
			/// <param name="areas" >the areas where this validation settings should be removed.</param>
			 virtual void 		RemoveAreas(intrusive_ptr<Aspose::Cells::Systems::Array1D<Aspose::Cells::ICellArea*>> areas)=0;
			/// <summary>
			/// Remove the validation settings in the cell.
			/// </summary>
			/// <param name="row" >The row index.</param>
			/// <param name="column" > The column index.</param>
			 virtual void RemoveACell(Aspose::Cells::Systems::Int32 row , Aspose::Cells::Systems::Int32 column)=0;
			/// <summary>
			/// Copy validation.
			/// </summary>
			/// <param name="source" >The source validation.</param>
			/// <param name="copyOption" >The copy option.</param>
			 virtual void 		Copy(intrusive_ptr<Aspose::Cells::IValidation> source , intrusive_ptr<Aspose::Cells::ICopyOptions> copyOption)=0;

	};
}
}
