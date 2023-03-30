#pragma once
#include "Aspose.Cells.Systems/Object.h"
#include "Aspose.Cells.Systems/Int32.h"
#include "Aspose.Cells.Systems/String.h"
#include "Aspose.Cells.Systems/Array1D.h"
#include "DirectoryType.h"
#include "Aspose.Cells.Systems/Collections/IEnumerator.h"
#include "Aspose.Cells.Systems/Collections/IEnumerable.h"

namespace Aspose {
	namespace Cells {
		enum DirectoryType;
		class IExternalLink;
	}
}
namespace Aspose{
namespace Cells{
			/// <summary>
			/// Represents external links collection in a workbook.
			/// </summary>
			/// <example><code>
			/// [C++]
			/// //Open a file with external links
			/// intrusive_ptr<IWorkbook> workbook = Factory::CreateIWorkbook(new String("d:\\book1.xls"));
			/// 
			/// //Change external link data source
			/// workbook->GetIWorksheets()->GetIExternalLinks()->GetObjectByIndex(0)->SetDataSource(new String("d:\\link.xls"));
			/// </code>
			/// </example>
	class ASPOSE_CELLS_API IExternalLinkCollection : public Aspose::Cells::Systems::Collections::IEnumerable , public Aspose::Cells::Systems::Object
	{
		public:
			/// <summary>
			/// Gets the number of elements actually contained in the collection.
			/// </summary>
			 virtual Aspose::Cells::Systems::Int32 GetCount()=0;
			/// <summary>
			/// Adds an external link.
			/// </summary>
			/// <param name="fileName" >The external file name.</param>
			/// <param name="sheetNames" >All sheet names of the external file.</param>
			/// <returns>The position of the external name in this list. </returns>
			 virtual Aspose::Cells::Systems::Int32 Add(intrusive_ptr<Aspose::Cells::Systems::String> fileName , intrusive_ptr<Aspose::Cells::Systems::Array1D<Aspose::Cells::Systems::String*>> sheetNames)=0;
			/// <summary>
			/// Add an external link .
			/// </summary>
			/// <param name="directoryType" >The directory type of the file name.</param>
			/// <param name="fileName" >the file name.</param>
			/// <param name="sheetNames" >All sheet names of the external file.</param>
			/// <returns>The position of the external name in this list. </returns>
			 virtual Aspose::Cells::Systems::Int32 Add(Aspose::Cells::DirectoryType directoryType , intrusive_ptr<Aspose::Cells::Systems::String> fileName , intrusive_ptr<Aspose::Cells::Systems::Array1D<Aspose::Cells::Systems::String*>> sheetNames)=0;
			/// <summary>
			///  Gets the <see cref="IExternalLink" />
			///  element at the specified index.
			///  </summary>
			/// <param name="index" >The zero based index of the element.</param>
			/// <returns>The element at the specified index.</returns>
			 virtual intrusive_ptr<Aspose::Cells::IExternalLink> 		GetObjectByIndex(Aspose::Cells::Systems::Int32 index)=0;
			/// <summary>
			/// Removes all external links.
			/// </summary>
			/// <remarks>
			/// When removing external links, all formulas that reference to them will be removed too because
			/// the references become invalid.
			/// </remarks>
			 virtual void Clear()=0;
			/// <summary>
			/// Removes all external links.
			/// </summary>
			/// <param name="updateReferencesAsLocal" >
			/// Whether update all references of external links as references of current workbook itself.
			/// </param>

			 virtual void Clear(bool updateReferencesAsLocal)=0;
			/// <summary>
			/// Removes the specified external link from the workbook.
			/// </summary>
			/// <param name="index" >the index of the external link to be removed.</param>
			/// <remarks>
			/// When removing the external link, all formulas that reference to it will be removed too because
			/// the references become invalid.
			/// </remarks>
			 virtual void RemoveAt(Aspose::Cells::Systems::Int32 index)=0;
			/// <summary>
			/// Removes the specified external link from the workbook.
			/// </summary>
			/// <param name="index" >the index of the external link to be removed.</param>
			/// <param name="updateReferencesAsLocal" >
			/// Whether update all references of given external link to reference of current workbook itself.
			/// </param>

			 virtual void RemoveAt(Aspose::Cells::Systems::Int32 index , bool updateReferencesAsLocal)=0;
			/// <summary>
			/// Get an enumerator that iterates through this collection.
			/// </summary>
			/// <returns/>
			/// 
			 virtual intrusive_ptr<Aspose::Cells::Systems::Collections::IEnumerator> GetEnumerator()=0;
public:
		public:
			virtual void add_ref() {++ref_count_;}
			virtual int release_ref() {return --ref_count_;}

	};
}
}
