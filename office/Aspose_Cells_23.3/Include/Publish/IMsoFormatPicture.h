#pragma once
#include "Aspose.Cells.Systems/Object.h"
#include "Aspose.Cells.Systems/Double.h"
#include "Aspose.Cells.Systems/Int32.h"

namespace Aspose {
	namespace Cells {
		class ICellsColor;
	}
}
namespace Aspose{
namespace Cells{
namespace Drawing{
			/// <summary>
			/// Represents the picture format.
			/// </summary>
	class ASPOSE_CELLS_API IMsoFormatPicture : public Aspose::Cells::Systems::Object
	{
		public:
			/// <summary>
			/// Represents the location of the top of the crop rectangle expressed, expressed as a ratio of the image's height.
			/// </summary>
			 virtual Aspose::Cells::Systems::Double GetTopCrop()=0;
			/// <summary>
			/// Represents the location of the top of the crop rectangle expressed, expressed as a ratio of the image's height.
			/// </summary>
			 virtual void SetTopCrop(Aspose::Cells::Systems::Double value)=0;
			/// <summary>
			/// Represents the location of the bottom of the crop rectangle expressed, expressed as a ratio of the image's height.
			/// </summary>
			 virtual Aspose::Cells::Systems::Double GetBottomCrop()=0;
			/// <summary>
			/// Represents the location of the bottom of the crop rectangle expressed, expressed as a ratio of the image's height.
			/// </summary>
			 virtual void SetBottomCrop(Aspose::Cells::Systems::Double value)=0;
			/// <summary>
			///  Represents the location of the left of the crop rectangle expressed, expressed as a ratio of the image's width.
			/// </summary>
			 virtual Aspose::Cells::Systems::Double GetLeftCrop()=0;
			/// <summary>
			///  Represents the location of the left of the crop rectangle expressed, expressed as a ratio of the image's width.
			/// </summary>
			 virtual void SetLeftCrop(Aspose::Cells::Systems::Double value)=0;
			/// <summary>
			/// Represents the location of the right of the crop rectangle expressed, expressed as a ratio of the image's width.
			/// </summary>
			 virtual Aspose::Cells::Systems::Double GetRightCrop()=0;
			/// <summary>
			/// Represents the location of the right of the crop rectangle expressed, expressed as a ratio of the image's width.
			/// </summary>
			 virtual void SetRightCrop(Aspose::Cells::Systems::Double value)=0;
			/// <summary>
			/// Returns or sets the degree of transparency of the area as a value from 0.0 (opaque) through 1.0 (clear).
			/// </summary>
			 virtual Aspose::Cells::Systems::Double GetTransparency()=0;
			/// <summary>
			/// Returns or sets the degree of transparency of the area as a value from 0.0 (opaque) through 1.0 (clear).
			/// </summary>
			 virtual void SetTransparency(Aspose::Cells::Systems::Double value)=0;
			/// <summary>
			/// Gets and sets the transparent color of the picture.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::ICellsColor> 		GetITransparentColor()=0;
			/// <summary>
			/// Gets and sets the transparent color of the picture.
			/// </summary>
			 virtual void 		SetITransparentColor(intrusive_ptr<Aspose::Cells::ICellsColor> value)=0;
			/// <summary>
			/// Represents the contrast modification for the picture.in unit of percentage.
			/// </summary>
			/// <remarks>
			/// It is between -100% and 100%. It works same as Excel 2007 or above version.
			/// </remarks>
			 virtual Aspose::Cells::Systems::Double GetContrast()=0;
			/// <summary>
			/// Represents the contrast modification for the picture.in unit of percentage.
			/// </summary>
			/// <remarks>
			/// It is between -100% and 100%. It works same as Excel 2007 or above version.
			/// </remarks>
			 virtual void SetContrast(Aspose::Cells::Systems::Double value)=0;
			/// <summary>
			/// Represents the brightness modification for the picture in unit of percentage.
			/// </summary>
			/// <remarks>
			/// It is between -100% and 100%. It works same as Excel 2007 or above version.
			/// </remarks>
			 virtual Aspose::Cells::Systems::Double GetBrightness()=0;
			/// <summary>
			/// Represents the brightness modification for the picture in unit of percentage.
			/// </summary>
			/// <remarks>
			/// It is between -100% and 100%. It works same as Excel 2007 or above version.
			/// </remarks>
			 virtual void SetBrightness(Aspose::Cells::Systems::Double value)=0;
			/// <summary>
			/// Represents gamma of the picture.
			/// </summary>
			 virtual Aspose::Cells::Systems::Double GetGamma()=0;
			/// <summary>
			/// Represents gamma of the picture.
			/// </summary>
			 virtual void SetGamma(Aspose::Cells::Systems::Double value)=0;
			/// <summary>
			/// Indicates whether this picture should be displayed in two-color black and white.
			/// </summary>
			 virtual bool IsBiLevel()=0;
			/// <summary>
			/// Indicates whether this picture should be displayed in two-color black and white.
			/// </summary>
			 virtual void SetBiLevel(bool value)=0;
			/// <summary>
			/// Indicates whether this picture SHOULD be displayed in grayscale.
			/// </summary>
			 virtual bool IsGray()=0;
			/// <summary>
			/// Indicates whether this picture SHOULD be displayed in grayscale.
			/// </summary>
			 virtual void SetGray(bool value)=0;
			/// <summary>
			/// Gets the hash code.
			/// </summary>
			/// <returns/>
			/// 
			  virtual Aspose::Cells::Systems::Int32 GetHashCode()=0;
			/// <summary>
			/// 
			/// </summary>
			/// <param name="obj" />
			/// 
			/// <returns/>
			/// 
			  virtual bool Equals(intrusive_ptr<Aspose::Cells::Systems::Object> obj)=0;

	};
}
}
}
