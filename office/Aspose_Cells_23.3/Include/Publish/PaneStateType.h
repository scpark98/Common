#pragma once
namespace Aspose {
namespace Cells {
			/// <summary>
			/// Represents state of the sheet's pane.
			/// </summary>
	enum PaneStateType 

	{

			/// <summary>
			/// Panes are frozen, but were not before being frozen.
			/// </summary>
			PaneStateType_Frozen ,
			/// <summary>
			/// Panes are frozen and were split before being frozen.
			/// </summary>
			PaneStateType_FrozenSplit ,
			/// <summary>
			/// Panes are split, but not frozen.
			/// </summary>
			PaneStateType_Split ,
			/// <summary>
			/// Panes are not frozen and not split.
			/// </summary>
			PaneStateType_Normal ,
	};


}

}
