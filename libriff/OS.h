
/** $VER: OS.h (2023.07.26) P. Stuer **/

#pragma once

#include <SDKDDKVer.h>
#include <Windows.h>

#include <string>

using namespace std;

class OS
{
public:
    OS() { }

public:
    static bool GetLastErrorMessage(wstring & errorMessage);
    static bool GetErrorMessage(DWORD errorNumber, wstring & errorMessage);
};
