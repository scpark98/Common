/*
 INI File Processor.
 Optimized INI text file processor for the embedded c++ system.
 Written by Huirak Lee (huirak.lee@gmail.com)
 Version 6.0.1, 2017-8-10

 Official Repository.
 https://bitbucket.org/mobileoff/ini

 Introduction.
 This toolkit was designed to achieve the best performance, and provide the convenience for managing the standard INI file.
 Not only best for the embedded operating system, but also good for the non embedded system.

 Copyright (C) Huirak Lee. All rights reserved.
 The copyright of this toolkit is guaranteed in accordance with the international copyright law.

 For the Personal or Educational or Open Source Project Use.
 You may use this whole program, or part of the codes, for the personal purpose or the educational purpose also open source projects under the following restrictions:
 This toolkit may be redistributed while keeping untouched whole codes and program informations, original author name, license agreement and revision history. Modified codes shall not be redistributed.
 Any modified or derived source codes will descend this license policy and the copyright, so it's ownership belongs to the original author.
 If you found bugs or any improvement idea, let the original author know that to enhance the code quality and to fix possible bugs.

 For the Commercial Use.
 Contact the original author if you want to use this toolkit under the commercial purpose.
 Even if you are using this toolkit that belongs to some open source projects used in commercial applications, it is subjected to the commercial licensing rules.
 
 Revision History.
 v6.0.1, 2017-08-10, huirak.lee, Corrected FlushFile so that use both fflush and fsync. Not save file if the contents unchanged. Fixed bug unable to reallocate string pool during the file loading. Enforced error handling while open a file and creating new item.
 v6.0.0, 2016-07-07, huirak.lee, Reallocating the string pool when it is out of space. Aborting loading file if there is no CRC checksum in spite of the checkCRC is true. Fixed valLen update missing bug. Added GetSectItemCount to get item count of specified section.
 v5.9.0, 2016-06-19, huirak.lee, Unlimited sect, key, and value length. Enhanced GetTimeStamp performance. Added ValidateFormat to validate character set and basic INI formatting. Changed CRC value format into string. Cygwin ports. FromString stops parsing when the string pool is out of space. Fixed fopen read option of the ValidateFile. Fixed app crash while saving empty INI file. Fixed first timestamp not updated in case of the time element is 0.
 v5.5.0, 2016-06-06, huirak.lee, Faster parsing speed for FromString, but source string will be touched. Removed dynamic allocation of sect, key, value in FromString. Processing dirty format ini. Added CreateNewItem. Reordering protected member variables and functions. Virtualized destructor ~Ini.
 v5.4.0, 2016-06-05, huirak.lee, Ported to Visual Studio 2015, WIN32 platform. Ini now have two license type: free and commercial. Separate test codes to the test-ini.cpp. EOL character will be automatically selected to the compiler platform. Logging function embeded to the Ini class.
 v5.3.5, 2016-06-04, huirak.lee, Fixed Stopwatch time miscalculated in the MINGW compiler.
 v5.3.4, 2016-06-03, huirak.lee, Fixed 'unsigned long' GetValueLong to 'long'. Add NDEBUG option for the release build recipe.
 v5.3.3, 2016-06-02, huirak.lee, Fixed FromString return value unchecked in the LoadFile.
 v5.3.2, 2016-06-01, huirak.lee, ValidateFile() logs 'No CRC checksum' error.
 v5.3.1, 2016-06-01, huirak.lee, Added StringNoCaseCompare for easily changing the string compare function.
 v5.3.0, 2016-06-01, huirak.lee, Added FromString(), ToString() to serialize from / to string.
 v5.2.0, 2016-06-01, huirak.lee, Added [ini] prefix and current time stamp to the logs. Added short log definitions. Renamed d_printf with ini_dprintf. Added GetLogLevel fcuntion.
 v5.1.0, 2016-06-01, huirak.lee, New GetValue & SetValue functions for the huge integer & floating types: long long, unsigned long long, long double.
 v5.0.0, 2016-05-31, huirak.lee, Moved CRC header in front of the file. Allowed empty section. Removed strict INI header checking. Allowed '#' to describe remark. Exiting LoadFile when there is no key to avoid crash. Enforced test functions. Refactored variable name, etc.
 v4.3.4, 2016-05-17, huirak.lee, TRACE has renamed d_printf. 
 v4.3.3, 2016-05-17, huirak.lee, Ported to MINGW32.
 v4.3.2, 2016-04-21, huirak.lee, Defined EOL character for the cross platform.
 v4.3.1, 2016-04-18, huirak.lee, Changed long to unsigned int of the CRC table, Fixed warnings under -Wall compile option.
 v4.3.0, 2016-03-01, huirak.lee, Fixed CRC checking pass hole. Fixed potential eternal parse loop in case of there is no CRC.
 v4.2.2, 2015-10-28, jungjun.lee, Fixed 'Time of check time of use' issue.
 v4.2.1, 2015-04-07, huirak.lee, Removed warning: variable 'footerSize' set but not used [-Wunused-but-set-variable]
 v4.2.0, 2014-11-26, huirak.lee, Added CheckFileIntegrity function to verify broken file after writing a file.
 v4.1.0, 2013-06-25, huirak.lee, Added length field on each key, value due to SaveFile function crash.
 v4.0.0, 2013-05-18, huirak.lee, Employed string pool. faster parsing(insertion) speed for the sorted file.
 v3.0.0, 2013-05-13, huirak.lee, Changed STL map to vector and binary search due to SIGSEGV in QNX platform.
 v2.5.0, 2013-02-08, huirak.lee, Improved buffer overrun safety with parsing, string compare. add debug function. easy set debug level.
 v2.4.0, 2013-02-06, huirak.lee, GetValueBuf, SetValueBuf added for easy buf handling.
 v2.3.0, 2012-11-26, huirak.lee, Optimized for faster reading & writing.
 v2.2.0, 2012-11-26, huirak.lee, Added key insurance check logic.
 v2.1.0, 2012-11-25, huirak.lee, Added footer for crc checking.
 v2.1.0, 2012-11-14, huirak.lee, Added SetValueStrBuf for buffer overrun safety.
 v2.0.0, 2012-11-14, huirak.lee, Added polymorphic functions of GetValue, SetValue.
 v1.9.0, 2012-11-12, huirak.lee, Added ByteArrayToHexString, HexStringToByteArray.
 v1.8.1, 2012-08-07, huirak.lee, Fixed first item omit bug.
 v1.8.0, 2012-08-07, huirak.lee, Added SetValue.
 v1.7.0, 2012-07-07, huirak.lee, Added GetValueRaw,SetValueRaw.
 v1.6.1, 2012-07-07, huirak.lee, Renamed GetValueDword as GetValueUInt, SetValue also.
 v1.6.0, 2012-03-20, huirak.lee, Ported to QNX 6.3.0
 v1.5.1, 2010-03-10, huirak.lee, Refactored FindFirstKey, FindNextKey.
 v1.5.0, 2010-02-22, huirak.lee, Supporting default value.
 v1.4.0, 2009-11-17, huirak.lee, Optimized performance.
 v1.3.0, 2009-11-12, huirak.lee, Optimized performance.
 v1.2.0, 2009-11-11, huirak.lee, Ported to Visual Studio 2005.
 v1.1.0, 2007-12-27, huirak.lee, Ported to Embedded Visual C++ 4.0, STL.
 v1.0.0, 2006-05-02, huirak.lee, Created.

 Known Bugs.
 * N/A

 To Do.
 * Support inline remarks feature. #like this.
 * Fix error while enabling the -effc++ compiler option.

 TBD.
 * Hash sections and keys and reuse it if new one is already in the string pool.
 * Lightweight mode : Don't allocate the memory for all contents, just index the ini file contents and search it. 
 * Set debug function of the caller.
 * Employ TDD.
 * Partially read, partially update the value, but concerning lower speed. 
*/
#pragma once

