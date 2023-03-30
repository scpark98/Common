#pragma once
namespace Aspose {
namespace Cells {
namespace Drawing {
			/// <summary>
			/// Represents path segment type.
			/// </summary>
	enum ShapePathType 

	{

			/// <summary>
			/// Straight line segment
			/// </summary>
			ShapePathType_LineTo = 0 ,
			/// <summary>
			/// Cubic Bezier curve
			/// </summary>
			ShapePathType_CubicBezierCurveTo ,
			/// <summary>
			/// Start a new path
			/// </summary>
			ShapePathType_MoveTo ,
			/// <summary>
			/// If the starting POINT and the end POINT are not the same, a single 
			/// straight line is drawn to connect the starting POINT and ending POINT of the path.
			/// </summary>
			ShapePathType_Close ,
			/// <summary>
			/// The end of the current path
			/// </summary>
			ShapePathType_End ,
			/// <summary>
			/// Escape
			/// </summary>
			ShapePathType_Escape ,
			/// <summary>
			/// An arc
			/// </summary>
			ShapePathType_ArcTo ,
			/// <summary>
			/// Unknown
			/// </summary>
			ShapePathType_Unknown ,
	};


}

}

}
