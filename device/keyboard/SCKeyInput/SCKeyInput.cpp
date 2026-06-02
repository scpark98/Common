#include "SCKeyInput.h"

#include <WinUser.h>
#include <thread>

#include "Common/Functions.h"

int nUnicodeHangleStart = 0xAC00;
int nUnicodeHangleEnd = 0xD79F;

CString m_astrChoHangle[] = {
    _T("ㄱ"), _T("ㄲ"), _T("ㄴ"), _T("ㄷ"), _T("ㄸ"),
    _T("ㄹ"), _T("ㅁ"), _T("ㅂ"), _T("ㅃ"), _T("ㅅ"),
    _T("ㅆ"), _T("ㅇ"), _T("ㅈ"), _T("ㅉ"), _T("ㅊ"),
    _T("ㅋ"), _T("ㅌ"), _T("ㅍ"), _T("ㅎ")
};

CString m_astrChoEnglish[] = {
    _T("r"), _T("R"), _T("s"), _T("e"), _T("E"),
    _T("f"), _T("a"), _T("q"), _T("Q"), _T("t"),
    _T("T"), _T("d"), _T("w"), _T("W"), _T("c"),
    _T("z"), _T("x"), _T("v"), _T("g")
};

int m_nNumOfCho = 19;
CString m_astrJungHangle[] = {
    _T("ㅏ"), _T("ㅐ"), _T("ㅑ"), _T("ㅒ"), _T("ㅓ"),
    _T("ㅔ"), _T("ㅕ"), _T("ㅖ"), _T("ㅗ"), _T("ㅘ"),
    _T("ㅙ"), _T("ㅚ"), _T("ㅛ"), _T("ㅜ"), _T("ㅝ"),
    _T("ㅞ"), _T("ㅟ"), _T("ㅠ"), _T("ㅡ"), _T("ㅢ"),
    _T("ㅣ")
};

CString m_astrJungEnglish[] = {
    _T("k"), _T("o"), _T("i"), _T("O"), _T("j"),
    _T("p"), _T("u"), _T("P"), _T("h"), _T("hk"),
    _T("ho"), _T("hl"), _T("y"), _T("n"), _T("nj"),
    _T("np"), _T("nl"), _T("b"), _T("m"), _T("ml"),
    _T("l")
};

int m_nNumOfJung = 21;// m_astrJungHangle.Length;
CString m_astrJongHangle[] = {
      _T(""), _T("ㄱ"), _T("ㄲ"), _T("ㄳ"), _T("ㄴ"),
    _T("ㄵ"), _T("ㄶ"), _T("ㄷ"), _T("ㄹ"), _T("ㄺ"),
    _T("ㄻ"), _T("ㄼ"), _T("ㄽ"), _T("ㄾ"), _T("ㄿ"),
    _T("ㅀ"), _T("ㅁ"), _T("ㅂ"), _T("ㅄ"), _T("ㅅ"),
    _T("ㅆ"), _T("ㅇ"), _T("ㅈ"), _T("ㅊ"), _T("ㅋ"),
    _T("ㅌ"), _T("ㅍ"), _T("ㅎ")
};

CString m_astrJongEnglish[] = {
    _T(""), _T("r"), _T("R"), _T("rt"), _T("s"),
    _T("sw"), _T("sg"), _T("e"), _T("f"), _T("fr"),
    _T("fa"), _T("fq"), _T("ft"), _T("fx"), _T("fv"),
    _T("fg"), _T("a"), _T("q"), _T("qt"), _T("t"),
    _T("T"), _T("d"), _T("w"), _T("c"), _T("z"),
    _T("x"), _T("v"), _T("g")
};

int m_nNumOfJong = 28;// m_astrJongHangle.Length;

CSCKeyInput::CSCKeyInput(CString str)
{
    std::thread th(&CSCKeyInput::thread_function, this);
    th.detach();

    input(str);
}