#include <stdio.h>
#include <string.h>//for gpp - 140103
#include <vector>
#include <string>

#define LOG_PREFIX "%s[INI]"

#define LOGD(fmt,...) Ini::Dprintf(Ini::Debug, LOG_PREFIX fmt, Ini::GetTimeStamp(), ##__VA_ARGS__)
#define LOGV(fmt,...) Ini::Dprintf(Ini::Verbose, LOG_PREFIX fmt, Ini::GetTimeStamp(), ##__VA_ARGS__)
#define LOGN(fmt,...) Ini::Dprintf(Ini::Normal, LOG_PREFIX fmt, Ini::GetTimeStamp(), ##__VA_ARGS__)
#define LOGE(fmt,...) Ini::Dprintf(Ini::Error, LOG_PREFIX fmt, Ini::GetTimeStamp(), ##__VA_ARGS__)

#ifdef NDEBUG
#undef LOGD
#define LOGD(fmt,...) ((void)0)
#endif

inline int StringNoCaseCompare(const char* sz1, const char* sz2, int maxlen) {
#if !defined(NDEBUG)
	if (sz1 == NULL || sz2 == NULL) {
		printf("SHOULD NOT FALLING HERE!\n");
		return 1;
	}
#endif
#if defined(WIN32)
	//error C4996: 'strnicmp': The POSIX name for this item is deprecated. Instead, use the ISO C and C++ conformant name: _strnicmp. See online help for details.
	return _strnicmp(sz1, sz2, maxlen);
#else
	return strncasecmp(sz1, sz2, maxlen);
#endif	
} 

