#pragma once

// ============================================================================
// KmsProto.h — KMS 원격 서버 및 관련 통신 채널의 프로토콜 상수 SSOT
// ============================================================================
//
// 이 파일에는 wire 상수 (enum) 만 담는다.
// wire 구조체 (msg_login, msg_keep_connection, msg_authenticate 등) 는
// 프로젝트마다 MAX_ID_LEN / MAX_PW_LEN 값이 달라 각 프로젝트의 로컬
// KmsProto.h / kmsProto.h 에 그대로 남긴다.
//
// 사용 프로젝트:
//   - Agent 1.0     (LMM_Koino_service\LMMHost\tvnserver-app)
//   - Agent 3.0 SE  (LMM_SE_Koino\lmmHost\tvnserver-app)
//   - Viewer 1.0    (LMM_Koino_service\KoinoViewerNew\...\KNORemoteCoreLibrary)
//   - Viewer 3.0 SE (LMM_SE_Koino\koinoviewerNew\...\KNORemoteCoreLibrary)
// 네 프로젝트가 이 헤더 하나를 include 하며, 상수 값을 프로젝트별로
// 재정의하지 않는다. 값 변경은 이 파일 하나만 편집.
//
// #define 대신 unscoped enum 사용 이유:
//   - 삽입/수정/삭제 시 값 관리가 명료하고 컴파일러가 중복 정의를 잡아준다.
//   - unscoped 라 기존 코드 (short command = NMS_CS_LOGIN; switch(cmd))
//     를 캐스팅 없이 그대로 사용 가능.
//
// wire 값을 임의로 바꾸면 원격 KMS 서버와의 프로토콜이 깨진다.
// 신규 상수 추가 시 뒤쪽 미사용 값으로만 확장.
// ============================================================================

// AutoPatcher (Viewer 사용)
enum kms_autopatcher_cmd
{
    NMS_AUTOPATCHER_CONNECT         = 111,
    NMS_AUTOPATCHER_VERSION         = 121,
    NMS_AUTOPAFTCHER_ISLATEST       = 122,  // 원본 철자 오타 유지 (호출부 호환)
    NMS_AUTOPATCHER_OK              = 131,
    NMS_AUTOPATCHER_FTPCONNECTION   = 141,
    NMS_AUTOPATCHER_FTPFINISH       = 142,
};

// NMS_CS_* Client-Server 공통 (200번대)
enum nms_cs_cmd
{
    NMS_CS_START                    = 200,
    NMS_CS_START_OK                 = 201,
    NMS_CS_RSA_PUBLIC_KEY           = 211,
    NMS_CS_AES_SECRET_KEY           = 212,
    NMS_CS_LOGIN                    = 221,
    NMS_CS_LOGIN_ERROR              = 222,
    NMS_CS_LOGIN_OK                 = 223,
    NMS_CS_SERVER_IP_NAME           = 224,
    NMS_CS_INCOMING_TEST            = 225,
    NMS_CS_INCOMING_TEST_SKIP       = 226,
    NMS_CS_LISTEN_READY             = 227,
    NMS_CS_INCOMING_SUCCESS         = 228,
    NMS_CS_INCOMING_FAIL            = 229,
    NMS_CS_AUTO_LOGIN               = 230,  // Agent
    NMS_CS_SERVER_FULL              = 231,
    NMS_CS_SERVER_OK                = 232,
    NMS_CS_SERVER_LIST              = 233,
    NMS_CS_SERVER_SELECT            = 234,
    NMS_CS_SERVER_NUM               = 235,
    NMS_CS_N2N_LIST                 = 241,
    NMS_CS_N2N_SELECT               = 242,
    NMS_CS_OK                       = 251,
    NMS_CS_EXCEED                   = 260,
    NMS_CS_LOGIN_AUTH_EXPIRED       = 270,  // Agent
    NMS_CS_LOGIN_ERROR_NOT_FOUND    = 272,  // Agent
    NMS_CS_AUTO_SETUP_LOGIN         = 273,  // Agent
};

