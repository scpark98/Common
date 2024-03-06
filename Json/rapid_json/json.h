#ifndef JSON_H_
#define JSON_H_

/*
* json_cpp는 3개의 파일로만 구성되어 간단하지만
* write시에 멤버들이 알파벳 순으로만 저장된다는 치명적 단점이 있다.
* 이 단점만 제외하면 json_cpp나 rapid_json 둘 다 좋으나 rapid_json을 권장.
* 
* Usage :
	//json.h와 json.cpp에서 참조하는 rapid_json/include 폴더의 파일들을 상대경로로 include했으므로
	//프로젝트 속성에서 굳이 추가 포함 디렉토리에 include 폴더를 추가할 필요는 없다.

	#include "../../Common/Json/rapid_json/json.h"
	...
	CString src = _T("{\"result\":true,\"user_id\":\"user9\",\"int_value\":12345,\"double_value\":3.141592,\"array\":[\"item1\",\"item2\"]}");
	
	Json json;
	json.parse(src);

	//read bool type
	bool b = json.doc["result"].GetBool();

	//assign bool type
	json.doc["result"] = false;

	//read array type
	rapidjson::Value& arr = json.doc["array"];
	for (int i = 0; i < arr.Size(); i++)
		TRACE(_T("arr[%d] = %s\n"), i, arr[i].GetCString());

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
	~Json() { doc.RemoveAllMembers(); }

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

	rapidjson::Value* get_member(std::string member, );

	//arr_name이라는 배열의 n번째 항목에서 member의 값을 리턴한다.
	rapidjson::Value* get_array_member(std::string arr_name, int n, std::string member);
	bool get_array_member(std::string arr_name, int n, std::string member, rapidjson::Value* value);

protected:
	void		traverse_rapid_json(const rapidjson::Value& oRoot, CString sKey, CString &result);
};

#endif