class Ini
{
public:
	//Don't looks good, but seems no better way out.
	static const int maxSectKeyLen = 256;
protected:
	struct Item
	{
		const char* key;
		size_t keyLen;
		const char* val;
		size_t valLen;
		size_t valRoom;

		Item() : key(NULL), keyLen(0), val(NULL), valLen(0), valRoom(0) {
		}
		
		static struct CompareItem {
			bool operator() ( const char* key, const Item& item) const {
				return StringNoCaseCompare(key, item.key, maxSectKeyLen) < 0;
			}
			bool operator() ( const char* key, const char* key1) const {
				return StringNoCaseCompare(key, key1, maxSectKeyLen) < 0;
			}
			bool operator() ( const Item& item, const char* key) const {
				return StringNoCaseCompare(item.key, key, maxSectKeyLen) < 0;
			}
			bool operator() ( const Item& item, const Item& item1) const {
				return StringNoCaseCompare(item.key, item1.key, maxSectKeyLen) < 0;
			}
		} Compare;
	};

	typedef std::vector<Item> ItemList;

	struct Section
	{
		const char* key;
		size_t keyLen;
		ItemList items;

		Section() : key(NULL), keyLen(0) {
			items.reserve(100);
		}
		
		static struct CompareSection {
			bool operator() ( const char* key, const Section& s) const {
				return StringNoCaseCompare(key, s.key, maxSectKeyLen) < 0;
			}
			bool operator() ( const char* key, const char* key1) const {
				return StringNoCaseCompare(key, key1, maxSectKeyLen) < 0;
			}
			bool operator() ( const Section& s, const char* key) const {
				return StringNoCaseCompare(s.key, key, maxSectKeyLen) < 0;
			}
			bool operator() ( const Section& s, const Section& s1) const {
				return StringNoCaseCompare(s.key, s1.key, maxSectKeyLen) < 0;
			}
		} Compare;
	};

	typedef std::vector<Section> SectionList;	

	friend Item;
	friend Section;

	SectionList sects;
	SectionList::iterator lastParsedSection;
	SectionList::iterator lastFoundSectionFFS;
	SectionList::iterator lastFoundSectionFFK;
	ItemList::iterator lastFoundItemFFK;
	Section emptySection;//for gpp - 140103

