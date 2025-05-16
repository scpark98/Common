#ifndef JSON_H_
#define JSON_H_

/*
* json_cpp는 3개의 파일로만 구성되어 간단하지만
* write시에 멤버들이 알파벳 순으로만 저장된다는 치명적 단점이 있다.
* 이 단점만 제외하면 json_cpp나 rapid_json 둘 다 좋으나 rapid_json을 사용하기로 결정함.
* 
* Usage :
	//json.h와 json.cpp에서 참조하는 rapid_json/include 폴더의 파일들을 상대경로로 include했으므로
	//프로젝트 속성에서 굳이 추가 포함 디렉토리에 include 폴더를 추가할 필요는 없다.

	#include "../../Common/Json/rapid_json/json.h"
	...
	//real json data
	{
		"result": false,
		"user_id": "홍길동",
		"int_value": 12345,
		"double_value": 3.141592,
		"person": {
			"name": "peter",
			"age": 21,
			"job": "student"
		},
		"array1": [
			"ar_item0",
			"ar_item1"
		]
	}

	CString src = _T("{\"result\":false,\"user_id\":\"홍길동\",\"int_value\":12345,\"double_value\":3.141592,\"person\": {\"name\":\"peter\",\"age\":21,\"job\":\"student\"},\"array1\":[\"ar_item0\",\"ar_item1\"]}");
	
	Json json;
	json.parse(src);

	//checking a member exists
	json.doc["asdfsd"]; => assert fail
	bool is_member = json.doc.HasMember("asdfsd");
	if (!is_member)
		TRACE(_T("asdfsd member not found.\n"));
	else
		TRACE(_T("asdfsd = %s\n"), json.doc["asdfsd"].GetCString());

	//read bool type value
	bool b = json.doc["result"].GetBool();

	//assign bool type value
	json.doc["result"] = false;

	//read objects member
	TRACE(_T("person : name = %s\n"), json.doc["person"]["name"].GetCString());

	//read array type
	rapidjson::Value& arr = json.doc["array1"];
	//ASSERT(arr.IsArray());
	for (int i = 0; i < arr.Size(); i++)
		TRACE(_T("array1[%d] = %s\n"), i, arr[i].GetCString());

*/

/*
* JsonCPP vs Rapid Json(https://joycecoder.tistory.com/9)
*/

#define RAPIDJSON_HAS_STDSTRING 1

#include <vector>
#include <map>
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

	//bool		read(std::string input_json);
	bool		load(CString input_json);

	//bool		write(std::string output_json);
	bool		save(CString output_json_file);

	//traverse로 구현된 TRACE 출력.
	CString		get_all_data();

	//pretty가 true이면 indent가 적용된 형식으로 리턴.
	CString		get_json_string(bool pretty = true);

	//어떤 멤버값을 읽어올 때 해당 멤버가 존재하지 않으면 exception이 발생하므로 이럴 경우 default_value를 리턴하도록 함수 추가.
	int			get_int(std::string member, int default_value = 0);
	CString		get_CString(std::string member, CString default_value = _T(""));

	//위와 같이 모든 타입에 대해 wrapper 함수를 추가하지 않고 template을 쓰려했으나 컴파일 에러 발생함. 수정 중...
	template<typename T> T get(std::string member, T default_value)
	{
		if (!doc.HasMember(member))
			return default_value;

		switch (doc[member].GetType())
		{
			//case rapidjson::kStringType:
			//{
			//	return dynamic_cast<CString>(doc[member].GetCString());
			//}
			case rapidjson::kNullType:
			{
				return default_value;
			}
			case rapidjson::kNumberType:
			{
				return (T)doc[member].GetInt();
			}
		}
		
		/*
		if (typeid(T) == typeid(int))
			return doc[member].GetInt();
		else if (typeid(T) == typeid(CString))
			return doc[member].GetCString();
		else if (typeid(T) == typeid(std::string))
			return doc[member].GetString();

		TRACE(_T("json::get. unknown type"));
		*/
		return default_value;
	}

	//array2:[{"name": "peter", "age": 21}, {"name": "mike", "age":24}]과 같이 항목과 값이 pair로 존재하는 array를
	//map으로 변환해준다. 단, 모든 필드값은 무조건 CString으로 강제 변환한다.
	//koino 프로젝트에서 사용한 Api::JsonToArray() 대체용
	//src는 CString, arr_name은 std::string으로 한 이유는 다음과 같다.
	//src는 request후에 params.result라는 CString 타입의 json 데이터이며
	//arr_name은 "objects" 또는 "data"와 같이 추출할 json 필드명이므로 호출할 때는 다음과 같이 호출하면 된다.
	//json.array_to_map(params.result, "objects", &arr);
	bool		array_to_map(CString src, std::string arr_name, std::vector<std::map<CString, CString>>* arr);

	//template을 이용해서 요청하는 타입으로 리턴하는 함수 작성 중...
	//해당 필드가 없을 경우 assert가 발생하는데 이 또한 미리 확인가능하지만 어떻게 처리할지...
	//CString str = json.read("person");
	//int num = json.read("person", "age");
#if 0
	rapidjson::Value* read(char* members)
	{
		return &doc[members];
		/*
		//char* member[] = { members... };
		if (doc[members].IsString())
			return doc[members].GetCString();
		else if (doc[members].IsInt())
			return doc[members].GetInt();
			*/
	}
#endif

	rapidjson::Document doc;

	//rapidjson::Value* get_member(std::string member, );

	//rapidjson::Value* ar = json.doc["array_name"]; 과 같이 사용할 경우
	//Debug모드에서는 해당 array가 존재하지 않으면 assert_fail이 발생하므로
	//먼저 해당 array가 존재하는지 확인한 후 리턴하도록 함수 추가
	rapidjson::Value* get_array(std::string array_name);
	rapidjson::Value& get_array1(std::string array_name);

	//arr_name이라는 배열의 n번째 항목에서 member의 값을 리턴한다.
	rapidjson::Value* get_array_member(std::string arr_name, int n, std::string member);
	bool		get_array_member(std::string arr_name, int n, std::string member, rapidjson::Value* value);

	void		traverse_rapid_json(const rapidjson::Value& oRoot, CString sKey, CString& result);

protected:
};

#endif