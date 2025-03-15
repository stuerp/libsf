
/** $VER: OS.cpp (2024.09.15) P. Stuer **/

#include "pch.h"

#include "OS.h"

bool OS::GetLastErrorMessage(wstring & errorMessage)
{
    return GetErrorMessage(::GetLastError(), errorMessage);
}

bool OS::GetErrorMessage(DWORD errorNumber, wstring & errorMessage)
{
    WCHAR Message[256];

    ::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, errorNumber, 0, Message, _countof(Message), NULL);

    errorMessage = Message;

    return true;
}
