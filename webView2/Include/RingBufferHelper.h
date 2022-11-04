#pragma once

#define INIT_RINGBUFFER_SIZE	100000000 // 100MB //4096

class CRingBufferObj;

class LIVEWEBEXT_API CRingBufferHelper
{
public:
	CRingBufferHelper(int nSize = INIT_RINGBUFFER_SIZE);
	virtual ~CRingBufferHelper();

public:
	int GetTotalSize();
	int GetCapacity();

	int GetFreeSize();
	int GetUsedSize();

	BOOL Get(BYTE& b);
	BOOL Get(BYTE* pb, int nSize);

	BOOL Put(BYTE& b);
	BOOL Put(BYTE* pb, int nSize);

	int Peek(BYTE& b);
	BOOL Peek(BYTE* pb, int nSize);

	BOOL Discard(int nSize);

protected:
	CRingBufferObj* m_pRingBufferObj;
};
