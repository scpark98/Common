#pragma once
namespace Aspose {
namespace Cells {
			/// <summary>
			/// Cache options for data access. Can be combined with | operator for multiple options together.
			/// </summary>
			/// <remarks>
			/// For some features, accessing large dataset requires a lot of repeated and complicated operations
			/// such as search, calculation, ...etc and those operations will take a lot of extra time.
			/// For common situations, all dependent data remains unchanged during the access, so some caches can be built and used to
			/// improve the access performance.
			/// For this purpose, we provide this API so that user can specify which kind of data access needs
			/// to be optimized by possible caching mechanism.
			/// <br/>
			/// 
			/// Please note, for different options, different data set may be required to be "read-only".
			/// And performance of accessing data depends on many aspects, the use of caching mechanism
			/// does not guarantee that performance will be improved. For some situations,
			/// such as the dataset to be accessed is small, using cache may cause even more time because
			/// caching itself also needs certain extra time.
			/// </remarks>
	enum AccessCacheOptions 

	{

			/// <summary>
			/// Auto-generated.
			/// </summary>
			AccessCacheOptions_None = 0 ,
			/// <summary>
			/// Auto-generated.
			/// </summary>
			AccessCacheOptions_All = -1 ,
			/// <summary>
			/// Auto-generated.
			/// </summary>
			AccessCacheOptions_PositionAndSize = 3 ,
			/// <summary>
			/// Auto-generated.
			/// </summary>
			AccessCacheOptions_CellsData = 9 ,
			/// <summary>
			/// Auto-generated.
			/// </summary>
			AccessCacheOptions_CellDisplay = 1048710 ,
			/// <summary>
			/// Auto-generated.
			/// </summary>
			AccessCacheOptions_GetFormula = 2097152 ,
			/// <summary>
			/// Auto-generated.
			/// </summary>
			AccessCacheOptions_SetFormula = 4194304 ,
			/// <summary>
			/// Auto-generated.
			/// </summary>
			AccessCacheOptions_CalculateFormula = 8388617 ,
			/// <summary>
			/// Auto-generated.
			/// </summary>
			AccessCacheOptions_ConditionalFormatting = 16777225 ,
			/// <summary>
			/// Auto-generated.
			/// </summary>
			AccessCacheOptions_Validation = 33554441 ,
	};


}

}
