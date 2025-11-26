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

	//프로젝트의 include directory에 Common의 parent 경로를 추가하고(ex. D:\1.Projects_C++;)
	#include "Common/Json/rapid_json/json.h"
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
	json.doc["asdfsd"]; => assert fail when "asdfsd" member does not exist.
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

#include <afxwin.h>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
//#include "string.h"
#include "include/document.h"
#include "include/rapidjson.h"

//변수이름과 변수값을 json으로 저장하기 위해 정의.
//int a = 3;
//CString str = _T("abc");
//json.add_member(a);
//위와 같이 호출하면 json에 { "a": 3, }과 같이 입력된다.
//json.add_member(a, str);과 같이 가변인자로 구현하려 했으나
//가변인자로 줄 경우는 #v가 변수명으로 치환될 수 없으므로 개별적으로 하나 하나 추가해야 한다.
#define add_member(v) add_member_func(#v, v)

#define STREAMOUT(a)  #a << "(" << a << ")" << " "



#define PRINT1(a1) { std::stringstream stream;  \
stream << STREAMOUT(a1) << std::endl;\
TRACE(_T("%S"), stream.str().c_str());\
}

#define PRINT2(a1, a2) { std::stringstream stream;   \
stream << STREAMOUT(a1) << STREAMOUT(a2) << std::endl; \
TRACE(_T("%S"), stream.str().c_str());\
}

#define PRINT3(a1, a2, a3) { std::stringstream stream;    \
 stream << STREAMOUT(a1) << STREAMOUT(a2) << STREAMOUT(a3) << std::endl; \
TRACE(_T("%S"), stream.str().c_str());\
}

#define PRINT4(a1, a2, a3, a4) { std::stringstream stream;    \
 stream << STREAMOUT(a1) << STREAMOUT(a2) << STREAMOUT(a3) << STREAMOUT(a4) << std::endl; \
TRACE(_T("%S"), stream.str().c_str());\
}

#define PRINT5(a1, a2, a3, a4, a5) { std::stringstream stream;    \
 stream << STREAMOUT(a1) << STREAMOUT(a2) << STREAMOUT(a3) << STREAMOUT(a4) << STREAMOUT(a5) << std::endl; \
TRACE(_T("%S"), stream.str().c_str());\
}

#define EXPAND(X)  X

#define PP_NARG(...)   EXPAND( PP_NARG_(__VA_ARGS__, PP_RSEQ_N()) )
#define PP_NARG_(...)  EXPAND( PP_ARG_N(__VA_ARGS__) )
#define PP_ARG_N(_1, _2, _3, _4, _5, N, ...)  N
#define PP_RSEQ_N()    5, 4, 3, 2, 1, 0
#define FOO_(N)     PRINT##N
#define FOO_EVAL(N)  FOO_(N)
#define PRINT(...)  EXPAND( FOO_EVAL(EXPAND( PP_NARG(__VA_ARGS__) ))(__VA_ARGS__) )
 
class Json
{
public:
	//using Document = rapidjson::Document;
	//using SizeType = rapidjson::SizeType;
	//using Value = rapidjson::Value;

	//생성자에서 비어있는 구분이라도 넣어서 parse()해 줘야 doc가 생성되고 멤버를 넣는 작업이 가능해진다.
	Json()
	{
		parse(std::string("{}"));
	}

	~Json()
	{
		doc.RemoveAllMembers();
	}

	bool		parse(std::string sstr);
	bool		parse(CString str);

	//bool		read(std::string input_json);
	bool		load(CString input_json_file);

	//bool		write(std::string output_json);
	bool		save(CString output_json_file);

	//traverse로 구현된 TRACE 출력.
	CString		get_all_data();

	//pretty가 true이면 indent가 적용된 형식으로 리턴.
	CString		get_json_string(bool pretty = true);

	//어떤 멤버값을 읽어올 때 해당 멤버가 존재하지 않으면 exception이 발생하므로 이럴 경우 default_value를 리턴하도록 함수 추가.
	int			get_int(std::string member, int default_value = 0);
	int64_t		get_int64(std::string member, int64_t default_value = 0);
	uint64_t	get_uint64(std::string member, uint64_t default_value = 0);
	CString		get_CString(std::string member, CString default_value = _T(""));


