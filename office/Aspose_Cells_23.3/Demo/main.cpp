/*The unique head file need to be included*/
#include "Aspose.Cells.h"
#include <iostream>
using namespace std;

/*Folders to store source files and result files to demo*/
static StringPtr sourcePath = new String("sourceFile\\");
static StringPtr resultPath = new String("resultFile\\");

/*Check result*/
#define EXPECT_TRUE(condition) \
		if (condition) printf("--%s,line:%d -> Ok --\n", __FUNCTION__, __LINE__); \
			 else  printf("--%s,line:%d->>>> Failed!!!! <<<<--\n", __FUNCTION__, __LINE__);

/*To avoid memory leak£¬we use smart pointer "intrusive_ptr" of boost,each class variable should be defined as a intrusive_ptr object*/

void HelloWorld()
{
	/*create a new workbook*/
	intrusive_ptr<IWorkbook> wb = Factory::CreateIWorkbook();

	/*get the first worksheet*/
	intrusive_ptr<IWorksheetCollection> wsc = wb->GetIWorksheets();
	intrusive_ptr<IWorksheet> ws = wsc->GetObjectByIndex(0);

	/*get cell(0,0)*/
	intrusive_ptr<ICells> cells = ws->GetICells();
	intrusive_ptr<ICell> cell = cells->GetObjectByIndex(0, 0);

	/*write "Hello World" to cell(0,0) of the first sheet*/
	intrusive_ptr<String> str = new String("Hello World£¡");
	cell->PutValue(str);

	/*save this workbook to resultFile folder*/
	wb->Save(resultPath->StringAppend(new String("book1.xlsx")));
}

void ChangeValue()
{
	/*open an existing Workbook from sourceFile folder*/
	intrusive_ptr<IWorkbook> wb = Factory::CreateIWorkbook(sourcePath->StringAppend(new String("book.xlsx")));

	/*get the first worsheet*/
	intrusive_ptr<IWorksheetCollection> wsc = wb->GetIWorksheets();
	intrusive_ptr<IWorksheet> ws = wsc->GetObjectByIndex(0);

	/*get cell(0,0)*/
	intrusive_ptr<ICells> cells = ws->GetICells();
	intrusive_ptr<ICell> cell = cells->GetObjectByIndex(0, 0);

	/*modify the value of the first cell from 100 to 200 */
	cell->PutValue(200);

	/*check value*/
	EXPECT_TRUE(cell->GetIntValue() == 200);

	/*save this workbook to resultFile folder*/
	wb->Save(resultPath->StringAppend(new String("book2.xlsx")));
}

void ValueType()
{
	/*create a new workbook*/
	intrusive_ptr<IWorkbook> wb = Factory::CreateIWorkbook();

	/*get the first worksheet*/
	intrusive_ptr<IWorksheetCollection> wsc = wb->GetIWorksheets();
	intrusive_ptr<IWorksheet> ws = wsc->GetObjectByIndex(0);

	/*get cells*/
	intrusive_ptr<ICells> cells = ws->GetICells();

	/*get cell(0,0)*/
	intrusive_ptr<ICell> cell0 = cells->GetObjectByIndex(0, 0);

	/*set DatetTime value to cell(0,0)*/
	cell0->PutValue((DateTimePtr)new DateTime(2000, 1, 1));

	/*check value*/
	EXPECT_TRUE(cell0->GetDateTimeValue()->Equals((DateTimePtr)new DateTime(2000, 1, 1)));

	/*get cell(1,0)*/
	intrusive_ptr<ICell> cell1 = cells->GetObjectByIndex(1, 0);

	/*set string type value to cell(1,0)*/
	cell1->PutValue((StringPtr)new String("20000101"));

	/*check value*/
	EXPECT_TRUE(cell1->GetStringValue()->Equals((StringPtr)new String("20000101")));

	/*get cell(2,0)*/
	intrusive_ptr<ICell> cell2 = cells->GetObjectByIndex(2, 0);

	/*set double type value to cell(2,0)*/
	cell2->PutValue(20000101.01);

	/*check value*/
	EXPECT_TRUE(cell2->GetDoubleValue() == 20000101.01);

	/*get cell(3,0)*/
	intrusive_ptr<ICell> cell3 = cells->GetObjectByIndex(3, 0);

	/*set int type value to cell(3,0)*/
	cell3->PutValue(20000101);

	/*check value*/
	EXPECT_TRUE(cell3->GetIntValue() == 20000101);

	/*get cell(4,0)*/
	intrusive_ptr<ICell> cell4 = cells->GetObjectByIndex(4, 0);

	/*set bool type value to cell(4,0)*/
	cell4->PutValue(true);

	/*check value*/
	EXPECT_TRUE(cell4->GetBoolValue() == true);

	/*save this workbook to resultFile*/
	wb->Save(resultPath->StringAppend(new String("book3.xlsx")));
}

