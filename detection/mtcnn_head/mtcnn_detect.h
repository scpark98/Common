/*
* https://github.com/lincolnhard/mtcnn-head-detection
* 얼굴인식이 아닌 head 인식 방식이라 마스크써도 잘 검출됨.
* NH투자신탁 프로젝트(x86, 정적 라이브러리에서 MFC 사용, 멀티바이트)에서 고객의 자리이탈을 감지하기 위해 사용.
* (세팅값 변경없이도 잘 검출됨)
* 유니코드, 멀티바이트 모두 잘 동작함.
* Debug모드에서는 Debug Assertion Failed "__acrt_first_block == header"가 뜨는데
* 이는 mtcnn이 사용하는 opencv라이브러리는 동적빌드, 이를 사용하는 프로젝트는 정적빌드를 하여 발생하는 충돌이므로
* 이를 사용하는 프로젝트에서 "고급->MFC 사용" 항목과 "C/C++->코드 생성" 항목을 다음과 같이 세팅하여 개발하자.
* Debug 모드   : "공유 DLL에서 MFC 사용" & /MDd
* Release 모드 : "정적 라이브러리에서 MFC 사용" & /MT
* 만약 이렇게 할 경우 아래와 같은 링크에러가 발생한다면
* 오류	LNK2005	"void * __cdecl operator new(unsigned int)" (??2@YAPAXI@Z)이(가) LIBCMT.lib(new_scalar.obj)에 이미 정의되어 있습니다.	Test_CamDetect	D:\1.project\1.Test\Test_CamDetect\nafxcw.lib(afxmem.obj)	1	
* 위와 같이 new, delete, new[], delete[] 등이 이미 정의가 되어 있다고 에러가 발생하는 경우
* 검색해보니 역시 opencv는 동적빌드, 이 프로젝트의 release는 정적빌드가 원인일 수 있고
* 문제가 된 nafxcw.lib를 "특정 기본 라이브러리 무시"에 명시하고 "추가 종속성"에 다시 nafxcw.lib를 명시해주니
* 해당 라이브러리를 링크하는 순서가 달라져서 그런지 위의 링크 에러는 사라졌다.
* (https://mindgear.tistory.com/182)
* 
* init_mtcnn(w, h, path_root)에서는 w, h를 640x360, 320x240 등을 줘도 크게 차이는 없었다.
* (어떠한 w, h를 줘도 얼굴의 크기가 10px ~ 90px 범위일 때 검출율 높음)
*/

#ifndef MTCNN_DETECT_H
#define MTCNN_DETECT_H

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/dnn.hpp>
#include <vector>

static const float pnet_th = 0.7f;
static const float rnet_th = 0.7f;
static const float onet_th = 0.2f;
static const float min_objsize = 24.0f;
static const float pyramid_factor = 0.666f;
static const float max_objsize = 70.0f;
static const int max_pnet_bbox_num = 100; // 100
static const int max_rnet_bbox_num = 50; // 50
static const float pnet_merge_th = 0.6f;
static const float rnet_merge_th = 0.5f;

typedef struct
    {
    float l;
    float t;
    float r;
    float b;
    float score;
    } obj_box;

typedef struct
    {
    float bbox_reg[4];
    obj_box bbox;
    } obj_info;

//w, h를 640x360, 320x240 등을 줘도 크게 차이는 없었다.
void init_mtcnn
    (
    const int srcw,
    const int srch,
    cv::String model_path_root
    );

void run_mtcnn
    (
    cv::Mat& im,
    std::vector<obj_info>& res//onet_boxes
    );

#endif // MTCNN_DETECT_H