// NMS_C_* / KMS_C_* Client-specific (300번대)
enum nms_c_cmd
{
    NMS_C_START                     = 300,  // Viewer
    NMS_C_START_OK                  = 301,  // Viewer
    NMS_C_AUTHENTICATE              = 321,
    NMS_C_AUTHENTICATE_ERROR        = 322,
    NMS_C_AUTHENTICATE_OK           = 323,
    NMS_C_CLIENT_INFO               = 324,
    NMS_C_INCOMING_TEST             = 325,
    NMS_C_INCOMING_TEST_SKIP        = 326,
    NMS_C_LISTEN_READY              = 327,
    NMS_C_INCOMING_SUCCESS          = 328,
    NMS_C_INCOMING_FAIL             = 329,
    NMS_C_N2N_SERVER                = 330,  // Viewer
    NMS_C_CTOS                      = 341,
    NMS_C_STOC                      = 342,
    NMS_C_CSTONMS                   = 343,
    NMS_C_SCTONMS                   = 343,  // 원본 alias 유지 (동일 값)
    NMS_C_SELF                      = 344,
    NMS_C_OK                        = 345,
    NMS_C_CTOS_NFTD                 = 346,
    NMS_C_STOC_NFTD                 = 347,
    NMS_C_CSTONMS_NFTD              = 348,
    NMS_C_SCTONMS_NFTD              = 348,  // 원본 alias 유지 (동일 값)
    KMS_C_CONNECTION_HISTORY        = 349,  // by hchkim
    KMS_C_CLIENT_INFO_FORCE_CONN    = 350,  // by hchkim
    KMS_C_N2N_CONNECTION            = 351,  // by hchkim
    NMS_C_EXCEED                    = 352,  // 20240909 scpark 동시접속자수 초과
};

// NMS_CS_TCP_* / KMS_CS_TCP_* TCP-only (400~500번대)
enum nms_cs_tcp_cmd
{
    NMS_CS_TCP_KEEP_CONNECTION      = 421,
    NMS_CS_TCP_VIEWER_CONNECT       = 431,
    NMS_CS_TCP_PORT_INFO            = 432,
    NMS_CS_TCP_CTOS                 = 441,
    NMS_CS_TCP_STOC                 = 442,
    NMS_CS_TCP_CSTONMS              = 443,
    NMS_CS_TCP_SCTONMS              = 443,  // 원본 alias 유지
    NMS_CS_TCP_SELF                 = 444,
    NMS_CS_TCP_CTOS_NFTD            = 445,
    NMS_CS_TCP_STOC_NFTD            = 446,
    NMS_CS_TCP_CSTONMS_NFTD         = 447,
    NMS_CS_TCP_SCTONMS_NFTD         = 447,  // 원본 alias 유지
    NMS_CS_TCP_OK                   = 451,
    NMS_CS_TCP_MESSAGE              = 461,
    NMS_CS_TCP_LOGOUT               = 462,
    NMS_CS_TCP_KILLALL              = 463,
    NMS_CS_TCP_UPGRADE              = 464,
    NMS_CS_TCP_KICK_UNDEAD          = 465,
    NMS_CS_TCP_COMMAND_OK           = 469,
    KMS_CS_TCP_VIEWER_CONNECT_NAT   = 470,  // by hchkim
    NMS_CS_TCP_UPGRADE_HOST         = 471,  // by min
    NMS_CS_TCP_SESSION_DELETED      = 472,  // by min
    NMS_CS_TCP_P2P                  = 501,  // Agent (Optimal - Park)
};

// NMS_P2P_* Peer-to-peer server-side (Viewer)
enum nms_p2p_cmd
{
    NMS_P2P_START                   = 500,
    NMS_P2P_REQUEST_AUTHENTICATION  = 520,
    NMS_P2P_LOGIN_INFO              = 521,
    NMS_P2P_AUTHENTICATION_ERROR    = 522,
    NMS_P2P_AUTHENTICATION_OK       = 523,
    NMS_P2P_REQUEST_P2P_SERVER_INFO = 541,
    NMS_P2P_P2P_SERVER_INFO         = 542,
    NMS_P2P_SERVER_INFO_UPDATE_OK   = 543,
    NMS_P2P_SERVERNUM_FROM_NMS      = 551,
    NMS_P2P_READY_CS                = 552,
    NMS_P2P_READY_VIEWER            = 553,
    NMS_P2P_END                     = 560,
};

