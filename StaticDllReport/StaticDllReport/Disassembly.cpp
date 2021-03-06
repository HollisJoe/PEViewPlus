#include "pch.h"
#include "Disassembly.h"

#include <iostream>
#include <stdio.h>
#include <string>
#include <cinttypes>
#include "capstone/capstone.h"
using namespace std;
#pragma comment(lib, "legacy_stdio_definitions.lib")

#ifndef  _WIN64
#pragma comment(lib,"capstone_static_x86.lib")
#else
#pragma comment(lib,"capstone_static_x64.lib")
#endif

#if _MSC_VER>=1900
#include "stdio.h"
_ACRTIMP_ALT FILE* __cdecl __acrt_iob_func(unsigned);
#ifdef __cplusplus
extern "C"
#endif
FILE* __cdecl __iob_func(unsigned i) {
	return __acrt_iob_func(i);
}
#endif /* _MSC_VER>=1900 */
int code_prefixsize(unsigned char* code, int base, int code_size) {
	csh handle;
	cs_insn* insn;
	int result = -1;
#ifndef _WIN64
	if (cs_open(CS_ARCH_X86, CS_MODE_32, &handle)) {
#else
	if (cs_open(CS_ARCH_X86, CS_MODE_64, &handle)) {
#endif
		printf("ERROR: Failed to initialize engine!\n");
		return -1;
	}
	if (base > code_size) {
		return -1;
	}
	size_t count = cs_disasm(handle, (unsigned char*)code, code_size, 0x00, 0, &insn);
	if (count) {
		for (size_t j = 0; j < count; j++) {
			if (insn[j].address >= base) {
				result = insn[j].address;
				break;
			}
		}
	} else {
		printf("ERROR: Failed to disassemble given code!\n");
		exit(-1);
	}
	cs_free(insn, count);
	return result;
}

string code_disassembly(unsigned char* code, int code_size, int start) {
	csh handle;
	cs_insn* insn;
#ifndef _WIN64
	if (cs_open(CS_ARCH_X86, CS_MODE_32, &handle)) {
#else
	if (cs_open(CS_ARCH_X86, CS_MODE_64, &handle)) {
#endif

		printf("ERROR: Failed to initialize engine!\n");
		exit(-1);
	}

	size_t count = cs_disasm(handle, (unsigned char*)code, code_size, start, 0, &insn);
	string result, tmp;
	if (count) {
		for (size_t j = 0; j < count; j++) {
			char buffer[80];
			memset(buffer, 0, 80);
#ifndef _WIN64
			sprintf_s(buffer, "%08llX:\t%s\t\t%s\n", insn[j].address, insn[j].mnemonic, insn[j].op_str);
#else
			sprintf_s(buffer, "%016I64X:\t%s\t\t%s\n", insn[j].address, insn[j].mnemonic, insn[j].op_str);
#endif
			result.append(buffer);
		}
	} else {
		printf("ERROR: Failed to disassemble given code!\n");
		exit(-1);
	}
	cs_free(insn, count);
	return result;
}