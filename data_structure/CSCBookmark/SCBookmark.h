#pragma once

#include <Afxwin.h>
#include <vector>
#include <memory>

#include "../../SCGdiplusBitmap.h"

//미디어 재생 위치 + 썸네일을 한 단위로 묶는 책갈피 데이터.
//Endorphin2 / 다른 미디어 프로젝트의 북마크 시스템에서 공통 사용.
//thumbnail 은 std::unique_ptr — CSCGdiplusBitmap 자체에 안전한 copy/move 가 없어 vector 에 value 로 담으면 shallow copy 위험.
class CSCBookmark
{
public:
	static constexpr int	thumbnail_size = 128;
	static constexpr UINT	serialization_version = 1;

	int									time_ms = 0;
	CString								fullpath;
	CString								name;
	std::unique_ptr<CSCGdiplusBitmap>	thumbnail;

	CSCBookmark() = default;
	CSCBookmark(const CSCBookmark&) = delete;
	CSCBookmark& operator=(const CSCBookmark&) = delete;
	CSCBookmark(CSCBookmark&&) noexcept = default;
	CSCBookmark& operator=(CSCBookmark&&) noexcept = default;

	bool			serialize(std::vector<BYTE>& out) const;
	bool			deserialize(const BYTE* data, UINT size);

	bool			is_same_media(const CString& path) const
	{
		return fullpath.CompareNoCase(path) == 0;
	}

	//캡처된 frame 으로부터 thumbnail 생성. src 를 deep_copy 한 뒤 영상 비율 유지하며 thumbnail_size x thumbnail_size 정사각에 letterbox.
	void			set_thumbnail(CSCGdiplusBitmap& src);
};
