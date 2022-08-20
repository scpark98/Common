#include "StandCameraModel.h"
#include <math.h>


#define pi		(3.1415926535897932384626433832795)

static float32_t Pixel_Width = (float32_t)__SCREEN_PIXEL_WIDTH;
static float32_t Pixel_Height = (float32_t)__SCREEN_PIXEL_HEIGHT;
static float32_t Pixel_Center_X = ((float32_t)__SCREEN_PIXEL_WIDTH)/2.f;
static float32_t Pixel_Center_Y = ((float32_t)__SCREEN_PIXEL_HEIGHT)/2.f;
static float32_t Pixel_Full_X = ((float32_t)__SCREEN_PIXEL_WIDTH)-1.f;
static float32_t Pixel_Full_Y = ((float32_t)__SCREEN_PIXEL_HEIGHT)-1.f;
static float32_t VFOV = (__CAMERA_VFOV__) * (pi/180.f);
static float32_t HFOV = (__CAMERA_HFOV__) * (pi/180.f);
static float32_t H_VFOV = (__CAMERA_VFOV__/2.f) * (pi/180.f);
static float32_t H_HFOV = (__CAMERA_HFOV__/2.f) * (pi/180.f);
static float32_t Camera_Height = __CAMERA_INSTALLTION_HEIGHT__;	
static float32_t Vanishing_Point_X = ((float32_t)__SCREEN_PIXEL_WIDTH)/2.f;
static float32_t Vanishing_Point_Y = ((float32_t)__SCREEN_PIXEL_HEIGHT)/2.f;
static float32_t Camera_Tilt = 0.0;
static float32_t Camera_Pan = 0.0;
float32_t TopviewStartDistanceY=0.0;


int32_t index[640*360];
float32_t RotateAngle=0.0;
void CreateIndex(float32_t angle)
{
	int32_t x,y;
	RotateAngle = angle * pi /180.f;

	for(y=0;y<360;y++)
	{
		for(x=0;x<640;x++)
		{
			const int32_t _x = x-320;
			const int32_t _y = y-180;
			const int32_t tx = 320 + (int32_t)(_x*cos(RotateAngle) + _y*sin(RotateAngle) + 0.5);
			const int32_t ty = 180 + (int32_t)(_y*cos(RotateAngle) - _x*sin(RotateAngle) + 0.5);

			if(tx>=0 && tx<640 && ty>=0 && ty<360){
				index[x+y*640]=tx+ty*640;
			}else{
				index[x+y*640]=-1;
			}
		}
	}
}
void SetRotateRollValue(float32_t angle)
{
	RotateAngle = angle * pi /180.f;
}

void RotateImage(uint8_t *dst, uint8_t *src)
{
	int32_t p=0;
	for(p=640*360-1;p>=0;p--)
	{
		if(index[p] != -1)
		{
			dst[p]=src[index[p]];
		}
	}
}

void Org2Rotate(float32_t ix, float32_t iy, float32_t *ox, float32_t *oy)
{
	ix = ix - 320.f;
	iy = iy - 180.f;

	*ox = 320.f + ix*cos(-RotateAngle) + iy*sin(-RotateAngle);
	*oy = 180.f + iy*cos(-RotateAngle) - ix*sin(-RotateAngle);
}
void Rotate2Org(float32_t ix, float32_t iy, float32_t *ox, float32_t *oy)
{
	ix = ix - 320.f;
	iy = iy - 180.f;

	*ox = 320.f + ix*cos(RotateAngle) + iy*sin(RotateAngle);
	*oy = 180.f + iy*cos(RotateAngle) - ix*sin(RotateAngle);
}



void SetCameraFOV(float32_t CameraHFOV , float32_t CameraVFOV)
{
	if ( CameraHFOV > 0 )
	{
		HFOV = (CameraHFOV) * (pi/180.f);
		H_HFOV = (CameraHFOV/2.f) * (pi/180.f);
	}

	if ( CameraVFOV > 0 )
	{
		VFOV = (CameraVFOV) * (pi/180.f);
		H_VFOV = (CameraVFOV/2.f) * (pi/180.f);
	}

	return;
}

void SetCameraHeight(float32_t InstalltionHeight)
{
	Camera_Height = InstalltionHeight;
	return;
}


