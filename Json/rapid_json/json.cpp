#include "json.h"

#include <sys/stat.h>
#include <fstream>
#include "../../Functions.h"

#ifdef UNICODE
#define CHARSET _T(",ccs=UTF-8")
#define __function__ __FUNCTIONW__
#else
#define CHARSET _T("")
#define __function__ __FUNCTION__
#endif

#pragma warning(disable : 4996)	//_CRT_SECURE_NO_WARNINGS

using std::string;
using rapidjson::Document;
using rapidjson::FileReadStream;
using rapidjson::FileWriteStream;
using rapidjson::PrettyWriter;

using namespace rapidjson;

bool Json::parse(char* str)
{
	doc.Parse<UTFType::kUTF8>(str);
	return !doc.HasParseError();
}

bool Json::parse(CString str)
{
	std::string sstr = CT2CA(str);
	//std::string sstr = CString2string(str);
	return parse(sstr);
}

bool Json::parse(std::string sstr)
{
	doc.Parse(sstr);
	return !doc.HasParseError();
}

bool Json::load(CString input_json_file)
{
	//return read(CT2CA(input_json));
	//위와 같이 한줄로 표현 안됨.

	//std::string sstr = CT2CA(input_json);
	//return read(sstr);

	FILE* fp = _tfopen(input_json_file, _T("rb"));
	if (!fp)
		return false;

	struct stat stat_buf;
	stat(CT2A(input_json_file), &stat_buf);
	int size = stat_buf.st_size;

	char* readBuffer = new char[size];
	FileReadStream readStream(fp, readBuffer, size);
	AutoUTFInputStream<unsigned, FileReadStream> eis(readStream);  // wraps bis into eis

	//EncodedInputStream<UTF16LE<>, FileReadStream> eis(readStream);
	bool result = !doc.ParseStream(eis).HasParseError();

	delete[] readBuffer;
	fclose(fp);

	return result;
}
/*
bool Json::read(std::string input_json)
{
	FILE* fp = NULL;
	errno_t err = fopen_s(&fp, input_json.c_str(), "rb");
	if (err) {
		return false;
	}

	struct stat stat_buf;
	stat(input_json.c_str(), &stat_buf);
	int size = stat_buf.st_size;

	char* readBuffer = new char[size];
	FileReadStream readStream(fp, readBuffer, size);
	bool result = !doc.ParseStream(readStream).HasParseError();

	delete[] readBuffer;
	fclose(fp);

	return result;
}
*/
bool Json::save(CString output_json_file)
{
	FILE* fp = _tfopen(output_json_file, _T("wb"));
	if (!fp)
		return false;

	char writeBuffer[10240];
	FileWriteStream writeStream(fp, writeBuffer, sizeof(writeBuffer));
	typedef AutoUTFOutputStream<unsigned, FileWriteStream> OutputStream;
	OutputStream(writeStream, UTFType::kUTF8, false);
	PrettyWriter<FileWriteStream> writer(writeStream);
	doc.Accept(writer);

	fclose(fp);

	return true;
}
/*
bool Json::write(std::string output_json)
{
	FILE* fp = NULL;

	errno_t err = ::fopen_s(&fp, output_json.c_str(), "wb");
	if (err) {
		return false;
	}

	char writeBuffer[10240];
	FileWriteStream writeStream(fp, writeBuffer, _countof(writeBuffer));
	PrettyWriter<FileWriteStream> writer(writeStream);
	doc.Accept(writer);

	fclose(fp);

	return true;
}
*/
CString Json::get_all_data()
{
	CString result;

	for (rapidjson::Value::ConstMemberIterator itr = doc.MemberBegin(); itr != doc.MemberEnd(); ++itr)
	{
		//CString sKey((LPSTR)itr->name.GetString(), strlen((LPSTR)itr->name.GetString()));
		//traverse_rapid_json(doc[itr->name.GetString()], sKey, result);
		CString sKey(itr->name.GetCString(), itr->name.GetCString().GetLength());
		traverse_rapid_json(doc[itr->name.GetString()], sKey, result);
	}

	TRACE(_T("all data = \n%s"), result);

	return result;
}

