#pragma once
#include "Aspose.Cells.Systems/Object.h"
#include "Aspose.Cells.Systems/String.h"
#include "Aspose.Cells.Systems/Int32.h"
#include "Aspose.Cells.Systems/Collections/CollectionBase.h"

namespace Aspose {
	namespace Cells {
		namespace Properties {
			class IDocumentProperty;
		}
	}
}
namespace Aspose{
namespace Cells{
namespace Properties{
			/// <summary>
			/// Base class for <see cref="IBuiltInDocumentPropertyCollection" />
			///  and <see cref="ICustomDocumentPropertyCollection" />
			///  collections.
			/// </summary>
			/// <example><code>
			/// 
			/// [C++]
			/// 
			///  //Instantiate an IWorkbook object, which must contains document property
			///  intrusive_ptr<Aspose::Cells::IWorkbook> workbook = Factory::CreateIWorkbook(new String("C:\\book1.xls"));
			///  //Retrieve a list of all custom document properties of the Excel file
			///  intrusive_ptr<Aspose::Cells::Properties::ICustomDocumentPropertyCollection> customProperties =
			///      workbook->GetIWorksheets()->GetICustomDocumentProperties();
			///  //Accessng a custom document property by using the property index
			///  intrusive_ptr<Aspose::Cells::Properties::IDocumentProperty> customProperty1 = customProperties->GetIndexObject(3);
			///  //Accessng a custom document property by using the property name
			///  intrusive_ptr<Aspose::Cells::Properties::IDocumentProperty> customProperty2 = customProperties->GetIndexObject(new String("Owner"));
			/// 
			/// </code>
			/// </example>
	class ASPOSE_CELLS_API IDocumentPropertyCollection : public Aspose::Cells::Systems::Collections::CollectionBase
	{
		public:
			/// <overloads>Returns a <see cref="DocumentProperty" />
			///  object.</overloads>
			/// <summary>
			/// Returns an <see cref="IDocumentProperty" />
			///  object by the name of the property.
			/// </summary>
			/// <remarks><p>Returns null if a property with the specified name is not found.</p>
			/// </remarks>
			/// <param name="name" >The case-insensitive name of the property to retrieve.</param>
			 virtual intrusive_ptr<Aspose::Cells::Properties::IDocumentProperty> 		GetObjectByIndex(intrusive_ptr<Aspose::Cells::Systems::String> name)=0;
			/// <summary>
			/// Returns an <see cref="IDocumentProperty" />
			///  object by index.
			/// </summary>
			/// <param name="index" >Zero-based index of the <see cref="IDocumentProperty" />
			///  to retrieve.</param>
			 virtual intrusive_ptr<Aspose::Cells::Properties::IDocumentProperty> 		GetObjectByIndex(Aspose::Cells::Systems::Int32 index)=0;
			/// <summary>
			/// Returns true if a property with the specified name exists in the collection.
			/// </summary>
			/// <param name="name" >The case-insensitive name of the property.</param>
			/// <returns>True if the property exists in the collection; false otherwise.</returns>
			 virtual bool Contains(intrusive_ptr<Aspose::Cells::Systems::String> name)=0;
			/// <summary>
			/// Gets the index of a property by name.
			/// </summary>
			/// <param name="name" >The case-insensitive name of the property.</param>
			/// <returns>The zero based index. Negative value if not found.</returns>
			 virtual Aspose::Cells::Systems::Int32 IndexOf(intrusive_ptr<Aspose::Cells::Systems::String> name)=0;
			/// <summary>
			/// Removes a property with the specified name from the collection.
			/// </summary>
			/// <param name="name" >The case-insensitive name of the property.</param>
			 virtual void Remove(intrusive_ptr<Aspose::Cells::Systems::String> name)=0;
			/// <summary>
			/// Removes a property at the specified index.
			/// </summary>
			/// <param name="index" >The zero based index.</param>
			 virtual void RemoveAt(Aspose::Cells::Systems::Int32 index)=0;

	};
}
}
}
