/*
	author: ls
*/

# include <QtCore/QCoreApplication>
# define _CRT_SECURE_NO_WARNINGS
# include "stdio.h"
# include "stdlib.h"
# include "windows.h"
# include "PE.h"
# include "tchar.h"
# include<qdebug.h>

//# define NEW_FILE "C:/Users/ls/Desktop/hello_out.exe"

/*
6��ͨ����д����̨���򣬽�һ��EXE�ļ���ȡ���ڴ棬�����Ŀ�ִ�н�(�����)�м�һ�������Ի���(MessgeBox)��ShellCode��
ͨ���޸ĳ���ִ�����ʵ���ļ���Ⱦ�������������С�
*/
//%49%6e%6a%65%63%74%65%64
/*BYTE ShellCode[] = {
	0x6A, 0x00,						//push 0x00
	0x6A, 0x00,						//push 0x00
	0x6A, 0x00,						//push 0x00
	0x6A, 0x00,						//push 0x00
	0xE8, 0x00, 0x00, 0x00, 0x00,	//jmp 0x00000000
	0xE9, 0x00, 0x00, 0x00, 0x00	//call 0x00000000
};*/

BYTE ShellCode[] = {
	0x33, 0xDB,						//xor ebx, ebx
	0x53,							//push ebx
	0x68, 0x63, 0x74, 0x65, 0x64,	//push 64657463               Injected
	0x68, 0x49, 0x6E, 0x6A, 0x65,	//push 656A6E49
	0x8B, 0xC4,						//mov eax, esp
	0x53,							//push ebx
	0x50,							//push eax			
	0x50,							//push eax
	0x53,//19						//push ebx
	0xE8, 0x00, 0x00, 0x00, 0x00,	//jmp 0x00000000
	0xE9, 0x00, 0x00, 0x00, 0x00	//call 0x00000000
};
int InfectionFile(PVOID FileAddress, PDWORD FileLength)
{
	int ret = 0;

	DWORD ShellLength = 29;
	DWORD InsertAddress = 0;

	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)(FileAddress);
	PIMAGE_FILE_HEADER pFileHeader = (PIMAGE_FILE_HEADER)((DWORD)pDosHeader + pDosHeader->e_lfanew + 4);
	PIMAGE_OPTIONAL_HEADER32 pOptionalHeader = (PIMAGE_OPTIONAL_HEADER32)((DWORD)pFileHeader + sizeof(IMAGE_FILE_HEADER));
	PIMAGE_SECTION_HEADER pSectionGroup = (PIMAGE_SECTION_HEADER)((DWORD)pOptionalHeader + pFileHeader->SizeOfOptionalHeader);
	PIMAGE_SECTION_HEADER pInsertSection = NULL;

	FILE* pf = fopen(FILE_PATH, "rb");
	if (pf == NULL)
	{
		ret = -1;
		printf("func fopen() Error: %d\n", ret);
		return ret;
	}
	ret = GetFileLength(pf, FileLength);
	if (ret != 0 && *FileLength == -1)
	{
		ret = -2;
		printf("func GetFileLength() Error!\n");
		return ret;
	}

	//1����ȡ��ǰ�����ImageBase	
	DWORD ImageBase = pOptionalHeader->ImageBase;

	//2����̬��ȡ������MessageBoxA������ַ
	HMODULE hModule = LoadLibraryA("User32.dll");
	DWORD FuncAddress = (DWORD)GetProcAddress(hModule, "MessageBoxA");

	//3����ȡShellCode�����λ��  Ϊ�˱��������ָ���ĳ�����ڵ���в���(������Щ�������ļ���СΪ0)
	//	(1)����ȡ������ڵ����ڵĽ��� �˿ڵ���RVA
	DWORD AddressOfEntryPoint_FOA = 0;
	ret = RVA_TO_FOA(FileAddress, pOptionalHeader->AddressOfEntryPoint, &AddressOfEntryPoint_FOA);
	if (ret != 0)
	{
		return ret;
	}

	for (int i = 0; i < pFileHeader->NumberOfSections; i++)
	{
		if (AddressOfEntryPoint_FOA >= pSectionGroup[i].PointerToRawData && AddressOfEntryPoint_FOA < pSectionGroup[i].PointerToRawData + pSectionGroup[i].SizeOfRawData)
		{
			pInsertSection = &pSectionGroup[i];
			break;
		}
	}

	//	(2)���жϸý����Ŀռ��Ƿ��㹻
	if (ShellLength >= pInsertSection->SizeOfRawData - pInsertSection->Misc.VirtualSize)
	{
		ret = -6;
		printf("func InfectionFile() Error:%d �ý����ռ䲻��!\n", ret);
		return ret;
	}

	//	(3)����ȡ�����ַ
	InsertAddress = pInsertSection->Misc.VirtualSize + pInsertSection->PointerToRawData;

	//3������E8 Call��ĵ�ַ
	DWORD E8_Next_FOA = 0;
	DWORD E8_Next_RVA = 0;
	DWORD E8_Next_VA = 0;
	DWORD E8_X_Address = 0;

	//	(1)��������һ��ָ���ַFOA
	E8_Next_FOA = InsertAddress + 24;

	//	(2)������һ��ָ��ĵ�ַת����RVA
	ret = FOA_TO_RVA(FileAddress, E8_Next_FOA, &E8_Next_RVA);
	if (ret != 0)
	{
		return ret;
	}

	//	(3)�����������ַVA
	E8_Next_VA = E8_Next_RVA + ImageBase;

	//	(4)������X  X = ����Ҫ��ת�ĵ�ַ - E8����ָ�����һ�е�ַ
	E8_X_Address = FuncAddress - E8_Next_VA;

	//	(5)���޸�ShellCode
	memcpy(&ShellCode[20], &E8_X_Address, 4);

	//4������E9 jmp��ĵ�ַ
	DWORD E9_Next_FOA = 0;
	DWORD E9_Next_RVA = 0;
	DWORD E9_Next_VA = 0;
	DWORD E9_X_Address = 0;

	//	(1)��������һ��ָ���ַFOA
	E9_Next_FOA = InsertAddress + 29;

	//	(2)������һ��ָ��ĵ�ַת����RVA
	ret = FOA_TO_RVA(FileAddress, E9_Next_FOA, &E9_Next_RVA);
	if (ret != 0)
	{
		return ret;
	}

	//	(3)�����������ַVA
	E9_Next_VA = E9_Next_RVA + ImageBase;

	//	(4)������X  X = ����Ҫ��ת�ĵ�ַ - E9����ָ�����һ�е�ַ
	E9_X_Address = pOptionalHeader->AddressOfEntryPoint + ImageBase - E9_Next_VA;

	//	(5)���޸�ShellCode
	memcpy(&ShellCode[25], &E9_X_Address, 4);

	//5������������
	DWORD OEP = 0;
	PDWORD pAddressOfEntryPoint = &pOptionalHeader->AddressOfEntryPoint;
	//	(1)����OEP��ַת����RVA
	ret = FOA_TO_RVA(FileAddress, InsertAddress, &OEP);
	if (ret != 0)
	{
		return ret;
	}

	//	(2)���޸�OEP
	*pAddressOfEntryPoint = OEP;

	//6����ShellCode�������ļ�
	memcpy((PVOID)((DWORD)FileAddress + InsertAddress), ShellCode, ShellLength);

	return ret;
}


int main()
{
	int ret = 0;
	//
	char newFile[50] = "C:/Users/ls/Desktop/hello_out.exe";
	PVOID FileAddress = NULL;
	DWORD FileLength = 0;

	//1�����ļ����뵽�ڴ�   
	ret = MyReadFile(&FileAddress);
	if (ret != 0)
	{
		if (FileAddress != NULL)
			free(FileAddress);
		return ret;
	}

	//2�����и�Ⱦ����
	ret = InfectionFile(FileAddress, &FileLength);
	if (ret != 0)
	{
		if (FileAddress != NULL)
			free(FileAddress);
		return ret;
	}

	//3�����޸ĺ���ļ�д��Ӳ��
	ret = MyWriteFile(FileAddress, FileLength, newFile);
	if (ret != 0)
	{
		if (FileAddress != NULL)
			free(FileAddress);
		return ret;
	}

	qDebug() << "PE inject success!\n";

	if (FileAddress != NULL)
		free(FileAddress);

	return ret;
}
