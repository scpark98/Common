/*
 Sample Test Code for INI File Processor.
 Optimized INI text file processor for the embedded c++ system.
 Written by Huirak Lee (huirak.lee@gmail.com)
*/

#if defined (__MINGW32__)
#include <sys/time.h>
#elif defined(WIN32)
#include <windows.h>
#else
#include <time.h>
#endif
#include <errno.h>
#include "ini.h"

//mode=0 stop
//mode=1 start
//mode=2 lab	TBD..
double
Stopwatch(int mode, const char* msg=NULL)
{
#if defined (__MINGW32__)
	static timeval start;
	static timeval stop;
#elif defined (WIN32)
	static SYSTEMTIME start;
	static SYSTEMTIME stop;
#else
	static struct timespec start;
	static struct timespec stop;
	const long BILLION=1000000000L;	
#endif
	double accum = 0.0;

	if (mode==0) {//STOP
#if defined __MINGW32__
		gettimeofday(&stop,NULL);
	    accum = ( stop.tv_sec - start.tv_sec ) +
	             (double)( stop.tv_usec - start.tv_usec ) / (double)1000 / 1000;
#elif defined WIN32
		GetSystemTime(&stop);
		accum = (stop.wSecond - start.wSecond)
			+ (double)(stop.wMilliseconds - start.wMilliseconds) / 1000;
#else
	    if( clock_gettime(CLOCK_MONOTONIC, &stop) == -1 ) {
	    	perror( "clock gettime" );
	    	return -1;
	    }
	    accum = ( stop.tv_sec - start.tv_sec )
	             + (double)( stop.tv_nsec - start.tv_nsec )/ (double)BILLION;
#endif
	    if (msg!=NULL) {
			LOGN("%s takes %0.3lf seconds.\n", msg,  accum);
		} else {
			LOGN("It takes %lf seconds.\n", accum );
		}
	    return accum;
	} else if (mode==1) {//START
		if (msg!=NULL) {
			LOGN( "Start %s\n", msg);
		} else {
			LOGN( "Start Stopwatch\n");
		}
#ifdef __MINGW32__
		gettimeofday(&start,NULL);
#elif defined WIN32
		GetSystemTime(&start);
#else	
		if( clock_gettime(CLOCK_MONOTONIC, &start) == -1 ) {
			perror( "clock gettime" );
			return -1;
		}
#endif
	} else if (mode==2) {
		//TBD
		return -1;
	}
	return -1;
}

void CreateTestSet(Ini &ini, int sectSize, int itemSize)
{
	for(int i=0; i<sectSize; i++) {
		char sect[Ini::maxSectKeyLen];
		snprintf(sect, Ini::maxSectKeyLen, "sect%d", i);
		for(int j=0; j<itemSize; j++) {
			char key[Ini::maxSectKeyLen];
			snprintf(key, Ini::maxSectKeyLen, "key%d", j);
			char val[Ini::maxSectKeyLen];
			snprintf(val, Ini::maxSectKeyLen, "val%d", j);
			ini.SetValueStr(sect, key, val);
		}
	}	
}

void CreateTestFile(const char* fileName)
{
	Ini ini(2*1024*1024);

	CreateTestSet(ini,100,1000);
	ini.SaveFile(fileName);
}

//////////////////////////////////////
// Benchmark Results.
// Platform : i.MX6 solo
//
// Write/Read 10000 items to/from flash with vbuf
// Elapsed Time for Saving =0.272
// Elapsed Time for Parsing=0.054
//
// Write/Read 10000 items to/from ramfs with vbuf
// Elapsed Time for Saving =0.035
// Elapsed Time for Parsing=0.048

void TestBenchmarks()
{
	LOGN( "<<%s>>\n",__FUNCTION__);

	Ini::SetLogLevel(Ini::Normal);
	Ini ini(2*1024*1024);

	CreateTestSet(ini,100,1000);

	const char* path="test-benchmarks.ini";

	Stopwatch(1,"SaveFile");
	ini.SaveFile(path);
	Stopwatch(0,"SaveFile");	

	ini.Reset();

	Stopwatch(1,"LoadFile");
	ini.LoadFile(path);
	Stopwatch(0,"LoadFile");
}

void TestLoadFile()
{
	LOGN( "<<%s>>\n",__FUNCTION__);

	Ini::SetLogLevel(Ini::Normal);
	Ini ini(2*1024*1024);

	const char* path="test-benchmarks.ini";	
	ini.LoadFile(path);	
}

