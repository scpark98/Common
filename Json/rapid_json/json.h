#ifndef JSON_H_
#define JSON_H_

/*
* json_cpp�� 3���� ���Ϸθ� �����Ǿ� ����������
* write�ÿ� ������� ���ĺ� �����θ� ����ȴٴ� ġ���� ������ �ִ�.
* �� ������ �����ϸ� json_cpp�� rapid_json �� �� ������ rapid_json�� ����.
* 
* Usage :
	//json.h�� json.cpp���� �����ϴ� rapid_json/include ������ ���ϵ��� ����η� include�����Ƿ�
	//������Ʈ �Ӽ����� ���� �߰� ���� ���丮�� include ������ �߰��� �ʿ�� ����.

	#include "../../Common/Json/rapid_json/json.h"
	...
	//real json data
	{
		"result": false,
		"user_id": "ȫ�浿",
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

	CString src = _T("{\"result\":false,\"user_id\":\"ȫ�浿\",\"int_value\":12345,\"double_value\":3.141592,\"person\": {\"name\":\"peter\",\"age\":21,\"job\":\"student\"},\"array1\":[\"ar_item0\",\"ar_item1\"]}");
	
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
	//ASSERT(a.IsArray());
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

	//traverse�� ������ TRACE ���.
	void		print();

	//pretty�� true�̸� indent�� ����� �������� ����.
	CString		get_json_string(bool pretty = true);

	//array2:[{"name": "peter", "age": 21}, {"name": "mike", "age":24}]�� ���� �׸�� ���� pair�� �����ϴ� array��
	//map���� ��ȯ���ش�. ��, ��� �ʵ尪�� ������ CString���� ���� ��ȯ�Ѵ�.
	//koino ������Ʈ���� ����� Api::JsonToArray() ��ü��
	//src�� CString, arr_name�� std::string���� �� ������ ������ ����.
	//src�� request�Ŀ� params.result��� CString Ÿ���� json �������̸�
	//arr_name�� "objects" �Ǵ� "data"�� ���� ������ json �ʵ���̹Ƿ� ȣ���� ���� ������ ���� ȣ���ϸ� �ȴ�.
	//json.array_to_map(params.result, "objects", &arr);
	bool		array_to_map(CString src, std::string arr_name, std::vector<std::map<CString, CString>>* arr);

	//template�� �̿��ؼ� ��û�ϴ� Ÿ������ �����ϴ� �Լ� �ۼ� ��...
	//�ش� �ʵ尡 ���� ��� assert�� �߻��ϴµ� �� ���� �̸� Ȯ�ΰ��������� ��� ó������...
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

	//arr_name�̶�� �迭�� n��° �׸񿡼� member�� ���� �����Ѵ�.
	rapidjson::Value* get_array_member(std::string arr_name, int n, std::string member);
	bool get_array_member(std::string arr_name, int n, std::string member, rapidjson::Value* value);

protected:
	void		traverse_rapid_json(const rapidjson::Value& oRoot, CString sKey, CString &result);
};

#endif