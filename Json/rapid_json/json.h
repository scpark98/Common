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

	//traverse�� ������ TRACE ���.
	void		print();

	//pretty�� true�̸� indent�� ����� �������� ����.
	CString		get_string(bool pretty);

	rapidjson::Document doc;

	rapidjson::Value* get_member(std::string member, );

	//arr_name�̶�� �迭�� n��° �׸񿡼� member�� ���� �����Ѵ�.
	rapidjson::Value* get_array_member(std::string arr_name, int n, std::string member);
	bool get_array_member(std::string arr_name, int n, std::string member, rapidjson::Value* value);

protected:
	void		traverse_rapid_json(const rapidjson::Value& oRoot, CString sKey, CString &result);
};

#endif