void TestLoadFileWithWrongFile()
{
	LOGN( "<<%s>>\n",__FUNCTION__);

	Ini::SetLogLevel(Ini::Debug);
	Ini ini(512);

	const char* path="test-ini.exe";		
	ini.LoadFile(path);	
}

void TestValidateFile()
{
	LOGN( "<<%s>>\n",__FUNCTION__);

	Ini::SetLogLevel(Ini::Normal);
	const char* testFileName = "test-crc.ini";
	CreateTestFile(testFileName);

	Ini::SetLogLevel(Ini::Debug);
	if (Ini::ValidateFile(testFileName)) {
		LOGN("CRC checksum ok!\n");
	}

	FILE *fp = fopen(testFileName,"a+b");
	if (fp) {
		if (-1==fseek(fp, -2, SEEK_END)) {
			LOGE("fseek fail : %s", strerror(errno));
		}
		LOGN("Corrupt at the offset %d of the file\n",ftell(fp));
		unsigned char dirt = 0xFF;
		if (fwrite(&dirt, sizeof(char), 1, fp) < 1) {
			LOGE("fwrite fail : %s", strerror(errno));
		}
		fclose(fp);
	}
	if (Ini::ValidateFile(testFileName)) {
		LOGN("CRC checksum ok!\n");
	}
}


void TestFindFunctions()
{
	LOGN("<<%s>>\n",__FUNCTION__);

	Ini ini;

	CreateTestSet(ini,5,5);

	Ini::SetLogLevel(Ini::Debug);

	LOGN("Lookup all sect\n");

	const char* sect=ini.FindFirstSection();
	while(sect) {
		LOGN("'%s'\n",sect);
		sect = ini.FindNextSection();
	}

	LOGN("Lookup [sect3]\n");

	const char *key=NULL;
	const char *val=NULL;
	
	if (ini.FindFirstKey("sect3",&key,&val)) {
		LOGN("FindFirstKey %s=%s\n",key,val);
		while(ini.FindNextKey(&key, &val)) {
			LOGN("FindNextKey %s=%s\n",key,val);
		} 
	}

	ini.FindNextKey(&key,&val);
	ini.FindNextKey(&key,&val);
	ini.FindNextKey(&key,&val);
}

void TestSetValueFunctions()
{
	LOGN( "<<%s>>\n",__FUNCTION__);

	Ini ini;
	Ini::SetLogLevel(Ini::Debug);

	ini.SetValueStr("sect0","SetValueStr","Set by SetValueStr");
	ini.SetValue("sect1","SetValue","It should be linked to SetValueStr");

	char c = 127;
	ini.SetValue("DataType","char_127",c);

	unsigned char uc = 0xFF;
	ini.SetValue("DataType","uchar_0xFF",uc);

	short s = 32767;
	ini.SetValue("DataType","short_32767",s);

	unsigned short us = 65535;
	ini.SetValue("DataType","ushort_65535",us);

	int i = -2147483647;
	ini.SetValue("DataType","int_-2147483647",i);

	unsigned int ui = 0xFFFFFFFF;
	ini.SetValue("DataType","uint_0xFFFFFFFF",ui);

	//long l = -2147483648;//mingw32-gcc: ../ini.cpp:1581:2: error: this decimal constant is unsigned only in ISO C90 [-Werror]
	long l = -2147483647;
	ini.SetValue("DataType","long_-2147483647",l);

	l = 2147483647;
	ini.SetValue("DataType","long_2147483647",l);

	unsigned long ul = 0xFFFFFFFF;
	ini.SetValue("DataType","ulong_0xFFFFFFFF",ul);

	//long long ll = -9223372036854775808; //mingw32-gcc: ../ini.cpp:1610:18: error: integer constant is so large that it is unsigned [-Werror]
	long long ll = -9223372036854775807;
	ini.SetValue("DataType","longlong_-9223372036854775807",ll);

	ll = 9223372036854775807;
	ini.SetValue("DataType","longlong_9223372036854775807",ll);

	//unsigned long long ull = 18446744073709551615;//mingw32-gcc: ../ini.cpp:1617:27: error: integer constant is so large that it is unsigned [-Werror]
	unsigned long long ull = 1844674407370955169;
	ini.SetValue("DataType","ulonglong_1844674407370955169",ull);

	bool b = false;
	ini.SetValue("DataType","bool_false",b);

	float f = 100.0;
	ini.SetValue("DataType","float_100.0",f);

	double d = 200.0;
	ini.SetValue("DataType","double_200.0",d);

	long double ld = 300.0;
	ini.SetValue("DataType","long_double_300.0",ld);

	char buf[256];
	for(unsigned int i=0; i<sizeof(buf); i++) {
		buf[i] = (char)i;
	}
	ini.SetValueBuf("DataType","buf",buf);

	ini.SaveFile("test-setvalue.ini");

	ini.Reset();
	Ini::SetLogLevel(Ini::Normal);

	ini.LoadFile("test-setvalue.ini");
	ini.Dump();
}

