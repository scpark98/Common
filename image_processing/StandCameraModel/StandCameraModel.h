#ifndef __STAND_CAMERA_MODELLING_H__
#define __STAND_CAMERA_MODELLING_H__

/*
*	Standing Camera Modelling
*
*		2017-09-12 ����
*		09-19 ����ȭ
*
*		2017-09-19 made by CJW
*/



//ī�޶� ȭ�� ���� ���� Degree , ��
#define __CAMERA_VFOV__ 27.01
#define __CAMERA_HFOV__ 49.01

//��ġ ���� ���� ���� , m
#define __CAMERA_INSTALLTION_HEIGHT__  80./100.

//ȭ�� �̹��� ���� Pixel
#define __SCREEN_PIXEL_WIDTH	640
#define __SCREEN_PIXEL_HEIGHT	360






#ifndef __TYPEDEF_H__
typedef unsigned __int8 uint8_t;
typedef signed __int16 int16_t;
typedef signed __int32 int32_t;
typedef float float32_t;
#endif

#ifdef __cplusplus
extern "C"{
#endif

/*
#define DST_WIDTH	200
#define DST_HEIGHT	200
#define SRC_WIDTH	640
#define SRC_HEIGHT	360
const float32_t resolving_power_y = 5./100.; // ���ش� .. ���� ����(m)
const float32_t resolving_power_x = 5./100.; // ���ش� .. ���� ����(m)
const float32_t search_left_range = -( DST_WIDTH/2.0 ) * ( resolving_power_x );
const float32_t search_right_range = ( DST_WIDTH/2.0 ) * ( resolving_power_x );
const float32_t search_front_range = (DST_HEIGHT) * resolving_power_y;
StandRealDistanceTopView(dst2.data, search_front_range, search_left_range, search_right_range, resolving_power_y, resolving_power_x,gray.data,SRC_WIDTH,SRC_HEIGHT);
*/

	extern int32_t index[640*360];
	void CreateIndex(float32_t angle);											//�ʱ�ȭ�ܿ��� ������Ʈ�ε����� ���� ���� �ε��� ���
	void RotateImage(uint8_t *dst, uint8_t *src);								//������Ʈ�� �̹��� dst�� ��ȯ
	void SetRotateRollValue(float32_t angle);									//Roll �� ����. ���� ��׸�
	void Org2Rotate(float32_t ix, float32_t iy, float32_t *ox, float32_t *oy);	//���� ��ǥ���� ������Ʈ�� ��ǥ ��ȯ
	void Rotate2Org(float32_t ix, float32_t iy, float32_t *ox, float32_t *oy);	//������Ʈ�� ��ǥ���� ���� ��ǥ ��ȯ

	extern float32_t TopviewStartDistanceY;		//ž��� ������⿡ ���� ���� �Ÿ�

	void SetCameraFOV(float32_t CameraHFOV , float32_t CameraVFOV); // ī�޶� ȭ��
	void SetCameraHeight(float32_t InstalltionHeight);				// ī�޶� ��ġ����
	//void SetVanishingPointX(float32_t vx);							//�ҽ��� ����
	//void SetVanishingPointY(float32_t vy);							//�ҽ��� ����
	void SetVanishingPointXY(float32_t vx, float32_t vy);
	float32_t GetCameraTilt(void);									//ī�޶� ƿƮ ��ȯ. ���� ����
	float32_t GetCameraPan(void);									//ī�޶� �� ��ȯ. ���� ����

	//���� ������ �� ���� ���� ���� �Ÿ��� �����Ѵ�. ���� : �ȼ�, ����
	void Pixel2DistanceY(const int32_t px, const int32_t py, float32_t* dx, float32_t* dy);
	void Pixel2DistanceX(const int32_t px, const int32_t py, float32_t* dx, float32_t* dy);
	float32_t Pixel2Distance(const int32_t px, const int32_t py, float32_t* dx, float32_t* dy);

	//x�� �Ÿ�(m), y�� �Ÿ�(m)�� �̿��Ͽ� ���� ���󿡼��� ��ǥ���� ���Ѵ�.
	//���� topview ���󿡼��� �� �� (tx, ty)�� �ش��ϴ� ���� ���󿡼��� ��ǥ�� ���Ϸ���
	//dx���� ((float32_t)tx*resolving_power_x)+search_left_range ����,
	//dy���� ((float32_t)ty*resolving_power_y)+TopviewStartDistanceY ���� �Ѱ��ָ� �ȴ�.
	void Distance2PixelY(const float32_t dx, const float32_t dy, int32_t* px, int32_t* py);
	void Distance2PixelX(const float32_t dx, const float32_t dy, int32_t* px, int32_t* py);
	void Distance2Pixel(const float32_t dx, const float32_t dy, int32_t* px, int32_t* py);
	
	void TestDistance2Pixel(const float32_t dx, const float32_t dy, int32_t* px, int32_t* py );
	//void StandRealDistanceTophatFilter(
	//uint8_t *dst,
	//const uint8_t *src,
	//const int16_t w,
	//const int16_t h,
	//const int32_t threshold,
	//const float32_t offset_left,
	//const float32_t offset_right
	//);


	void SetTopViewRange(float32_t *search_left_range,
		float32_t *search_right_range,
		float32_t resolving_power_x,
		float32_t TopViewWidth);
	
	void StandRealDistanceTopView(
	uint8_t *dst,
	const float32_t dst_top,
	const float32_t dst_left,
	const float32_t dst_right,
	const float32_t resolving_power_y,
	const float32_t resolving_power_x,
	const uint8_t *src,
	const int16_t src_w,
	const int16_t src_h
	);

#ifdef __cplusplus
}
#endif

#endif	