void SetStyle()
{
	/*create a new workbook and get the first worksheet*/
	intrusive_ptr<IWorkbook> wb = Factory::CreateIWorkbook();
	intrusive_ptr<IWorksheet> ws = wb->GetIWorksheets()->GetObjectByIndex(0);

	/*get cells style*/
	intrusive_ptr<IStyle> style = ws->GetICells()->GetIStyle();

	/*set font color*/
	style->GetIFont()->SetColor(Systems::Drawing::Color::GetGreen());

	/*set Background*/
	style->SetPattern(BackgroundType::BackgroundType_Gray12);
	style->SetBackgroundColor(Systems::Drawing::Color::GetRed());

	/*set Border*/
	style->SetBorder((BorderType_LeftBorder), CellBorderType_Thin, Systems::Drawing::Color::GetBlue());
	style->SetBorder((BorderType_RightBorder), CellBorderType_Double, Systems::Drawing::Color::GetGold());

	/*set string value to cell 'A1'*/
	intrusive_ptr<ICells> cells = ws->GetICells();
	intrusive_ptr<ICell> cell = cells->GetObjectByIndex(new String("A1"));
	cell->PutValue((StringPtr)new String("Text"));

	/*apply style to cell 'A1'*/
	cell->SetIStyle(style);

	/*save this workbook to resultFile*/
	wb->Save(resultPath->StringAppend(new String("book4.xlsx")));
}

void FormulaCalculate()
{
	/*create a new workbook*/
	intrusive_ptr<IWorkbook> wb = Factory::CreateIWorkbook();

	/*get the first worksheet*/
	intrusive_ptr<IWorksheetCollection> wsc = wb->GetIWorksheets();
	intrusive_ptr<IWorksheet> ws = wsc->GetObjectByIndex(0);

	/*get cells*/
	intrusive_ptr<ICells> cells = ws->GetICells();

	/*set value to cell(0,0) and cell(1,0)*/
	cells->GetObjectByIndex(0, 0)->PutValue(3);
	cells->GetObjectByIndex(1, 0)->PutValue(2);

	/*set formula*/
	cells->GetObjectByIndex(0, 1)->SetFormula(new String("=SUM(A1,A2)"));

	/*formula calculation*/
	wb->CalculateFormula();

	/*check result*/
	EXPECT_TRUE(5 == cells->GetObjectByIndex(new String("B1"))->GetIntValue());

	/*save this workbook to resultFile*/
	wb->Save(resultPath->StringAppend(new String("book5.xlsx")));
}

void bookwithChartTest()
{
	/*ope a workbook with chart*/
	intrusive_ptr<IWorkbook> wb = Factory::CreateIWorkbook(sourcePath->StringAppend(new String("bookwithChart.xlsx")));
	/*save this workbook to resultFile,you can see a chart while open the file with MS-Excel*/
	wb->Save(resultPath->StringAppend(new String("book6.xlsx")));
}
int main(int argc, char** argv)
{
	StringPtr info = ICellsHelper::GetVersion();
	Console::WriteLine(info);
	/*write "Hello World" to a cell*/
	HelloWorld();
	/*modify the value of a cell*/
	ChangeValue();
	/*put different type of value to cells */
	ValueType();
	/*set style on a cell*/
	SetStyle();
	/*formula calculation*/
	FormulaCalculate();
	/*open/save a workbook with chart*/
	bookwithChartTest();
}