void SetVanishingPointY(float32_t vy)
{
	float32_t dx,dy;
	
	Vanishing_Point_Y = vy;
	Camera_Tilt = ((Pixel_Center_Y - Vanishing_Point_Y)/Pixel_Center_Y)*(H_VFOV);

	Pixel2DistanceY(0,(int32_t)Pixel_Full_Y,&dx,&dy);
	//Pixel2Distance(0,(int32_t)Pixel_Full_Y,&dx,&dy);
	TopviewStartDistanceY = dy;

	return;
}

void SetVanishingPointX(float32_t vx)
{
	Vanishing_Point_X = vx;
	Camera_Pan = ((Pixel_Center_X - Vanishing_Point_X)/Pixel_Center_X)*(H_HFOV);
	return;
}

void SetVanishingPointXY(float32_t vx, float32_t vy)
{
	float32_t dx1,dy1;
	float32_t dx2,dy2;
	Vanishing_Point_Y = vy;
	Camera_Tilt = ((Pixel_Center_Y - Vanishing_Point_Y)/Pixel_Center_Y)*(H_VFOV);

	Vanishing_Point_X = vx;
	Camera_Pan = ((Pixel_Center_X - Vanishing_Point_X)/Pixel_Center_X)*(H_HFOV);

	Pixel2Distance((int32_t)0,(int32_t)Pixel_Full_Y,&dx1,&dy1);
	Pixel2Distance((int32_t)Pixel_Full_X,(int32_t)Pixel_Full_Y,&dx2,&dy2);

	if(dy1 > dy2){
		TopviewStartDistanceY = dy2;
	}else{
		TopviewStartDistanceY = dy1;
	}




	return;
}

float32_t GetCameraTilt(void)
{
	return Camera_Tilt*180./pi;
}

float32_t GetCameraPan(void)
{
	return Camera_Pan*180./pi;
}

void Distance2PixelY(const float32_t dx, const float32_t dy, int32_t* px, int32_t* py)
{
	const float32_t ddy = dy*cos(Camera_Pan) + dx*(sin(Camera_Pan)); //Pan 각도 보정 실제 Y거리
	const float32_t BHC = atan2 ( Camera_Height , ddy );			  //카메라 높이와 Y축 실거리에 의한 각도형성

	const float32_t FVA = H_VFOV + Camera_Tilt;						  //카메라로 찍을 수 있는 각도 형성
	const float32_t Y = BHC/FVA * ( Pixel_Full_Y -Vanishing_Point_Y );//카메라로 찍을 수 있는 곳과 실제 거리를 이용하여 픽셀Y좌표 추출
	*py = (int32_t)Vanishing_Point_Y + (int32_t)(Y+0.5);
	return;
}
void Distance2PixelX(const float32_t dx, const float32_t dy, int32_t* px, int32_t* py)
{
	const float32_t ddy = dy*cos(Camera_Pan) + dx*(sin(Camera_Pan)); //Pan 각도 보정 실제 Y거리
	const float32_t ddx = dx*cos(Camera_Pan) + dy*(-sin(Camera_Pan)); //Pan 각도 보정 실제 X거리

	const float32_t A = atan2(ddx,ddy);								//실거리기반 XY의 사이각 구함
	const float32_t X = A * ( Pixel_Center_X / H_HFOV );			//카메라 화각에 의한 픽셀X좌표 추출
	*px = (int32_t)Pixel_Center_X + (int32_t)(X+0.5);
	return;
}

void Distance2Pixel(const float32_t dx, const float32_t dy, int32_t* px, int32_t* py)
{
	const float32_t ddy = dy*cos(Camera_Pan) + dx*(sin(Camera_Pan));
	const float32_t ddx = dx*cos(Camera_Pan) + dy*(-sin(Camera_Pan));

	const float32_t FVA = H_VFOV + Camera_Tilt;
	const float32_t BHC = atan2 ( Camera_Height  , ddy );
	const float32_t Y = BHC/FVA * ( Pixel_Full_Y -Vanishing_Point_Y );
	const float32_t A = atan2(ddx,ddy);
	const float32_t X = A * ( Pixel_Center_X / H_HFOV );

	*py = (int32_t)Vanishing_Point_Y + (int32_t)(Y+0.5);
	*px = (int32_t)Pixel_Center_X + (int32_t)(X+0.5);
	return;
}

