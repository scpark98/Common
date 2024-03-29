/*------------------------------------------------------
   CCmdLine

   A utility for parsing command lines.

   Copyright (C) 1999 Chris Losinger, Smaller Animals Software.
   http://www.smalleranimals.com

   This software is provided 'as-is', without any express
   or implied warranty.  In no event will the authors be 
   held liable for any damages arising from the use of this software.

   Permission is granted to anyone to use this software 
   for any purpose, including commercial applications, and 
   to alter it and redistribute it freely, subject to the 
   following restrictions:

     1. The origin of this software must not be misrepresented; 
   you must not claim that you wrote the original software. 
   If you use this software in a product, an acknowledgment 
   in the product documentation would be appreciated but is not required.
   
     2. Altered source versions must be plainly marked as such, 
   and must not be misrepresented as being the original software.
   
     3. This notice may not be removed or altered from any source 
   distribution.

   See SACmds.h for more info.
------------------------------------------------------*/

#include "CmdLine.h"
#include "crtdbg.h"

/*------------------------------------------------------
  int CCmdLine::SplitLine(int argc, TCHAR **argv)

  parse the command line into switches and arguments

  returns number of switches found
------------------------------------------------------*/
int CCmdLine::SplitLine(int argc, TCHAR **argv)
{
   clear();

   StringType curParam; // current argv[x]

   // skip the exe name (start with i = 1)
   for (int i = 1; i < argc; i++)
   {
      // if it's a switch, start a new CCmdLine
      if (IsSwitch(argv[i]))
      {
         curParam = argv[i];

         StringType arg;

         // look at next input string to see if it's a switch or an argument
         if (i + 1 < argc)
         {
            if (!IsSwitch(argv[i + 1]))
            {
               // it's an argument, not a switch
               arg = argv[i + 1];

               // skip to next
               i++;
            }
            else
            {
               arg = "";
            }
         }

         // add it
         CCmdParam cmd;

         // only add non-empty args
         if (arg != "")
         {
            cmd.m_strings.push_back(arg);
         }

         // add the CCmdParam to 'this'
         pair<CCmdLine::iterator, bool> res = insert(CCmdLine::value_type(curParam, cmd));

      }
      else
      {
         // it's not a new switch, so it must be more stuff for the last switch

         // ...let's add it
 	      CCmdLine::iterator theIterator;

         // get an iterator for the current param
         theIterator = find(curParam);
	      if (theIterator!=end())
         {
            (*theIterator).second.m_strings.push_back(argv[i]);
         }
         else
         {
            // ??
         }
      }
   }

   return size();
}

//scpark add
//encrypt+base64_encode되어 넘어온 파라미터를 base64_decode+decrypt하면
//"-i 3.38.18.113 -p 7002 -sn 1122 -fr 1572807 -id 1 -t 0 -rh 133534 -rd 10001 -gi None -gp 0 -tn 0 -p2p 0 -p2pi None -p2pp 0 -sizex 0 -sizey 0 -wm 1 -wms None -dm 1 -ra 1"
//위와 같이 하나의 문자열이 되는데 이를 옵션이름(switch)와 옵션값(value)로 분리하여 map에 넣어줌.
int CCmdLine::SplitLine(CString params)
{
    clear();

    //우선 각 파라미터들을 공백으로 분리한다.
    std::vector<CString> ar;

    int curPos = 0;
    CString token, arg;
    CString curSwitch;

    while (true)
    {
        //공백이 연속될 경우에도 잘 추출됨을 확인 완료!
        token = params.Tokenize(_T(" "), curPos);
        if (token.IsEmpty())
            break;
        ar.push_back(token);
    }

    //분리된 각 파라미터를 스위치와 값으로 map에 넣는다.
    for (int i = 0; i < ar.size(); i++)
    {
        //현재 값이 스위치라면
        if (IsSwitch(ar[i]))
        {
            //현재 스위치를 기억해두고
            curSwitch = ar[i];

            //다음 값이 스위치가 아니면
            if (i + 1 < ar.size())
            {
                if (!IsSwitch(ar[i + 1]))
                {
                    //다음 값을 일단 기억해두고
                    arg = ar[i + 1];

                    // skip to next
                    i++;
                }
                else
                {
                    arg = _T("");
                }
            }

            // add it
            CCmdParam cmd;

            // only add non-empty args
            if (!arg.IsEmpty())
            {
                cmd.m_strings.push_back(arg);
            }

            // add the CCmdParam to 'this'
            pair<CCmdLine::iterator, bool> res = insert(CCmdLine::value_type(curSwitch, cmd));
        }
        //스위치가 아니면 연속된 옵션값이므로 해당 스위치에 추가한다.
        else
        {
            // it's not a new switch, so it must be more stuff for the last switch
            // ...let's add it
            CCmdLine::iterator it;

            // get an iterator for the current param
            it = find(curSwitch);
            if (it != end())
            {
                (*it).second.m_strings.push_back(ar[i]);
            }
            else
            {
                //switch가 없는데 값이 나오면 파라미터 양식 오류다.
            }
        }
    }

    return size();
}

