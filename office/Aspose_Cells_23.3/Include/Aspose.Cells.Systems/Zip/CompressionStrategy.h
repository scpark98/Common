#ifndef _SYSTEM_ZIP_CONSSTRA_H_
#define _SYSTEM_ZIP_CONSSTRA_H_

namespace Aspose {
	namespace Cells {
		namespace Zip
		{
			enum CompressionStrategy
			{
				/// <summary>
				/// The default strategy is probably the best for normal data.
				/// </summary>
				CompressionStrategy_Default = 0,

				/// <summary>
				/// The <c>Filtered</c> strategy is intended to be used most effectively with data produced by a
				/// filter or predictor.  By this definition, filtered data consists mostly of small
				/// values with a somewhat random distribution.  In this case, the compression algorithm
				/// is tuned to compress them better.  The effect of <c>Filtered</c> is to force more Huffman
				/// coding and less string matching; it is a half-step between <c>Default</c> and <c>HuffmanOnly</c>.
				/// </summary>
				CompressionStrategy_Filtered = 1,

				/// <summary>
				/// Using <c>HuffmanOnly</c> will force the compressor to do Huffman encoding only, with no
				/// string matching.
				/// </summary>
				CompressionStrategy_HuffmanOnly = 2,
			};
		}
	}
}
#endif// 