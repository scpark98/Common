#pragma once

#include <afxwin.h>

//20260825 by claude. 드롭 대상에게 위임하는 공용 메시지 — 드래그 소스가 대상의 구체 타입을 몰라도 되게 한다.
//drag_scroll_message.h(Message_DragScrollBy)와 같은 취지이고, 그때 미처 걷어내지 못한 나머지 두 가지를 덮는다.
//
//예전엔 CSCTreeCtrl 이 아래 두 가지를 위해 CSCListCtrl 을 직접 참조했다.
//    if (pDropWnd->IsKindOf(RUNTIME_CLASS(CSCListCtrl)))
//        ((CSCListCtrl*)pDropWnd)->hit_test(...);
//그 결과 트리만 쓰는 프로젝트도 SCListCtrl.cpp 를 링크해야 했다(Test_CSCTreeCtrl 링크 에러).
//대상이 이 메시지를 처리하면 1 을, 처리하지 않으면 0 을 돌려주므로 호출측은 폴백을 그대로 유지하면 된다.

//드롭 위치의 항목 인덱스를 대상에게 묻는다.
//native HitTest 는 스무스 스크롤(m_scroll_y)을 무시해 커서 밑과 다른 행을 짚으므로,
//자체 스크롤을 가진 컨트롤은 자기 좌표계로 계산한 값을 돌려줘야 한다.
//  WPARAM = const POINT*  (대상 창의 클라이언트 좌표)
//  LPARAM = int*          [out] 항목 인덱스. 없으면 -1
//  반환   = 1 이면 처리함(인덱스 유효), 0 이면 미처리 → 호출측이 native HitTest 로 폴백
static const UINT Message_DropHitTest = ::RegisterWindowMessage(_T("MessageString_DropHitTest"));

//이 컨트롤이 드롭을 받는지 묻는다. 드래그 자동스크롤 대상을 고를 때 쓴다.
//  WPARAM / LPARAM = 사용하지 않음
//  반환 = 1 이면 드롭 가능, 그 외는 불가(미처리 포함)
static const UINT Message_QueryAcceptDrop = ::RegisterWindowMessage(_T("MessageString_QueryAcceptDrop"));