void Json::traverse_rapid_json(const rapidjson::Value& oRoot, CString sKey, CString &result)
{
	CString sDebugStr;
	switch (oRoot.GetType())
	{
		case kNullType:
		{
			sDebugStr.Format(_T("[%s]=null\n"), (LPCTSTR)sKey);
			OutputDebugString(sDebugStr);
			result += sDebugStr;
		}
		break;
		case kFalseType:
		case kTrueType:
		{
			sDebugStr.Format(_T("[%s]=%s\n"), (LPCTSTR)sKey, oRoot.GetBool() ? _T("true") : _T("false"));
			OutputDebugString(sDebugStr);
			result += sDebugStr;
		}
		break;
		case kStringType:
		{
			CString sValue;
			sValue = oRoot.GetCString();
			sDebugStr.Format(_T("[%s]=%s\n"), (LPCTSTR)sKey, (LPCTSTR)sValue);
			OutputDebugString(sDebugStr);
			result += sDebugStr;
		}
		break;
		case kNumberType:
		{
			if (oRoot.IsInt())
			{
				sDebugStr.Format(_T("[%s]=%d\n"), (LPCTSTR)sKey, oRoot.GetInt());
				OutputDebugString(sDebugStr);
				result += sDebugStr;
			}
			else if (oRoot.IsUint())
			{
				sDebugStr.Format(_T("[%s]=%u\n"), (LPCTSTR)sKey, oRoot.GetUint());
				OutputDebugString(sDebugStr);
				result += sDebugStr;
			}
			else if (oRoot.IsDouble())
			{
				sDebugStr.Format(_T("[%s]=%f\n"), (LPCTSTR)sKey, oRoot.GetDouble());
				OutputDebugString(sDebugStr);
				result += sDebugStr;
			}
			else if (oRoot.IsInt64())
			{
				sDebugStr.Format(_T("[%s]=%I64d\n"), (LPCTSTR)sKey, oRoot.GetInt64());
				OutputDebugString(sDebugStr);
				result += sDebugStr;
			}
			else if (oRoot.IsUint64())
			{
				sDebugStr.Format(_T("[%s]=%I64u\n"), (LPCTSTR)sKey, oRoot.GetUint64());
				OutputDebugString(sDebugStr);
				result += sDebugStr;
			}
		}
		break;
		case kObjectType:
		{
			sDebugStr.Format(_T("[%s]=Object\n"), (LPCTSTR)sKey);
			OutputDebugString(sDebugStr);
			result += sDebugStr;

			CString sPath;
			for (rapidjson::Value::ConstMemberIterator itr = oRoot.MemberBegin(); itr != oRoot.MemberEnd(); ++itr)
			{
				CString sName;
				sName = itr->name.GetCString();
				if (sKey != _T(""))
				{
					sPath.Format(_T("%s/%s"), (LPCTSTR)sKey, (LPCTSTR)sName);
				}
				else
				{
					sPath = sName;
				}

				traverse_rapid_json(oRoot[itr->name.GetString()], sPath, result);
			}
		}
		break;
		case kArrayType:
		{
			unsigned int nArrCnt = oRoot.Size();
			sDebugStr.Format(_T("[%s]=Array. count = %d\n"), (LPCTSTR)sKey, nArrCnt);
			OutputDebugString(sDebugStr);
			result += sDebugStr;

			CString sPath;
			for (unsigned int index = 0; index < nArrCnt; ++index)
			{
				sDebugStr.Format(_T("[%s][%d]"), (LPCTSTR)sKey, index);
				if (sKey != _T(""))
				{
					sPath.Format(_T("%s[%d]"), (LPCTSTR)sKey, index);
				}
				else
				{
					sPath.Format(_T("%d"), index);
				}

				traverse_rapid_json(oRoot[index], sPath, result);
			}
		}
		break;
	}
}

CString Json::get_json_string(bool pretty)
{
	StringBuffer buffer;

	if (pretty)
	{
		PrettyWriter<StringBuffer> writer(buffer);
		doc.Accept(writer);
	}
	else
	{
		Writer<StringBuffer> writer(buffer);
		doc.Accept(writer);
	}

	return buffer.GetCString();
}

