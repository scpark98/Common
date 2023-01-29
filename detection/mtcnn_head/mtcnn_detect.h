/*
* https://github.com/lincolnhard/mtcnn-head-detection
* ���ν��� �ƴ� head �ν� ����̶� ����ũ�ᵵ �� �����.
* NH���ڽ�Ź ������Ʈ(x86, ���� ���̺귯������ MFC ���, ��Ƽ����Ʈ)���� ���� �ڸ���Ż�� �����ϱ� ���� ���.
* (���ð� ������̵� �� �����)
* �����ڵ�, ��Ƽ����Ʈ ��� �� ������.
* Debug��忡���� Debug Assertion Failed "__acrt_first_block == header"�� �ߴµ�
* �̴� mtcnn�� ����ϴ� opencv���̺귯���� ��������, �̸� ����ϴ� ������Ʈ�� �������带 �Ͽ� �߻��ϴ� �浹�̹Ƿ�
* �̸� ����ϴ� ������Ʈ���� "���->MFC ���" �׸�� "C/C++->�ڵ� ����" �׸��� ������ ���� �����Ͽ� ��������.
* Debug ���   : "���� DLL���� MFC ���" & /MDd
* Release ��� : "���� ���̺귯������ MFC ���" & /MT
* ���� �̷��� �� ��� �Ʒ��� ���� ��ũ������ �߻��Ѵٸ�
* ����	LNK2005	"void * __cdecl operator new(unsigned int)" (??2@YAPAXI@Z)��(��) LIBCMT.lib(new_scalar.obj)�� �̹� ���ǵǾ� �ֽ��ϴ�.	Test_CamDetect	D:\1.project\1.Test\Test_CamDetect\nafxcw.lib(afxmem.obj)	1	
* ���� ���� new, delete, new[], delete[] ���� �̹� ���ǰ� �Ǿ� �ִٰ� ������ �߻��ϴ� ���
* �˻��غ��� ���� opencv�� ��������, �� ������Ʈ�� release�� �������尡 ������ �� �ְ�
* ������ �� nafxcw.lib�� "Ư�� �⺻ ���̺귯�� ����"�� ����ϰ� "�߰� ���Ӽ�"�� �ٽ� nafxcw.lib�� ������ִ�
* �ش� ���̺귯���� ��ũ�ϴ� ������ �޶����� �׷��� ���� ��ũ ������ �������.
* (https://mindgear.tistory.com/182)
* 
* init_mtcnn(w, h, path_root)������ w, h�� 640x360, 320x240 ���� �൵ ũ�� ���̴� ������.
* (��� w, h�� �൵ ���� ũ�Ⱑ 10px ~ 90px ������ �� ������ ����)
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

//w, h�� 640x360, 320x240 ���� �൵ ũ�� ���̴� ������.
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
