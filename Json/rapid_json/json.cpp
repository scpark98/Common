#include "json.h"

#include <sys/stat.h>
#include <fstream>
#include "include/filereadstream.h"
#include "include/filewritestream.h"
#include "include/prettywriter.h"

using std::string;
using rapidjson::Document;
using rapidjson::FileReadStream;
using rapidjson::FileWriteStream;
using rapidjson::PrettyWriter;

using namespace rapidjson;

bool Json::parse(CString str)
{
	std::string sstr = CT2CA(str);
	return parse(sstr);
}

bool Json::parse(std::string sstr)
{
	doc.Parse(sstr);
	return !doc.HasParseError();
}

bool Json::read(CString input_json)
{
	//return read(CT2CA(input_json));
	//위와 같이 한줄로 표현 안됨.

	std::string sstr = CT2CA(input_json);
	return read(sstr);
}

bool Json::read(std::string input_json)
{
	FILE* fp = NULL;
	errno_t err = ::fopen_s(&fp, input_json.c_str(), "rb");
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

bool Json::write(CString output_json)
{
	std::string sstr = CT2CA(output_json);
	return write(sstr);
}

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

void Json::print()
{
	CString result;

	for (rapidjson::Value::ConstMemberIterator itr = doc.MemberBegin(); itr != doc.MemberEnd(); ++itr)
	{
		//CString sKey((LPSTR)itr->name.GetString(), strlen((LPSTR)itr->name.GetString()));
		//traverse_rapid_json(doc[itr->name.GetString()], sKey, result);
		CString sKey(itr->name.GetCString(), itr->name.GetCString().GetLength());
		traverse_rapid_json(doc[itr->name.GetString()], sKey, result);
	}

	TRACE(_T("[result]\n%s"), result);
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
				sDebugStr.Format(_T("[%s]=%d\n"), (LPCTSTR)sKey, oRoot.GetUint());
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
			}
			else if (oRoot.IsUint64())
			{
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
			sDebugStr.Format(_T("[%s]=Array\n"), (LPCTSTR)sKey);
			OutputDebugString(sDebugStr);
			result += sDebugStr;

			unsigned int nArrCnt = oRoot.Size();

			CString sPath;
			for (unsigned int index = 0; index < nArrCnt; ++index)
			{
				sDebugStr.Format(_T("[%s][%d]"), (LPCTSTR)sKey, index);
				if (sKey != _T(""))
				{
					sPath.Format(_T("%s/%d"), (LPCTSTR)sKey, index);
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

CString Json::get_string(bool pretty)
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