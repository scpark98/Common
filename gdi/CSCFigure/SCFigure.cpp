#include "SCFigure.h"

CSCFigure::CSCFigure()
{
	type = 0;
	r = Gdiplus::Rect(0, 0, 160, 40); 
	round[0] = round[1] = round[2] = round[3] = 10;
	rotate = 0.0f;
	cr_fill = Gdiplus::Color::RoyalBlue;
	cr_stroke = Gdiplus::Color::Navy;
	stroke_alpha = 255;
	stroke_width = 2;
	//shadow_depth = 8;
	cr_shadow = Gdiplus::Color::Gray;
	shadow_sigma = 25;	//실제 적용할때에는 10으로 나눈 값을 사용한다.
	shadow_offset_x = 2;
	shadow_offset_y = 2;

	//PRINT(type, stroke_alpha, shadow_sigma, type, stroke_alpha);

	json.add_member(type);
	json.add_member(r);
	json.add_member(round[0]);
	json.add_member(rotate);
	json.add_member(cr_fill);
	json.add_member(fill_alpha);
	json.add_member(cr_stroke);
	json.add_member(stroke_alpha);
	json.add_member(stroke_width);
	json.add_member(cr_shadow);
	json.add_member(shadow_sigma);
	json.add_member(shadow_offset_x);
	json.add_member(shadow_offset_y);
	//json.add_member(cr_stroke);// , stroke_alpha, stroke_width, cr_shadow, shadow_sigma, shadow_offset_x, shadow_offset_y);
	trace(json.get_json_string());
	//printf("%s\n", json.get_json_string());
	int a = json.doc["type"].GetInt();
	//std::string res = json.to_json(type, r, round, cr_fill, cr_stroke, stroke_alpha, stroke_width, cr_shadow, shadow_sigma, shadow_offset_x, shadow_offset_y);
	/*
	CString js = 
		_T("{\"type\", %d},")
	json.doc["type"] = type;
	json.doc["r"]["x"] = r.X;
	json.doc["r"]["y"] = r.Y;
	json.doc["r"]["width"] = r.Width;
	json.doc["r"]["height"] = r.Height;
	*/
	//json.save(_T("d:\\fig.json"));
}

CSCFigure::~CSCFigure()
{

}

void CSCFigure::load(CString fig_file)
{
	if (!json.load(fig_file))
		return;

	type = json.doc["type"].GetInt();
	rapidjson::Value& ar = json.doc["r"];

	r.X = ar[0].GetFloat();
	r.Y = ar[1].GetFloat();
	r.Width = ar[2].GetFloat();
	r.Height = ar[3].GetFloat();
	rotate = json.get<float>("rotate", 0.0);// json.doc["rotate"].GetFloat();
	cr_fill = Gdiplus::Color(json.doc["cr_fill"].GetUint());
	fill_alpha = json.doc["fill_alpha"].GetInt();
	cr_stroke = Gdiplus::Color(json.doc["cr_stroke"].GetUint());
	stroke_alpha = json.doc["stroke_alpha"].GetInt();
	stroke_width = json.doc["stroke_width"].GetInt();
	cr_shadow = Gdiplus::Color(json.doc["cr_shadow"].GetUint());
	shadow_sigma = json.doc["shadow_sigma"].GetInt();
	shadow_offset_x = json.doc["shadow_offset_x"].GetInt();
	shadow_offset_y = json.doc["shadow_offset_y"].GetInt();
}

void CSCFigure::save(CString fig_file)
{
	json.doc["type"] = type;
	json.doc["r"][0] = r.X;
	json.doc["r"][1] = r.Y;
	json.doc["r"][2] = r.Width;
	json.doc["r"][3] = r.Height;
	json.doc["rotate"] = rotate;
	json.doc["cr_fill"] = (UINT)cr_fill.GetValue();
	json.doc["fill_alpha"] = fill_alpha;
	json.doc["cr_stroke"] = (UINT)cr_stroke.GetValue();
	json.doc["stroke_alpha"] = stroke_alpha;
	json.doc["stroke_width"] = stroke_width;
	json.doc["cr_shadow"] = (UINT)cr_shadow.GetValue();
	json.doc["shadow_sigma"] = shadow_sigma;
	json.doc["shadow_offset_x"] = shadow_offset_x;
	json.doc["shadow_offset_y"] = shadow_offset_y;

	json.save(fig_file);
}


void CSCFigure::draw(CSCGdiplusBitmap* img, bool draw_fore, bool draw_shadow)
{
	CSCGdiplusBitmap fore;

	fore.create(r.Width, r.Height);

	Gdiplus::Graphics g(fore);
	Gdiplus::GraphicsPath path;

	Gdiplus::Pen pen(cr_stroke, stroke_width);
	Gdiplus::SolidBrush br(cr_fill);

	//pen.SetAlignment(Gdiplus::PenAlignmentInset);
	//pen.SetAlignment(Gdiplus::PenAlignmentCenter);

	g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);
	g.SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeAntiAlias);
	g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);

	if (type == figure_type_rect)
	{
		//fore.create_round_rect(r.Width, r.Height, round[0], cr_fill, cr_stroke, stroke_width);
		get_round_rect_path(&path, Gdiplus::Rect(0, 0, r.Width - stroke_width, r.Height - stroke_width), round[0]);
	}
	else if (type == figure_type_bowl_rect)
	{
		//r.Height에서 stroke_width를 뺀 높으로 생성해야 펜이 아래까지 제대로 그려진다.
		get_bowl_rect_path(&path, Gdiplus::Rect(0, 0, r.Width - stroke_width, r.Height - stroke_width), round[0]);
	}

	g.FillPath(&br, &path);

	fore.save(_T("d:\\fore.png"));

	//shadow 이미지에는 stroke가 적용되지 않아야 하므로 create_back_shadow_image() 함수를 사용해서는 안된다.
	CSCGdiplusBitmap shadow(&fore);

	if (draw_shadow)
	{
		shadow.gray();
		shadow.resize_canvas(r.Width + 8 + shadow_offset_x, r.Height + 8 + shadow_offset_y);
		shadow.blur((float)shadow_sigma / 10.0f);
		img->draw(&shadow, r.X - 4 + shadow_offset_x, r.Y - 4 + shadow_offset_y);
	}

	//stroke가 투명이 아니고 두께가 0보다 크다면 stroke를 그려준다.
	if (cr_stroke.GetValue() != Gdiplus::Color::Transparent && stroke_width > 0.0f)
		g.DrawPath(&pen, &path);


	//for test text output
	CString text = _T("<b><size=12><cr=white>Any</b></cr><cr=blue>Support");

	std::deque<std::deque<CSCParagraph>> para;
	CSCTextProperty text_prop;

	CSCParagraph::build_paragraph_str(text, para, &text_prop);
	CRect rtext(0, 0, fore.width, fore.height);

	CDC* pDC = CDC::FromHandle(::GetDC(::GetDesktopWindow()));
	rtext = CSCParagraph::calc_text_rect(rtext, pDC, para, DT_CENTER | DT_VCENTER);
	CSCParagraph::draw_text(g, para);


	if (draw_fore)
		img->draw(&fore, r.X, r.Y);
}
