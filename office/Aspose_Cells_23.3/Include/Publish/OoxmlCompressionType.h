#pragma once
namespace Aspose {
namespace Cells {
			/// <summary>
			/// The Ooxml compression type 
			/// </summary>
	enum OoxmlCompressionType 

	{

			/// <summary>
			/// The fastest but least effective compression.
			///  </summary>
			OoxmlCompressionType_Level1 = 1 ,
			/// <summary>
			/// A little slower, but better, than level 1.
			/// </summary>
			OoxmlCompressionType_Level2 = 2 ,
			/// <summary>
			/// A little slower, but better, than level 2.
			/// </summary>
			OoxmlCompressionType_Level3 = 3 ,
			/// <summary>
			/// A little slower, but better, than level 3.
			/// </summary>
			OoxmlCompressionType_Level4 = 4 ,
			/// <summary>
			/// A little slower than level 4, but with better compression.
			/// </summary>
			OoxmlCompressionType_Level5 = 5 ,
			/// <summary>
			/// A good balance of speed and compression efficiency.
			/// </summary>
			OoxmlCompressionType_Level6 = 6 ,
			/// <summary>
			/// Pretty good compression!
			/// </summary>
			OoxmlCompressionType_Level7 = 7 ,
			/// <summary>
			///  Better compression than Level7!
			/// </summary>
			OoxmlCompressionType_Level8 = 8 ,
			/// <summary>
			/// The "best" compression, where best means greatest reduction in size of the input data stream.
			/// This is also the slowest compression.
			/// </summary>
			OoxmlCompressionType_Level9 = 9 ,
	};


}

}
