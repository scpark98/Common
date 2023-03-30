#ifndef _SYSTEM_IO_STRINGREADER_H_
#define _SYSTEM_IO_STRINGREADER_H_

#include "Aspose.Cells.Systems/Object.h"
#include "Aspose.Cells.Systems/String.h"
#include "Aspose.Cells.Systems/IO/TextReader.h"

namespace Aspose {
	namespace Cells {
		namespace Systems {
			namespace IO {
				class ASPOSE_CELLS_API StringReader : public TextReader 
				{
					private:
						StringPtr _s;
					    int _pos;
					    int _length;
 
					public:	
						StringReader(StringPtr s);
						void Close();
						void Dispose(bool disposing);
						int Peek();				    
						int Read();
				public:
						virtual ~StringReader() {}
						virtual void add_ref() {
							++ref_count_;
						}
						virtual int release_ref() {
							return --ref_count_;
						}
				};

			}
		}
	}
}
#endif//_SYSTEM_IO_TEXTREADER_H_