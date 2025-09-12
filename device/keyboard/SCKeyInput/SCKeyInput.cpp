#include "SCKeyInput.h"

#include <WinUser.h>
#include <thread>

#include "Common/Functions.h"

int nUnicodeHangleStart = 0xAC00;
int nUnicodeHangleEnd = 0xD79F;

CString m_astrChoHangle[] = {
    _T("��"), _T("��"), _T("��"), _T("��"), _T("��"),
    _T("��"), _T("��"), _T("��"), _T("��"), _T("��"),
    _T("��"), _T("��"), _T("��"), _T("��"), _T("��"),
    _T("��"), _T("��"), _T("��"), _T("��")
};

CString m_astrChoEnglish[] = {
    _T("r"), _T("R"), _T("s"), _T("e"), _T("E"),
    _T("f"), _T("a"), _T("q"), _T("Q"), _T("t"),
    _T("T"), _T("d"), _T("w"), _T("W"), _T("c"),
    _T("z"), _T("x"), _T("v"), _T("g")
};

int m_nNumOfCho = 19;
CString m_astrJungHangle[] = {
    _T("��"), _T("��"), _T("��"), _T("��"), _T("��"),
    _T("��"), _T("��"), _T("��"), _T("��"), _T("��"),
    _T("��"), _T("��"), _T("��"), _T("��"), _T("��"),
    _T("��"), _T("��"), _T("��"), _T("��"), _T("��"),
    _T("��")
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
      _T(""), _T("��"), _T("��"), _T("��"), _T("��"),
    _T("��"), _T("��"), _T("��"), _T("��"), _T("��"),
    _T("��"), _T("��"), _T("��"), _T("��"), _T("��"),
    _T("��"), _T("��"), _T("��"), _T("��"), _T("��"),
    _T("��"), _T("��"), _T("��"), _T("��"), _T("��"),
    _T("��"), _T("��"), _T("��")
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
	add(str);
}

CSCKeyInput::~CSCKeyInput()
{

}

void CSCKeyInput::add(CString str)
{
	for (int i = 0; i < str.GetLength(); i++)
		add(str[i]);
}

void CSCKeyInput::add(TCHAR ch)
{
	m_key.push_back(ch);

    if (!m_thread_running)
    {
        std::thread th(&CSCKeyInput::thread_function, this);
        th.detach();
    }
}

//Ű�Է� thread�� ���� �����Ų��.
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
		if (m_key.size())
		{
			TCHAR ch = m_key[0];
			m_key.pop_front();
            PressKey(ch, 10);
			TRACE(_T("ch = %c\n"), ch);
            if (!m_thread_running)
                break;
		}
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
	}

    m_thread_running = false;
    m_thread_terminated = true;
	TRACE(_T("thread_function() terminated.\n"));
}

// <summary>���ڸ� Ű���� �Է����� ó���Ѵ�.</summary>
// <param name="chChar">���ڰ�</param>
// <param name="nDelay">Ű �Է½� Delay</param>
// <note>SendInput(): http://msdn.microsoft.com/ko-kr/library/windows/desktop/ms646310(v=vs.85).aspx </note>
void CSCKeyInput::PressKey(TCHAR chChar, int delay)
{
    try
    {
        CString astrSplit = SplitHangle(chChar, true);

        if (!astrSplit.IsEmpty())
        {
            //���� �� Ư�����ڴ� �� ���̰� 1, �ѱ��̸� 2 �̻��̶�� ������ ó����.
            bool bHangle = (astrSplit.GetLength() > 1);

            // �ѱ� ����
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
                                bool bCapsLock = false;// Control.IsKeyLocked(Keys.CapsLock);    // true�� ��� �빮�� �Է� ����
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
                            // Caps Lock�� ���¿� ���� ��/�ҹ��� ó��
                            if (bShift)
                            {
                                keybd_event(VK_LSHIFT, 0x00, 0, 0);
                                //Sleep(nDelay);
                                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                            }

                            // Key ���� ó����.
                            //int nValue = Convert.ToInt32(chValue);
                            //int nValue = (int)Keys.Oemtilde;
                            keybd_event(nValue, 0x00, 0, 0);
                            //Sleep(nDelay);
                            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                            keybd_event(nValue, 0x00, KEYEVENTF_KEYUP, 0);
                            //Sleep(nDelay);
                            std::this_thread::sleep_for(std::chrono::milliseconds(delay));

                            // Caps Lock ���¸� ȸ����.
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

            // �ѱ� ����
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


/// <summary>�Է� ���ڰ� �ѱ��� ��� �ڸ� �и��� ������ �����ϸ�, �׷��� ���� ��� �Է� ���ڸ� �����Ѵ�,</summary>
/// <param name="ch">��ȯ�� ����</param>
/// <param name="bToEnglish">true�� ��� �ѱ� �ڸ� ���� Ű ������ ��ȯ�ϸ�, �׷��� ���� ��� �ѱ� �ڸ� �����Ѵ�.</param>
/// <returns>ch�� �ѱ��� ��� �ѱ� Ǯ�� ���� ���ڿ��� �����ϸ�, �׷��� ���� ��� ch�� �����Ѵ�.</returns>
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