CSCKeyInput::~CSCKeyInput()
{
	//ctor 에서 std::thread 를 detach 했기 때문에 thread 자신이 종료되어야 args control block(tuple<func, this>,
	//16 bytes) 이 해제된다. destructor 가 stop() 호출 없이 빈 채로 두면 thread 가 영원히 살아 control block 이
	//leak 으로 보고된다 (_CrtSetBreakAlloc 확인 결과 정확히 이 경로). stop() 가 m_thread_running=false 설정 +
	//thread_function 의 m_thread_terminated=true 까지 polling 대기하므로 안전한 정상 종료.
	stop();
}

void CSCKeyInput::input(CString str)
{
	for (int i = 0; i < str.GetLength(); i++)
        input(str[i]);
}

void CSCKeyInput::input(TCHAR ch)
{
    m_mutex.lock();
	m_key.push_back(ch);
	m_mutex.unlock();
}

int	CSCKeyInput::get_key_count()
{
    m_mutex.lock();
    int count = m_key.size();
    m_mutex.unlock();
    return count;
}

//키입력 thread를 정상 종료시킨다.
void CSCKeyInput::stop()
{
    m_thread_running = false;
    while (!m_thread_terminated)
        Wait(10);

    TRACE(_T("thread_function() stop() success.\n"));
}