void TestEmptySection()
{
	LOGN("<<%s>>\n",__FUNCTION__);

	Ini ini;
	Ini::SetLogLevel(Ini::Debug);

	CreateTestSet(ini,3,3);	
	for (int i=0; i<5; i++) {
		char key[10];
		snprintf(key,sizeof(key),"key%d",i);
		ini.SetValue("",key,i);
		ini.GetValueStr("",key);
	}
	ini.SaveFile("test-empty-sect.ini");
	ini.Reset();
	ini.LoadFile("test-empty-sect.ini");
	ini.Dump();
}

void TestToString()
{
	LOGN("<<%s>>\n",__FUNCTION__);

	Ini ini;
	Ini::SetLogLevel(Ini::Debug);

	CreateTestSet(ini,3,3);

	std::string str = ini.ToString();
	LOGN("%s\n", str.c_str());
}

void TestSquareBracket()
{
	LOGN("<<%s>>\n", __FUNCTION__);

	Ini ini;
	Ini::SetLogLevel(Ini::Debug);

	ini.SetValue("sect[0]", "key[0]", "val[0]");
	ini.SaveFile("test-bracket.ini");
	ini.LoadFile("test-bracket.ini");
}

void TestDirtyIni()
{
	const char dirty[] = {
		"    [sect0   ]   \n"
		"[    sect1   ]\r"
		"key0=   val0     z\r\n"
		"    key1 =  val1\n"
		"; key2 =  val2\n"
		"    #key3 = val3\n"
		" key4 = val4   #remarks\n"
		"    = val4   #no key!\n"
	};
	LOGN("<<%s>>\n", __FUNCTION__);

	Ini ini;
	Ini::SetLogLevel(Ini::Debug);
	ini.FromString(dirty,sizeof(dirty));
	ini.SaveFile("test-dirty.ini");
	ini.LoadFile("test-dirty.ini");
}

void TestGetTimeStampBenchmark()
{
	//Intel i7-3770, CYGWIN, -fstack-protector-all
	//  snprintf takes 0.583 secs
	//  hardcode takes 0.079 secs
	//Intel i7-3770, MINGW, -fstack-protector-all
	//  snprintf takes 0.084 secs
	//  hardcode takes 0.083 secs
	//Intel i7-3770, VS2015
	//  snprintf takes 0.076 secs
	//  hardcode takes 0.083 secs
	Stopwatch(1,"GetTimeStamp");
	for(int i=0; i<1000000; i++) {
		Ini::GetTimeStamp();
	}
	Stopwatch(0,"GetTimeStamp");
}

void TestReallocStrPool()
{
	LOGN("<<%s>>\n", __FUNCTION__);

	Ini ini;
	Ini::SetLogLevel(Ini::Normal);
	CreateTestSet(ini, 100, 1000);
	ini.SaveFile("test-realloc.ini");
}

void TestSaveContentsChangedFileOnly()
{
	LOGN("<<%s>>\n", __FUNCTION__);

	Ini ini;
	Ini::SetLogLevel(Ini::Normal);
	CreateTestSet(ini, 10, 10);
	ini.SaveFile("test-contents-changed.ini");
	ini.SaveFile("test-contents-changed.ini");
}

int main()
{
	Ini ini;
	ini.LoadFile("dsm_config.ini", false);
	printf( "%d\n", ini.GetValueInt("driver_info", "distance", 123) );
	ini.SetValueInt("driver_info", "distance", 456);
	ini.SaveFile("dsm_config.ini", false);

	return 0;
	TestGetTimeStampBenchmark();
	TestBenchmarks();
	TestLoadFile();
	TestLoadFileWithWrongFile();
	TestValidateFile();
	TestSetValueFunctions();
	TestFindFunctions();
	TestEmptySection();
	TestToString();
	TestSquareBracket();
	TestDirtyIni();
	TestReallocStrPool();
	TestSaveContentsChangedFileOnly();
	return 0;
}