#pragma once

// ============================================================================
// LsProto.h — LoginManager(Supporter) ↔ Viewer/Agent 프로세스 간 IPC
//             Windows Message 상수 SSOT
// ============================================================================
//
// LS prefix = LoginManager (Supporter) — 이 창을 대상으로 Viewer/Agent 가
// WM_COPYDATA / SendMessage 로 알림/요청을 보낸다.
// UM prefix = User Message — Viewer 내부 창간 통신 및 Host↔Viewer 이벤트.
//
// 정의 값은 원격 wire 프로토콜이 아니라 로컬 프로세스간 IPC 규약이라
// KMS 서버 상수(KmsProto.h)와 별도로 관리한다. 다만 값을 프로젝트별로
// 재정의하면 Viewer-LoginManager IPC 가 소리 없이 깨지므로 이 헤더
// 하나를 SSOT 로 삼는다.
//
// 사용 프로젝트:
//   - Agent 1.0 / 3.0        (WM_Cliphelper_*, UM_SENDSCREEN_*, WM_LS_*)
//   - Viewer 1.0 / 3.0       (WM_LS_*, WM_ASSERVER*, WM_LOGOFF_*, UM_*)
//   - 통합 LoginManager      (WM_LS_* 수신)
//
// wire struct (msg_ls_invite_agent, msg_ls_agent_info 등) 은 프로젝트마다
// MAX_ID_LEN 이 달라 각 프로젝트 로컬 파일에 유지한다.
// ============================================================================

#include <windows.h>   // WM_USER

// Clipboard helper (Agent tvnserver-app 계열)
enum ls_clip_msg
{
    WM_Clip_Notify_FileCopyEvent    = WM_USER + 14,
    WM_Cliphelper_Notify_ToFolder   = WM_USER + 15,
    WM_Cliphelper_Req_ToFolder      = WM_USER + 16,
    WM_Cliphelper_StopReq_ToFolder  = WM_USER + 17,
    WM_Cliphelper_Ready             = WM_USER + 18,
    WM_Cliphelper_FileCopyEvent     = WM_USER + 19,
    WM_Cliphelper_Req_Filelist      = WM_USER + 20,
    WM_Cliphelper_Ack_Filelist      = WM_USER + 21,
};

// LoginManager Supporter 및 Viewer↔Agent 세션 제어
enum ls_supporter_msg
{
    // WM_USER + 99 는 Viewer 의 rfb/MsgDefs.h 재정의분(SESSION_RESTORE) 흡수.
    WM_LS_SESSION_RESTORE           = WM_USER + 99,
    WM_LS_RECONNECT_REQ             = WM_USER + 100,
    WM_LS_RECONNECT_ACK             = WM_USER + 101,
    WM_ASSERVERREADY                = WM_USER + 102,
    WM_ASSERVERREADY2               = WM_USER + 103,
    WM_LS_REQUEST_AGENTLIST         = WM_USER + 104,
    WM_LS_REQUEST_AGENTLIST_ACK     = WM_USER + 105,
    WM_LS_INVITEAGENT               = WM_USER + 106,
    WM_LS_INVITEAGENT_ACK           = WM_USER + 107,
    WM_LS_VIEWER_END                = WM_USER + 108,
    WM_LS_VIEWER_CONNECTED          = WM_USER + 109,
    WM_ASSERVER_END                 = WM_USER + 129,
    WM_LS_FOWAREDVIEWER_CONNECTED   = WM_USER + 130,  // 원본 철자 유지(FOWARDED)
    WM_LOGOFF_RECONNECT_REQ         = WM_USER + 210,
    WM_LOGOFF_RECONNECT_ACK         = WM_USER + 211,
    WM_REQUEST_PREVIEW              = WM_USER + 212,
    WM_REQUEST_PREVIEW_ACK          = WM_USER + 213,
};

// Viewer 내부 창간 통신 및 Host↔Viewer 이벤트
enum um_viewer_msg
{
    UM_VIEWERWND_SYSTEMINFO                 = WM_USER + 10000,
    UM_VIEWERWND_PROCESSINFO                = WM_USER + 10001,
    UM_VIEWERWND_DISPLAY_COUNT              = WM_USER + 10002,
    UM_VIEWERWND_CHAT_COMMAND               = WM_USER + 10003,
    UM_VIEWERWND_AUDIO_CHAT_COMMAND         = WM_USER + 10004,
    UM_SENDSCREEN_READY                     = WM_USER + 10005,  // Agent
    UM_SENDSCREEN_ENDED                     = WM_USER + 10006,  // Agent
    UM_HOST_RESOLUTION                      = WM_USER + 10007,
    UM_VIEWERWND_REBOOT_RES                 = WM_USER + 10008,
    UM_CHANGE_MONITOR                       = WM_USER + 10009,
    UM_CHANGE_LP                            = WM_USER + 10010,
    UM_VIEWERWND_SERVER_WHITEBOARD_REQUEST  = WM_USER + 10011,
    UM_VIEWER_TO_HOST_PINGPONG_TIMEOUT      = WM_USER + 10025,
    WM_USER_NEW_LISTENING                   = WM_USER + 20007,
};