void CSCKeyInput::thread_function()
{
	m_thread_running = true;
    m_thread_terminated = false;

	while (m_thread_running)
	{
        int size = 0;

        m_mutex.lock();
        size = m_key.size();
		m_mutex.unlock();

		if (size > 0)
		{
            TCHAR ch = m_key[0];
            TRACE(_T("size = %d, ch = %c\n"), m_key.size(), ch);
			m_mutex.lock();
			m_key.pop_front();
			m_mutex.unlock();
            PressKey(ch, 10);
			TRACE(_T("ch = %c\n"), ch);
            if (!m_thread_running)
                break;
		}
        else
        {
            //TRACE(_T("m_key.size() = 0\n"));
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
	}

    m_thread_running = false;
    m_thread_terminated = true;
	TRACE(_T("thread_function() terminated.\n"));
}

// <summary>문자를 키보드 입력으로 처리한다.</summary>
// <param name="chChar">문자값</param>
// <param name="nDelay">키 입력시 Delay</param>
// <note>SendInput(): http://msdn.microsoft.com/ko-kr/library/windows/desktop/ms646310(v=vs.85).aspx </note>
void CSCKeyInput::PressKey(TCHAR chChar, int delay)
{
    try
    {
        CString astrSplit = SplitHangle(chChar, true);

        if (!astrSplit.IsEmpty())
        {
            //영문 및 특수문자는 그 길이가 1, 한글이면 2 이상이라는 전제로 처리함.
            bool bHangle = (astrSplit.GetLength() > 1);

            // 한글 설정
            if (bHangle)
            {
                keybd_event(VK_HANGUL, 0x00, 0, 0);
                //Sleep(nDelay);
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));

                keybd_event(VK_HANGUL, 0x00, KEYEVENTF_KEYUP, 0);
                //Sleep(nDelay);
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            }

            for (int i = 0; i < astrSplit.GetLength(); i++)
            {
                CString strSplit = (CString)(astrSplit[i]);

                if (!strSplit.IsEmpty())
                {
                    for (int j = 0; j < strSplit.GetLength(); j++)
                    {
                        TCHAR chSplit = strSplit[j];
                        bool bUpper = ('A' <= chSplit) && (chSplit <= 'Z');
                        bool bLower = ('a' <= chSplit) && (chSplit <= 'z');
                        int nValue;
                        bool bShift = false;

                        if (bUpper || bLower)
                        {
                            if (bHangle)
                            {
                                bShift = false;
                            }
                            else
                            {
                                bool bCapsLock = false;// Control.IsKeyLocked(Keys.CapsLock);    // true인 경우 대문자 입력 상태
                                bShift = bCapsLock != bUpper;
                            }

                            if (bUpper)
                            {
                                nValue = (int)chSplit;
                            }
                            else
                            {
                                nValue = _toupper(chSplit);
                            }
                        }
                        else
                        {
                            //https://learn.microsoft.com/ko-kr/windows/win32/inputdev/virtual-key-codes
                            switch (chSplit)
                            {
                            case '~': bShift = true; nValue = (int)VK_OEM_3; break;
                            case '_': bShift = true; nValue = (int)VK_OEM_MINUS; break;
                            case '+': bShift = true; nValue = (int)VK_OEM_PLUS; break;
                            case '{': bShift = true; nValue = (int)VK_OEM_4; break;
                            case '}': bShift = true; nValue = (int)VK_OEM_6; break;
                            case '|': bShift = true; nValue = (int)VK_OEM_5; break;
                            case ':': bShift = true; nValue = (int)VK_OEM_1; break;
                            case '"': bShift = true; nValue = (int)VK_OEM_7; break;
                            case '<': bShift = true; nValue = (int)VK_OEM_COMMA; break;
                            case '>': bShift = true; nValue = (int)VK_OEM_PERIOD; break;
                            case '?': bShift = true; nValue = (int)VK_OEM_2; break;

                            case '!': bShift = true; nValue = (int)'1'; break;
                            case '@': bShift = true; nValue = (int)'2'; break;
                            case '#': bShift = true; nValue = (int)'3'; break;
                            case '$': bShift = true; nValue = (int)'4'; break;
                            case '%': bShift = true; nValue = (int)'5'; break;
                            case '^': bShift = true; nValue = (int)'6'; break;
                            case '&': bShift = true; nValue = (int)'7'; break;
                            case '*': bShift = true; nValue = (int)'8'; break;
                            case '(': bShift = true; nValue = (int)'9'; break;
                            case ')': bShift = true; nValue = (int)'0'; break;

                            case '`': bShift = false; nValue = (int)VK_OEM_3; break;
                            case '-': bShift = false; nValue = (int)VK_OEM_MINUS; break;
                            case '=': bShift = false; nValue = (int)VK_OEM_PLUS; break;
                            case '[': bShift = false; nValue = (int)VK_OEM_4; break;
                            case ']': bShift = false; nValue = (int)VK_OEM_6; break;
                            case '\\': bShift = false; nValue = (int)VK_OEM_5; break;
                            case ';': bShift = false; nValue = (int)VK_OEM_1; break;
                            case '\'': bShift = false; nValue = (int)VK_OEM_7; break;
                            case ',': bShift = false; nValue = (int)VK_OEM_COMMA; break;
                            case '.': bShift = false; nValue = (int)VK_OEM_PERIOD; break;
                            case '/': bShift = false; nValue = (int)VK_OEM_2; break;

                            case '1': bShift = false; nValue = (int)'1'; break;
                            case '2': bShift = false; nValue = (int)'2'; break;
                            case '3': bShift = false; nValue = (int)'3'; break;
                            case '4': bShift = false; nValue = (int)'4'; break;
                            case '5': bShift = false; nValue = (int)'5'; break;
                            case '6': bShift = false; nValue = (int)'6'; break;
                            case '7': bShift = false; nValue = (int)'7'; break;
                            case '8': bShift = false; nValue = (int)'8'; break;
                            case '9': bShift = false; nValue = (int)'9'; break;
                            case '0': bShift = false; nValue = (int)'0'; break;

                            case ' ': bShift = false; nValue = (int)VK_SPACE; break;
                            case '\x1b': bShift = false; nValue = (int)VK_ESCAPE; break;
                            case '\b': bShift = false; nValue = (int)VK_BACK; break;
                            case '\t': bShift = false; nValue = (int)VK_TAB; break;
                            //case '\a': bShift = false; nValue = (int)Keys.LineFeed; break;
                            case '\r': bShift = false; nValue = (int)VK_RETURN; break;

                            default:
                                bShift = false; nValue = 0; break;
                            }
                        }

                        if (nValue != 0)
                        {
                            // Caps Lock의 상태에 따른 대/소문자 처리
                            if (bShift)
                            {
                                keybd_event(VK_LSHIFT, 0x00, 0, 0);
                                //Sleep(nDelay);
                                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                            }

                            // Key 눌림 처리함.
                            //int nValue = Convert.ToInt32(chValue);
                            //int nValue = (int)Keys.Oemtilde;
                            keybd_event(nValue, 0x00, 0, 0);
                            //Sleep(nDelay);
                            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                            keybd_event(nValue, 0x00, KEYEVENTF_KEYUP, 0);
                            //Sleep(nDelay);
                            std::this_thread::sleep_for(std::chrono::milliseconds(delay));

                            // Caps Lock 상태를 회복함.
                            if (bShift)
                            {
                                keybd_event(VK_LSHIFT, 0x00, KEYEVENTF_KEYUP, 0);
                                //Sleep(nDelay);
                                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                            }
                        }
                    }
                }
            }

            // 한글 해제
            if (bHangle)
            {
                keybd_event(VK_HANGUL, 0x00, 0, 0);
                //Sleep(nDelay);
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));

                keybd_event(VK_HANGUL, 0x00, KEYEVENTF_KEYUP, 0);
                //Sleep(nDelay);
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            }
        }
    }
    catch (...)//Exception ex)
    {
        //ex.ToString();
    }
}