/*------------------------------------------------------

   protected member function
   test a parameter to see if it's a switch :

   switches are of the form : -x
   where 'x' is one or more TCHARacters.
   the first TCHARacter of a switch must be non-numeric!

------------------------------------------------------*/

bool CCmdLine::IsSwitch(const TCHAR *pParam)
{
   if (pParam==NULL)
      return false;

   // switches must non-empty
   // must have at least one TCHARacter after the '-'
   int len = _tcslen(pParam);
   if (len <= 1)
   {
      return false;
   }

   // switches always start with '-'
   if (pParam[0]=='-')
   {
      // allow negative numbers as arguments.
      // ie., don't count them as switches
      return (!isdigit(pParam[1]));
   }
   else
   {
      return false;
   }
}

/*------------------------------------------------------
   bool CCmdLine::HasSwitch(const TCHAR *pSwitch)

   was the switch found on the command line ?

   ex. if the command line is : app.exe -a p1 p2 p3 -b p4 -c -d p5

   call                          return
   ----                          ------
   cmdLine.HasSwitch("-a")       true
   cmdLine.HasSwitch("-z")       false
------------------------------------------------------*/

bool CCmdLine::HasSwitch(const TCHAR *pSwitch)
{
	CCmdLine::iterator theIterator;
	theIterator = find(pSwitch);
	return (theIterator!=end());
}

/*------------------------------------------------------

   StringType CCmdLine::GetSafeArgument(const TCHAR *pSwitch, int iIdx, const TCHAR *pDefault)

   fetch an argument associated with a switch . if the parameter at
   index iIdx is not found, this will return the default that you
   provide.

   example :
  
   command line is : app.exe -a p1 p2 p3 -b p4 -c -d p5

   call                                      return
   ----                                      ------
   cmdLine.GetSafeArgument("-a", 0, "zz")    p1
   cmdLine.GetSafeArgument("-a", 1, "zz")    p2
   cmdLine.GetSafeArgument("-b", 0, "zz")    p4
   cmdLine.GetSafeArgument("-b", 1, "zz")    zz

------------------------------------------------------*/

StringType CCmdLine::GetSafeArgument(const TCHAR *pSwitch, int iIdx, const TCHAR *pDefault)
{
   StringType sRet;
   
   if (pDefault!=NULL)
      sRet = pDefault;

   try
   {
      sRet = GetArgument(pSwitch, iIdx);
   }
   catch (...)
   {
   }

   return sRet;
}

/*------------------------------------------------------

   StringType CCmdLine::GetArgument(const TCHAR *pSwitch, int iIdx)

   fetch a argument associated with a switch. throws an exception 
   of (int)0, if the parameter at index iIdx is not found.

   example :
  
   command line is : app.exe -a p1 p2 p3 -b p4 -c -d p5

   call                             return
   ----                             ------
   cmdLine.GetArgument("-a", 0)     p1
   cmdLine.GetArgument("-b", 1)     throws (int)0, returns an empty string

------------------------------------------------------*/

StringType CCmdLine::GetArgument(const TCHAR *pSwitch, int iIdx)
{
   if (HasSwitch(pSwitch))
   {
	   CCmdLine::iterator theIterator;

      theIterator = find(pSwitch);
	   if (theIterator!=end())
      {
         if ((*theIterator).second.m_strings.size() > iIdx)
         {
            return (*theIterator).second.m_strings[iIdx];
         }
      }
   }

   throw (int)0;

   return StringType("");
}

/*------------------------------------------------------
   int CCmdLine::GetArgumentCount(const TCHAR *pSwitch)

   returns the number of arguments found for a given switch.

   returns -1 if the switch was not found

------------------------------------------------------*/

int CCmdLine::GetArgumentCount(const TCHAR *pSwitch)
{
   int iArgumentCount = -1;

   if (HasSwitch(pSwitch))
   {
	   CCmdLine::iterator theIterator;

      theIterator = find(pSwitch);
	   if (theIterator!=end())
      {
         iArgumentCount = (*theIterator).second.m_strings.size();
      }
   }

   return iArgumentCount;
}