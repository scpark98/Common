#pragma once
#include "Aspose.Cells.Systems/Object.h"
#include "Aspose.Cells.Systems/String.h"
#include "Aspose.Cells.Systems/Int32.h"
#include "Aspose.Cells.Systems/Double.h"
#include "ShapeLockType.h"
#include "MsoDrawingType.h"
#include "AutoShapeType.h"
#include "ShapeAnchorType.h"
#include "PlacementType.h"
#include "Aspose.Cells.Systems/Array2D.h"
#include "Aspose.Cells.Systems/Single.h"
#include "Aspose.Cells.Systems/IO/Stream.h"
#include "Aspose.Cells.Systems/Drawing/Imaging/ImageFormat.h"
#include "ImageType.h"
#include "Aspose.Cells.Systems/Drawing/Bitmap.h"
#include "Aspose.Cells.Systems/Array1D.h"
#include "Aspose.Cells.Systems/Collections/ArrayList.h"
#include "TextOverflowType.h"
#include "TextOrientationType.h"
#include "TextAlignmentType.h"
#include "TextDirectionType.h"
#include "Aspose.Cells.Systems/Byte.h"
#include "Aspose.Cells.Systems/Guid.h"

namespace Aspose {
	namespace Cells {
		namespace Drawing {
			class IGroupShape;
			class IMsoLineFormat;
			class IMsoFillFormat;
			class ILineFormat;
			class IFillFormat;
			class IShadowEffect;
			class IMsoTextFrame;
			class IMsoFormatPicture;
			enum ShapeLockType;
			class ITextEffectFormat;
			enum MsoDrawingType;
			enum AutoShapeType;
			enum ShapeAnchorType;
			enum PlacementType;
			enum ImageType;
			enum TextOverflowType;
			class IShapePathCollection;
		}
	}
}
namespace Aspose {
	namespace Cells {
		class IHyperlink;
		class IWorksheet;
		class IFont;
		class IStyleFlag;
		class IFontSetting;
		enum TextOrientationType;
		enum TextAlignmentType;
		enum TextDirectionType;
	}
}
namespace Aspose {
	namespace Cells {
		namespace Rendering {
			class IImageOrPrintOptions;
		}
	}
}
namespace Aspose{
namespace Cells{
namespace Drawing{
			/// <summary>
			/// Represents the msodrawing object.
			/// </summary>
	class ASPOSE_CELLS_API IShape : public Aspose::Cells::Systems::Object
	{
		public:
			/// <summary>
			/// Gets the name of macro.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Systems::String> GetMacroName()=0;
			/// <summary>
			/// Sets the name of macro.
			/// </summary>
			 virtual void SetMacroName(intrusive_ptr<Aspose::Cells::Systems::String> value)=0;
			/// <summary>
			/// Indicates whether the shape only contains an equation.
			/// </summary>
			 virtual bool IsEquation()=0;
			/// <summary>
			/// Indicates whether the shape is smart art.
			/// </summary>
			/// <remarks>
			/// Only for ooxml file.
			/// </remarks>
			 virtual bool IsSmartArt()=0;
			/// <summary>
			/// Converting smart art to grouped shapes.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Drawing::IGroupShape> 		GetIGroupShapeResult()=0;
			/// <summary>
			/// Brings the shape to the front or sends the shape to back.
			/// </summary>
			/// <param name="orders" >
			/// If it's less than zero, sets the shape to back.
			/// If it's greater than zero, brings the shape to front.
			/// </param>
			 virtual void ToFrontOrBack(Aspose::Cells::Systems::Int32 orders)=0;
			/// <summary>
			/// Returns the position of a shape in the z-order. 
			/// </summary>
			 virtual Aspose::Cells::Systems::Int32 GetZOrderPosition()=0;
			/// <summary>
			/// Returns the position of a shape in the z-order. 
			/// </summary>
			 virtual void SetZOrderPosition(Aspose::Cells::Systems::Int32 value)=0;
			/// <summary>
			/// Gets the name of the shape. 
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Systems::String> GetName()=0;
			/// <summary>
			/// Sets the name of the shape. 
			/// </summary>
			 virtual void SetName(intrusive_ptr<Aspose::Cells::Systems::String> value)=0;
			/// <summary>
			/// Returns or sets the descriptive (alternative) text string of the <see cref="IShape" />
			///  object.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Systems::String> GetAlternativeText()=0;
			/// <summary>
			/// Returns or sets the descriptive (alternative) text string of the <see cref="IShape" />
			///  object.
			/// </summary>
			 virtual void SetAlternativeText(intrusive_ptr<Aspose::Cells::Systems::String> value)=0;
			/// <summary>
			/// Specifies the title (caption) of the current shape object. 
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Systems::String> GetTitle()=0;
			/// <summary>
			/// Specifies the title (caption) of the current shape object. 
			/// </summary>
			 virtual void SetTitle(intrusive_ptr<Aspose::Cells::Systems::String> value)=0;
			/// <summary>
			/// Returns a MsoLineFormat object that contains line formatting properties for the specified shape.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Drawing::IMsoLineFormat> 		GetIMsoLineFormat()=0;
			/// <summary>
			/// Returns a MsoFillFormat object that contains fill formatting properties for the specified shape.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Drawing::IMsoFillFormat> 		GetIMsoFillFormat()=0;
			/// <summary>
			/// Gets line style
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Drawing::ILineFormat> 		GetILineFormat()=0;
			/// <summary>
			/// Returns a <see cref="FillFormat" />
			///  object that contains fill formatting properties for the specified shape.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Drawing::IFillFormat> 		GetIFillFormat()=0;
			/// <summary>
			/// Represents a <seealso cref="IShadowEffect" />
			///  object that specifies shadow effect for the chart element or shape.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Drawing::IShadowEffect> 		GetIShadowEffect()=0;
			/// <summary>
			/// Gets the radius of blur to apply to the edges, in unit of points.
			/// </summary>
			 virtual Aspose::Cells::Systems::Double GetSoftEdges()=0;
			/// <summary>
			/// Sets the radius of blur to apply to the edges, in unit of points.
			/// </summary>
			 virtual void SetSoftEdges(Aspose::Cells::Systems::Double value)=0;
			/// <summary>
			/// Returns a TextFrame object that contains the alignment and anchoring properties for the specified shape.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Drawing::IMsoTextFrame> 		GetIMsoTextFrame()=0;
			/// <summary>
			/// Gets the options of the picture format.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Drawing::IMsoFormatPicture> 		GetIMsoFormatPicture()=0;
			/// <summary>
			/// Indicates whether the object is visible.
			/// </summary>
			 virtual bool IsHidden()=0;
			/// <summary>
			/// Indicates whether the object is visible.
			/// </summary>
			 virtual void SetHidden(bool value)=0;
			/// <summary>
			/// True means that don't allow changes in aspect ratio.
			/// </summary>
			 virtual bool IsLockAspectRatio()=0;
			/// <summary>
			/// True means that don't allow changes in aspect ratio.
			/// </summary>
			 virtual void SetLockAspectRatio(bool value)=0;
			/// <summary>
			/// Gets the value of locked property.
			/// </summary>
			/// <param name="type" >The type of the shape locked property.</param>
			/// <returns>Returns  the value of locked property.</returns>
			 virtual bool GetLockedProperty(Aspose::Cells::Drawing::ShapeLockType type)=0;
			/// <summary>
			/// Set the locked property.
			/// </summary>
			/// <param name="type" >The locked type.</param>
			/// <param name="value" >The value of the property.</param>
			 virtual void SetLockedProperty(Aspose::Cells::Drawing::ShapeLockType type , bool value)=0;
			/// <summary>
			/// Gets the rotation of the shape.
			/// </summary>
			 virtual Aspose::Cells::Systems::Double GetRotationAngle()=0;
			/// <summary>
			/// Sets the rotation of the shape.
			/// </summary>
			 virtual void SetRotationAngle(Aspose::Cells::Systems::Double value)=0;
			/// <summary>
			/// Adds a hyperlink to the shape.
			/// </summary>
			/// <param name="address" >Address of the hyperlink.</param>
			/// <returns>Return the new hyperlink object.</returns>
			 virtual intrusive_ptr<Aspose::Cells::IHyperlink> 		AddIHyperlink(intrusive_ptr<Aspose::Cells::Systems::String> address)=0;
			/// <summary>
			/// Remove the hyperlink of the shape.
			/// </summary>
			 virtual void RemoveHyperlink()=0;
			/// <summary>
			/// Gets the hyperlink of the shape.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::IHyperlink> 		GetIHyperlink()=0;
			/// <summary>
			/// Moves the shape to a specified range.
			/// </summary>
			/// <param name="upperLeftRow" >Upper left row index.</param>
			/// <param name="upperLeftColumn" >Upper left column index.</param>
			/// <param name="lowerRightRow" >Lower right row index</param>
			/// <param name="lowerRightColumn" >Lower right column index</param>
			 virtual void MoveToRange(Aspose::Cells::Systems::Int32 upperLeftRow , Aspose::Cells::Systems::Int32 upperLeftColumn , Aspose::Cells::Systems::Int32 lowerRightRow , Aspose::Cells::Systems::Int32 lowerRightColumn)=0;
			/// <summary>
			/// Moves the picture to the top-right corner.
			/// </summary>
			/// <param name="topRow" >the row index.</param>
			/// <param name="rightColumn" >the column index.</param>
			 virtual void AlignTopRightCorner(Aspose::Cells::Systems::Int32 topRow , Aspose::Cells::Systems::Int32 rightColumn)=0;
			/// <summary>
			/// Gets the identifier of this shape.
			/// </summary>
			 virtual Aspose::Cells::Systems::Int32 GetId()=0;
			/// <summary>
			/// Specifies an optional string that an application can use to Identify the particular shape. 
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Systems::String> GetSpid()=0;
			/// <summary>
			/// Specifies an optional number that an application can use to associate the particular shape with a defined shape type.  
			/// </summary>
			 virtual Aspose::Cells::Systems::Int32 GetSpt()=0;
			/// <summary>
			/// Gets the <see cref="IWorksheet" />
			///  object which contains this shape.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::IWorksheet> 		GetIWorksheet()=0;
			/// <summary>
			/// Indicates whether the shape is a group.
			/// </summary>
			 virtual bool IsGroup()=0;
			/// <summary>
			/// Indicates whether the shape is grouped.
			/// </summary>
			 virtual bool IsInGroup()=0;
			/// <summary>
			/// Indicates whether this shape is a word art.
			/// </summary>
			/// <remarks>
			/// Only for the Legacy Shape of xls file.
			/// </remarks>
			 virtual bool IsWordArt()=0;
			/// <summary>
			/// Returns a TextEffectFormat object that contains text-effect formatting properties for the specified shape. 
			/// Applies to Shape objects that represent WordArt.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Drawing::ITextEffectFormat> 		GetITextEffectFormat()=0;
			/// <summary>
			/// True if the object is locked, False if the object can be modified when the sheet is protected. 
			/// </summary>
			 virtual bool IsLocked()=0;
			/// <summary>
			/// True if the object is locked, False if the object can be modified when the sheet is protected. 
			/// </summary>
			 virtual void SetLocked(bool value)=0;
			/// <summary>
			/// True if the object is printable
			/// </summary>
			 virtual bool IsPrintable()=0;
			/// <summary>
			/// True if the object is printable
			/// </summary>
			 virtual void SetPrintable(bool value)=0;
			/// <summary>
			/// Gets mso drawing type.
			/// </summary>
			 virtual Aspose::Cells::Drawing::MsoDrawingType GetMsoDrawingType()=0;
			/// <summary>
			/// Gets the auto shape type.
			/// </summary>
			 virtual Aspose::Cells::Drawing::AutoShapeType GetAutoShapeType()=0;
			/// <summary>
			/// Gets the auto shape type.
			/// </summary>
			 virtual void SetAutoShapeType(Aspose::Cells::Drawing::AutoShapeType value)=0;
			/// <summary>
			/// Gets the shape anchor placeholder.
			/// </summary>
			 virtual Aspose::Cells::Drawing::ShapeAnchorType GetAnchorType()=0;
			/// <summary>
			/// Sets the shape anchor placeholder.
			/// </summary>
			 virtual void SetAnchorType(Aspose::Cells::Drawing::ShapeAnchorType value)=0;
			/// <summary>
			/// Represents the way the drawing object is attached to the cells below it.
			/// The property controls the placement of an object on a worksheet.
			/// </summary>
			 virtual Aspose::Cells::Drawing::PlacementType GetPlacement()=0;
			/// <summary>
			/// Represents the way the drawing object is attached to the cells below it.
			/// The property controls the placement of an object on a worksheet.
			/// </summary>
			 virtual void SetPlacement(Aspose::Cells::Drawing::PlacementType value)=0;
			/// <summary>
			/// Represents upper left corner row index. 
			/// </summary>
			/// <remarks>If the shape is in the shape or in the group , UpperLeftRow will be ignored.</remarks>
			 virtual Aspose::Cells::Systems::Int32 GetUpperLeftRow()=0;
			/// <summary>
			/// Represents upper left corner row index. 
			/// </summary>
			/// <remarks>If the shape is in the shape or in the group , UpperLeftRow will be ignored.</remarks>
			 virtual void SetUpperLeftRow(Aspose::Cells::Systems::Int32 value)=0;
			/// <summary>
			/// Gets the shape's vertical offset from its upper left corner row.
			/// <remarks>The range of value is 0 to 256.</remarks>
			/// </summary>
			 virtual Aspose::Cells::Systems::Int32 GetUpperDeltaY()=0;
			/// <summary>
			/// Sets the shape's vertical offset from its upper left corner row.
			/// <remarks>The range of value is 0 to 256.</remarks>
			/// </summary>
			 virtual void SetUpperDeltaY(Aspose::Cells::Systems::Int32 value)=0;
			/// <summary>
			/// Represents upper left corner column index.
			/// </summary>
			 virtual Aspose::Cells::Systems::Int32 GetUpperLeftColumn()=0;
			/// <summary>
			/// Represents upper left corner column index.
			/// </summary>
			 virtual void SetUpperLeftColumn(Aspose::Cells::Systems::Int32 value)=0;
			/// <summary>
			/// Gets the shape's horizontal offset from its upper left corner column.
			/// <remarks>The range of value is 0 to 1024.</remarks>
			/// </summary>
			 virtual Aspose::Cells::Systems::Int32 GetUpperDeltaX()=0;
			/// <summary>
			/// Sets the shape's horizontal offset from its upper left corner column.
			/// <remarks>The range of value is 0 to 1024.</remarks>
			/// </summary>
			 virtual void SetUpperDeltaX(Aspose::Cells::Systems::Int32 value)=0;
			/// <summary>
			/// Represents lower right corner row index.
			/// </summary>
			 virtual Aspose::Cells::Systems::Int32 GetLowerRightRow()=0;
			/// <summary>
			/// Represents lower right corner row index.
			/// </summary>
			 virtual void SetLowerRightRow(Aspose::Cells::Systems::Int32 value)=0;
			/// <summary>
			/// Gets the shape's vertical offset from its lower right corner row.
			/// <remarks>The range of value is 0 to 256.</remarks>
			/// </summary>
			 virtual Aspose::Cells::Systems::Int32 GetLowerDeltaY()=0;
			/// <summary>
			/// Sets the shape's vertical offset from its lower right corner row.
			/// <remarks>The range of value is 0 to 256.</remarks>
			/// </summary>
			 virtual void SetLowerDeltaY(Aspose::Cells::Systems::Int32 value)=0;
			/// <summary>
			/// Represents lower right corner column index.
			/// </summary>
			 virtual Aspose::Cells::Systems::Int32 GetLowerRightColumn()=0;
			/// <summary>
			/// Represents lower right corner column index.
			/// </summary>
			 virtual void SetLowerRightColumn(Aspose::Cells::Systems::Int32 value)=0;
			/// <summary>
			/// Gets the shape's horizontal  offset from its lower right corner column.
			/// <remarks>The range of value is 0 to 1024.</remarks>
			/// </summary>
			 virtual Aspose::Cells::Systems::Int32 GetLowerDeltaX()=0;
			/// <summary>
			/// Gets the shape's horizontal  offset from its lower right corner column.
			/// <remarks>The range of value is 0 to 1024.</remarks>
			/// </summary>
			 virtual void SetLowerDeltaX(Aspose::Cells::Systems::Int32 value)=0;
			/// <summary>
			/// Represents the width of the shape's horizontal  offset from its lower right corner column, in unit of pixels.
			/// </summary>
			 virtual Aspose::Cells::Systems::Int32 GetRight()=0;
			/// <summary>
			/// Represents the width of the shape's horizontal  offset from its lower right corner column, in unit of pixels.
			/// </summary>
			 virtual void SetRight(Aspose::Cells::Systems::Int32 value)=0;
			/// <summary>
			/// Represents the width of the shape's vertical offset from its lower bottom corner row, in unit of pixels.
			/// </summary>
			 virtual Aspose::Cells::Systems::Int32 GetBottom()=0;
			/// <summary>
			/// Represents the width of the shape's vertical offset from its lower bottom corner row, in unit of pixels.
			/// </summary>
			 virtual void SetBottom(Aspose::Cells::Systems::Int32 value)=0;
			/// <summary>
			/// Represents the width of shape, in unit of pixels.
			/// </summary>
			 virtual Aspose::Cells::Systems::Int32 GetWidth()=0;
			/// <summary>
			/// Represents the width of shape, in unit of pixels.
			/// </summary>
			 virtual void SetWidth(Aspose::Cells::Systems::Int32 value)=0;
			/// <summary>
			///  Represents the width of the shape, in unit of inch. 
			/// </summary>
			 virtual Aspose::Cells::Systems::Double GetWidthInch()=0;
			/// <summary>
			///  Represents the width of the shape, in unit of inch. 
			/// </summary>
			 virtual void SetWidthInch(Aspose::Cells::Systems::Double value)=0;
			/// <summary>
			///  Represents the width of the shape, in unit of point. 
			/// </summary>
			 virtual Aspose::Cells::Systems::Double GetWidthPt()=0;
			/// <summary>
			///  Represents the width of the shape, in unit of point. 
			/// </summary>
			 virtual void SetWidthPt(Aspose::Cells::Systems::Double value)=0;
			/// <summary>
			/// Represents the width of the shape, in unit of centimeters. 
			/// </summary>
			 virtual Aspose::Cells::Systems::Double GetWidthCM()=0;
			/// <summary>
			/// Represents the width of the shape, in unit of centimeters. 
			/// </summary>
			 virtual void SetWidthCM(Aspose::Cells::Systems::Double value)=0;
			/// <summary>
			/// Represents the height of shape, in unit of pixel.
			/// </summary>
			 virtual Aspose::Cells::Systems::Int32 GetHeight()=0;
			/// <summary>
			/// Represents the height of shape, in unit of pixel.
			/// </summary>
			 virtual void SetHeight(Aspose::Cells::Systems::Int32 value)=0;
			/// <summary>
			/// Represents the height of the shape, in unit of inches. 
			/// </summary>
			 virtual Aspose::Cells::Systems::Double GetHeightInch()=0;
			/// <summary>
			/// Represents the height of the shape, in unit of inches. 
			/// </summary>
			 virtual void SetHeightInch(Aspose::Cells::Systems::Double value)=0;
			/// <summary>
			/// Represents the height of the shape, in unit of points. 
			/// </summary>
			 virtual Aspose::Cells::Systems::Double GetHeightPt()=0;
			/// <summary>
			/// Represents the height of the shape, in unit of points. 
			/// </summary>
			 virtual void SetHeightPt(Aspose::Cells::Systems::Double value)=0;
			/// <summary>
			/// Represents the height of the shape, in unit of inches. 
			/// </summary>
			 virtual Aspose::Cells::Systems::Double GetHeightCM()=0;
			/// <summary>
			/// Represents the height of the shape, in unit of inches. 
			/// </summary>
			 virtual void SetHeightCM(Aspose::Cells::Systems::Double value)=0;
			/// <summary>
			/// Represents the horizontal offset of shape from its left column, in unit of pixels.
			/// </summary>
			 virtual Aspose::Cells::Systems::Int32 GetLeft()=0;
			/// <summary>
			/// Represents the horizontal offset of shape from its left column, in unit of pixels.
			/// </summary>
			 virtual void SetLeft(Aspose::Cells::Systems::Int32 value)=0;
			/// <summary>
			/// Represents the horizontal offset of shape from its left column, in unit of inches.
			/// </summary>
			 virtual Aspose::Cells::Systems::Double GetLeftInch()=0;
			/// <summary>
			/// Represents the horizontal offset of shape from its left column, in unit of inches.
			/// </summary>
			 virtual void SetLeftInch(Aspose::Cells::Systems::Double value)=0;
			/// <summary>
			/// Represents the horizontal offset of shape from its left column, in unit of centimeters.
			/// </summary>
			 virtual Aspose::Cells::Systems::Double GetLeftCM()=0;
			/// <summary>
			/// Represents the horizontal offset of shape from its left column, in unit of centimeters.
			/// </summary>
			 virtual void SetLeftCM(Aspose::Cells::Systems::Double value)=0;
			/// <summary>
			/// Represents the vertical offset of shape from its top row, in unit of pixels.
			/// </summary>
			/// <remarks>If the shape is in the chart, represents the vertical offset of shape from its top border.</remarks>
			 virtual Aspose::Cells::Systems::Int32 GetTop()=0;
			/// <summary>
			/// Represents the vertical offset of shape from its top row, in unit of pixels.
			/// </summary>
			/// <remarks>If the shape is in the chart, represents the vertical offset of shape from its top border.</remarks>
			 virtual void SetTop(Aspose::Cells::Systems::Int32 value)=0;
			/// <summary>
			///  Represents the vertical offset of shape from its top row, in unit of inches.
			/// </summary>
			 virtual Aspose::Cells::Systems::Double GetTopInch()=0;
			/// <summary>
			///  Represents the vertical offset of shape from its top row, in unit of inches.
			/// </summary>
			 virtual void SetTopInch(Aspose::Cells::Systems::Double value)=0;
			/// <summary>
			///  Represents the vertical offset of shape from its top row, in unit of centimeters.
			/// </summary>
			 virtual Aspose::Cells::Systems::Double GetTopCM()=0;
			/// <summary>
			///  Represents the vertical offset of shape from its top row, in unit of centimeters.
			/// </summary>
			 virtual void SetTopCM(Aspose::Cells::Systems::Double value)=0;
			/// <summary>
			/// Gets and sets the vertical offset of shape from worksheet top boder,in unit of pixel
			/// </summary>
			 virtual Aspose::Cells::Systems::Int32 GetTopToCorner()=0;
			/// <summary>
			/// Gets and sets the vertical offset of shape from worksheet top boder,in unit of pixel
			/// </summary>
			 virtual void SetTopToCorner(Aspose::Cells::Systems::Int32 value)=0;
			/// <summary>
			/// Gets the horizonal offset of shape from worksheet left border.
			/// </summary>
			 virtual Aspose::Cells::Systems::Int32 GetLeftToCorner()=0;
			/// <summary>
			/// Sets the horizonal offset of shape from worksheet left border.
			/// </summary>
			 virtual void SetLeftToCorner(Aspose::Cells::Systems::Int32 value)=0;
			/// <summary>
			/// Gets the horizontal offset of shape from worksheet left border,in unit of pixels.
			/// </summary>
			 virtual Aspose::Cells::Systems::Int32 GetX()=0;
			/// <summary>
			/// Sets the horizontal offset of shape from worksheet left border,in unit of pixels.
			/// </summary>
			 virtual void SetX(Aspose::Cells::Systems::Int32 value)=0;
			/// <summary>
			/// Gets the vertical offset of shape from worksheet top border,in unit of pixels.
			/// </summary>
			 virtual Aspose::Cells::Systems::Int32 GetY()=0;
			/// <summary>
			/// Sets the vertical offset of shape from worksheet top border,in unit of pixels.
			/// </summary>
			 virtual void SetY(Aspose::Cells::Systems::Int32 value)=0;
			/// <summary>
			/// Gets the width scale, in unit of percent of the original picture width.
			/// If the shape is not picture ,the WidthScale property only returns 100;
			/// </summary>
			 virtual Aspose::Cells::Systems::Int32 GetWidthScale()=0;
			/// <summary>
			/// Sets the width scale, in unit of percent of the original picture width.
			/// If the shape is not picture ,the WidthScale property only returns 100;
			/// </summary>
			 virtual void SetWidthScale(Aspose::Cells::Systems::Int32 value)=0;
			/// <summary>
			///  Gets the height scale,in unit of percent of the original picture height.
			/// If the shape is not picture ,the HeightScale property only returns 100;
			/// </summary>
			 virtual Aspose::Cells::Systems::Int32 GetHeightScale()=0;
			/// <summary>
			/// Sets the height scale,in unit of percent of the original picture height.
			/// If the shape is not picture ,the HeightScale property only returns 100;
			/// </summary>
			 virtual void SetHeightScale(Aspose::Cells::Systems::Int32 value)=0;
			/// <summary>
			/// Represents the vertical offset of shape from the top border of the parent shape, 
			/// in unit of 1/4000 of height of the parent shape.
			/// </summary>
			/// <remarks>Only Applies when this shape in the group or chart.</remarks>
			 virtual Aspose::Cells::Systems::Int32 GetTopInShape()=0;
			/// <summary>
			/// Represents the vertical offset of shape from the top border of the parent shape, 
			/// in unit of 1/4000 of height of the parent shape.
			/// </summary>
			/// <remarks>Only Applies when this shape in the group or chart.</remarks>
			 virtual void SetTopInShape(Aspose::Cells::Systems::Int32 value)=0;
			/// <summary>
			/// Represents the horizontal offset of shape from the left border of the parent shape, 
			/// in unit of 1/4000 of width of the parent shape.
			/// </summary>
			/// <remarks>Only Applies when this shape in the group or chart.</remarks>
			 virtual Aspose::Cells::Systems::Int32 GetLeftInShape()=0;
			/// <summary>
			/// Represents the horizontal offset of shape from the left border of the parent shape, 
			/// in unit of 1/4000 of width of the parent shape.
			/// </summary>
			/// <remarks>Only Applies when this shape in the group or chart.</remarks>
			 virtual void SetLeftInShape(Aspose::Cells::Systems::Int32 value)=0;
			/// <summary>
			/// Represents the width of the shape, in unit of 1/4000 of the parent shape.
			/// </summary>
			/// <remarks>Only Applies when this shape in the group or chart.</remarks>
			 virtual Aspose::Cells::Systems::Int32 GetWidthInShape()=0;
			/// <summary>
			/// Represents the width of the shape, in unit of 1/4000 of the parent shape.
			/// </summary>
			/// <remarks>Only Applies when this shape in the group or chart.</remarks>
			 virtual void SetWidthInShape(Aspose::Cells::Systems::Int32 value)=0;
			/// <summary>
			/// Represents the vertical offset of shape from the top border of the parent shape, in unit of 1/4000 of height of the parent shape..
			/// </summary>
			/// <remarks>Only Applies when this shape in the group or chart.</remarks>
			 virtual Aspose::Cells::Systems::Int32 GetHeightInShape()=0;
			/// <summary>
			/// Represents the vertical offset of shape from the top border of the parent shape, in unit of 1/4000 of height of the parent shape..
			/// </summary>
			/// <remarks>Only Applies when this shape in the group or chart.</remarks>
			 virtual void SetHeightInShape(Aspose::Cells::Systems::Int32 value)=0;
			/// <summary>
			/// Gets the group shape which contains this shape.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Drawing::IGroupShape> 		GetIGroupShape()=0;
			/// <summary>
			/// Gets the auto shape type.
			/// </summary>
			 virtual Aspose::Cells::Drawing::AutoShapeType GetType()=0;
			/// <summary>
			/// Gets and sets the line border of the shape is visible.
			/// </summary>
			 virtual bool HasLine()=0;
			/// <summary>
			/// Gets and sets the line border of the shape is visible.
			/// </summary>
			 virtual void SetHasLine(bool value)=0;
			/// <summary>
			/// Indicates whether the fill format is visible.
			/// </summary>
			 virtual bool IsFilled()=0;
			/// <summary>
			/// Indicates whether the fill format is visible.
			/// </summary>
			 virtual void SetFilled(bool value)=0;
			/// <summary>
			/// Gets whether shape is horizontally flipped .
			/// </summary>
			 virtual bool IsFlippedHorizontally()=0;
			/// <summary>
			/// Sets whether shape is horizontally flipped .
			/// </summary>
			 virtual void SetFlippedHorizontally(bool value)=0;
			/// <summary>
			/// Gets whether shape is vertically flipped .
			/// </summary>
			 virtual bool IsFlippedVertically()=0;
			/// <summary>
			/// Sets whether shape is vertically flipped .
			/// </summary>
			 virtual void SetFlippedVertically(bool value)=0;
			/// <summary>
			/// Get the actual bottom row.
			/// </summary>
			 virtual Aspose::Cells::Systems::Int32 GetActualLowerRightRow()=0;
			/// <summary>
			/// Get the connection points
			/// </summary>
			/// <returns/>
			/// 
			 virtual intrusive_ptr<Aspose::Cells::Systems::Array2D<Aspose::Cells::Systems::Single>> GetConnectionPoints()=0;
			/// <summary>
			/// Creates the shape image and saves it to a stream in the specified format.
			/// </summary>
			/// <param name="stream" >The output stream.</param>
			/// <param name="imageFormat" >The format in which to save the image.</param>
			/// <remarks>NOTE: This member is now obsolete. Instead, 
			/// please use Shape.ToImage(Stream, ImageType) method. 
			/// This property will be removed 12 months later since July 2022. 
			/// Aspose apologizes for any inconvenience you may have experienced.</remarks>
			 virtual void ToImage(intrusive_ptr<Aspose::Cells::Systems::IO::Stream> stream , intrusive_ptr<Aspose::Cells::Systems::Drawing::Imaging::ImageFormat> imageFormat)=0;
			/// <summary>
			/// Creates the shape image and saves it to a stream in the specified format.
			/// </summary>
			/// <param name="stream" >The output stream.</param>
			/// <param name="imageFormat" >The format in which to save the image.</param>
			/// <remarks><p>The following formats are supported: 
			/// .bmp, .gif, .jpg, .jpeg, .tiff, .emf.</p>
			/// </remarks>
			 virtual void ToImage(intrusive_ptr<Aspose::Cells::Systems::IO::Stream> stream , Aspose::Cells::Drawing::ImageType imageType)=0;
			/// <summary>
			/// Saves the shape to a file.
			/// </summary>
			 virtual void 		ToImage(intrusive_ptr<Aspose::Cells::Systems::String> imageFile , intrusive_ptr<Aspose::Cells::Rendering::IImageOrPrintOptions> options)=0;
			/// <summary>
			/// Saves the shape to a stream.
			/// </summary>
			 virtual void 		ToImage(intrusive_ptr<Aspose::Cells::Systems::IO::Stream> stream , intrusive_ptr<Aspose::Cells::Rendering::IImageOrPrintOptions> options)=0;
			/// <summary>
			/// Returns the bitmap object of the shape .
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Systems::Drawing::Bitmap> 		ToImage(intrusive_ptr<Aspose::Cells::Rendering::IImageOrPrintOptions> options)=0;
			/// <summary>
			/// Indicates whether shape is relative to original picture size.
			/// </summary>
			 virtual bool GetRelativeToOriginalPictureSize()=0;
			/// <summary>
			/// Indicates whether shape is relative to original picture size.
			/// </summary>
			 virtual void SetRelativeToOriginalPictureSize(bool value)=0;
			/// <summary>
			/// Gets the worksheet range linked to the control's value.
			/// </summary>
			  virtual intrusive_ptr<Aspose::Cells::Systems::String> GetLinkedCell()=0;
			/// <summary>
			/// Sets the worksheet range linked to the control's value.
			/// </summary>
			  virtual void SetLinkedCell(intrusive_ptr<Aspose::Cells::Systems::String> value)=0;
			/// <summary>
			/// Gets the worksheet range used to fill the specified combo box. 
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Systems::String> GetInputRange()=0;
			/// <summary>
			/// Sets the worksheet range used to fill the specified combo box. 
			/// </summary>
			 virtual void SetInputRange(intrusive_ptr<Aspose::Cells::Systems::String> value)=0;
			/// <summary>
			/// Gets the range linked to the control's value.
			/// </summary>
			/// <param name="isR1C1" >Whether the formula needs to be formatted as R1C1.</param>
			/// <param name="isLocal" >Whether the formula needs to be formatted by locale.</param>
			/// <returns>The range linked to the control's value.</returns>
			 virtual intrusive_ptr<Aspose::Cells::Systems::String> GetLinkedCell(bool isR1C1 , bool isLocal)=0;
			/// <summary>
			/// Sets the range linked to the control's value.
			/// </summary>
			/// <param name="formula" >The range linked to the control's value.</param>
			/// <param name="isR1C1" >Whether the formula needs to be formatted as R1C1.</param>
			/// <param name="isLocal" >Whether the formula needs to be formatted by locale.</param>
			 virtual void SetLinkedCell(intrusive_ptr<Aspose::Cells::Systems::String> formula , bool isR1C1 , bool isLocal)=0;
			/// <summary>
			/// Gets the range used to fill the control.
			/// </summary>
			/// <param name="isR1C1" >Whether the formula needs to be formatted as R1C1.</param>
			/// <param name="isLocal" >Whether the formula needs to be formatted by locale.</param>
			/// <returns>The range used to fill the control.</returns>
			 virtual intrusive_ptr<Aspose::Cells::Systems::String> GetInputRange(bool isR1C1 , bool isLocal)=0;
			/// <summary>
			/// Sets the range used to fill the control.
			/// </summary>
			/// <param name="formula" >The range used to fill the control.</param>
			/// <param name="isR1C1" >Whether the formula needs to be formatted as R1C1.</param>
			/// <param name="isLocal" >Whether the formula needs to be formatted by locale.</param>
			 virtual void SetInputRange(intrusive_ptr<Aspose::Cells::Systems::String> formula , bool isR1C1 , bool isLocal)=0;
			/// Update the selected value by the value of the linked cell.
			/// </summary>
			 virtual void UpdateSelectedValue()=0;
			/// <summary>
			/// Gets the preset text shape type.
			/// </summary>
			 virtual Aspose::Cells::Drawing::AutoShapeType GetTextShapeType()=0;
			/// <summary>
			/// Sets the preset text shape type.
			/// </summary>
			 virtual void SetTextShapeType(Aspose::Cells::Drawing::AutoShapeType value)=0;
			/// <summary>
			/// Represents the font of shape.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::IFont> 		GetIFont()=0;
			/// <summary>
			/// Represents the font of shape.
			/// </summary>
			 virtual void 		SetIFont(intrusive_ptr<Aspose::Cells::IFont> value)=0;
			/// <summary>
			/// Recalculate the text area
			/// </summary>
			/// <returns>Text's Size in an array(width and height).</returns>
			 virtual intrusive_ptr<Aspose::Cells::Systems::Array1D<Aspose::Cells::Systems::Int32>> CalculateTextSize()=0;
			/// <summary>
			/// Represents the string in this TextBox object.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Systems::String> GetText()=0;
			/// <summary>
			/// Represents the string in this TextBox object.
			/// </summary>
			 virtual void SetText(intrusive_ptr<Aspose::Cells::Systems::String> value)=0;
			/// <summary>
			/// Whether or not the text is rich text.
			/// </summary>
			 virtual bool IsRichText()=0;
			/// <summary>
			/// Gets the html string which contains data and some formats in this textbox.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Systems::String> GetHtmlText()=0;
			/// <summary>
			/// Sets the html string which contains data and some formats in this textbox.
			/// </summary>
			 virtual void SetHtmlText(intrusive_ptr<Aspose::Cells::Systems::String> value)=0;
			/// <summary>
			/// Format some chararcters with the font setting.
			/// </summary>
			/// <param name="startIndex" >The start index.</param>
			/// <param name="length" >The the length.</param>
			/// <param name="font" >The font setting.</param>
			/// <param name="flag" >The flag of the font setting.</param>
			 virtual void 		FormatCharactersByIFont(Aspose::Cells::Systems::Int32 startIndex , Aspose::Cells::Systems::Int32 length , intrusive_ptr<Aspose::Cells::IFont> font , intrusive_ptr<Aspose::Cells::IStyleFlag> flag)=0;
			/// <summary>
			/// Format some chararcters with the font setting.
			/// </summary>
			/// <param name="startIndex" >The start index.</param>
			/// <param name="length" >The the length.</param>
			/// <param name="font" >The font setting.</param>
			 virtual void 		FormatCharactersByIFont(Aspose::Cells::Systems::Int32 startIndex , Aspose::Cells::Systems::Int32 length , intrusive_ptr<Aspose::Cells::IFont> font)=0;
			/// <summary>
			/// Returns a Characters object that represents a range of characters within the text.
			/// </summary>
			/// <param name="startIndex" >The index of the start of the character.</param>
			/// <param name="length" >The number of characters.</param>
			/// <returns>Characters object.</returns>
			/// <remarks>This method only works on shape with title.</remarks>
			 virtual intrusive_ptr<Aspose::Cells::IFontSetting> 		GetCharacters(Aspose::Cells::Systems::Int32 startIndex , Aspose::Cells::Systems::Int32 length)=0;
			/// <summary>
			/// Returns all Characters objects 
			/// that represents a range of characters within the text .
			/// </summary>
			/// <returns>All Characters objects </returns>
			 virtual intrusive_ptr<Aspose::Cells::Systems::Collections::ArrayList> GetCharacters()=0;
			/// <summary>
			/// Gets the text vertical overflow type of the shape which contains text.
			/// </summary>
			 virtual Aspose::Cells::Drawing::TextOverflowType GetTextVerticalOverflow()=0;
			/// <summary>
			/// Sets the text vertical overflow type of the shape which contains text.
			/// </summary>
			 virtual void SetTextVerticalOverflow(Aspose::Cells::Drawing::TextOverflowType value)=0;
			/// <summary>
			/// Gets the text horizontal overflow type of the shape which contains text.
			/// </summary>
			 virtual Aspose::Cells::Drawing::TextOverflowType GetTextHorizontalOverflow()=0;
			/// <summary>
			/// Sets the text horizontal overflow type of the shape which contains text.
			/// </summary>
			 virtual void SetTextHorizontalOverflow(Aspose::Cells::Drawing::TextOverflowType value)=0;
			/// <summary>
			///  Gets the text wrapped type of the shape which contains text.
			/// </summary>
			 virtual bool IsTextWrapped()=0;
			/// <summary>
			///  Sets the text wrapped type of the shape which contains text.
			/// </summary>
			 virtual void SetTextWrapped(bool value)=0;
			/// <summary>
			/// Gets the text orientation type of the shape.
			/// </summary>
			 virtual Aspose::Cells::TextOrientationType GetTextOrientationType()=0;
			/// <summary>
			/// Sets the text orientation type of the shape.
			/// </summary>
			 virtual void SetTextOrientationType(Aspose::Cells::TextOrientationType value)=0;
			/// <summary>
			/// Gets the text horizontal alignment type of the shape.
			/// </summary>
			 virtual Aspose::Cells::TextAlignmentType GetTextHorizontalAlignment()=0;
			/// <summary>
			/// Sets the text horizontal alignment type of the shape.
			/// </summary>
			 virtual void SetTextHorizontalAlignment(Aspose::Cells::TextAlignmentType value)=0;
			/// <summary>
			/// Gets the text vertical alignment type of the shape.
			/// </summary>
			 virtual Aspose::Cells::TextAlignmentType GetTextVerticalAlignment()=0;
			/// <summary>
			/// Sets the text vertical alignment type of the shape.
			/// </summary>
			 virtual void SetTextVerticalAlignment(Aspose::Cells::TextAlignmentType value)=0;
			/// <summary>
			///  Gets the direction of the text flow for this object.
			/// </summary>
			 virtual Aspose::Cells::TextDirectionType GetTextDirection()=0;
			/// <summary>
			///  Sets the direction of the text flow for this object.
			/// </summary>
			 virtual void SetTextDirection(Aspose::Cells::TextDirectionType value)=0;
			/// <summary>
			/// Gets the data of control.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Systems::Array1D<Aspose::Cells::Systems::Byte>> GetControlData()=0;
			/// <summary>
			/// Remove activeX control.
			/// </summary>
			 virtual void RemoveActiveXControl()=0;
			/// <summary>
			/// Gets the paths of a custom geometric shape.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Drawing::IShapePathCollection> 		GetIShapePaths()=0;
			/// <summary>
			/// Returns whether the shape is same.
			/// </summary>
			/// <param name="obj" />
			  virtual bool IsSameSetting(intrusive_ptr<Aspose::Cells::Systems::Object> obj)=0;
			/// <summary>
			/// Gets and sets create id for this shape.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Systems::Guid> GetCreateId()=0;
			/// <summary>
			/// Gets and sets create id for this shape.
			/// </summary>
			 virtual void SetCreateId(intrusive_ptr<Aspose::Cells::Systems::Guid> value)=0;

	};
}
}
}