	//위와 같이 모든 타입에 대해 wrapper 함수를 추가하지 않고 template을 쓰려했으나 컴파일 에러 발생함. 수정 중...
	template<typename T> T get(std::string member, T default_value)
	{
		if (!doc.HasMember(member))
			return default_value;

		if constexpr (std::is_same_v<T, CString>)
			return doc[member].GetCString();
		else if constexpr (std::is_same_v<T, std::string>)
			return doc[member].GetString();
		else if constexpr (std::is_same_v<T, bool>)
			return doc[member].GetBool();
		else if constexpr (std::is_same_v<T, int>)
			return doc[member].GetInt();
		else if constexpr (std::is_same_v<T, int64_t>)
			return doc[member].GetInt64();
		else if constexpr (std::is_same_v<T, uint64_t>)
			return doc[member].GetUint64();
		else if constexpr (std::is_same_v<T, double>)
			return doc[member].GetDouble();
		/*
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
		*/
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

	template<typename T> void parse_args(CString var_name, const T& arg)
	{
		if constexpr (std::is_same_v<T, int>)
			to_json_result.Format(_T("%s, %d, "), var_name, arg);
		else if constexpr (std::is_same_v<T, DWORD>)
			to_json_result.Format(_T("%s, %d, "), var_name, arg);
	}

	CString to_json_result;
	
	template<typename T> void add_member_func(std::string name, T value)
	{
		if constexpr (std::is_same_v<T, Gdiplus::Color>)
		{
			UINT cr = ((Gdiplus::Color)(value)).GetValue();
			doc.AddMember(rapidjson::Value(name, doc.GetAllocator()).Move(), cr, doc.GetAllocator());
		}
		else if constexpr (std::is_same_v<T, Gdiplus::Rect>)
		{
			Gdiplus::Rect r = (Gdiplus::Rect)value;
			rapidjson::Value ar(rapidjson::kArrayType);
			ar.PushBack(r.X, doc.GetAllocator());
			ar.PushBack(r.Y, doc.GetAllocator());
			ar.PushBack(r.Width, doc.GetAllocator());
			ar.PushBack(r.Height, doc.GetAllocator());
			doc.AddMember(rapidjson::Value(name, doc.GetAllocator()).Move(), ar, doc.GetAllocator());
		}
		else if constexpr (std::is_same_v<T, Gdiplus::RectF>)
		{
			Gdiplus::RectF r = (Gdiplus::RectF)value;
			rapidjson::Value ar(rapidjson::kArrayType);
			ar.PushBack(r.X, doc.GetAllocator());
			ar.PushBack(r.Y, doc.GetAllocator());
			ar.PushBack(r.Width, doc.GetAllocator());
			ar.PushBack(r.Height, doc.GetAllocator());
			doc.AddMember(rapidjson::Value(name, doc.GetAllocator()).Move(), ar, doc.GetAllocator());
		}
		else
		{
			doc.AddMember(rapidjson::Value(name, doc.GetAllocator()).Move(), value, doc.GetAllocator());
		}
	}

	template<typename... Args> CString to_json(Args... args)
	{
		//to_json_result.clear();
		(PARSE_ARGS(args), ...);
		return to_json_result;
		//이 코드는 심플하지만 기본 타입들만 가능한 듯 하다.
		/*
		std::ostringstream os;
		os << "[";
		((os << "\"" << args << "\"" << ","), ...);
		std::string s = os.str();
		if (s.back() == ',') s.pop_back();
		os.str("");
		return s + "]";
		*/
	}

	//array2:[{"name": "peter", "age": 21}, {"name": "mike", "age":24}]과 같이 항목과 값이 pair로 존재하는 array를
	//map으로 변환해준다. 단, 모든 필드값은 무조건 CString으로 강제 변환한다.
	//koino 프로젝트에서 사용한 Api::JsonToArray() 대체용.
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
	template <class T> T get_array_member(std::string arr_name, int n, std::string member, T default)
	{
		//if (typeid(T) == typeid(int))
		//	return doc[arr_name][n][member].GetInt();
		//else if (typeid(T) == typeid(CString))
		//	return doc[arr_name][n][member].GetCString();
		if (!doc[arr_name][n].HasMember(member))
			return default;

		TRACE(_T("typename = %S\n"), typeid(T).name());

		if constexpr (std::is_same_v<T, int>)
		{
			return doc[arr_name][n][member].GetInt();
		}
		else if constexpr (std::is_same_v<T, bool>)
		{
			return doc[arr_name][n][member].GetBool();
		}
		else if constexpr (std::is_same_v<T, CString>)
		{
			return doc[arr_name][n][member].GetCString();
		}
		else if constexpr (std::is_same_v<T, const TCHAR*>)
		{
			return doc[arr_name][n][member].GetCString();
		}
		else if constexpr (std::is_same_v<T, std::string>)
		{
			return doc[arr_name][n][member].GetString();
		}
		else if constexpr (std::is_same_v<T, const char*>)
		{
			return doc[arr_name][n][member].GetString();
		}
		else if constexpr (std::is_same_v<T, UINT>)
		{
			return doc[arr_name][n][member].GetUint();
		}
		else if constexpr (std::is_same_v<T, float>)
		{
			return doc[arr_name][n][member].GetFloat();
		}
		else if constexpr (std::is_same_v<T, double>)
		{
			return doc[arr_name][n][member].GetDouble();
		}

		return NULL;
	}

	bool		get_array_member(std::string arr_name, int n, std::string member, rapidjson::Value* value);

	void		traverse_rapid_json(const rapidjson::Value& oRoot, CString sKey, CString& result);

protected:
};

#endif