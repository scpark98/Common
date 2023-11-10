#ifndef JSON_H_
#define JSON_H_

/*
* Usage :
	//json.h와 json.cpp에서 참조하는 rapid_json/include 폴더의 파일들을 상대경로로 include했으므로
	//프로젝트 속성에서 굳이 추가 포함 디렉토리에 include 폴더를 추가할 필요는 없다.

	#include "../../Common/Json/rapid_json/json.h"
	...
	CString src = _T("{\"result\":true,\"user_id\":\"user9\",\"int_value\":12345,\"double_value\":3.141592,\"array\":[\"item1\",\"item2\"]}");
	
	Json json;
	json.parse(src);
	bool b = json.doc["result"].GetBool();
	json.doc["result"] = false;
*/

/*
* JsonCPP vs Rapid Json(https://joycecoder.tistory.com/9)
*/

#define RAPIDJSON_HAS_STDSTRING 1

#include <string>
#include <afxwin.h>
#include "string.h"
#include "include/document.h"
#include "include/rapidjson.h"

class Json
{
public:
	//using Document = rapidjson::Document;
	//using SizeType = rapidjson::SizeType;
	//using Value = rapidjson::Value;

	Json() {}
	~Json() {}

	bool		parse(std::string sstr);
	bool		parse(CString str);

	bool		read(std::string input_json);
	bool		read(CString input_json);

	bool		write(std::string output_json);
	bool		write(CString output_json);

	//traverse로 구현된 TRACE 출력.
	void		print();

	//pretty가 true이면 indent가 적용된 형식으로 리턴.
	CString		get_string(bool pretty);

	rapidjson::Document doc;

protected:
	void		traverse_rapid_json(const rapidjson::Value& oRoot, CString sKey, CString &result);
};

#endif