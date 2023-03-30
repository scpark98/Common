#pragma once
#include "Aspose.Cells.Systems/Object.h"
#include "FileFormatType.h"
#include "Aspose.Cells.Systems/String.h"
#include "Aspose.Cells.Systems/IO/Stream.h"
#include "AccessCacheOptions.h"
#include "SaveFormat.h"
#include "Aspose.Cells.Systems/IO/MemoryStream.h"
#include "BuiltinStyleType.h"
#include "Aspose.Cells.Systems/Int32.h"
#include "Aspose.Cells.Systems/Double.h"
#include "Aspose.Cells.Systems/Array1D.h"
#include "Aspose.Cells.Systems/Drawing/Color.h"
#include "EncryptionType.h"
#include "ProtectionType.h"
#include "ThemeColorType.h"
#include "Aspose.Cells.Systems/IDisposable.h"

namespace Aspose {
	namespace Cells {
		class IWorkbookSettings;
		enum FileFormatType;
		class ILoadOptions;
		enum AccessCacheOptions;
		enum SaveFormat;
		class ISaveOptions;
		class IWorksheetCollection;
		class IStyle;
		enum BuiltinStyleType;
		class ICellsColor;
		class IReplaceOptions;
		class IWorkbook;
		class ICopyOptions;
		class IFont;
		class ICustomFunction;
		class ICalculationOptions;
		enum EncryptionType;
		enum ProtectionType;
		class IDataSorter;
		enum ThemeColorType;
	}
}
namespace Aspose {
	namespace Cells {
		namespace Vba {
			class IVbaProject;
		}
	}
}
namespace Aspose {
	namespace Cells {
		namespace Properties {
			class IBuiltInDocumentPropertyCollection;
			class ICustomDocumentPropertyCollection;
			class IContentTypePropertyCollection;
		}
	}
}
namespace Aspose {
	namespace Cells {
		namespace DigitalSignatures {
			class IDigitalSignatureCollection;
		}
	}
}
namespace Aspose{
namespace Cells{
			/// <summary>
			///  Represents a root object to create an Excel spreadsheet. 	
			///  </summary>
			/// 
			/// <remarks>The IWorkbook class denotes an Excel spreadsheet. Each spreadsheet can contain multiple worksheets.
			///  The basic feature of the class is to open and save native excel files.
			///  The class has some advanced features like copying data from other Workbooks, combining two Workbooks and protecting the Excel spreadsheet.
			///  </remarks>
			/// 
			/// <example>
			///  The following example opens a file named designer.xls. It replaces two string values with an Integer value and string value respectively within the spreadsheet.
			///  In order to run this case normally, you should create a file named designer.xls using MS Excels, and set two string values "OldInt" and "OldString"  between any two cells
			///  then save it in the root directory of your D: drive.
			///  <code>
			///		
			///  [C++]
			///  //Open a file named designer.xls
			///  StringPtr mapPath = new String("D:");
			///  StringPtr designerFile = mapPath->StringAppend(new String("\\designer.xls"));
			///  intrusive_ptr<IWorkbook> workbook = Factory::CreateIWorkbook(designerFile);
			///    
			///  //Replace the placeholder string with new values
			///  int newInt = 100;
			///  workbook->Replace(new String("OldInt"), newInt);
			///  
			///  StringPtr newString = new String("Hello!");
			///  workbook->Replace((StringPtr)(new String("OldString")), (StringPtr)newString);
			///  workbook->Save(designerFile);
			///      
			///  </code>
			/// 
			///  </example>
			/// 
			/// 
	class ASPOSE_CELLS_API IWorkbook : public Aspose::Cells::Systems::IDisposable , public Aspose::Cells::Systems::Object
	{
		public:
			/// <summary>
			/// Represents the workbook settings.
			/// </summary>
			/// 
			/// 
			 virtual intrusive_ptr<Aspose::Cells::IWorkbookSettings> 		GetISettings()=0;
			/// <summary>
			/// Parses all formulas which have not been parsed when they were loaded from template file or set to a cell.
			/// </summary>
			/// <param name="ignoreError" >whether ignore error for invalid formula.
			/// For one invalid formula, if ignore error then this formula will be ignored
			/// and the process will continue to parse other formulas, otherwise exception will be thrown.</param>
			 virtual void ParseFormulas(bool ignoreError)=0;
			/// <summary>
			/// Starts the session that uses caches to access data.
			/// </summary>
			/// <param name="opts" >options of data access</param>
			/// <remarks>If the cache of specified data access requires some data models in worksheet to be "read-only",
			/// then corresponding data models in every worksheet in this workbook will be taken as "read-only"
			/// and user should not change any of them.
			/// <br/>
			/// 
			/// After finishing the access to the data, <see cref="CloseAccessCache(AccessCacheOptions)" />
			///  should
			/// be invoked with same options to clear all caches and recover normal access mode.
			/// <br/>
			/// </remarks>
			 virtual void StartAccessCache(Aspose::Cells::AccessCacheOptions opts)=0;
			/// <summary>
			/// Closes the session that uses caches to access data.
			/// </summary>
			/// <param name="opts" >options of data access</param>
			 virtual void CloseAccessCache(Aspose::Cells::AccessCacheOptions opts)=0;
			/// <summary>
			/// Saves the workbook to the disk.
			/// </summary>
			/// <param name="fileName" >The file name.</param>
			/// <param name="saveFormat" >The save format type.</param>
			 virtual void Save(intrusive_ptr<Aspose::Cells::Systems::String> fileName , Aspose::Cells::SaveFormat saveFormat)=0;
			/// <summary>
			/// Save the workbook to the disk.
			/// </summary>
			/// <param name="fileName" />
			/// 
			 virtual void Save(intrusive_ptr<Aspose::Cells::Systems::String> fileName)=0;
			/// <summary>
			/// Saves the workbook to the disk.
			/// </summary>
			/// <param name="fileName" >The file name.</param>
			/// <param name="saveOptions" >The save options.</param>
			 virtual void 		Save(intrusive_ptr<Aspose::Cells::Systems::String> fileName , intrusive_ptr<Aspose::Cells::ISaveOptions> saveOptions)=0;
			/// <summary>
			/// Saves the workbook to the stream.
			/// </summary>
			/// 
			/// <param name="stream" >The file stream.</param>
			/// 
			/// <param name="saveFormat" >The save file format type.</param>
			/// 
			/// 
			 virtual void Save(intrusive_ptr<Aspose::Cells::Systems::IO::Stream> stream , Aspose::Cells::SaveFormat saveFormat)=0;
			/// <summary>
			/// Saves the workbook to the stream.
			/// </summary>
			/// <param name="stream" >The file stream.</param>
			/// <param name="saveOptions" >The save options.</param>
			 virtual void 		Save(intrusive_ptr<Aspose::Cells::Systems::IO::Stream> stream , intrusive_ptr<Aspose::Cells::ISaveOptions> saveOptions)=0;
			/// <summary>
			/// Saves Excel file to a MemoryStream object and returns it.
			/// </summary>
			/// <returns>MemoryStream object which contains an Excel file.</returns>
			/// <remarks>
			/// 
			/// This method provides same function as Save method and only save the workbook as Excel97-2003 xls file. 
			/// It's mainly for calling from COM clients.</remarks>
			 virtual intrusive_ptr<Aspose::Cells::Systems::IO::MemoryStream> SaveToStream()=0;
			/// <summary>
			/// Gets the <see cref="WorksheetCollection" />
			///  collection in the spreadsheet.
			/// </summary>
			/// <returns><see cref="WorksheetCollection" />
			///  collection</returns>
			 virtual intrusive_ptr<Aspose::Cells::IWorksheetCollection> 		GetIWorksheets()=0;
			/// <summary>
			/// Indicates whether license is set.
			/// </summary>
			 virtual bool IsLicensed()=0;
			/// <summary>
			/// Remove all unused styles.
			/// </summary>
			 virtual void RemoveUnusedStyles()=0;
			/// <summary>
			/// Creates a new style.
			/// </summary>
			/// 
			/// <returns>Returns a style object.</returns>
			/// 
			/// 
			 virtual intrusive_ptr<Aspose::Cells::IStyle> 		CreateIStyle()=0;
			/// <summary>
			/// Creates built-in style by given type.
			/// </summary>
			/// 
			/// <param name="type" />
			/// 
			/// <returns>IStyle object</returns>
			/// 
			/// 
			 virtual intrusive_ptr<Aspose::Cells::IStyle> 		CreateBuiltinIStyle(Aspose::Cells::BuiltinStyleType type)=0;
			/// <summary>
			/// Creates an <see cref="ICellsColor" />
			///  object.
			/// </summary>
			/// <returns>Returns an <see cref="ICellsColor" />
			///  object.</returns>
			 virtual intrusive_ptr<Aspose::Cells::ICellsColor> 		CreateICellsColor()=0;
			/// <summary>
			///  Replaces a cell's value with a new string.
			/// </summary>
			/// 
			/// <example>
			///  <code>
			///  [C++]
			///  
			///  intrusive_ptr<IWorkbook> workbook = Factory::CreateIWorkbook();
			///  ......
			///  workbook->Replace((StringPtr)new String("AnOldValue"), (StringPtr)new String("NewValue"));
			///  
			///  </code>
			/// 
			///  </example>
			/// 
			/// <param name="placeHolder" >Cell placeholder</param>
			/// 
			/// <param name="newValue" >String value to replace</param>
			/// 
			/// 
			 virtual Aspose::Cells::Systems::Int32 Replace(intrusive_ptr<Aspose::Cells::Systems::String> placeHolder , intrusive_ptr<Aspose::Cells::Systems::String> newValue)=0;
			/// <summary>
			///  Replaces a cell's value with a new integer.
			/// </summary>
			/// 
			/// <example>
			///  <code>
			///  [C++]
			///  
			///  intrusive_ptr<IWorkbook> workbook = Factory::CreateIWorkbook();
			///  ......
			///  int newValue = 100;
			///  workbook->Replace(StringPtr)new String("AnOldValue"), newValue);
			///  
			///  </code>
			/// 
			///  </example>
			/// 
			/// <param name="placeHolder" >Cell placeholder</param>
			/// 
			/// <param name="newValue" >Integer value to replace</param>
			/// 
			/// 
			 virtual Aspose::Cells::Systems::Int32 Replace(intrusive_ptr<Aspose::Cells::Systems::String> placeHolder , Aspose::Cells::Systems::Int32 newValue)=0;
			/// <summary>
			///  Replaces a cell's value with a new double.
			/// </summary>
			/// 
			/// <example>
			///  <code>
			///  
			///  [C++]
			///  
			///  intrusive_ptr<IWorkbook> workbook = Factory::CreateIWorkbook();
			///  ......
			///  double newValue = 100.0;
			///  workbook.Replace(new String("AnOldValue"), newValue);
			///  
			///  </code>
			/// 
			///  </example>
			/// 
			/// <param name="placeHolder" >Cell placeholder</param>
			/// 
			/// <param name="newValue" >Double value to replace</param>
			/// 
			/// 
			 virtual Aspose::Cells::Systems::Int32 Replace(intrusive_ptr<Aspose::Cells::Systems::String> placeHolder , Aspose::Cells::Systems::Double newValue)=0;
			/// <summary>
			///  Replaces a cell's value with a new string array.
			/// </summary>
			/// 
			/// <example>
			///  <code>
			///  
			///  [C++]
			///  intrusive_ptr<IWorkbook> workbook = Factory::CreateIWorkbook();
			///  intrusive_ptr<Array1D<String*>> newValues = new Array1D<String*>(3);
			///  newValues->SetValue(new String("Tom"), 0);
			///  newValues->SetValue(new String("Alice"), 1);
			///  newValues->SetValue(new String("Jerry"), 2);
			///  workbook->Replace(new String("AnOldValue"), newValues, true);
			///  
			///  </code>
			/// 
			///  </example>
			/// 
			/// <param name="placeHolder" >Cell placeholder</param>
			/// 
			/// <param name="newValues" >String array to replace</param>
			/// 
			/// <param name="isVertical" >True - Vertical, False - Horizontal</param>
			/// 
			/// 
			 virtual Aspose::Cells::Systems::Int32 Replace(intrusive_ptr<Aspose::Cells::Systems::String> placeHolder , intrusive_ptr<Aspose::Cells::Systems::Array1D<Aspose::Cells::Systems::String*>> newValues , bool isVertical)=0;
			/// <summary>
			///  Replaces cells' values with an integer array.
			/// </summary>
			/// 
			/// <example>
			///  <code>
			///  [C++]
			///  
			///  intrusive_ptr<IWorkbook> workbook = Factory::CreateIWorkbook();
			///  ......
			///  intrusive_ptr<Array1D<int>> newValues = new Array1D<int>(3);
			///  newValues->SetValue(1, 0);
			///  newValues->SetValue(2, 1);
			///  newValues->SetValue(3, 2);
			///  workbook->Replace(new String("AnOldValue"), newValues, true);
			///  </code>
			/// 
			///  </example>
			/// 
			/// <param name="placeHolder" >Cell placeholder</param>
			/// 
			/// <param name="newValues" >Integer array to replace</param>
			/// 
			/// <param name="isVertical" >True - Vertical, False - Horizontal</param>
			/// 
			/// 
			 virtual Aspose::Cells::Systems::Int32 Replace(intrusive_ptr<Aspose::Cells::Systems::String> placeHolder , intrusive_ptr<Aspose::Cells::Systems::Array1D<Aspose::Cells::Systems::Int32>> newValues , bool isVertical)=0;
			/// <summary>
			///  Replaces cells' values with a double array.
			/// </summary>
			/// 
			/// <example>
			///  <code>
			///  
			///  [C++]
			///  
			///  intrusive_ptr<IWorkbook> workbook = Factory::CreateIWorkbook();
			///  ......
			///  intrusive_ptr<Array1D<double>> newValues = new Array1D<double>(3);
			///  newValues->SetValue(1.23, 0);
			///  newValues->SetValue(2.56, 1);
			///  newValues->SetValue(3.14159, 2);
			///  workbook->Replace(new String("AnOldValue"), newValues, true);
			///  
			///  </code>
			/// 
			///  </example>
			/// 
			/// <param name="placeHolder" >Cell placeholder</param>
			/// 
			/// <param name="newValues" >Double array to replace</param>
			/// 
			/// <param name="isVertical" >True - Vertical, False - Horizontal</param>
			/// 
			/// 
			 virtual Aspose::Cells::Systems::Int32 Replace(intrusive_ptr<Aspose::Cells::Systems::String> placeHolder , intrusive_ptr<Aspose::Cells::Systems::Array1D<Aspose::Cells::Systems::Double>> newValues , bool isVertical)=0;
			/// <summary>
			/// Replaces cells' values with new data.
			/// </summary>
			/// 
			/// <param name="boolValue" >The boolean value to be replaced.</param>
			/// 
			/// <param name="newValue" >New value. Can be string, integer, double or DateTime value.</param>
			/// 
			/// 
			 virtual Aspose::Cells::Systems::Int32 Replace(bool boolValue , intrusive_ptr<Aspose::Cells::Systems::Object> newValue)=0;
			/// <summary>
			/// Replaces cells' values with new data.
			/// </summary>
			/// 
			/// <param name="intValue" >The integer value to be replaced.</param>
			/// 
			/// <param name="newValue" >New value. Can be string, integer, double or DateTime value.</param>
			/// 
			/// 
			 virtual Aspose::Cells::Systems::Int32 Replace(Aspose::Cells::Systems::Int32 intValue , intrusive_ptr<Aspose::Cells::Systems::Object> newValue)=0;
			/// <summary>
			///  Replaces a cell's value with a new string.
			/// </summary>
			/// <param name="placeHolder" >Cell placeholder</param>
			/// <param name="newValue" >String value to replace</param>
			/// <param name="options" > The replace options</param>
			 virtual Aspose::Cells::Systems::Int32 		Replace(intrusive_ptr<Aspose::Cells::Systems::String> placeHolder , intrusive_ptr<Aspose::Cells::Systems::String> newValue , intrusive_ptr<Aspose::Cells::IReplaceOptions> options)=0;
			/// <summary>
			/// Copies data from a source Workbook object.
			/// </summary>
			/// <param name="source" >Source Workbook object.</param>
			/// <param name="copyOptions" />
			/// 
			 virtual void 		Copy(intrusive_ptr<Aspose::Cells::IWorkbook> source , intrusive_ptr<Aspose::Cells::ICopyOptions> copyOptions)=0;
			/// <summary>
			/// Copies data from a source Workbook object.
			/// </summary>
			/// <param name="source" >Source Workbook object.</param>
			 virtual void 		Copy(intrusive_ptr<Aspose::Cells::IWorkbook> source)=0;
			/// <summary>
			/// Combines another Workbook object.
			/// </summary>
			/// <param name="secondWorkbook" >Another Workbook object.</param>
			/// <remarks>Currently, only cell data and cell style of the second Workbook object can be combined. Images, charts and other drawing objects are not supported.
			/// </remarks>
			 virtual void 		CombineIWorkbook(intrusive_ptr<Aspose::Cells::IWorkbook> secondWorkbook)=0;
			/// <summary>
			/// Returns colors in the palette for the spreadsheet. 
			/// </summary>
			/// <remarks>The palette has 56 entries, each represented by an RGB value.</remarks>
			 virtual intrusive_ptr<Aspose::Cells::Systems::Array1D<Aspose::Cells::Systems::Drawing::Color*>> GetColors()=0;
			/// <summary>
			/// Gets the style in the style pool.
			/// All styles in the workbook will be gathered into a pool.
			/// There is only a simple reference index in the cells.
			/// </summary>
			/// <param name="index" >The index.</param>
			/// <returns>
			/// The style in the pool corresponds to given index, may be null.
			/// </returns>
			/// <remarks>
			/// If the returned style is changed, the style of all cells(which refers to this style) will be changed.
			/// </remarks>
			 virtual intrusive_ptr<Aspose::Cells::IStyle> 		GetIStyleInPool(Aspose::Cells::Systems::Int32 index)=0;
			/// <summary>
			/// Gets number of the styles in the style pool.
			/// </summary>
			/// 
			/// 
			 virtual Aspose::Cells::Systems::Int32 GetCountOfStylesInPool()=0;
			/// <summary>
			/// Gets all fonts in the style pool.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Systems::Array1D<Aspose::Cells::IFont*>> 		GetIFonts()=0;
			/// <summary>
			/// Gets the named style in the style pool.
			/// </summary>
			/// 
			/// <param name="name" >name of the style</param>
			/// 
			/// <returns>named style, maybe null.</returns>
			/// 
			/// 
			 virtual intrusive_ptr<Aspose::Cells::IStyle> 		GetNamedIStyle(intrusive_ptr<Aspose::Cells::Systems::String> name)=0;
			/// <summary>
			///  Changes the palette for the spreadsheet in the specified index.
			///  </summary>
			/// <param name="color" >Color structure.</param>
			/// <param name="index" >Palette index, 0 - 55.</param>
			/// <remarks>The palette has 56 entries, each represented by an RGB value. 
			///  If you set a color which is not in the palette, it will not take effect.
			///  So if you want to set a custom color, please change the palette at first.
			///  <p>The following is the standard color palette.</p>
			/// <table class="dtTABLE"  cellspacing="0" >
			///   <tr>
			///     <td width="25%" ><font color="gray" ><b>Color</b>
			/// </font>
			/// 　</td>
			/// 
			///     <td width="25%" ><font color="gray" ><b>Red</b>
			/// </font>
			/// 　</td>
			/// 
			///     <td width="25%" ><font color="gray" ><b>Green</b>
			/// </font>
			/// 　</td>
			/// 
			///     <td width="25%" ><font color="gray" ><b>Blue</b>
			/// </font>
			/// 　</td>
			/// 
			///   
			///   
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Black　</td>
			/// 
			///     <td width="25%" >0　</td>
			/// 
			///     <td width="25%" >0　</td>
			/// 
			///     <td width="25%" >0　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >White　</td>
			/// 
			///     <td width="25%" >255　</td>
			/// 
			///     <td width="25%" >255　</td>
			/// 
			///     <td width="25%" >255　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Red　</td>
			/// 
			///     <td width="25%" >255　</td>
			/// 
			///     <td width="25%" >0　</td>
			/// 
			///     <td width="25%" >0　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Lime　</td>
			/// 
			///     <td width="25%" >0　</td>
			/// 
			///     <td width="25%" >255　</td>
			/// 
			///     <td width="25%" >0　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Blue　</td>
			/// 
			///     <td width="25%" >0　</td>
			/// 
			///     <td width="25%" >0　</td>
			/// 
			///     <td width="25%" >255　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Yellow　</td>
			/// 
			///     <td width="25%" >255　</td>
			/// 
			///     <td width="25%" >255　</td>
			/// 
			///     <td width="25%" >0　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Magenta　</td>
			/// 
			///     <td width="25%" >255　</td>
			/// 
			///     <td width="25%" >0　</td>
			/// 
			///     <td width="25%" >255　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Cyan　</td>
			/// 
			///     <td width="25%" >0　</td>
			/// 
			///     <td width="25%" >255　</td>
			/// 
			///     <td width="25%" >255　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Maroon　</td>
			/// 
			///     <td width="25%" >128　</td>
			/// 
			///     <td width="25%" >0　</td>
			/// 
			///     <td width="25%" >0　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Green　</td>
			/// 
			///     <td width="25%" >0　</td>
			/// 
			///     <td width="25%" >128　</td>
			/// 
			///     <td width="25%" >0　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Navy　</td>
			/// 
			///     <td width="25%" >0　</td>
			/// 
			///     <td width="25%" >0　</td>
			/// 
			///     <td width="25%" >128　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Olive　</td>
			/// 
			///     <td width="25%" >128　</td>
			/// 
			///     <td width="25%" >128　</td>
			/// 
			///     <td width="25%" >0　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Purple　</td>
			/// 
			///     <td width="25%" >128　</td>
			/// 
			///     <td width="25%" >0　</td>
			/// 
			///     <td width="25%" >128　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Teal　</td>
			/// 
			///     <td width="25%" >0　</td>
			/// 
			///     <td width="25%" >128　</td>
			/// 
			///     <td width="25%" >128　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Silver　</td>
			/// 
			///     <td width="25%" >192　</td>
			/// 
			///     <td width="25%" >192　</td>
			/// 
			///     <td width="25%" >192　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Gray　</td>
			/// 
			///     <td width="25%" >128　</td>
			/// 
			///     <td width="25%" >128　</td>
			/// 
			///     <td width="25%" >128　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Color17　</td>
			/// 
			///     <td width="25%" >153　</td>
			/// 
			///     <td width="25%" >153　</td>
			/// 
			///     <td width="25%" >255　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Color18　</td>
			/// 
			///     <td width="25%" >153　</td>
			/// 
			///     <td width="25%" >51　</td>
			/// 
			///     <td width="25%" >102　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Color19　</td>
			/// 
			///     <td width="25%" >255　</td>
			/// 
			///     <td width="25%" >255　</td>
			/// 
			///     <td width="25%" >204　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Color20　</td>
			/// 
			///     <td width="25%" >204　</td>
			/// 
			///     <td width="25%" >255　</td>
			/// 
			///     <td width="25%" >255　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Color21　</td>
			/// 
			///     <td width="25%" >102　</td>
			/// 
			///     <td width="25%" >0　</td>
			/// 
			///     <td width="25%" >102　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Color22　</td>
			/// 
			///     <td width="25%" >255　</td>
			/// 
			///     <td width="25%" >128　</td>
			/// 
			///     <td width="25%" >128　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Color23　</td>
			/// 
			///     <td width="25%" >0　</td>
			/// 
			///     <td width="25%" >102　</td>
			/// 
			///     <td width="25%" >204　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Color24　</td>
			/// 
			///     <td width="25%" >204　</td>
			/// 
			///     <td width="25%" >204　</td>
			/// 
			///     <td width="25%" >255　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Color25　</td>
			/// 
			///     <td width="25%" >0　</td>
			/// 
			///     <td width="25%" >0　</td>
			/// 
			///     <td width="25%" >128　</td>
			/// 
			///   </tr>
			/// <tr>
			///     <td width="25%" >Color26　</td>
			/// 
			///     <td width="25%" >255　</td>
			/// 
			///     <td width="25%" >0　</td>
			/// 
			///     <td width="25%" >255　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Color27　</td>
			/// 
			///     <td width="25%" >255　</td>
			/// 
			///     <td width="25%" >255　</td>
			/// 
			///     <td width="25%" >0　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Color28　</td>
			/// 
			///     <td width="25%" >0　</td>
			/// 
			///     <td width="25%" >255　</td>
			/// 
			///     <td width="25%" >255　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Color29　</td>
			/// 
			///     <td width="25%" >128　</td>
			/// 
			///     <td width="25%" >0　</td>
			/// 
			///     <td width="25%" >128　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Color30　</td>
			/// 
			///     <td width="25%" >128　</td>
			/// 
			///     <td width="25%" >0　</td>
			/// 
			///     <td width="25%" >0　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Color31　</td>
			/// 
			///     <td width="25%" >0　</td>
			/// 
			///     <td width="25%" >128　</td>
			/// 
			///     <td width="25%" >128　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Color32　</td>
			/// 
			///     <td width="25%" >0　</td>
			/// 
			///     <td width="25%" >0　</td>
			/// 
			///     <td width="25%" >255　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Color33　</td>
			/// 
			///     <td width="25%" >0　</td>
			/// 
			///     <td width="25%" >204　</td>
			/// 
			///     <td width="25%" >255　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Color34　</td>
			/// 
			///     <td width="25%" >204　</td>
			/// 
			///     <td width="25%" >255　</td>
			/// 
			///     <td width="25%" >255　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Color35　</td>
			/// 
			///     <td width="25%" >204　</td>
			/// 
			///     <td width="25%" >255　</td>
			/// 
			///     <td width="25%" >204　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Color36　</td>
			/// 
			///     <td width="25%" >255　</td>
			/// 
			///     <td width="25%" >255　</td>
			/// 
			///     <td width="25%" >153　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Color37　</td>
			/// 
			///     <td width="25%" >153　</td>
			/// 
			///     <td width="25%" >204　</td>
			/// 
			///     <td width="25%" >255　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Color38　</td>
			/// 
			///     <td width="25%" >255　</td>
			/// 
			///     <td width="25%" >153　</td>
			/// 
			///     <td width="25%" >204　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Color39　</td>
			/// 
			///     <td width="25%" >204　</td>
			/// 
			///     <td width="25%" >153　</td>
			/// 
			///     <td width="25%" >255　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Color40　</td>
			/// 
			///     <td width="25%" >255　</td>
			/// 
			///     <td width="25%" >204　</td>
			/// 
			///     <td width="25%" >153　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Color41　</td>
			/// 
			///     <td width="25%" >51　</td>
			/// 
			///     <td width="25%" >102　</td>
			/// 
			///     <td width="25%" >255　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Color42　</td>
			/// 
			///     <td width="25%" >51　</td>
			/// 
			///     <td width="25%" >204　</td>
			/// 
			///     <td width="25%" >204　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Color43　</td>
			/// 
			///     <td width="25%" >153　</td>
			/// 
			///     <td width="25%" >204　</td>
			/// 
			///     <td width="25%" >0　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Color44　</td>
			/// 
			///     <td width="25%" >255　</td>
			/// 
			///     <td width="25%" >204　</td>
			/// 
			///     <td width="25%" >0　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Color45　</td>
			/// 
			///     <td width="25%" >255　</td>
			/// 
			///     <td width="25%" >153　</td>
			/// 
			///     <td width="25%" >0　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Color46　</td>
			/// 
			///     <td width="25%" >255　</td>
			/// 
			///     <td width="25%" >102　</td>
			/// 
			///     <td width="25%" >0　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Color47　</td>
			/// 
			///     <td width="25%" >102　</td>
			/// 
			///     <td width="25%" >102　</td>
			/// 
			///     <td width="25%" >153　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Color48　</td>
			/// 
			///     <td width="25%" >150　</td>
			/// 
			///     <td width="25%" >150　</td>
			/// 
			///     <td width="25%" >150　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Color49　</td>
			/// 
			///     <td width="25%" >0　</td>
			/// 
			///     <td width="25%" >51　</td>
			/// 
			///     <td width="25%" >102　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Color50　</td>
			/// 
			///     <td width="25%" >51　</td>
			/// 
			///     <td width="25%" >153　</td>
			/// 
			///     <td width="25%" >102　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Color51　</td>
			/// 
			///     <td width="25%" >0　</td>
			/// 
			///     <td width="25%" >51　</td>
			/// 
			///     <td width="25%" >0　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Color52　</td>
			/// 
			///     <td width="25%" >51　</td>
			/// 
			///     <td width="25%" >51　</td>
			/// 
			///     <td width="25%" >0　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Color53　</td>
			/// 
			///     <td width="25%" >153　</td>
			/// 
			///     <td width="25%" >51　</td>
			/// 
			///     <td width="25%" >0　</td>
			/// 
			///   </tr>
			/// <tr>
			///     <td width="25%" >Color54　</td>
			/// 
			///     <td width="25%" >153　</td>
			/// 
			///     <td width="25%" >51　</td>
			/// 
			///     <td width="25%" >102　</td>
			/// 
			///   </tr>
			/// 
			///   <tr>
			///     <td width="25%" >Color55　</td>
			/// 
			///     <td width="25%" >51　</td>
			/// 
			///     <td width="25%" >51　</td>
			/// 
			///     <td width="25%" >153　</td>
			/// 
			///   </tr>
			/// 
			/// 
			///   <tr>
			///     <td width="25%" >Color56　</td>
			/// 
			///     <td width="25%" >51　</td>
			/// 
			///     <td width="25%" >51　</td>
			/// 
			///     <td width="25%" >51　</td>
			/// 
			///   </tr>
			/// 
			/// 
			/// 
			/// </table>
			/// 
			///  </remarks>
			 virtual void ChangePalette(intrusive_ptr<Aspose::Cells::Systems::Drawing::Color> color , Aspose::Cells::Systems::Int32 index)=0;
			/// <summary>
			/// Checks if a color is in the palette for the spreadsheet.
			/// </summary>
			/// 
			/// <param name="color" >Color structure.</param>
			/// 
			/// <returns>Returns true if this color is in the palette. Otherwise, returns false</returns>
			/// 
			/// 
			 virtual bool IsColorInPalette(intrusive_ptr<Aspose::Cells::Systems::Drawing::Color> color)=0;
			/// <summary>
			///  Calculates the result of formulas
			///  </summary>
			/// 
			 virtual void CalculateFormula()=0;
			/// <summary>
			/// Calculates the result of formulas
			/// </summary>
			/// 
			/// <param name="ignoreError" >Indicates if hide the error in calculating formulas. The error may be unsupported function, external links, etc.</param>
			/// 
			/// 
			 virtual void CalculateFormula(bool ignoreError)=0;
			/// <summary>
			/// Calculates the result of formulas.
			/// </summary>
			/// <param name="ignoreError" >Indicates if hide the error in calculating formulas. The error may be unsupported function, external links, etc.</param>
			/// <param name="customFunction" >The custom formula calculation functions to extend the calculation engine.</param>
			 virtual void CalculateFormula(bool ignoreError , intrusive_ptr<Aspose::Cells::ICustomFunction> customFunction)=0;
			/// <summary>
			/// Calcualting formulas in this workbook.
			/// </summary>
			/// <param name="options" >Options for calculation</param>
			 virtual void 		CalculateFormula(intrusive_ptr<Aspose::Cells::ICalculationOptions> options)=0;
			/// <summary>
			/// Refreshes dynamic array formulas(spill into new range of neighboring cells according to current data)
			/// </summary>
			/// <param name="calculate" >Whether calculates and update cell values for those dynamic array formulas when
			/// the spilled range changes.</param>
			 virtual void RefreshDynamicArrayFormulas(bool calculate)=0;
			/// <summary>
			/// Refreshes dynamic array formulas(spill into new range of neighboring cells according to current data)
			/// </summary>
			/// <param name="calculate" >Whether calculates and updates cell values for those dynamic array formulas</param>
			/// <param name="copts" >The options for calculating formulas</param>
			 virtual void 		RefreshDynamicArrayFormulas(bool calculate , intrusive_ptr<Aspose::Cells::ICalculationOptions> copts)=0;
			/// <summary>
			///  Gets the default <see cref="IStyle" />
			///  object of the workbook.
			///  </summary>
			/// 
			/// <remarks>
			///  The DefaultStyle is useful to implement an IStyle for the whole Workbook. 
			///  </remarks>
			/// 
			/// <example>
			///  The following code creates and instantiates a new Workbook and sets a default <see cref="IStyle" />
			///  to it.
			///  <code>
			///  [C++]
			///  intrusive_ptr<IWorkbook> workbook = Factory::CreateIWorkbook();
			///  intrusive_ptr<IStyle> defaultStyle = workbook->GetDefaultIStyle();
			///  defaultStyle->GetIFont()->SetName(new String("Tahoma"));
			///  workbook->SetDefaultIStyle(defaultStyle);
			/// 
			///  </code>
			/// 
			///  </example>
			/// 
			/// 
			 virtual intrusive_ptr<Aspose::Cells::IStyle> 		GetDefaultIStyle()=0;
			/// <summary>
			///  Sets the default <see cref="IStyle" />
			///  object of the workbook.
			///  </summary>
			/// 
			/// <remarks>
			///  The DefaultStyle is useful to implement an IStyle for the whole Workbook. 
			///  </remarks>
			/// 
			/// <example>
			///  The following code creates and instantiates a new Workbook and sets a default <see cref="IStyle" />
			///  to it.
			///  <code>
			///  [C++]
			///  intrusive_ptr<IWorkbook> workbook = Factory::CreateIWorkbook();
			///  intrusive_ptr<IStyle> defaultStyle = workbook->GetDefaultIStyle();
			///  defaultStyle->GetIFont()->SetName(new String("Tahoma"));
			///  workbook->SetDefaultIStyle(defaultStyle);
			/// 
			///  </code>
			/// 
			///  </example>
			/// 
			/// 
			 virtual void 		SetDefaultIStyle(intrusive_ptr<Aspose::Cells::IStyle> value)=0;
			/// <summary>
			/// Find best matching Color in current palette.
			/// </summary>
			/// <param name="rawColor" >Raw color.</param>
			/// <returns>Best matching color.</returns>
			 virtual intrusive_ptr<Aspose::Cells::Systems::Drawing::Color> GetMatchingColor(intrusive_ptr<Aspose::Cells::Systems::Drawing::Color> rawColor)=0;
			/// <summary>
			/// Indicates if this spreadsheet is digitally signed.
			/// </summary>
			/// 
			/// 
			 virtual bool IsDigitallySigned()=0;
			/// <summary>
			/// Set Encryption Options.
			/// </summary>
			/// <param name="encryptionType" >The encryption type.</param>
			/// <param name="keyLength" >The key length.</param>
			 virtual void SetEncryptionOptions(Aspose::Cells::EncryptionType encryptionType , Aspose::Cells::Systems::Int32 keyLength)=0;
			/// <summary>
			/// Protects a workbook.
			/// </summary>
			/// <param name="protectionType" >Protection type.</param>
			/// <param name="password" >Password to protect the workbook.</param>
			 virtual void Protect(Aspose::Cells::ProtectionType protectionType , intrusive_ptr<Aspose::Cells::Systems::String> password)=0;
			/// <summary>
			/// Protects a shared workbook.
			/// </summary>
			/// <param name="password" >Password to protect the workbook.</param>
			 virtual void ProtectSharedWorkbook(intrusive_ptr<Aspose::Cells::Systems::String> password)=0;
			/// <summary>
			/// Unprotects a workbook.
			/// </summary>
			/// <param name="password" >Password to unprotect the workbook.</param>
			 virtual void Unprotect(intrusive_ptr<Aspose::Cells::Systems::String> password)=0;
			/// <summary>
			/// Unprotects a shared workbook.
			/// </summary>
			/// <param name="password" >Password to unprotect the workbook.</param>
			 virtual void UnprotectSharedWorkbook(intrusive_ptr<Aspose::Cells::Systems::String> password)=0;
			/// <summary>
			/// Indicates whether structure or window is protected with password.
			/// </summary>
			 virtual bool IsWorkbookProtectedWithPassword()=0;
			/// <summary>
			/// Removes VBA/macro from this spreadsheet.
			/// </summary>
			 virtual void RemoveMacro()=0;
			/// <summary>
			/// Gets the <see cref="IVbaProject" />
			///  in a spreadsheet.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Vba::IVbaProject> 		GetIVbaProject()=0;
			/// <summary>
			/// Removes digital signature from this spreadsheet.
			/// </summary>
			 virtual void RemoveDigitalSignature()=0;
			/// <summary>
			/// Accepts all tracked changes in the workbook. 
			/// </summary>
			 virtual void AcceptAllRevisions()=0;
			/// <summary>
			/// Indicates if this spreadsheet contains macro/VBA.
			/// </summary>
			 virtual bool HasMacro()=0;
			/// <summary>
			/// Gets if the workbook has any tracked changes
			/// </summary>
			 virtual bool HasRevisions()=0;
			/// <summary>
			/// Gets the current file name.
			/// </summary>
			/// <remarks>
			/// If the file is opened by stream and there are some external formula references,
			/// please set the file name.
			/// </remarks>
			 virtual intrusive_ptr<Aspose::Cells::Systems::String> GetFileName()=0;
			/// <summary>
			/// Sets the current file name.
			/// </summary>
			/// <remarks>
			/// If the file is opened by stream and there are some external formula references,
			/// please set the file name.
			/// </remarks>
			 virtual void SetFileName(intrusive_ptr<Aspose::Cells::Systems::String> value)=0;
			/// <summary>
			/// Gets a DataSorter object to sort data.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::IDataSorter> 		GetIDataSorter()=0;
			/// <summary>
			/// Removes all external links in the workbook.
			/// </summary>
			 virtual void RemoveExternalLinks()=0;
			/// <summary>
			/// Gets theme color.
			/// </summary>
			/// <param name="type" >The theme color type.</param>
			/// <returns>The theme color.</returns>
			 virtual intrusive_ptr<Aspose::Cells::Systems::Drawing::Color> GetThemeColor(Aspose::Cells::ThemeColorType type)=0;
			/// <summary>
			/// Sets the theme color
			/// </summary>
			/// <param name="type" >The theme color type.</param>
			/// <param name="color" >the theme color</param>
			 virtual void SetThemeColor(Aspose::Cells::ThemeColorType type , intrusive_ptr<Aspose::Cells::Systems::Drawing::Color> color)=0;
			/// <summary>
			/// Gets the theme name. 
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Systems::String> GetTheme()=0;
			/// <summary>
			/// Customs the theme.
			/// </summary>
			/// <param name="themeName" >The theme name</param>
			/// <param name="colors" >The theme colors</param>
			/// <remarks>
			/// The length of colors should be 12.
			/// <table class="dtTABLE"  cellspacing="0" >
			///  <tr>
			///    <td width="50%" ><font color="gray" ><b>Array index</b>
			/// </font>
			/// 　</td>
			/// 
			///    <td width="50%" ><font color="gray" ><b>Theme type</b>
			/// </font>
			/// 　</td>
			/// 
			///  </tr>
			/// 
			///  <tr>
			///    <td width="50%" ><font color="gray" ><b>0</b>
			/// </font>
			/// 　</td>
			/// 
			///    <td width="50%" ><font color="gray" ><b>Backgournd1</b>
			/// </font>
			/// 　</td>
			/// 
			///  </tr>
			/// 
			///  <tr>
			///    <td width="50%" ><font color="gray" ><b>1</b>
			/// </font>
			/// 　</td>
			/// 
			///    <td width="50%" ><font color="gray" ><b>Text1</b>
			/// </font>
			/// 　</td>
			/// 
			///  </tr>
			/// 
			///  <tr>
			///    <td width="50%" ><font color="gray" ><b>2</b>
			/// </font>
			/// 　</td>
			/// 
			///    <td width="50%" ><font color="gray" ><b>Backgournd2</b>
			/// </font>
			/// 　</td>
			/// 
			///  </tr>
			/// 
			///  <tr>
			///    <td width="50%" ><font color="gray" ><b>3</b>
			/// </font>
			/// 　</td>
			/// 
			///    <td width="50%" ><font color="gray" ><b>Text2</b>
			/// </font>
			/// 　</td>
			/// 
			///  </tr>
			/// 
			///  <tr>
			///    <td width="50%" ><font color="gray" ><b>4</b>
			/// </font>
			/// 　</td>
			/// 
			///    <td width="50%" ><font color="gray" ><b>Accent1</b>
			/// </font>
			/// 　</td>
			/// 
			///  </tr>
			/// 
			///  <tr>
			///    <td width="50%" ><font color="gray" ><b>5</b>
			/// </font>
			/// 　</td>
			/// 
			///    <td width="50%" ><font color="gray" ><b>Accent2</b>
			/// </font>
			/// 　</td>
			/// 
			///  </tr>
			/// 
			///  <tr>
			///    <td width="50%" ><font color="gray" ><b>6</b>
			/// </font>
			/// 　</td>
			/// 
			///    <td width="50%" ><font color="gray" ><b>Accent3</b>
			/// </font>
			/// 　</td>
			/// 
			///  </tr>
			/// 
			///  <tr>
			///    <td width="50%" ><font color="gray" ><b>7</b>
			/// </font>
			/// 　</td>
			/// 
			///    <td width="50%" ><font color="gray" ><b>Accent4</b>
			/// </font>
			/// 　</td>
			/// 
			///  </tr>
			/// 
			///  <tr>
			///    <td width="50%" ><font color="gray" ><b>8</b>
			/// </font>
			/// 　</td>
			/// 
			///    <td width="50%" ><font color="gray" ><b>Accent5</b>
			/// </font>
			/// 　</td>
			/// 
			///  </tr>
			/// 
			///  <tr>
			///    <td width="50%" ><font color="gray" ><b>9</b>
			/// </font>
			/// 　</td>
			/// 
			///    <td width="50%" ><font color="gray" ><b>Accent6</b>
			/// </font>
			/// 　</td>
			/// 
			///  </tr>
			/// 
			///  <tr>
			///    <td width="50%" ><font color="gray" ><b>10</b>
			/// </font>
			/// 　</td>
			/// 
			///    <td width="50%" ><font color="gray" ><b>Hyperlink</b>
			/// </font>
			/// 　</td>
			/// 
			///  </tr>
			/// 
			///  <tr>
			///    <td width="50%" ><font color="gray" ><b>11</b>
			/// </font>
			/// 　</td>
			/// 
			///    <td width="50%" ><font color="gray" ><b>Followed Hyperlink</b>
			/// </font>
			/// 　</td>
			/// 
			///  </tr>
			///  
			/// </table>
			/// </remarks>
			 virtual void CustomTheme(intrusive_ptr<Aspose::Cells::Systems::String> themeName , intrusive_ptr<Aspose::Cells::Systems::Array1D<Aspose::Cells::Systems::Drawing::Color*>> colors)=0;
			/// <summary>
			/// Copies the theme from another workbook.
			/// </summary>
			/// <param name="source" >Source workbook.</param>
			 virtual void 		CopyITheme(intrusive_ptr<Aspose::Cells::IWorkbook> source)=0;
			/// <summary>
			/// Indicates whether this workbook contains external links to other data sources.
			/// </summary>
			/// <returns>Whether this workbook contains external links to other data sources.</returns>
			 virtual bool HasExernalLinks()=0;
			/// <summary>
			/// If this workbook contains external links to other data source,
			/// Aspose.Cells will attempt to retrieve the latest data.
			/// </summary>
			/// <param name="externalWorkbooks" >
			/// External workbooks are referenced by this workbook.
			/// If it's null, we will directly open the external linked files..
			/// If it's not null, 
			/// we will check whether the external link in the array first;
			/// if not, we will open the external linked files again.
			/// </param>
			/// <remarks>
			/// If the method is not called before calculating formulas,
			/// Aspose.Cells will use the previous information(cached in the file);
			/// Please set ICellsHelper::GetStartupPath(), ICellsHelper::GetAltStartPath(), ICellsHelper::GetLibraryPath(). 
			/// And please set the FilePath of Workbook if this workbook is from a stream,
			/// otherwise Aspose.Cells could not get the external link full path sometimes.
			/// </remarks>
			/// 
			/// 
			 virtual void 		UpdateLinkedDataSource(intrusive_ptr<Aspose::Cells::Systems::Array1D<Aspose::Cells::IWorkbook*>> externalWorkbooks)=0;
			/// <summary>
			/// Returns a DocumentProperties collection that represents all the built-in document properties of the spreadsheet. 
			/// </summary>
			/// <remarks>A new property cannot be added to built-in document properties list. You can only get a built-in property and change its value.
			/// The following is the built-in properties name list:
			/// <p>Title</p>
			/// <p>Subject</p>
			/// <p>Author</p>
			/// <p>Keywords</p>
			/// <p>Comments</p>
			/// <p>Template</p>
			/// <p>Last Author</p>
			/// <p>Revision Number</p>
			/// <p>Application Name</p>
			/// <p>Last Print Date</p>
			/// <p>Creation Date</p>
			/// <p>Last Save Time</p>
			/// <p>Total Editing Time</p>
			/// <p>Number of Pages</p>
			/// <p>Number of Words</p>
			/// <p>Number of Characters</p>
			/// <p>Security</p>
			/// <p>Category</p>
			/// <p>Format</p>
			/// <p>Manager</p>
			/// <p>Company</p>
			/// <p>Number of Bytes</p>
			/// <p>Number of Lines</p>
			/// <p>Number of Paragraphs</p>
			/// <p>Number of Slides</p>
			/// <p>Number of Notes</p>
			/// <p>Number of Hidden Slides</p>
			/// <p>Number of Multimedia Clips</p>
			/// </remarks>
			/// <example><code>
			/// [C++]
			/// intrusive_ptr<Aspose::Cells::IWorkbook> workbook = Factory::CreateIWorkbook();
			/// intrusive_ptr<Aspose::Cells::Properties::IDocumentProperty> doc = 
			/// workbook->GetIWorksheets()->GetIBuiltInDocumentProperties()->GetObjectByIndex(new String("Author"));
			/// doc->SetValue(new String("John Smith"));
			/// 
			/// </code>
			/// </example>
			 virtual intrusive_ptr<Aspose::Cells::Properties::IBuiltInDocumentPropertyCollection> 		GetIBuiltInDocumentProperties()=0;
			/// <summary>
			/// Returns an ICustomDocumentPropertyCollection that represents all the custom document properties of the spreadsheet. 
			/// </summary>
			/// <example><code>
			/// [C++]
			/// intrusive_ptr<Aspose::Cells::IWorkbook> workbook = Factory::CreateIWorkbook();
			/// intrusive_ptr<Aspose::Cells::Properties::ICustomDocumentPropertyCollection> doc =
			/// 	workbook->GetIWorksheets()->GetICustomDocumentProperties();
			/// doc->AddLinkToContentProperty(new String("Checked by"), new String("Jane"));
			/// 
			/// </code>
			/// </example>
			 virtual intrusive_ptr<Aspose::Cells::Properties::ICustomDocumentPropertyCollection> 		GetICustomDocumentProperties()=0;
			/// <summary>
			/// Gets the file format.
			/// </summary>
			/// 
			/// 
			 virtual Aspose::Cells::FileFormatType GetFileFormat()=0;
			/// <summary>
			/// Sets the file format.
			/// </summary>
			/// 
			/// 
			 virtual void SetFileFormat(Aspose::Cells::FileFormatType value)=0;
			/// <summary>
			/// Gets the IContentTypePropertyCollection in the workbook.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::Properties::IContentTypePropertyCollection> 		GetIContentTypeProperties()=0;
			/// <summary>
			/// Gets the XML file that defines the Ribbon UI.
			/// </summary>
			/// 
			/// 
			 virtual intrusive_ptr<Aspose::Cells::Systems::String> GetRibbonXml()=0;
			/// <summary>
			/// Sets the XML file that defines the Ribbon UI.
			/// </summary>
			/// 
			/// 
			 virtual void SetRibbonXml(intrusive_ptr<Aspose::Cells::Systems::String> value)=0;
			/// <summary>
			/// Gets the Absolute path of the file.
			/// </summary>
			/// <remarks>
			/// Only used for external links.
			/// </remarks>
			 virtual intrusive_ptr<Aspose::Cells::Systems::String> GetAbsolutePath()=0;
			/// <summary>
			/// Sets the Absolute path of the file.
			/// </summary>
			/// <remarks>
			/// Only used for external links.
			/// </remarks>
			 virtual void SetAbsolutePath(intrusive_ptr<Aspose::Cells::Systems::String> value)=0;
			/// <summary>
			/// Imports an xml file into the workbook.
			/// </summary>
			/// <param name="url" >the path of the xml file.</param>
			/// <param name="sheetName" >the destination sheet name .</param>
			/// <param name="row" >the destination row of the xml.</param>
			/// <param name="col" >the destination column of the xml.</param>
			 virtual void ImportXml(intrusive_ptr<Aspose::Cells::Systems::String> url , intrusive_ptr<Aspose::Cells::Systems::String> sheetName , Aspose::Cells::Systems::Int32 row , Aspose::Cells::Systems::Int32 col)=0;
			/// <summary>
			/// Imports an xml file into the workbook.
			/// </summary>
			/// <param name="stream" >the xml file stream.</param>
			/// <param name="sheetName" >the destination sheet name .</param>
			/// <param name="row" >the destination row of the xml.</param>
			/// <param name="col" >the destination column of the xml.</param>
			 virtual void ImportXml(intrusive_ptr<Aspose::Cells::Systems::IO::Stream> stream , intrusive_ptr<Aspose::Cells::Systems::String> sheetName , Aspose::Cells::Systems::Int32 row , Aspose::Cells::Systems::Int32 col)=0;
			/// <summary>
			/// Export XML data.
			/// </summary>
			/// <param name="mapName" >name of the XML map that need to be exported</param>
			/// <param name="path" >the export path</param>
			 virtual void ExportXml(intrusive_ptr<Aspose::Cells::Systems::String> mapName , intrusive_ptr<Aspose::Cells::Systems::String> path)=0;
			/// <summary>
			/// Export XML data.
			/// </summary>
			/// <param name="mapName" >name of the XML map that need to be exported</param>
			/// <param name="stream" >the export stream</param>
			 virtual void ExportXml(intrusive_ptr<Aspose::Cells::Systems::String> mapName , intrusive_ptr<Aspose::Cells::Systems::IO::Stream> stream)=0;
			/// <summary>
			/// Sets digital signature to an spreadsheet file (Excel2007 and later).
			/// </summary>
			/// <param name="digitalSignatureCollection" />
			/// 
			/// <remarks>Only support adding Xmldsig Digital Signature</remarks>
			 virtual void 		SetIDigitalSignature(intrusive_ptr<Aspose::Cells::DigitalSignatures::IDigitalSignatureCollection> digitalSignatureCollection)=0;
			/// <summary>
			/// Adds digital signature to an OOXML spreadsheet file (Excel2007 and later).
			/// </summary>
			/// <param name="digitalSignatureCollection" />
			/// 
			/// <remarks>Only support adding Xmldsig Digital Signature to an OOXML spreadsheet file</remarks>
			 virtual void 		AddDigitalSignature(intrusive_ptr<Aspose::Cells::DigitalSignatures::IDigitalSignatureCollection> digitalSignatureCollection)=0;
			/// <summary>
			/// Gets digital signature from file.
			/// </summary>
			 virtual intrusive_ptr<Aspose::Cells::DigitalSignatures::IDigitalSignatureCollection> 		GetIDigitalSignature()=0;
			/// <summary>
			/// Removes personal information.
			/// </summary>
			 virtual void RemovePersonalInformation()=0;
			/// <summary>
			/// Performs application-defined tasks associated with freeing, releasing, or
			/// resetting unmanaged resources.
			/// </summary>
			 virtual void Dispose()=0;
public:
		public:
			virtual void add_ref() {++ref_count_;}
			virtual int release_ref() {return --ref_count_;}

	};
}
}
