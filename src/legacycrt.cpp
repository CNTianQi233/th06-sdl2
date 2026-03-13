// legacycrt.cpp — compiled with modern MSVC (cc_imgui rule).
// Provides legacy CRT symbol stubs that VC7-compiled objects and d3dx8.lib
// reference but that modern libcmt/ucrt does NOT export (they are inlined).
//
// Do NOT include <stdio.h>: it would inline vsprintf/sprintf and cause conflicts.
#include <stdarg.h>

extern "C" {

// Modern ucrt exports _vsnprintf as a real (non-inline) function.
int __cdecl _vsnprintf(char* _Buffer, unsigned int _MaxCount, const char* _Format, va_list _ArgList);

// vsprintf — VC7 objects call vsprintf() which generates linker ref _vsprintf.
// Modern libcmt inlines vsprintf() so _vsprintf is absent from the lib.
int __cdecl vsprintf(char* _Buffer, const char* _Format, va_list _ArgList)
{
    return _vsnprintf(_Buffer, 0x7FFFFFFF, _Format, _ArgList);
}

// sprintf — VC7 objects call sprintf() which generates linker ref _sprintf.
// Modern libcmt inlines sprintf() so _sprintf is absent from the lib.
int __cdecl sprintf(char* _Buffer, const char* _Format, ...)
{
    va_list _ArgList;
    va_start(_ArgList, _Format);
    int _Result = _vsnprintf(_Buffer, 0x7FFFFFFF, _Format, _ArgList);
    va_end(_ArgList);
    return _Result;
}

// _snprintf — d3dx8.lib calls _snprintf() which generates linker ref __snprintf.
// Modern libcmt does not export __snprintf.
int __cdecl _snprintf(char* _Buffer, unsigned int _MaxCount, const char* _Format, ...)
{
    va_list _ArgList;
    va_start(_ArgList, _Format);
    int _Result = _vsnprintf(_Buffer, _MaxCount, _Format, _ArgList);
    va_end(_ArgList);
    return _Result;
}

} // extern "C"
