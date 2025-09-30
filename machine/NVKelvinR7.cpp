#define NDEBUG
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <combaseapi.h>
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "d3d8types.h"
#include "d3d8caps.h"
typedef float D3DVALUE;
#define D3DSIO_DCL 31
#define D3DSIO_DEF 81

#define INTERFACE ID3DXBuffer
DECLARE_INTERFACE_(ID3DXBuffer,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** ID3DXBuffer methods ***/
    STDMETHOD_(LPVOID,GetBufferPointer)(THIS) PURE;
    STDMETHOD_(DWORD,GetBufferSize)(THIS) PURE;
};
#undef INTERFACE

extern "C" HRESULT WINAPI D3DXCreateBuffer(DWORD NumBytes, ID3DXBuffer** ppBuffer);

extern "C" int _fltused = 1;
extern "C" const GUID IID_IUnknown = {0,0,0,{0,0,0,0,0,0,0,0x46}};

void* operator new(size_t size)
{
    return malloc(size);
}

void operator delete(void* ptr) noexcept
{
    free(ptr);
}

bool outputMemoryEnable;
static char* outputMemoryData;
static size_t outputMemorySize;

static int DebugPrintf(bool breakline, const char* format, ...)
{
    va_list args;
    va_start(args, format);

    int length = vsnprintf(NULL, 0, format, args);
    char* buffer = (char*)malloc(length + 4);
    length = vsnprintf(buffer, length + 4, format, args);

    if (breakline) {
        buffer[length + 0] = '\n';
        buffer[length + 1] = 0;
        length++;
    }

    if (strncmp(buffer, "000:", 4) == 0 || strncmp(buffer, "ps. ", 4) == 0) {
        outputMemoryEnable = true;
        outputMemorySize = 0;
    }
    if (strncmp(buffer, " ", 1) == 0) {
        outputMemoryEnable = false;
    }
    if (outputMemoryEnable) {
        outputMemoryData = (char*)realloc(outputMemoryData, outputMemorySize + length);
        memcpy(outputMemoryData + outputMemorySize, buffer, length);
        outputMemorySize += length;
    }

    printf("%s", buffer);
    free(buffer);

    va_end(args);

    return 0;
}

#define _DEBUG
#define DEBUG
#define dbgLevel 0xFFFFFFFF
#define DPF(...) DebugPrintf(true, __VA_ARGS__)
#define DPF_PLAIN(...) DebugPrintf(false, __VA_ARGS__)
#define DPF_LEVEL(level, ...) DebugPrintf(true, __VA_ARGS__)
#define DPF_LEVEL_PLAIN(level, ...) DebugPrintf(false, __VA_ARGS__)
#define AllocIPM malloc
#define FreeIPM free
#define NVARCH 0x20
#define nvAssert assert

#include "nvKelvinProgram.cpp"
#include "nvPShad.cpp"
#include "vpcompilekelvin.c"
#include "vpoptimize.c"

static GLOBALDATA DriverData;
GLOBALDATA* pDriverData = &DriverData;

struct CPixelShaderPublic : public CPixelShader {
    using CPixelShader::m_dwStage;
    using CPixelShader::m_cw;
    using CPixelShader::m_pixelShaderConsts;
    using CPixelShader::m_dwPixelShaderConstFlags;
    using CPixelShader::GetShaderProgram;
    using CPixelShader::PSProgramNames;
};

#pragma comment(linker, "/export:NvCompileShader=_NvCompileShader@20")

extern "C"
HRESULT WINAPI NvCompileShader(const uint32_t* shader, size_t size, const char* folder, const char* gpu, void** binary)
{
    if (shader && size >= 4 && (shader[0] & 0xFFFF0000) == D3DVS_VERSION(0, 0)) {
        if ((shader[0] & 0xFFFF) > 0x0101) {
            return 0x80000000;
        }

        KELVIN_PROGRAM program = {};
        ParsedProgram parsed = {};
        nvKelvinParseVertexShaderCode(&program, &parsed, (DWORD*)shader, size / sizeof(DWORD));

        parsed.firstInstruction = program.code;
        parsed.liveOnEntry = FALSE;
        parsed.IsStateProgram = FALSE;
        vp_Optimize(&parsed, program.dwNumInstructions, 0);

        VtxProgCompileKelvin env = {};
        env.caller_id = CALLER_ID_D3D;
        env.malloc = [](void*, size_t size) { return malloc(size); };
        env.free = [](void*, void* pointer) { return free(pointer); };

        VertexProgramOutput output = {};
        vp_CompileKelvin(&env, &parsed, program.dwNumInstructions, &output);
        free(output.residentProgram);
    }
    else if (shader && size >= 4 && (shader[0] & 0xFFFF0000) == D3DPS_VERSION(0, 0)) {
        if ((shader[0] & 0xFFFF) > 0x0103) {
            return 0x80000000;
        }

        DriverData.nvD3DPerfData.dwNVClasses = NVCLASS_FAMILY_KELVIN;
//      if (strlen(gpu) > 2 && gpu[2] == '1') {
//          DriverData.nvD3DPerfData.dwNVClasses = NVCLASS_FAMILY_CELSIUS;
//      }

        CPixelShaderPublic pixelShader;
        pixelShader.create(nullptr, 0, size / sizeof(DWORD), (DWORD*)shader);
    }

    ID3DXBuffer* blob = nullptr;
    if (outputMemoryData && outputMemorySize) {
        D3DXCreateBuffer(DWORD(outputMemorySize), &blob);
        if (blob) {
            memcpy(blob->GetBufferPointer(), outputMemoryData, outputMemorySize); 
        }        
        (*binary) = blob;
        free(outputMemoryData);
    }

    return blob ? 0 : 0x80000000;
}

#if 0
            case D3DSIO_COMMENT:
                DPF_LEVEL_PLAIN(NVDBG_LEVEL_VSHADER_INS, "%2d COMMENT (TODO)", dwInstruction);
                pCode += (dwToken & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT;
                dwInstruction--;
                nvAssert(0);
                break;
            case D3DSIO_DCL:
                DPF_LEVEL_PLAIN(NVDBG_LEVEL_VSHADER_INS, "%2d DCL ", dwInstruction);
                pCode += 2;
                dwInstruction--;
                break;
            case D3DSIO_DEF:
                DPF_LEVEL_PLAIN(NVDBG_LEVEL_VSHADER_INS, "%2d DEF ", dwInstruction);
                pCode += 5;
                dwInstruction--;
                break;
#endif
