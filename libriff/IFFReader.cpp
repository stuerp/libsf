
/** $VER: IFFReader.cpp (2023.07.26) P. Stuer **/

#include "IFFReader.h"

bool IFFReader::Open(const WCHAR * filePath, Option options)
{
    if (filePath == nullptr)
        throw RIFFException(L"Invalid argument");

    _Options = options;

    _hFile = ::CreateFileW(filePath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);

    if (_hFile == INVALID_HANDLE_VALUE)
        throw Win32Exception();

    return true;
}

void IFFReader::Close() noexcept
{
    if (_hFile != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(_hFile);
        _hFile = INVALID_HANDLE_VALUE;
    }
}

bool IFFReader::ReadHeader(uint32_t & formType)
{
    Read(&_Header, sizeof(_Header));

    if (_Header.Id != FOURCC_FORM)
        throw RIFFException(L"Invalid header chunk");

    _Header.Size = ((_Header.Size >> 24) & 0xFF) | ((_Header.Size >> 8) & 0xFF00) | ((_Header.Size << 8) & 0xFF0000) | ((_Header.Size << 24) & 0xFF000000);

    if (_Header.Size < sizeof(formType))
        throw RIFFException(L"Invalid header chunk");

    Read(&formType, sizeof(formType));

    return true;
}