// P2P_* Peer client (600번대)
enum p2p_cmd
{
    P2P_CS_SERVERNUM                = 610,
    P2P_CS_PINGCHECK                = 611,
    P2P_CS_PINGCHECK_ACK            = 612,
    P2P_CS_PINGCHECK_ACK2           = 613,
    P2P_CS_SERVERNUM2               = 614,  // [연결2]
    P2P_C_SERVERNUM                 = 620,
    P2P_C_SERVERNUM2                = 621,  // [연결2]
    P2P_CS_MOBILE_SERVERNUM         = 630,  // [Mobile] Viewer
    P2P_C_MOBILE_SERVERNUM          = 631,  // [Mobile] Viewer
};

// NMS_LS_NO_MEANING* keep-alive ping/pong (Agent)
enum nms_ls_nomeaning_cmd
{
    NMS_LS_NO_MEANING               = 790,
    NMS_LS_NO_MEANING2              = 791,
    NMS_LS_NO_MEANING3              = 792,
    NMS_LS_NO_MEANING4              = 793,
};

// NMS_CS_WEB_* Web-side channel (Agent)
enum nms_cs_web_cmd
{
    NMS_CS_WEB_START                = 800,
    NMS_CS_WEB_START_OK             = 801,
    NMS_CS_WEB_END                  = 802,
    NMS_CS_WEB_SESSION_RESTORE      = 803,
    NMS_CS_WEB_AUTHENTICATE         = 821,
    NMS_CS_WEB_AUTHENTICATE_ERROR   = 822,
    NMS_CS_WEB_AUTHENTICATE_OK      = 823,
    NMS_CS_WEB_SERVER_IP_NAME       = 824,
    NMS_CS_WEB_INCOMING_TEST        = 825,
    NMS_CS_WEB_INCOMING_TEST_SKIP   = 826,
    NMS_CS_WEB_LISTEN_READY         = 827,
    NMS_CS_WEB_INCOMING_SUCCESS     = 828,
    NMS_CS_WEB_INCOMING_FAIL        = 829,
    NMS_CS_WEB_SERVER_FULL          = 831,
    NMS_CS_WEB_CONNINFO             = 832,
    NMS_CS_WEB_SERVER_NUM           = 835,
    NMS_CS_WEB_N2N_LIST             = 841,
    NMS_CS_WEB_N2N_SELECT           = 842,
    NMS_CS_WEB_OK                   = 851,
};

// NMS_LMM_* LinkMeMine service commands (Agent)
enum nms_lmm_cmd
{
    NMS_LMM_START                   = 900,
    NMS_LMM_START_OK                = 901,
    NMS_LMM_LOGIN                   = 902,
    NMS_LMM_LOGIN_ERROR             = 903,
    NMS_LMM_ALREADY_LOGIN           = 904,
    NMS_LMM_LOGIN_OK                = 905,
    NMS_LMM_FORCE_SEND_MSG          = 906,
    NMS_LMM_FORCE_CHAT              = 907,
    NMS_LMM_FORCE_LOGIN             = 908,
    NMS_LMM_FORCE_LOGIN_OK          = 909,
    NMS_LMM_REQUEST_INFO            = 910,
    NMS_LMM_REQUEST_INFO_ACK        = 911,
    NMS_LMM_LOGOFF                  = 912,
    NMS_LMM_UPDATE_HOST             = 913,
    NMS_LMM_COMMAND                 = 914,
    NMS_LMM_COMMAND_WEB             = 915,
    NMS_LMM_ADD_INFO                = 917,
    NMS_LMM_ADD_INFO_OK             = 918,
};

// Misc
enum kms_misc_cmd
{
    NMS_CS_PROTOCOL_TEST            = 950,
    NMS_NO_CRYPT                    = 1000,
    NMS_YES_CRYPT                   = 1001,
};