/// <summary>입력 문자가 한글인 경우 자모 분리된 문자을 응답하며, 그렇지 않은 경우 입력 문자를 응답한다,</summary>
/// <param name="ch">변환할 문자</param>
/// <param name="bToEnglish">true인 경우 한글 자모를 영문 키 값으로 변환하며, 그렇지 않은 경우 한글 자모를 응답한다.</param>
/// <returns>ch가 한글인 경우 한글 풀어 쓰기 문자열을 응답하며, 그렇지 않은 경우 ch를 응답한다.</returns>
CString CSCKeyInput::SplitHangle(TCHAR ch, bool bToEnglish)
{
    try
    {
        if ((nUnicodeHangleStart <= ch) && (ch <= nUnicodeHangleEnd))
        {
            int nHangleValue = ch - nUnicodeHangleStart;
            int nJong = nHangleValue % m_nNumOfJong; nHangleValue /= m_nNumOfJong;
            int nJung = nHangleValue % m_nNumOfJung; nHangleValue /= m_nNumOfJung;
            int nCho = nHangleValue % m_nNumOfCho;
            /*
            String[] astrCho, astrJung, astrJong;

            if (bToEnglish)
            {
                astrCho = m_astrChoEnglish;
                astrJung = m_astrJungEnglish;
                astrJong = m_astrJongEnglish;
            }
            else
            {
                astrCho = m_astrChoHangle;
                astrJung = m_astrJungHangle;
                astrJong = m_astrJongHangle;
            }

            //String[] astrHangle = new String[3];
            //astrHangle[0] = astrCho[nCho];
            //astrHangle[1] = astrJung[nJung];
            //astrHangle[2] = astrJong[nJong];
            */

            CString astrHangle;
            astrHangle += (bToEnglish ? m_astrChoEnglish[nCho] : m_astrChoHangle[nCho]);
            astrHangle += (bToEnglish ? m_astrJungEnglish[nJung] : m_astrJungHangle[nJung]);
            astrHangle += (bToEnglish ? m_astrJongEnglish[nJong] : m_astrJongHangle[nJong]);

            return astrHangle;
        }
    }
    catch (...)//Exception ex)
    {
        //ex.ToString();
    }

    return CString(ch);// new String[1]{ ch.ToString() };
}