/*
rapidjson::Value* Json::get_member(std::string member)
{
	if (!doc.HasMember(member))
		return NULL;

	return &doc[member];
}
*/

//rapidjson::Value* ar = json.doc["array_name"]; 과 같이 사용할 경우
//Debug모드에서는 해당 array가 존재하지 않으면 assert_fail이 발생하므로
//먼저 해당 array가 존재하는지 확인한 후 리턴하도록 함수 추가
rapidjson::Value* Json::get_array(std::string array_name)
{
	if (!doc.HasMember(array_name))
		return NULL;

	return &doc[array_name];
}

/*
rapidjson::Value& Json::get_array1(std::string array_name)
{
	if (!doc.HasMember(array_name))
		return rapidjson::Value;

	return doc[array_name];
}
*/

//arr_name이라는 배열의 n번째 항목에서 member의 값을 리턴한다.
/*
rapidjson::Value* Json::get_array_member(std::string arr_name, int n, std::string member)
{
	if (!doc[arr_name].IsArray())
		return NULL;

	if (n >= doc[arr_name].Size())
		return NULL;

	if (!doc[arr_name][n].HasMember(member))
		return NULL;

	return &doc[arr_name][n][member];
}
*/

/*
bool Json::get_array_member(std::string arr_name, int n, std::string member, rapidjson::Value* value)
{
	value = get_array_member(arr_name, n, member);

	return (value != NULL);
}
*/

//array2:[{"name": "peter", "age": 21}, {"name": "mike", "age":24}]과 같이 항목과 값이 pair로 존재하는 array를
//map으로 변환해준다. 단, 모든 필드값은 무조건 CString으로 강제 변환한다.
//koino 프로젝트에서 사용한 Api::JsonToArray() 대체용
//src는 CString, arr_name은 std::string으로 한 이유는 다음과 같다.
//src는 request후에 params.result라는 CString 타입의 json 데이터이며
//arr_name은 "objects" 또는 "data"와 같이 추출할 json 필드명이므로 호출할 때는 다음과 같이 호출하면 된다.
//json.array_to_map(params.result, "objects", &arr);
bool Json::array_to_map(CString src, std::string arr_name, std::vector<std::map<CString, CString>>* arr)
{
	arr->clear();

	if (parse(src) == false)
		return false;

	CString name;
	CString value;

	//우선 arr_name이라는 array가 존재하는지 검사해야 한다.
	if (!doc[arr_name.c_str()].IsArray())
		return false;


	rapidjson::Value& arr_pair = doc[arr_name];

	for (int i = 0; i < arr_pair.Size(); i++)
	{
		std::map<CString, CString> m;

		for (rapidjson::Value::ConstMemberIterator it = arr_pair[i].MemberBegin(); it != arr_pair[i].MemberEnd(); it++)
		{
			name = it->name.GetCString();

			switch (it->value.GetType())
			{
			//case kNullType:
			//	value = _T("");
			//	break;
			case kNumberType:
				value.Format(_T("%d"), it->value.GetInt());
				break;
			case kStringType:
				value.Format(_T("%s"), it->value.GetCString());
				break;
			default:
				value = _T("");
			}

			m.insert(std::pair<CString, CString>(name, value));
		}

		arr->push_back(m);
	}

	return true;
}

int	Json::get_int(std::string member, int default_value)
{
	if (!doc.HasMember(member))
		return default_value;

	return doc[member].GetInt();
}

int64_t	Json::get_int64(std::string member, int64_t default_value)
{
	if (!doc.HasMember(member))
		return default_value;

	return doc[member].GetInt64();
}

uint64_t Json::Json::get_uint64(std::string member, uint64_t default_value)
{
	if (!doc.HasMember(member))
		return default_value;

	return doc[member].GetUint64();
}

CString	Json::get_CString(std::string member, CString default_value)
{
	if (!doc.HasMember(member))
		return default_value;

	return doc[member].GetCString();
}
