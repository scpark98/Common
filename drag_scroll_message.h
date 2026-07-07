#pragma once

#include <afxwin.h>

//20260707 by claude. 드래그 자동스크롤 위임용 공용 메시지 — SCTreeCtrl 이 드롭 대상(리스트/트리)의 구체 타입을 몰라도
//대상 창에 이 메시지를 보내면 CVtListCtrlEx / CSCListCtrl / CSCTreeCtrl 가 각자 drag_scroll_by() 를 수행한다.
//이로써 SCTreeCtrl 이 특정 리스트 클래스(CVtListCtrlEx)를 하드 참조하지 않아, 프로젝트별로 CVtListCtrlEx→CSCListCtrl 점진 이행 가능.
//WPARAM = 가로 스크롤 픽셀, LPARAM = 세로 라인 수. 처리한 컨트롤은 1 을 반환(미처리=0 → 호출측이 raw Scroll 로 폴백).
static const UINT Message_DragScrollBy = ::RegisterWindowMessage(_T("MessageString_DragScrollBy"));
