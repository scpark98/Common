#pragma once

#include <string>
#include <vector>

namespace Gdiplus { class Bitmap; }

//Windows.Media.Ocr (OS 내장, Win10+) 로 이미지의 텍스트를 인식해 반환.
//- 외부 lib / traineddata 불필요. C++/WinRT 로 OcrEngine 호출.
//- 영어 인식기 우선(숫자·콜론이면 충분). 인식기가 없으면 빈 문자열.
//- OcrEngine.RecognizeAsync 는 block-wait 가 필요해 내부에서 MTA worker 스레드로 실행하므로
//  STA 인 UI 스레드에서 그대로 호출해도 안전(데드락 없음).
//- bmp 픽셀은 내부에서 32bpp BGRA 로 lock 해 SoftwareBitmap(Bgra8) 으로 전달.
std::wstring sc_win_ocr(Gdiplus::Bitmap* bmp);

//숫자 인식 안정화를 위해 호출 전 crop 영역을 확대(예: 3~4x)해 넘기는 것을 권장.

//진단용 — 빈 결과의 원인(언어팩 없음 / 예외 / 정말 미인식)을 구분하기 위한 상세 반환.
//20260912 by claude. 인식 품질이 들쭉날쭉할 때 "엔진이 무엇을 받았고 무엇을 돌려줬는지" 를
//호출자가 로그로 남길 수 있도록 계측값을 함께 채운다. 이 모듈은 SCLog 를 쓰지 않는다 —
//일반 모듈은 로그를 포함한 채 push 하지 않는 것이 규칙이라, 기록은 호출자 몫으로 남긴다.
//20260912 by claude. 단어 하나와 그 위치. 색·글자 크기는 엔진이 알려주지 않으므로,
//호출자가 이 사각형으로 원본 픽셀을 되짚어 직접 뽑아야 한다.
struct SCOcrWord
{
	std::wstring	text;
	int				line = 0;	//몇 번째 줄에 속한 단어인지 (같은 줄이면 같은 값)
	int				x = 0;		//엔진에 넘긴 이미지 기준 픽셀 좌표
	int				y = 0;
	int				width = 0;
	int				height = 0;
};

struct SCOcrResult
{
	std::wstring	text;		//인식 전체 텍스트
	std::wstring	languages;	//사용 가능한 인식기 언어 표시명(", " 구분). 빈 문자열이면 OCR 언어팩 없음
	std::wstring	language;	//실제로 선택된 인식기의 BCP-47 태그 (예: "ko"). 빈 문자열이면 엔진 생성 실패
	std::wstring	error;		//예외 발생 시 메시지

	int				width = 0;		//엔진에 넘긴 픽셀 크기
	int				height = 0;
	int				alpha_min = 0;	//LockBits 로 읽은 원본 alpha 범위. 둘 다 0 이면 전면 투명 —
	int				alpha_max = 0;	//그 상태로 넘기면 엔진이 아무것도 못 본다 (호출자가 32bppRGB 로 넘겼는지 확인용).
	int				line_count = 0;
	int				word_count = 0;
	int				median_word_height = 0;	//단어 bounding box 높이의 중앙값. 글자가 너무 작거나 큰지 판단하는 지표.
	int				elapsed_ms = 0;

	std::vector<SCOcrWord>	words;
};
SCOcrResult sc_win_ocr_ex(Gdiplus::Bitmap* bmp);
