#include "SCBookmark.h"
#include "../../colors.h"

namespace
{
	template<typename T>
	bool read_pod(const BYTE*& p, const BYTE* end, T& v)
	{
		if (p + sizeof(T) > end)
			return false;
		memcpy(&v, p, sizeof(T));
		p += sizeof(T);
		return true;
	}

	template<typename T>
	void write_pod(std::vector<BYTE>& buf, const T& v)
	{
		const BYTE* s = reinterpret_cast<const BYTE*>(&v);
		buf.insert(buf.end(), s, s + sizeof(T));
	}

	void write_string(std::vector<BYTE>& buf, const CString& s)
	{
		UINT byte_count = (UINT)(s.GetLength() * sizeof(TCHAR));
		write_pod(buf, byte_count);
		if (byte_count > 0)
		{
			const BYTE* p = reinterpret_cast<const BYTE*>((LPCTSTR)s);
			buf.insert(buf.end(), p, p + byte_count);
		}
	}

	bool read_string(const BYTE*& p, const BYTE* end, CString& out)
	{
		UINT byte_count = 0;
		if (!read_pod(p, end, byte_count))
			return false;
		if (byte_count == 0)
		{
			out.Empty();
			return true;
		}
		if (p + byte_count > end || (byte_count % sizeof(TCHAR)) != 0)
			return false;
		int char_count = (int)(byte_count / sizeof(TCHAR));
		out = CString(reinterpret_cast<const TCHAR*>(p), char_count);
		p += byte_count;
		return true;
	}
}

bool CSCBookmark::serialize(std::vector<BYTE>& out) const
{
	out.clear();

	UINT version = serialization_version;
	write_pod(out, version);

	write_pod(out, time_ms);
	write_string(out, fullpath);
	write_string(out, name);

	HGLOBAL hg = nullptr;
	if (thumbnail && thumbnail->is_valid() && thumbnail->create_png_hglobal(&hg) && hg)
	{
		UINT byte_count = (UINT)GlobalSize(hg);
		write_pod(out, byte_count);
		if (byte_count > 0)
		{
			BYTE* src = (BYTE*)GlobalLock(hg);
			out.insert(out.end(), src, src + byte_count);
			GlobalUnlock(hg);
		}
		GlobalFree(hg);
	}
	else
	{
		write_pod(out, (UINT)0);
	}

	return true;
}

bool CSCBookmark::deserialize(const BYTE* data, UINT size)
{
	if (!data || size == 0)
		return false;

	//전체 blob 의 비합리적 크기 차단 — registry 부분 쓰기 / 손상으로 거대한 size 가 보고될 가능성.
	//일반 bookmark = 시간 (4) + 경로 (~수백 byte) + 이름 (~수백 byte) + 128x128 PNG (~수십 KB).
	//안전 마진으로 5MB 상한.
	if (size > 5 * 1024 * 1024)
		return false;

	const BYTE* p = data;
	const BYTE* end = data + size;

	UINT version = 0;
	if (!read_pod(p, end, version) || version != serialization_version)
		return false;

	if (!read_pod(p, end, time_ms))
		return false;
	//time_ms 범위 — 0 이상, 24시간 이내.
	if (time_ms < 0 || time_ms > 24 * 3600 * 1000)
		return false;

	if (!read_string(p, end, fullpath))
		return false;
	//path 길이 sanity — Windows MAX_PATH 의 4 배 정도까지 허용 (확장 경로 \\?\ 형식 포함).
	if (fullpath.GetLength() > 4096)
		return false;

	if (!read_string(p, end, name))
		return false;
	//북마크 이름 — 사용자 입력. 합리적 상한.
	if (name.GetLength() > 1024)
		return false;

	UINT thumb_size = 0;
	if (!read_pod(p, end, thumb_size))
		return false;

	//thumbnail PNG 합리적 상한 — 128x128 PNG 가 최악 ~50KB. 1MB 이상이면 손상 의심.
	if (thumb_size > 1024 * 1024)
		return false;

	if (thumb_size > 0)
	{
		if (p + thumb_size > end)
			return false;
		thumbnail = std::make_unique<CSCGdiplusBitmap>();
		if (!thumbnail->load_from_buffer(p, thumb_size))
		{
			thumbnail.reset();
			return false;
		}
		p += thumb_size;
	}
	else
	{
		thumbnail.reset();
	}

	return true;
}

void CSCBookmark::set_thumbnail(CSCGdiplusBitmap& src)
{
	if (!src.is_valid())
	{
		thumbnail.reset();
		return;
	}

	thumbnail = std::make_unique<CSCGdiplusBitmap>();
	src.deep_copy(thumbnail.get());

	//원본 frame 비율 유지하며 한 변을 thumbnail_size 로 맞춘 뒤, 정사각 캔버스로 letterbox.
	if (thumbnail->width >= thumbnail->height)
		thumbnail->resize(thumbnail_size, 0);
	else
		thumbnail->resize(0, thumbnail_size);

	thumbnail->resize_canvas(thumbnail_size, thumbnail_size, DT_CENTER | DT_VCENTER, gRGB(32, 32, 32));
}
