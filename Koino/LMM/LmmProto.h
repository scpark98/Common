#pragma once

// ============================================================================
// LmmProto.h — Agent ↔ LoginManager UDP 프로토콜 상수 (단일 SSOT)
// ============================================================================
//
// 정의된 상수는 오직 다음 한 경로에서만 사용된다:
//     Agent (LMMHost/LMMSEAgent 서비스) → LoginManager (LMMLgiMgr.exe)
//     via CAsyncSocket UDP, 127.0.0.1:LOGIN_MGR_PORT (기본 20177 / SE 20178)
//
// 다른 프로세스 (KMS 원격 서버, AP2P, nScreen*, hwmon, AutoPatcher 등) 는
// 이 상수 공간을 공유하지 않는다. KMS 서버 TCP 통신용 NMS_* / KMS_C_* /
// NMS_LMM_* 는 KmsProto.h 에 별도로 두고 여기와 섞지 않는다.
//
// prefix 의미:  LM = LoginManager (수신자), AGENT = 송신 주체 (Agent).
// 즉 "Agent 가 LoginManager 로 보내는 UDP 커맨드" 라는 의미.
//
// 이 파일을 참조하는 프로젝트:
//   - 통합 LoginManager  (LMM_Koino_service\LMMLoginManager)
//   - Agent 1.0          (LMM_Koino_service\LMMHost)
//   - Agent 3.0 SE       (LMM_SE_Koino\lmmHost)
// 세 프로젝트는 이 헤더 하나를 include 하며, 값을 프로젝트별로 재정의하지
// 않는다. 값 변경은 세 프로젝트가 함께 재배포될 때만 안전.
//
// enum 사용 이유: #define 대비 삽입/수정/삭제 시 값 관리가 명료하고,
// 컴파일러가 중복 정의를 잡아준다. unscoped enum 으로 두어 기존 코드
// (msg.command = LM_AGENT_LOGIN_OK; / switch (msg.command)) 를 캐스팅
// 없이 그대로 사용한다.
//
// 값 재할당 히스토리 (통합 시점):
//   - 1.0 이 이미 108/109 를 P2P_EXECUTE_OK / CONFIG_CHANGED 로 실사용 중.
//   - 3.0 SE 는 109/110 자리에 MESSAGEBOX / URL_OPEN 을 두었으나 소스 상
//     주석 상태로만 존재 (활성 송신 지점 없음, 3.0 조사 결과).
//   - 따라서 1.0 값을 유지하고 3.0 SE 계열을 110~ 로 이동.
// ============================================================================

enum lm_agent_cmd
{
    LM_AGENT_EXECUTE_OK          = 100,
    LM_AGENT_LOGIN_OK            = 102,
    LM_AGENT_SERVER_CON_FAIL     = 103,
    LM_AGENT_ID_PASS_FAIL        = 104,
    LM_AGENT_LOGOUT              = 105,
    LM_AGENT_VOLUME_FULL         = 106,
    LM_AGENT_LICENSE_EXPIRED     = 107,
    LM_AGENT_P2P_EXECUTE_OK      = 108,
    LM_AGENT_CONFIG_CHANGED      = 109,

    // 3.0 SE 유래 — 원래 109~113. 108/109 충돌 회피로 110~ 재할당.
    // 3.0 SE 활성 송신은 없었으므로 프로덕션 wire 호환성 영향 없음.
    LM_AGENT_MESSAGEBOX          = 110,
    LM_AGENT_URL_OPEN            = 111,
    LM_AGENT_LOGIN_ID_IS_EMPTY   = 112,
    LM_AGENT_LOGIN_ID_NOT_FOUND  = 113,
    LM_AGENT_LOGIN_FAILED        = 114,
};