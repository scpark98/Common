#ifndef __STAND_CAMERA_MODELLING_H__
#define __STAND_CAMERA_MODELLING_H__

/*
*	Standing Camera Modelling
*
*		2017-09-12 만듬
*		09-19 최적화
*
*		2017-09-19 made by CJW
*/



//카메라 화각 정보 단위 Degree , 도
#define __CAMERA_VFOV__ 27.01
#define __CAMERA_HFOV__ 49.01

//설치 높이 단위 미터 , m
#define __CAMERA_INSTALLTION_HEIGHT__  80./100.

//화면 이미지 단위 Pixel
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
const float32_t resolving_power_y = 5./100.; // 분해능 .. 단위 미터(m)
const float32_t resolving_power_x = 5./100.; // 분해능 .. 단위 미터(m)
const float32_t search_left_range = -( DST_WIDTH/2.0 ) * ( resolving_power_x );
const float32_t search_right_range = ( DST_WIDTH/2.0 ) * ( resolving_power_x );
const float32_t search_front_range = (DST_HEIGHT) * resolving_power_y;
StandRealDistanceTopView(dst2.data, search_front_range, search_left_range, search_right_range, resolving_power_y, resolving_power_x,gray.data,SRC_WIDTH,SRC_HEIGHT);
*/

	extern int32_t index[640*360];
	void CreateIndex(float32_t angle);											//초기화단에서 로테이트인덱스에 대해 원본 인덱스 계산
	void RotateImage(uint8_t *dst, uint8_t *src);								//로테이트된 이미지 dst에 반환
	void SetRotateRollValue(float32_t angle);									//Roll 각 설정. 단위 디그리
	void Org2Rotate(float32_t ix, float32_t iy, float32_t *ox, float32_t *oy);	//원본 좌표에서 로테이트된 좌표 변환
	void Rotate2Org(float32_t ix, float32_t iy, float32_t *ox, float32_t *oy);	//로테이트된 좌표에서 원본 좌표 변환

	extern float32_t TopviewStartDistanceY;		//탑뷰어 정면방향에 대한 시작 거리

	void SetCameraFOV(float32_t CameraHFOV , float32_t CameraVFOV); // 카메라 화각
	void SetCameraHeight(float32_t InstalltionHeight);				// 카메라 설치높이
	//void SetVanishingPointX(float32_t vx);							//소실점 설정
	//void SetVanishingPointY(float32_t vy);							//소실점 설정
	void SetVanishingPointXY(float32_t vx, float32_t vy);
	float32_t GetCameraTilt(void);									//카메라 틸트 반환. 단위 라디안
	float32_t GetCameraPan(void);									//카메라 펜 반환. 단위 라디안

	//실제 영상의 한 점에 대한 실제 거리를 리턴한다. 단위 : 픽셀, 미터
	void Pixel2DistanceY(const int32_t px, const int32_t py, float32_t* dx, float32_t* dy);
	void Pixel2DistanceX(const int32_t px, const int32_t py, float32_t* dx, float32_t* dy);
	float32_t Pixel2Distance(const int32_t px, const int32_t py, float32_t* dx, float32_t* dy);

	//x축 거리(m), y축 거리(m)를 이용하여 실제 영상에서의 좌표값을 구한다.
	//만약 topview 영상에서의 한 점 (tx, ty)에 해당하는 실제 영상에서의 좌표를 구하려면
	//dx에는 ((float32_t)tx*resolving_power_x)+search_left_range 값을,
	//dy에는 ((float32_t)ty*resolving_power_y)+TopviewStartDistanceY 값을 넘겨주면 된다.
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
