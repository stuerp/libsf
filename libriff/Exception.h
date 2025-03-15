
/** $VER: Exception.h (2025.03.09) P. Stuer **/

#pragma once

#include <Windows.h>
#include <strsafe.h>

#include <exception>
#include <string>
#include <ranges>

#pragma warning(disable: 4820)

namespace riff
{
class exception : public std::exception
{
public:
    exception() { }

    exception(const std::string & message) : _Message(message) { }
   
    exception(const char * message) : _Message(message) { }

    exception(const exception & re) : _Message(re._Message) { }

    virtual ~exception() noexcept {}

    virtual const char * what() const noexcept
    {
       return _Message.c_str();
    }

private:
    std::string _Message;
};

#pragma warning(disable: 4820)

class win32_exception : public exception
{
public:
    win32_exception() : exception(), _ErrorCode(::GetLastError()) { Init(); }

    win32_exception(DWORD errorCode) : exception(), _ErrorCode(errorCode) { Init(); }

    win32_exception(const std::string & description) : exception(description), _ErrorCode(::GetLastError()) { Init(); }

    win32_exception(const std::string & description, DWORD errorCode) : exception(description), _ErrorCode(errorCode) { Init(); }

    virtual const char * what() const noexcept
    {
        return _ErrorMessage.c_str();
    }

private:
    void Init()
    {
        _ErrorMessage.resize(512, '\0');

        if (::FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, _ErrorCode, 0, (LPSTR) _ErrorMessage.data(), (DWORD) _ErrorMessage.size(), NULL) > 1)
        {
            size_t Index = _ErrorMessage.rfind("\r\n");

            if (Index != std::string::npos)
                _ErrorMessage.erase(Index, 2);
        }
        else
            ::StringCchPrintfA((LPSTR) _ErrorMessage.data(), _ErrorMessage.capacity(), "Error %d", _ErrorCode);
    }

private:
    DWORD _ErrorCode;
    std::string _ErrorMessage;
};
}
