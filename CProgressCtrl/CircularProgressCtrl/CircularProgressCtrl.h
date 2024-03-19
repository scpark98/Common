#pragma once

#include <afxcmn.h>
#include <stdint.h>

// CCircularProgressCtrl

class CCircularProgressCtrl : public CStatic
{
	DECLARE_DYNAMIC(CCircularProgressCtrl)

public:
	CCircularProgressCtrl();
	virtual ~CCircularProgressCtrl();

	void		SetRange(int64_t lower, int64_t upper) { m_lower = lower; m_upper = upper; Invalidate(); }
	void		SetPos(int64_t pos, bool ani = false);

	//rc ������ ratio ������ŭ �۰� �׸���. 1.0�̸� 100%�� ������ �׸���. �ּҰ��� 10%
	//ũ�⿡ ���� ����ũ�⵵ �ڵ� �����Ǿ�� �Ѵ�(���� �̱���)
	void		set_size_ratio(float ratio);
	void		set_line_width(float width) { m_line_width = width; Invalidate(); }

protected:
	int64_t		m_lower = 0;
	int64_t		m_upper = 100;
	int64_t		m_pos = 0;

	float		m_line_width = 12.0f;
	float		m_size_ratio = 0.90f;

	//animation effect�� �ֱ� ����.
	enum TIMER_ID
	{
		timer_ani_setpos = 0,
	};

	int64_t		m_ani_pos;
	int64_t		m_ani_pos_final;
	bool		m_ani_pos_increase;



protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};


