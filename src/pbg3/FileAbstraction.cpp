#include "pbg3/FileAbstraction.hpp"

#ifdef _WIN32

namespace th06
{
FileAbstraction::FileAbstraction()
{
    handle = INVALID_HANDLE_VALUE;
    access = 0;
}

i32 FileAbstraction::Open(char *filename, char *mode)
{
    int creationDisposition;
    i32 goToEnd = FALSE;

    this->Close();

    char *curMode;
    for (curMode = mode; *curMode != '\0'; curMode += 1)
    {
        if (*curMode == 'r')
        {
            this->access = GENERIC_READ;
            creationDisposition = OPEN_EXISTING;
            break;
        }
        else if (*curMode == 'w')
        {
            DeleteFileA(filename);
            this->access = GENERIC_WRITE;
            creationDisposition = OPEN_ALWAYS;
            break;
        }
        else if (*curMode == 'a')
        {
            goToEnd = true;
            this->access = GENERIC_WRITE;
            creationDisposition = OPEN_ALWAYS;
            break;
        }
    }

    if (*curMode == '\0')
    {
        return 0;
    }
    this->handle = CreateFileA(filename, this->access, FILE_SHARE_READ, NULL, creationDisposition,
                               FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    if (this->handle == INVALID_HANDLE_VALUE)
        return 0;

    if (goToEnd)
    {
        SetFilePointer(this->handle, 0, NULL, FILE_END);
    }
    return 1;
}

i32 FileAbstraction::OpenW(const wchar_t *filename, char *mode)
{
    int creationDisposition;
    i32 goToEnd = FALSE;

    this->Close();

    char *curMode;
    for (curMode = mode; *curMode != '\0'; curMode += 1)
    {
        if (*curMode == 'r')
        {
            this->access = GENERIC_READ;
            creationDisposition = OPEN_EXISTING;
            break;
        }
        else if (*curMode == 'w')
        {
            DeleteFileW(filename);
            this->access = GENERIC_WRITE;
            creationDisposition = OPEN_ALWAYS;
            break;
        }
        else if (*curMode == 'a')
        {
            goToEnd = true;
            this->access = GENERIC_WRITE;
            creationDisposition = OPEN_ALWAYS;
            break;
        }
    }

    if (*curMode == '\0')
    {
        return 0;
    }
    this->handle = CreateFileW(filename, this->access, FILE_SHARE_READ, NULL, creationDisposition,
                               FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    if (this->handle == INVALID_HANDLE_VALUE)
        return 0;

    if (goToEnd)
    {
        SetFilePointer(this->handle, 0, NULL, FILE_END);
    }
    return 1;
}

void FileAbstraction::Close()
{
    if (this->handle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(this->handle);
        this->handle = INVALID_HANDLE_VALUE;
        this->access = 0;
    }
}

i32 FileAbstraction::Read(u8 *data, u32 dataLen, u32 *numBytesRead)
{
    if (this->access != GENERIC_READ)
    {
        return FALSE;
    }

    return ReadFile(this->handle, data, dataLen, reinterpret_cast<DWORD *>(numBytesRead), NULL);
}

i32 FileAbstraction::Write(u8 *data, u32 dataLen, u32 *outWritten)
{
    if (this->access != GENERIC_WRITE)
    {
        return FALSE;
    }

    return WriteFile(this->handle, data, dataLen, reinterpret_cast<DWORD *>(outWritten), NULL);
}

i32 FileAbstraction::ReadByte()
{
    u8 data;
    u32 outBytesRead;

    if (this->Read(&data, 1, &outBytesRead) == FALSE)
    {
        return -1;
    }
    else
    {
        if (outBytesRead == 0)
        {
            return -1;
        }
        return data;
    }
}

i32 FileAbstraction::WriteByte(u32 b)
{
    u8 outByte;
    u32 outBytesWritten;

    outByte = b;
    if (this->Write(&outByte, 1, &outBytesWritten) == FALSE)
    {
        return -1;
    }
    else
    {
        if (outBytesWritten == 0)
        {
            return -1;
        }
        return b;
    }
}

i32 FileAbstraction::Seek(u32 amount, u32 seekFrom)
{
    if (this->handle == INVALID_HANDLE_VALUE)
    {
        return 0;
    }

    SetFilePointer(this->handle, amount, NULL, seekFrom);
    return 1;
}

u32 FileAbstraction::Tell()
{
    if (this->handle == INVALID_HANDLE_VALUE)
    {
        return 0;
    }

    return SetFilePointer(this->handle, 0, NULL, FILE_CURRENT);
}

u32 FileAbstraction::GetSize()
{
    if (this->handle == INVALID_HANDLE_VALUE)
    {
        return 0;
    }

    return GetFileSize(this->handle, NULL);
}

u8 *FileAbstraction::ReadWholeFile(u32 maxSize)
{
    if (this->access != GENERIC_READ)
    {
        return NULL;
    }

    u32 dataLen = this->GetSize();
    u32 outDataLen;
    if (dataLen <= maxSize)
    {
        u8 *data = reinterpret_cast<u8 *>(LocalAlloc(LPTR, dataLen));
        if (data != NULL)
        {
            u32 oldLocation = this->Tell();
            // Pretty sure the plan here was to seek to 0, but woops the code
            // is buggy.
            if (this->Seek(oldLocation, FILE_BEGIN) != 0)
            {
                if (this->Read(data, dataLen, &outDataLen) == 0)
                {
                    LocalFree(data);
                    return NULL;
                }
                this->Seek(oldLocation, FILE_BEGIN);
                return data;
            }
            // Yes, this case leaks the data. Amazing, I know.
        }
    }
    return NULL;
}

FileAbstraction::~FileAbstraction()
{
    this->Close();
}
}; // namespace th06

#else // !_WIN32 — POSIX implementation using standard C FILE*

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <unistd.h>

namespace th06
{

// On Linux, we store a FILE* in the handle field (cast to void*).
// access: 1 = read, 2 = write.

static FILE *AsFile(HANDLE h) { return reinterpret_cast<FILE *>(h); }

FileAbstraction::FileAbstraction()
{
    handle = INVALID_HANDLE_VALUE;
    access = 0;
}

i32 FileAbstraction::Open(char *filename, char *mode)
{
    this->Close();

    const char *fopenMode = "rb";
    for (char *curMode = mode; *curMode != '\0'; curMode++)
    {
        if (*curMode == 'r') { fopenMode = "rb"; this->access = 1; break; }
        else if (*curMode == 'w') { fopenMode = "wb"; this->access = 2; break; }
        else if (*curMode == 'a') { fopenMode = "ab"; this->access = 2; break; }
    }

    FILE *f = fopen(filename, fopenMode);
    if (!f)
    {
        // Case-insensitive fallback: try lowercase filename
        char lower[512];
        size_t len = strlen(filename);
        if (len < sizeof(lower))
        {
            for (size_t i = 0; i <= len; i++)
                lower[i] = (filename[i] >= 'A' && filename[i] <= 'Z') ? filename[i] + 32 : filename[i];
            f = fopen(lower, fopenMode);
        }
    }
    if (!f)
    {
        this->handle = INVALID_HANDLE_VALUE;
        return 0;
    }

    this->handle = reinterpret_cast<HANDLE>(f);
    return 1;
}

i32 FileAbstraction::OpenW(const wchar_t *filename, char *mode)
{
    // Convert wchar_t filename to UTF-8 for POSIX
    char utf8Name[512];
    size_t len = wcslen(filename);
    if (len >= sizeof(utf8Name)) return 0;
    for (size_t i = 0; i <= len; i++)
        utf8Name[i] = (char)(filename[i] & 0x7F); // ASCII subset only (game file names)
    return this->Open(utf8Name, mode);
}

void FileAbstraction::Close()
{
    if (this->handle != INVALID_HANDLE_VALUE && this->handle != NULL)
    {
        fclose(AsFile(this->handle));
        this->handle = INVALID_HANDLE_VALUE;
        this->access = 0;
    }
}

i32 FileAbstraction::Read(u8 *data, u32 dataLen, u32 *numBytesRead)
{
    if (this->access != 1) return FALSE;
    size_t n = fread(data, 1, dataLen, AsFile(this->handle));
    if (numBytesRead) *numBytesRead = (u32)n;
    return TRUE;
}

i32 FileAbstraction::Write(u8 *data, u32 dataLen, u32 *outWritten)
{
    if (this->access != 2) return FALSE;
    size_t n = fwrite(data, 1, dataLen, AsFile(this->handle));
    if (outWritten) *outWritten = (u32)n;
    return TRUE;
}

i32 FileAbstraction::ReadByte()
{
    u8 data;
    u32 outBytesRead;
    if (this->Read(&data, 1, &outBytesRead) == FALSE) return -1;
    if (outBytesRead == 0) return -1;
    return data;
}

i32 FileAbstraction::WriteByte(u32 b)
{
    u8 outByte = (u8)b;
    u32 outBytesWritten;
    if (this->Write(&outByte, 1, &outBytesWritten) == FALSE) return -1;
    if (outBytesWritten == 0) return -1;
    return b;
}

i32 FileAbstraction::Seek(u32 amount, u32 seekFrom)
{
    if (this->handle == INVALID_HANDLE_VALUE) return 0;
    int whence = SEEK_SET;
    if (seekFrom == 1) whence = SEEK_CUR;
    else if (seekFrom == 2) whence = SEEK_END;
    fseek(AsFile(this->handle), (long)amount, whence);
    return 1;
}

u32 FileAbstraction::Tell()
{
    if (this->handle == INVALID_HANDLE_VALUE) return 0;
    return (u32)ftell(AsFile(this->handle));
}

u32 FileAbstraction::GetSize()
{
    if (this->handle == INVALID_HANDLE_VALUE) return 0;
    FILE *f = AsFile(this->handle);
    long cur = ftell(f);
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, cur, SEEK_SET);
    return (u32)size;
}

u8 *FileAbstraction::ReadWholeFile(u32 maxSize)
{
    if (this->access != 1) return NULL;
    u32 dataLen = this->GetSize();
    u32 outDataLen;
    if (dataLen <= maxSize)
    {
        u8 *data = (u8 *)calloc(1, dataLen);
        if (data != NULL)
        {
            u32 oldLocation = this->Tell();
            if (this->Seek(oldLocation, 0) != 0)
            {
                if (this->Read(data, dataLen, &outDataLen) == 0)
                {
                    free(data);
                    return NULL;
                }
                this->Seek(oldLocation, 0);
                return data;
            }
        }
    }
    return NULL;
}

FileAbstraction::~FileAbstraction()
{
    this->Close();
}
}; // namespace th06

#endif // _WIN32