	char* strPool;
	size_t sizPool;
	size_t remPool; //remaining pool size
	unsigned int posPool;

	char iniFileName[256];
	static int logLevel;

	bool contentsChanged;
	bool saveChangedFileOnly;

	int CreateItem(Item& newItem, const char* key, const char* val);
	const char* PushString(const char* s);
	SectionList::iterator FindSection(const char* sect);
	ItemList::iterator FindItem(const char* sect, const char*key);
public:
	// Life Cycle
	Ini(const int strPoolSize=64*1024);
	virtual ~Ini(void);
	bool LoadFile(const char* iniFileName, bool checkCRC=true);
	bool SaveFile(const char* iniFileName=NULL, bool writeCRC=true);
	void SetFileName(const char* iniFileName);
	const char* GetFileName();
	bool FromString(const char* buf, size_t buflen, bool sorted=false);
	std::string ToString();
	void Reset();
	static bool ValidateFile(const char* iniFileName);
	static bool ValidateFormat(const char* buf, size_t buflen);
	// Property
	inline int GetSectCount() {return sects.size();}
	int GetItemCount();
	int GetSectItemCount(const char* sect);
	size_t GetPoolRoom();
	// Search Functions
	bool IsSection(const char* sect);
	bool IsKey(const char* sect, const char* key);
	int FindFirstKey(const char* sect, const char** key, const char** val);
	int FindNextKey(const char** key, const char** val);
	const char* FindFirstSection();
	const char* FindNextSection();
	// Get Functions
	inline char GetValue(const char* sect, const char* key, char& val, char _default=0) {return(val = (char)GetValueInt(sect,key,_default));}
	inline unsigned char GetValue(const char* sect, const char* key, unsigned char& val, unsigned char _default=0) {return(val = (unsigned char)GetValueInt(sect,key,_default));}
	inline short GetValue(const char* sect, const char* key, short& val, char _default=0) {return(val = (short)GetValueInt(sect,key,_default));}
	inline unsigned short GetValue(const char* sect, const char* key, unsigned short& val, unsigned short _default=0) {return(val = (unsigned short)GetValueInt(sect,key,_default));}
	inline int GetValue(const char* sect, const char* key, int& val, int _default=0) {return(val = GetValueInt(sect,key,_default));}
	inline unsigned int GetValue(const char* sect, const char* key, unsigned int& val, int _default=0) {return(val = GetValueUInt(sect,key,_default));}
	inline long GetValue(const char* sect, const char* key, long& val, long _default=0) {return (val=GetValueLong(sect,key,_default));}
	inline unsigned long GetValue(const char* sect, const char* key, unsigned long& val, unsigned long _default=0) {return (val=GetValueULong(sect, key, _default));}
	inline bool GetValue(const char* sect, const char* key, bool& val, bool _default=0) {return(val = GetValueInt(sect,key,_default) ? true : false) ;}
	inline float GetValue(const char* sect, const char* key, float& val, float _default=0.0) {return(val = GetValueFloat(sect,key,_default));}
	inline double GetValue(const char* sect, const char* key, double& val, float _default=0.0) {return(val = GetValueDouble(sect,key,_default));}
	inline long double GetValue(const char* sect, const char* key, long double& val, long double _default=0.0) {return (val = GetValueLongDouble(sect,key,_default));}
	const char* GetValueStr(const char* sect, const char* key, const char* _default="");
	void GetValueStrBuf(const char* sect, const char* key, char* buf, size_t bufSize, const char* _default="");
	int GetValueInt(const char* sect, const char* key, int _default=0);
	unsigned int GetValueUInt(const char* sect, const char* key, unsigned int _default=0);
	long GetValueLong(const char* sect, const char* key, long _default=0);
	unsigned long GetValueULong(const char* sect, const char* key, unsigned long _default=0);
	float GetValueFloat(const char* sect, const char* key, float _default=0.0);
	double GetValueDouble(const char* sect, const char* key, double _default=0.0);
	inline long double GetValueLongDouble(const char* sect, const char* key, long double _default=0.0) {return GetValueDouble(sect,key,_default);}
	void GetValueRaw(const char* sect, const char* key, void* byteArray, const size_t byteArraySize, unsigned char _default=0x00);
	#define GetValueBuf(sect,key,buf) GetValueRaw(sect,key,&buf,sizeof(buf))
	// Set Functions
	inline void SetValue(const char* sect, const char* key, char val) {SetValueInt(sect, key, val);}
	inline void SetValue(const char* sect, const char* key, unsigned char val) {SetValueInt(sect, key, val);}
	inline void SetValue(const char* sect, const char* key, const char val[]) {SetValueStr(sect, key, val);} //+Fix SetValue("key","value") linked to SetValue(const char*,bool) - 160531
	//inline void SetValue(const char* sect, const char* key, const unsigned char* val,size_t size) {SetValueRaw(sect, key, val, size);}
	inline void SetValue(const char* sect, const char* key, short val) {SetValueInt(sect, key, val);}
	inline void SetValue(const char* sect, const char* key, unsigned short val) {SetValueInt(sect, key, val);}
	inline void SetValue(const char* sect, const char* key, int val) {SetValueInt(sect, key, val);}
	inline void SetValue(const char* sect, const char* key, unsigned int val) {SetValueUInt(sect, key, val);}
	inline void SetValue(const char* sect, const char* key, long val) {SetValueLong(sect, key, val);}
	inline void SetValue(const char* sect, const char* key, unsigned long val) {SetValueULong(sect, key, val);}
	inline void SetValue(const char* sect, const char* key, long long val) {SetValueLongLong(sect, key, val);}
	inline void SetValue(const char* sect, const char* key, unsigned long long val)	{SetValueULongLong(sect, key, val);}	
	inline void SetValue(const char* sect, const char* key, bool val) {SetValueUInt(sect, key, val);}
	inline void SetValue(const char* sect, const char* key, float val) {SetValueFloat(sect, key, val);}
	inline void SetValue(const char* sect, const char* key, double val) {SetValueDouble(sect, key, val);}
	inline void SetValue(const char* sect, const char* key, long double val) {SetValueLongDouble(sect, key, val);}
	int SetValueStr(const char* sect, const char* key, const char* val, bool sortedFile = false);
	void SetValueStrBuf(const char* sect, const char* key, char* buf, size_t bufSize);
	//TBD: void SetValueStrMulti(const char* sect, const char* key, const char* val);
	void SetValueInt(const char* sect, const char* key, int val);
	void SetValueUInt(const char* sect, const char* key, unsigned int val);
	void SetValueLong(const char* sect, const char* key, long val);
	void SetValueULong(const char* sect, const char* key, unsigned long val);
	void SetValueLongLong(const char* sect, const char* key, long long val);
	void SetValueULongLong(const char* sect, const char* key, unsigned long long val);
	void SetValueFloat(const char* sect, const char* key, float val);
	void SetValueDouble(const char* sect, const char* key, double val);
	inline void SetValueLongDouble(const char* sect, const char* key, long double val) {SetValueDouble(sect,key,val);}
	void SetValueRaw(const char* sect, const char* key, const void* buf, const size_t bufLen);
	#define SetValueBuf(sect,key,buf) SetValueRaw(sect,key,&buf,sizeof(buf))	
	//Helper func.
	static char* ByteArrayToHexString(const unsigned char* byteArray, size_t sizeArray);
	static int HexStringToByteArray(const char* hexString, unsigned char* byteArray, size_t sizeByteArray);
	void Dump(void);
	enum LogLevel {
		Debug = 0,
		Verbose = 1,
		Normal = 2,
		Error = 3,
	};
	static void SetLogLevel(int level) {logLevel = level;}
	static int GetLogLevel() {return logLevel;}
	static void Dprintf(int level, const char* fmt, ...);
	static const char* GetTimeStamp();
};