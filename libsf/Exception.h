
/** $VER: Exception.h (2025.03.09) P. Stuer **/

#pragma once

#include <Windows.h>
#include <strsafe.h>

#include <exception>
#include <string>

#include "Encoding.h"

#pragma warning(disable: 4820)

namespace sf
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

}