void TestDistance2Pixel(const float32_t dx, const float32_t dy, int32_t* px, int32_t* py )
{
	const float32_t ddy = dy*cos(Camera_Pan) + dx*(sin(Camera_Pan));
	const float32_t ddx = dx*cos(Camera_Pan) + dy*(-sin(Camera_Pan));

	const float32_t P = 1./tan((Camera_Height/ddy));
	const float32_t Y = ((P/H_VFOV)*Pixel_Center_Y)+Vanishing_Point_Y;
	const float32_t A = 1./tan(ddx/ddy);
	const float32_t X = ((A/H_HFOV)*Pixel_Center_X)+Pixel_Center_X;

	*py = (int32_t)(Y);
	*px = (int32_t)(X);
}

void Pixel2DistanceY(const int32_t px, const int32_t py, float32_t* dx, float32_t* dy )
{
	const float32_t P = ((((float32_t)py) - Vanishing_Point_Y)/Pixel_Center_Y) *(H_VFOV);
	const float32_t Y = ( 1./ tan(P) )*Camera_Height;

	*dy = Y;
	return;
}
void Pixel2DistanceX(const int32_t px, const int32_t py, float32_t* dx, float32_t* dy )
{
	const float32_t A = (((float32_t)px - Pixel_Center_X)/Pixel_Center_X)*H_HFOV;
	const float32_t X = tan(A)*(*dy);
	*dx = X;
	return ;
}

//실제 영상의 한 점에 대한 실제 거리를 리턴한다. 단위 : 픽셀, 미터
float32_t Pixel2Distance(const int32_t px, const int32_t py, float32_t* dx, float32_t* dy )
{
	const float32_t P = ((((float32_t)py) - Vanishing_Point_Y)/Pixel_Center_Y) *(H_VFOV);
	const float32_t Y = ( 1./ tan(P) )*Camera_Height;
	const float32_t A = (((float32_t)px - Pixel_Center_X)/Pixel_Center_X)*H_HFOV;
	const float32_t X = tan(A)*Y;

	const float32_t ddy = Y*cos(-Camera_Pan) + X*(sin(-Camera_Pan));
	const float32_t ddx = X*cos(-Camera_Pan) + Y*(-sin(-Camera_Pan));

	*dy = ddy;
	*dx = ddx;

	return sqrt(ddy*ddy+ddx*ddx);
}






float32_t TopViewImageWidth	= 100.f;
float32_t TopViewImageHeight= 100.f;
float32_t resolving_power_y = 1.f / 100.f;
float32_t resolving_power_x = 1.f / 100.f;
float32_t search_left_range = -( 100.f/2.f ) * ( 1.f / 100.f );
float32_t search_right_range = ( 100.f/2.f ) * ( 1.f / 100.f );
float32_t search_front_range = (100.f) * (1.f / 100.f);

uint8_t TopViewFlag=0x00;

void SetSourceSize(float32_t Width, float32_t Height)
{
	Pixel_Width = (float32_t)Width;
	Pixel_Height = (float32_t)Height;
	Pixel_Center_X = ((float32_t)Width)/2.f;
	Pixel_Center_Y = ((float32_t)Height)/2.f;
	Pixel_Full_Y = ((float32_t)Height)-1.f;
}

void SetTopViewSize(float32_t Width, float32_t Height)
{
	TopViewImageWidth = Width;
	TopViewImageHeight = Height;
	TopViewFlag |= 0x01;
	if(TopViewFlag & 0x02){
		search_front_range = TopViewImageHeight * resolving_power_y;
		search_left_range = - (TopViewImageWidth/2.f) * resolving_power_x;
		search_right_range = (TopViewImageWidth/2.f) * resolving_power_x;
	}
}
void SetTopViewResolvingPower(float32_t x, float32_t y)
{
	resolving_power_x = x;
	resolving_power_y = y;
	TopViewFlag |= 0x02;
	if(TopViewFlag & 0x01){
		search_front_range = TopViewImageHeight * resolving_power_y;
		search_left_range = - (TopViewImageWidth/2.f) * resolving_power_x;
		search_right_range = (TopViewImageWidth/2.f) * resolving_power_x;
	}
}

