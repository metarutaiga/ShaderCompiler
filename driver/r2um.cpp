#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "r2um.h"

#pragma comment(linker, "/export:Rad2Compile=_Rad2Compile@16")

extern "C" {

int _fltused = 1;

uintptr_t __security_cookie = 0;

void __fastcall __security_check_cookie(uintptr_t cookie)
{
}

__declspec(naked) void _ftol2()
{
    __asm fisttp qword ptr [esp-8]
    __asm mov eax, [esp-8]
    __asm mov edx, [esp-4]
    __asm ret
}

__declspec(naked) void _ftol2_sse()
{
    __asm fisttp dword ptr [esp-4]
    __asm mov eax, [esp-4]
    __asm ret
}

void dummy()
{
    va_list args;
    _vsnprintf(nullptr, 0, nullptr, args);
}

static ATID3DCNTX CTX = {};
static RAD2PIXELSHADER PS = {};
static ATIVTXSHADER VS = {};
static ATIVECTOR VS_Code[256];
static ATIVECTOR VS_PosCode[256];

HRESULT APIENTRY Rad2Compile(DWORD* shader, DWORD length, void** binary, DWORD* pBinaryLength)
{
    if (shader == nullptr || length < 4)
        return 0x80000000 + __LINE__;

    if ((shader[0] & 0xFFFF0000) == 0xFFFF0000) {
        DWORD PSRegAssign[8] = { 0, 0, 0, 0, 0, 0, 0xFFFFFFFF, 0xFFFFFFFF };
        Rad2CompilePixelShader(&CTX, &PS, shader, length, PSRegAssign); 

        if (binary && pBinaryLength) {
            DWORD length = sizeof(ATIVECTOR) * (PS.dwInstrCount[0] + PS.dwInstrCount[1]);
            ATIVECTOR* pBinary = (ATIVECTOR*)malloc(length);
            if (pBinary == nullptr)
                return 0x80000000 + __LINE__;
            (*binary) = pBinary;
            (*pBinaryLength) = length;
            for (DWORD i = 0; i < 2; ++i) {
                for (DWORD j = 0; j < PS.dwInstrCount[i]; ++j) {
                    (*pBinary++) = PS.psInstructions[i][j];
                }
            }
            return 0;
        }
    }
    else if ((shader[0] & 0xFFFF0000) == 0xFFFE0000) {
        VS.dwDxCodeSize = length;
        VS.pDxCode = shader;
        VS.pCode = VS_Code;
        VS.pPosCode = VS_PosCode;
        VS_HwAssemble(&CTX, &VS);

        if (binary) {
            DWORD length = sizeof(ATIVECTOR) * VS.dwCodeLength;
            ATIVECTOR* pBinary = (ATIVECTOR*)malloc(length);
            if (pBinary == nullptr)
                return 0x80000000 + __LINE__;
            (*binary) = pBinary;
            (*pBinaryLength) = length;
            for (DWORD i = 0; i < VS.dwCodeLength; ++i) {
                (*pBinary++) = VS.pCode[i];
            }
            return 0;
         }
    }

    return 0x80000000 + __LINE__;
}

}  // extern "C"
