#pragma once

inline void LIVEWEBEXT_API StartGDIPlus() {}

namespace GDIPLUSEX
{
	LIVEWEBEXT_API void DrawImage(CDC*, CString);
}



class LIVEWEBEXT_API CGDIPlusEx
{
public:
	CGDIPlusEx()
	{
	}

	virtual ~CGDIPlusEx()
	{
	}
};


