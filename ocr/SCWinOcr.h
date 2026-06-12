#pragma once

#include <string>

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
struct SCOcrResult
{
	std::wstring	text;		//인식 전체 텍스트
	std::wstring	languages;	//사용 가능한 인식기 언어 표시명(", " 구분). 빈 문자열이면 OCR 언어팩 없음
	std::wstring	error;		//예외 발생 시 메시지
};
SCOcrResult sc_win_ocr_ex(Gdiplus::Bitmap* bmp);
