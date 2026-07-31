#include "SCApng.h"

#ifdef SC_USE_APNG

#include <cstring>
#include <csetjmp>
#include "png.h"   // 벤더 libpng(APNG) + zlib, 소스 정적 컴파일

namespace
{
	struct mem_src { const uint8_t* data; size_t size; size_t off; };

	void read_fn(png_structp png, png_bytep out, png_size_t len)
	{
		mem_src* s = (mem_src*)png_get_io_ptr(png);
		if (s->off + len > s->size) len = (s->off < s->size) ? (s->size - s->off) : 0;
		if (len) { memcpy(out, s->data + s->off, len); s->off += len; }
	}

	inline int mul255(int a, int b) { return (a * b + 127) / 255; }   // a*b/255 반올림
}

namespace sc_apng
{

bool decode(const uint8_t* data, size_t size,
			int& out_w, int& out_h,
			std::vector<std::vector<uint8_t>>& frames,
			std::vector<int>& delays_ms)
{
	frames.clear();
	delays_ms.clear();
	if (!data || size < 8 || png_sig_cmp((png_const_bytep)data, 0, 8))
		return false;

	png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (!png) return false;
	png_infop info = png_create_info_struct(png);
	if (!info) { png_destroy_read_struct(&png, nullptr, nullptr); return false; }

	// 캔버스는 premultiplied BGRA 로 관리(합성 over 수식이 단순)하고, 스냅샷 시 un-premultiply 한다.
	// setjmp/longjmp 는 C++ 소멸자를 돌리지 않으므로 정상 경로만 누수 0(드문 손상파일 오류 경로는
	// 아래 지역 벡터가 누수될 수 있으나 무해). 오류 시 frames 를 비우고 false.
	if (setjmp(png_jmpbuf(png)))
	{
		frames.clear();
		delays_ms.clear();
		png_destroy_read_struct(&png, &info, nullptr);
		return false;
	}

	mem_src src{ data, size, 0 };
	png_set_read_fn(png, &src, read_fn);
	png_read_info(png, info);

	const png_byte color = png_get_color_type(png, info);
	const png_byte depth = png_get_bit_depth(png, info);
	if (depth == 16) png_set_strip_16(png);
	if (color == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png);
	if (color == PNG_COLOR_TYPE_GRAY && depth < 8) png_set_expand_gray_1_2_4_to_8(png);
	if (png_get_valid(png, info, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png);
	if (color == PNG_COLOR_TYPE_GRAY || color == PNG_COLOR_TYPE_GRAY_ALPHA) png_set_gray_to_rgb(png);
	png_set_add_alpha(png, 0xFF, PNG_FILLER_AFTER);   // 알파 없으면 불투명 → 항상 4채널
	png_set_bgr(png);                                  // RGBA → BGRA
	png_set_interlace_handling(png);
	png_read_update_info(png, info);

	const int W = (int)png_get_image_width(png, info);
	const int H = (int)png_get_image_height(png, info);
	if (W <= 0 || H <= 0) png_error(png, "apng: bad size");
	out_w = W; out_h = H;

	png_uint_32 num_frames = 1, num_plays = 0;
	const bool is_apng = png_get_valid(png, info, PNG_INFO_acTL) != 0;
	if (is_apng) png_get_acTL(png, info, &num_frames, &num_plays);
	if (num_frames == 0) num_frames = 1;

	std::vector<uint8_t> canvas((size_t)W * H * 4, 0);   // premultiplied, 완전 투명
	std::vector<uint8_t> prev;                            // DISPOSE_PREVIOUS 복원용
	std::vector<uint8_t> sub;                             // 서브프레임(straight BGRA)
	std::vector<png_bytep> rows;

	for (png_uint_32 f = 0; f < num_frames; ++f)
	{
		png_uint_32 fw = (png_uint_32)W, fh = (png_uint_32)H, fx = 0, fy = 0;
		png_uint_16 dnum = 0, dden = 100;
		png_byte dispose = PNG_DISPOSE_OP_NONE, blend = PNG_BLEND_OP_SOURCE;

		if (is_apng)
		{
			png_read_frame_head(png, info);
			if (png_get_valid(png, info, PNG_INFO_fcTL))
				png_get_next_frame_fcTL(png, info, &fw, &fh, &fx, &fy, &dnum, &dden, &dispose, &blend);
		}
		if (fw == 0 || fh == 0 || (int)(fx + fw) > W || (int)(fy + fh) > H)
		{
			fw = (png_uint_32)W; fh = (png_uint_32)H; fx = fy = 0;
		}

		sub.assign((size_t)fw * fh * 4, 0);
		rows.resize(fh);
		for (png_uint_32 y = 0; y < fh; ++y) rows[y] = sub.data() + (size_t)y * fw * 4;
		png_read_image(png, rows.data());

		if (dispose == PNG_DISPOSE_OP_PREVIOUS)
			prev = canvas;   // 합성 전 상태 저장

		// 서브프레임을 캔버스(premultiplied)에 합성.
		for (png_uint_32 row = 0; row < fh; ++row)
		{
			uint8_t* dst = canvas.data() + (((size_t)(fy + row) * W + fx) * 4);
			const uint8_t* s = sub.data() + ((size_t)row * fw * 4);
			for (png_uint_32 col = 0; col < fw; ++col, dst += 4, s += 4)
			{
				const int sA = s[3];
				const int pB = mul255(s[0], sA), pG = mul255(s[1], sA), pR = mul255(s[2], sA);
				if (blend == PNG_BLEND_OP_SOURCE || dst[3] == 0)
				{
					dst[0] = (uint8_t)pB; dst[1] = (uint8_t)pG; dst[2] = (uint8_t)pR; dst[3] = (uint8_t)sA;
				}
				else   // OVER (premultiplied): out = src_pm + dst_pm*(255-sA)/255
				{
					const int ia = 255 - sA;
					dst[0] = (uint8_t)(pB + mul255(dst[0], ia));
					dst[1] = (uint8_t)(pG + mul255(dst[1], ia));
					dst[2] = (uint8_t)(pR + mul255(dst[2], ia));
					dst[3] = (uint8_t)(sA + mul255(dst[3], ia));
				}
			}
		}

		// 스냅샷: premultiplied → straight BGRA.
		std::vector<uint8_t> frame((size_t)W * H * 4);
		for (size_t i = 0; i < (size_t)W * H; ++i)
		{
			const int A = canvas[i * 4 + 3];
			if (A == 0) { frame[i*4]=frame[i*4+1]=frame[i*4+2]=frame[i*4+3]=0; }
			else
			{
				int b = (canvas[i*4]   * 255 + A/2) / A; if (b > 255) b = 255;
				int g = (canvas[i*4+1] * 255 + A/2) / A; if (g > 255) g = 255;
				int r = (canvas[i*4+2] * 255 + A/2) / A; if (r > 255) r = 255;
				frame[i*4]=(uint8_t)b; frame[i*4+1]=(uint8_t)g; frame[i*4+2]=(uint8_t)r; frame[i*4+3]=(uint8_t)A;
			}
		}
		frames.push_back(std::move(frame));

		int ms = (int)((double)dnum * 1000.0 / (double)(dden ? dden : 100));
		if (ms < 10) ms = 10;
		delays_ms.push_back(ms);

		// 다음 프레임 위한 dispose.
		if (dispose == PNG_DISPOSE_OP_BACKGROUND)
		{
			for (png_uint_32 row = 0; row < fh; ++row)
			{
				uint8_t* dst = canvas.data() + (((size_t)(fy + row) * W + fx) * 4);
				memset(dst, 0, (size_t)fw * 4);
			}
		}
		else if (dispose == PNG_DISPOSE_OP_PREVIOUS && !prev.empty())
		{
			canvas.swap(prev);
			prev.clear();
		}
	}

	png_destroy_read_struct(&png, &info, nullptr);
	return !frames.empty();
}

} // namespace sc_apng

#endif // SC_USE_APNG