void GetTopViewSize(float32_t front_range, float32_t left_range, float32_t right_range, float32_t resolving_x , float32_t resolving_y, int32_t *return_w, int32_t *return_h)
{
	*return_w = (int32_t)((right_range - left_range) * (1./resolving_x) +0.5);
	*return_h = (int32_t)((front_range - TopviewStartDistanceY) * (1./resolving_y)+0.5);
}

void SetTopViewRange(float32_t *search_left_range,float32_t *search_right_range,float32_t resolving_power_x,float32_t TopViewWidth)
{
	float32_t dx1,dy1;
	float32_t dx2,dy2;

	Pixel2Distance((int32_t)0,(int32_t)Pixel_Full_Y,&dx1,&dy1);
	Pixel2Distance((int32_t)Pixel_Full_X,(int32_t)Pixel_Full_Y,&dx2,&dy2);

	if(dy1 > dy2){
		*search_left_range = dx1;
		*search_right_range = *search_left_range + (resolving_power_x*TopViewWidth);
	}else{
		*search_right_range = dx2;
		*search_left_range = *search_right_range - (resolving_power_x*TopViewWidth);
	}

}

void StandRealDistanceTopView(uint8_t *dst,
							  const float32_t dst_top,
							  const float32_t dst_left,
							  const float32_t dst_right,
							  const float32_t resolving_power_y,
							  const float32_t resolving_power_x,
							  const uint8_t *src,
							  const int16_t src_w,
							  const int16_t src_h
)
{
	float32_t dx,dy;
	int32_t ix,iy;
	int32_t s=0;
	int32_t s2=0;
	int32_t px,py;
	int32_t D;
	const int32_t dst_w = (dst_right - dst_left) * (1./resolving_power_x) +0.5;
	const int32_t dst_h = (dst_top) * (1./resolving_power_y)+0.5;
	const int16_t idx[9] = {-src_w-1, -src_w, -src_w+1, -1, 0, 1, src_w-1 , src_w, src_w+1};

	dy = TopviewStartDistanceY;
	//dy =0;
#if 0
	for(iy=0; iy<dst_h; iy++)
	{
		dx=dst_left;
		Distance2PixelY(dx, dy, &px, &py);
		s2 = src_w * py;
		if(py >= 1 && py < src_h-1)
		{
			for(ix = 0; ix<dst_w; ix++)
			{
				Distance2PixelX(dx, dy, &px, &py);
				
				if(px >= 1 && px <src_w-1)
				{
					D= src[px+s2+idx[0]];
					D+=src[px+s2+idx[1]]<<1;
					D+=src[px+s2+idx[2]];
					D+=src[px+s2+idx[3]]<<1;
					D+=src[px+s2+idx[4]]<<2;
					D+=src[px+s2+idx[5]]<<1;
					D+=src[px+s2+idx[6]];
					D+=src[px+s2+idx[7]]<<1;
					D+=src[px+s2+idx[8]];
					D=D>>4;
					dst[s+ix] = 0xFF&D;
				}
				dx+= resolving_power_x;
			}
		}
		s+=dst_w;
		dy+=resolving_power_y;
	}
#else
	for(iy=0; iy<dst_h; iy++)
	{
		dx=dst_left;
		for(ix = 0; ix<dst_w; ix++)
		{
			Distance2Pixel(dx, dy, &px, &py);
			s2 = src_w * py;
			if(py >= 1 && py < src_h-1)
			if(px >= 1 && px <src_w-1)
			{
				D= src[px+s2+idx[0]];
				D+=src[px+s2+idx[1]]<<1;
				D+=src[px+s2+idx[2]];
				D+=src[px+s2+idx[3]]<<1;
				D+=src[px+s2+idx[4]]<<2;
				D+=src[px+s2+idx[5]]<<1;
				D+=src[px+s2+idx[6]];
				D+=src[px+s2+idx[7]]<<1;
				D+=src[px+s2+idx[8]];
				D=D>>4;
				dst[s+ix] = 0xFF&D;
			}
			dx+= resolving_power_x;
		}
		s+=dst_w;
		dy+=resolving_power_y;
	}
#endif
}
