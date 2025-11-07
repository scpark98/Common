/*
-라인부터 다각형까지 템플릿을 제공하고 벡터형태로 그려준다(ppt 도형과 같이)

- CSCFigure를 상속받아 CSCTriangle, CSCRect...등을 만드느냐
  CSCFigure 하나로 하고 m_type 변수로 처리하느냐...
  만약 get_area()라는 함수를 구현한다면 if 또는 switch로 구분.
  그럼 제어점은 ?
*/

#pragma once

#include <afxwin.h>
#include "../../SCGdiplusBitmap.h"
#include "../../Json/rapid_json/json.h"

enum SCFIGURE_TYPES
{
	figure_type_line,
	figure_type_rect,
	figure_type_bowl_rect,
	figure_type_ellipse,
	figure_type_polygon,
};

class CSCFigure
{
public:
	CSCFigure();
	~CSCFigure();

	Json			json;

	void			load(CString fig_file);
	void			save(CString fig_file);

	//타깃에 현재 도형의 속성대로 그려준다.
	void			draw(Gdiplus::Graphics& g);
	void			draw(CSCGdiplusBitmap* img, bool draw_fore = true, bool draw_shadow = true);

	int				type = figure_type_rect;
	Gdiplus::Rect	r;					//도형은 자기 스스로의 위치를 기억할 수 있어야 하므로 w, h가 아닌 Rect로 정의함.

	//사격형의 경우 각 코너를 각각 라운드 처리 선택할 수 있다. round[0] = 10인 경우 lt 코너를 10라운드 처리한다.
	//round[0] = -10이라면 라운드가 안쪽이 아닌 바깥쪽으로 휘는데 round[1] = -10과 같이 rt도 -10인 경우는
	//\__/ 와 같은 top floating toolbar 디자인도 가능하다.
	float			round[4] = { 0, 0, 0, 0 };

	//rotate degree
	float			rotate = 0.0f;

	Gdiplus::Color 	cr_fill;
	int				fill_alpha;
	Gdiplus::Color 	cr_stroke;
	int				stroke_alpha;
	int				stroke_width;
	//int				shadow_depth;
	Gdiplus::Color	cr_shadow;
	int				shadow_sigma;
	int				shadow_offset_x;
	int				shadow_offset_y;
};

