/*
	author: ls
*/

# define _CRT_SECURE_NO_WARNINGS
# include <QtCore/QCoreApplication>
# include "stdio.h"
# include "stdlib.h"
# include "windows.h"
# include "PE.h"
# include "string"
# include "qdebug.h"
#include <IMAGEHLP.H>
#pragma comment(lib, "ImageHlp.lib")

/*
1����λ���������ӡ��������е����ݡ�ͬʱ��ӡ��INT���IAT��
*/

int PrintImportTable(PVOID FileAddress)
{
	int ret = 0;
	//1��ָ���������
	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)(FileAddress);
	PIMAGE_FILE_HEADER pFileHeader = (PIMAGE_FILE_HEADER)((DWORD)pDosHeader + pDosHeader->e_lfanew + 4);
	PIMAGE_OPTIONAL_HEADER32 pOptionalHeader = (PIMAGE_OPTIONAL_HEADER32)((DWORD)pFileHeader + sizeof(IMAGE_FILE_HEADER));

	//2����ȡ�����ĵ�ַ
	DWORD ImportDirectory_RVAAdd = pOptionalHeader->DataDirectory[1].VirtualAddress;
	DWORD ImportDirectory_FOAAdd = 0;
	//	(1)���жϵ�����Ƿ����
	if (ImportDirectory_RVAAdd == 0)
	{
		printf("ImportDirectory ������!\n");
		return ret;
	}
	//	(2)����ȡ������FOA��ַ
	ret = RVA_TO_FOA(FileAddress, ImportDirectory_RVAAdd, &ImportDirectory_FOAAdd);
	if (ret != 0)
	{
		printf("func RVA_TO_FOA() Error!\n");
		return ret;
	}

	//3��ָ�����
	PIMAGE_IMPORT_DESCRIPTOR ImportDirectory = (PIMAGE_IMPORT_DESCRIPTOR)((DWORD)FileAddress + ImportDirectory_FOAAdd);

	//4��ѭ����ӡÿһ����������Ϣ  ��Ҫ��ԱΪ0ʱ����ѭ��
	while (ImportDirectory->FirstThunk && ImportDirectory->OriginalFirstThunk)
	{
		//	(1)��ȡ�����ļ�������
		DWORD ImportNameAdd_RVA = ImportDirectory->Name;
		DWORD ImportNameAdd_FOA = 0;
		ret = RVA_TO_FOA(FileAddress, ImportNameAdd_RVA, &ImportNameAdd_FOA);
		if (ret != 0)
		{
			printf("func RVA_TO_FOA() Error!\n");
			return ret;
		}
		PCHAR pImportName = (PCHAR)((DWORD)FileAddress + ImportNameAdd_FOA);

		printf("=========================ImportTable %s Start=============================\n", pImportName);
		printf("OriginalFirstThunk RVA:%08X\n", ImportDirectory->OriginalFirstThunk);

		//	(2)ָ��INT��
		DWORD OriginalFirstThunk_RVA = ImportDirectory->OriginalFirstThunk;
		DWORD OriginalFirstThunk_FOA = 0;
		ret = RVA_TO_FOA(FileAddress, OriginalFirstThunk_RVA, &OriginalFirstThunk_FOA);
		if (ret != 0)
		{
			printf("func RVA_TO_FOA() Error!\n");
			return ret;
		}
		PDWORD OriginalFirstThunk_INT = (PDWORD)((DWORD)FileAddress + OriginalFirstThunk_FOA);

		//	(3)ѭ����ӡINT�������		������Ϊ0ʱ����
		while (*OriginalFirstThunk_INT)
		{
			//	(4)�����ж�,������λΪ1���ǰ���ŵ�����Ϣ,ȥ�����λ���Ǻ������,���������ֵ���
			if ((*OriginalFirstThunk_INT) >> 31)	//���λ��1,��ŵ���
			{
				//	(5)��ȡ�������
				DWORD Original = *OriginalFirstThunk_INT << 1 >> 1;	//ȥ����߱�־λ��
				printf("����ŵ���: %08Xh -- %08dd\n", Original, Original);	//16���� -- 10 ����
			}
			else	//���ֵ���
			{
				//	(5)��ȡ������
				DWORD ImportNameAdd_RAV = *OriginalFirstThunk_INT;
				DWORD ImportNameAdd_FOA = 0;
				ret = RVA_TO_FOA(FileAddress, ImportNameAdd_RAV, &ImportNameAdd_FOA);
				if (ret != 0)
				{
					printf("func RVA_TO_FOA() Error!\n");
					return ret;
				}
				PIMAGE_IMPORT_BY_NAME ImportName = (PIMAGE_IMPORT_BY_NAME)((DWORD)FileAddress + ImportNameAdd_FOA);
				printf("�����ֵ���[HINT/NAME]: %02X--%s\n", ImportName->Hint, ImportName->Name);
			}

			//	(6)ָ����һ����ַ
			OriginalFirstThunk_INT++;
		}
		printf("----------------------------------------------------------------\n");
		printf("FirstThunk RVA   :%08X\n", ImportDirectory->FirstThunk);
		printf("TimeDateStamp    :%08X\n", ImportDirectory->TimeDateStamp);

		//	(2)ָ��IAT��
		DWORD FirstThunk_RVA = ImportDirectory->FirstThunk;
		DWORD FirstThunk_FOA = 0;
		ret = RVA_TO_FOA(FileAddress, FirstThunk_RVA, &FirstThunk_FOA);
		if (ret != 0)
		{
			printf("func RVA_TO_FOA() Error!\n");
			return ret;
		}
		PDWORD FirstThunk_IAT = (PDWORD)((DWORD)FileAddress + FirstThunk_FOA);

		//	(3)�ж�IAT���Ƿ񱻰�	ʱ��� = 0:û�а󶨵�ַ	ʱ��� = 0xFFFFFFFF:�󶨵�ַ	����֪ʶ�ڰ󶨵������
		if (ImportDirectory->TimeDateStamp == 0xFFFFFFFF)
		{
			while (*FirstThunk_IAT)
			{
				printf("�󶨺�����ַ: %08X\n", *FirstThunk_IAT);
				FirstThunk_IAT++;
			}
		}
		else
		{
			//	(4)ѭ����ӡIAT�������		������Ϊ0ʱ����	��ӡ������INT��һ��
			while (*FirstThunk_IAT)
			{
				//	(5)�����ж�,������λΪ1���ǰ���ŵ�����Ϣ,ȥ�����λ���Ǻ������,���������ֵ���
				if ((*FirstThunk_IAT) >> 31)	//���λ��1,��ŵ���
				{
					//	(6)��ȡ�������
					DWORD Original = *FirstThunk_IAT << 1 >> 1;	//ȥ����߱�־λ��
					printf("����ŵ���: %08Xh -- %08dd\n", Original, Original);	//16���� -- 10 ����
				}
				else	//���ֵ���
				{
					//	(7)��ȡ������
					DWORD ImportNameAdd_RAV = *FirstThunk_IAT;
					DWORD ImportNameAdd_FOA = 0;
					ret = RVA_TO_FOA(FileAddress, ImportNameAdd_RAV, &ImportNameAdd_FOA);
					if (ret != 0)
					{
						printf("func RVA_TO_FOA() Error!\n");
						return ret;
					}
					PIMAGE_IMPORT_BY_NAME ImportName = (PIMAGE_IMPORT_BY_NAME)((DWORD)FileAddress + ImportNameAdd_FOA);
					printf("�����ֵ���[HINT/NAME]: %02X--%s\n", ImportName->Hint, ImportName->Name);
				}

				FirstThunk_IAT++;
			}

		}

		printf("=========================ImportTable %s End  =============================\n", pImportName);

		//	(8)ָ����һ�������
		ImportDirectory++;
	}

	return ret;
}

/*
	��64λ�����
*/

int PrintImportTable_64(PVOID FileAddress)
{
	int ret = 0;
	PIMAGE_NT_HEADERS64 pNTHeader64;
	PIMAGE_DOS_HEADER pDosHeader;
	pDosHeader = (PIMAGE_DOS_HEADER)FileAddress;
	pNTHeader64 = (PIMAGE_NT_HEADERS64)((DWORD)FileAddress + pDosHeader->e_lfanew);
	PIMAGE_IMPORT_DESCRIPTOR ImportDirectory;
	PIMAGE_THUNK_DATA64 _pThunk = NULL;
	DWORD dwThunk = NULL;
	USHORT Hint;
	if (pNTHeader64->OptionalHeader.DataDirectory[1].VirtualAddress == 0)
	{
		printf("no import table!");
		return ret;
	}
	ImportDirectory = (PIMAGE_IMPORT_DESCRIPTOR)ImageRvaToVa((PIMAGE_NT_HEADERS)pNTHeader64, pDosHeader, pNTHeader64->OptionalHeader.DataDirectory[1].VirtualAddress, NULL);
	printf("=================== import table start ===================\n");
	for (; ImportDirectory->Name != NULL;)
	{
		char* szName = (PSTR)ImageRvaToVa((PIMAGE_NT_HEADERS)pNTHeader64, pDosHeader, (ULONG)ImportDirectory->Name, 0);
		printf("%s\n", szName);
		if (ImportDirectory->OriginalFirstThunk != 0)
		{

			dwThunk = ImportDirectory->OriginalFirstThunk;

			_pThunk = (PIMAGE_THUNK_DATA64)ImageRvaToVa((PIMAGE_NT_HEADERS)pNTHeader64, pDosHeader, (ULONG)ImportDirectory->OriginalFirstThunk, NULL);
		}
		else
		{

			dwThunk = ImportDirectory->FirstThunk;

			_pThunk = (PIMAGE_THUNK_DATA64)ImageRvaToVa((PIMAGE_NT_HEADERS)pNTHeader64, pDosHeader, (ULONG)ImportDirectory->FirstThunk, NULL);
		}
		for (; _pThunk->u1.AddressOfData != NULL;)
		{

			char* szFun = (PSTR)ImageRvaToVa((PIMAGE_NT_HEADERS)pNTHeader64, pDosHeader, (ULONG)(((PIMAGE_IMPORT_BY_NAME)_pThunk->u1.AddressOfData)->Name), 0);
			if (szFun != NULL)
				memcpy(&Hint, szFun - 2, 2);
			else
				Hint = -1;
			printf("\t%0.4x\t%0.8x\t%s\n", Hint, dwThunk, szFun);
			dwThunk += 8;
			_pThunk++;
		}
		ImportDirectory++;
	}
	printf("=================== import table end ===================\n");
	return ret;
}



/*
	��64λpe�����
*/
int PrintExportTable_64(PVOID FileAddress)
{
	int ret = 0;
	//1.ָ���������
	PIMAGE_NT_HEADERS64 pNTHeader64;
	PIMAGE_DOS_HEADER pDosHeader;
	PIMAGE_EXPORT_DIRECTORY pExportDirectory;
	pDosHeader = (PIMAGE_DOS_HEADER)FileAddress;
	pNTHeader64 = (PIMAGE_NT_HEADERS64)((DWORD)FileAddress + pDosHeader->e_lfanew);

	if (pNTHeader64->OptionalHeader.DataDirectory[0].VirtualAddress == 0)
	{
		printf("no export table!");
		return ret;
	}
	pExportDirectory = (PIMAGE_EXPORT_DIRECTORY)ImageRvaToVa((PIMAGE_NT_HEADERS)pNTHeader64, pDosHeader, pNTHeader64->OptionalHeader.DataDirectory[0].VirtualAddress, NULL);
	DWORD i = 0;
	DWORD NumberOfNames = pExportDirectory->NumberOfNames;
	//printf("%d\n", NumberOfNames);
	//printf("%d\n", pExportDirectory->NumberOfFunctions);
	//DWORD NumberOF
	ULONGLONG** NameTable = (ULONGLONG**)pExportDirectory->AddressOfNames;
	NameTable = (PULONGLONG*)ImageRvaToVa((PIMAGE_NT_HEADERS)pNTHeader64, pDosHeader, (ULONG)NameTable, NULL);

	ULONGLONG** AddressTable = (ULONGLONG**)pExportDirectory->AddressOfFunctions;
	AddressTable = (PULONGLONG*)ImageRvaToVa((PIMAGE_NT_HEADERS)pNTHeader64, pDosHeader, (DWORD)AddressTable, NULL);

	ULONGLONG** OrdinalTable = (ULONGLONG**)ImageRvaToVa((PIMAGE_NT_HEADERS)pNTHeader64, pDosHeader, (ULONG)pExportDirectory->AddressOfNameOrdinals, NULL);
	//OrdinalTable = (PULONGLONG*)ImageRvaToVa((PIMAGE_NT_HEADERS)pNTHeader64, pDosHeader, (DWORD)OrdinalTable, NULL);
	//PIMAGE_IMPORT_BY_NAME;

	char* szFun = (PSTR)ImageRvaToVa((PIMAGE_NT_HEADERS)pNTHeader64, pDosHeader, (ULONG)*NameTable, NULL);
	printf("=================== export table start ===================\n");
	for (i = 0; i < NumberOfNames; i++)
	{
		printf("%0.4x\t%0.8x\t%s\n", i + pExportDirectory->Base, *AddressTable, szFun);
		//printf("%s\n", szFun);
		szFun = szFun + strlen(szFun) + 1;
		AddressTable++;
		/*if (i % 200 == 0 && i / 200 >= 1)
		{
			printf("{Press [ENTER] to continue...}");
			getchar();
		}*/
	}
	printf("=================== export table end ===================\n");
	return ret;
}

// IAT
int PrintFunctionAddressTable(PVOID FileAddress, DWORD AddressOfFunctions_RVA, DWORD NumberOfFunctions)
{
	int ret = 0;
	DWORD AddressOfFunctions_FOA = 0;

	//1��RVA --> FOA
	ret = RVA_TO_FOA(FileAddress, AddressOfFunctions_RVA, &AddressOfFunctions_FOA);
	if (ret != 0)
	{
		printf("func RVA_TO_FOA() Error!\n");
		return ret;
	}

	//2��ָ������ַ��
	PDWORD FuncAddressTable = (PDWORD)((DWORD)FileAddress + AddressOfFunctions_FOA);

	//2��ѭ����ӡ������ַ��
	printf("=================== ������ַ�� Start ===================\n");
	for (DWORD i = 0; i < NumberOfFunctions; i++)
	{
		DWORD FuncAddress_RVA = FuncAddressTable[i];
		DWORD FuncAddress_FOA = 0;
		ret = RVA_TO_FOA(FileAddress, FuncAddress_RVA, &FuncAddress_FOA);
		if (ret != 0)
		{
			printf("func RVA_TO_FOA() Error!\n");
			return ret;
		}

		printf("������ַRVA    : %08X  |������ַFOA    : %08X  \n", FuncAddress_RVA, FuncAddress_FOA);
	}
	printf("=================== ������ַ�� End   ===================\n\n");
	return ret;
}

//��ӡ������ű�
int PrintFunctionOrdinalTable(PVOID FileAddress, DWORD AddressOfOrdinal_RVA, DWORD NumberOfNames, DWORD Base)
{
	int ret = 0;
	DWORD AddressOfOrdinal_FOA = 0;

	//1��RVA --> FOA
	ret = RVA_TO_FOA(FileAddress, AddressOfOrdinal_RVA, &AddressOfOrdinal_FOA);
	if (ret != 0)
	{
		printf("func RVA_TO_FOA() Error!\n");
		return ret;
	}

	//2��ָ������ű�
	PWORD OrdinalTable = (PWORD)((DWORD)FileAddress + AddressOfOrdinal_FOA);

	//3��ѭ����ӡ������ű�
	printf("=================== ������ű� Start ===================\n");
	for (DWORD i = 0; i < NumberOfNames; i++)
	{
		printf("�������  :%04X  |Base+Ordinal   :%04X\n", OrdinalTable[i], OrdinalTable[i] + Base);
	}
	printf("=================== ������ű� End   ===================\n\n");
	return ret;
}

//��ӡ�������ֱ�
int PrintFunctionNameTable(PVOID FileAddress, DWORD AddressOfNames_RVA, DWORD NumberOfNames)
{
	int ret = 0;
	DWORD AddressOfNames_FOA = 0;

	//1��RVA --> FOA
	ret = RVA_TO_FOA(FileAddress, AddressOfNames_RVA, &AddressOfNames_FOA);
	if (ret != 0)
	{
		printf("func RVA_TO_FOA() Error!\n");
		return ret;
	}

	//2��ָ��������
	PDWORD NameTable = (PDWORD)((DWORD)FileAddress + AddressOfNames_FOA);

	//3��ѭ����ӡ������ű�
	printf("=================== �������� Start ===================\n");
	for (DWORD i = 0; i < NumberOfNames; i++)
	{
		DWORD FuncName_RVA = NameTable[i];
		DWORD FuncName_FOA = 0;
		ret = RVA_TO_FOA(FileAddress, FuncName_RVA, &FuncName_FOA);
		if (ret != 0)
		{
			printf("func RVA_TO_FOA() Error!\n");
			return ret;
		}
		PCHAR FuncName = (PCHAR)((DWORD)FileAddress + FuncName_FOA);

		printf("������  :%s\n", FuncName);
	}
	printf("=================== �������� End   ===================\n\n");

	return ret;
}

/*
	�����
*/

int PrintExportTable(PVOID FileAddress)
{
	int ret = 0;

	//1��ָ���������
	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)(FileAddress);
	PIMAGE_FILE_HEADER pFileHeader = (PIMAGE_FILE_HEADER)((DWORD)pDosHeader + pDosHeader->e_lfanew + 4);
	PIMAGE_OPTIONAL_HEADER32 pOptionalHeader = (PIMAGE_OPTIONAL_HEADER32)((DWORD)pFileHeader + sizeof(IMAGE_FILE_HEADER));
	printf("%02X\n", pOptionalHeader->Magic);
	//2����ȡ������ĵ�ַ
	DWORD ExportDirectory_RAVAdd = pOptionalHeader->DataDirectory[0].VirtualAddress;
	DWORD ExportDirectory_FOAAdd = 0;
	//	(1)���жϵ������Ƿ����
	if (ExportDirectory_RAVAdd == 0)
	{
		printf("ExportDirectory ������!\n");
		return ret;
	}
	//	(2)����ȡ�������FOA��ַ
	ret = RVA_TO_FOA(FileAddress, ExportDirectory_RAVAdd, &ExportDirectory_FOAAdd);
	if (ret != 0)
	{
		printf("func RVA_TO_FOA() Error!\n");
		return ret;
	}

	//3��ָ�򵼳���
	PIMAGE_EXPORT_DIRECTORY ExportDirectory = (PIMAGE_EXPORT_DIRECTORY)((DWORD)FileAddress + ExportDirectory_FOAAdd);

	//4���ҵ��ļ���
	DWORD FileName_RVA = ExportDirectory->Name;
	DWORD FileName_FOA = 0;
	ret = RVA_TO_FOA(FileAddress, FileName_RVA, &FileName_FOA);
	if (ret != 0)
	{
		printf("func RVA_TO_FOA() Error!\n");
		return ret;
	}
	PCHAR FileName = (PCHAR)((DWORD)FileAddress + FileName_FOA);

	//5����ӡ��������Ϣ
	printf("*********************************************************\n");
	printf("=======================ExportTable=======================\n");
	printf("DWORD Characteristics;        :  %08X\n", ExportDirectory->Characteristics);
	printf("DWORD TimeDateStamp;          :  %08X\n", ExportDirectory->TimeDateStamp);
	printf("WORD  MajorVersion;           :  %04X\n", ExportDirectory->MajorVersion);
	printf("WORD  MinorVersion;           :  %04X\n", ExportDirectory->MinorVersion);
	printf("DWORD Name;                   :  %08X     \"%s\"\n", ExportDirectory->Name, FileName);
	printf("DWORD Base;                   :  %08X\n", ExportDirectory->Base);
	printf("DWORD NumberOfFunctions;      :  %08X\n", ExportDirectory->NumberOfFunctions);
	printf("DWORD NumberOfNames;          :  %08X\n", ExportDirectory->NumberOfNames);
	printf("DWORD AddressOfFunctions;     :  %08X\n", ExportDirectory->AddressOfFunctions);
	printf("DWORD AddressOfNames;         :  %08X\n", ExportDirectory->AddressOfNames);
	printf("DWORD AddressOfNameOrdinals;  :  %08X\n", ExportDirectory->AddressOfNameOrdinals);
	printf("=========================================================\n");
	printf("*********************************************************\n");

	//6����ӡ������ַ�� ������NumberOfFunctions����
	ret = PrintFunctionAddressTable(FileAddress, ExportDirectory->AddressOfFunctions, ExportDirectory->NumberOfFunctions);
	if (ret != 0)
	{
		printf("func PrintFunctionAddressTable() Error!\n");
		return ret;
	}

	//7����ӡ������ű� ������NumberOfNames����
	ret = PrintFunctionOrdinalTable(FileAddress, ExportDirectory->AddressOfNameOrdinals, ExportDirectory->NumberOfNames, ExportDirectory->Base);
	if (ret != 0)
	{
		printf("func PrintFunctionOrdinalTable() Error!\n");
		return ret;
	}

	//8����ӡ�������� ������NumberOfNames����
	ret = PrintFunctionNameTable(FileAddress, ExportDirectory->AddressOfNames, ExportDirectory->NumberOfNames);
	if (ret != 0)
	{
		printf("func PrintFunctionNameTable() Error!\n");
		return ret;
	}

	return ret;
}

/*
	�ض�λ��
*/
int PrintReloactionTable(PVOID FileAddress)
{
	int ret = 0;

	//1��ָ���������
	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)(FileAddress);
	PIMAGE_FILE_HEADER pFileHeader = (PIMAGE_FILE_HEADER)((DWORD)pDosHeader + pDosHeader->e_lfanew + 4);
	PIMAGE_OPTIONAL_HEADER32 pOptionalHeader = (PIMAGE_OPTIONAL_HEADER32)((DWORD)pFileHeader + sizeof(IMAGE_FILE_HEADER));

	//2����ȡ�ض�λ��ĵ�ַ
	DWORD RelocationDirectory_RAVAdd = pOptionalHeader->DataDirectory[5].VirtualAddress;
	DWORD RelocationDirectory_FOAAdd = 0;
	//	(1)���ж��ض�λ���Ƿ����
	if (RelocationDirectory_RAVAdd == 0)
	{
		printf("RelocationDirectory ������!\n");
		return ret;
	}
	//	(2)����ȡ�ض�λ���FOA��ַ
	ret = RVA_TO_FOA(FileAddress, RelocationDirectory_RAVAdd, &RelocationDirectory_FOAAdd);
	if (ret != 0)
	{
		printf("func RVA_TO_FOA() Error!\n");
		return ret;
	}

	//3��ָ���ض�λ��
	PIMAGE_BASE_RELOCATION RelocationDirectory = (PIMAGE_BASE_RELOCATION)((DWORD)FileAddress + RelocationDirectory_FOAAdd);

	//4��ѭ����ӡ�ض�λ��Ϣ  ��VirtualAddress��SizeOfBlock��Ϊ0ʱ�������
	while (RelocationDirectory->VirtualAddress && RelocationDirectory->SizeOfBlock)
	{
		printf("VirtualAddress    :%08X\n", RelocationDirectory->VirtualAddress);
		printf("SizeOfBlock       :%08X\n", RelocationDirectory->SizeOfBlock);
		printf("================= BlockData Start ======================\n");
		//5�������ڵ�ǰ���е����ݸ���
		DWORD DataNumber = (RelocationDirectory->SizeOfBlock - 8) / 2;

		//6��ָ�����ݿ�
		PWORD DataGroup = (PWORD)((DWORD)RelocationDirectory + 8);

		//7��ѭ����ӡ���ݿ��е�����
		for (DWORD i = 0; i < DataNumber; i++)
		{
			//(1)�жϸ�4λ�Ƿ�Ϊ0
			if (DataGroup[i] >> 12 != 0)
			{
				//(2)��ȡ���ݿ��е���Ч���� ��12λ
				WORD BlockData = DataGroup[i] & 0xFFF;

				//(3)�������ݿ��RVA��FOA
				DWORD Data_RVA = RelocationDirectory->VirtualAddress + BlockData;
				DWORD Data_FOA = 0;
				ret = RVA_TO_FOA(FileAddress, Data_RVA, &Data_FOA);
				if (ret != 0)
				{
					printf("func RVA_TO_FOA() Error!\n");
					return ret;
				}
				//(4)��ȡ��Ҫ�ض�λ������
				PDWORD RelocationData = (PDWORD)((DWORD)FileAddress + Data_FOA);

				printf("��[%04X]��    |���� :[%04X]   |���ݵ�RVA :[%08X]  |�������� :[%X]  |�ض�λ����  :[%08X]\n", i + 1, BlockData, Data_RVA, (DataGroup[i] >> 12), *RelocationData);
			}
		}
		printf("================= BlockData End ========================\n");

		//ָ����һ�����ݿ�
		RelocationDirectory = (PIMAGE_BASE_RELOCATION)((DWORD)RelocationDirectory + RelocationDirectory->SizeOfBlock);
	}
	return ret;
}

/*
	��Դ��
*/

int PrintResourceTable(PVOID FileAddress)
{
	/*string szResType[0x11] = { 0, "���ָ��", "λͼ", "ͼ��", "�˵�",
						 "�Ի���", "�ַ����б�", "����Ŀ¼", "����",
						 "���ټ�", "�Ǹ�ʽ����Դ", "��Ϣ�б�", "���ָ����",
						 "zz", "ͼ����", "xx", "�汾��Ϣ" };
	*/
	int szResType[17] = { '0', '1', '2', '3', '4', '5', '6', '7', '8',
		'9', '10', '11', '12', '13', '14', '15', '16' };

	int ret = 0;
	//1��ָ���������
	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)(FileAddress);
	PIMAGE_FILE_HEADER pFileHeader = (PIMAGE_FILE_HEADER)((DWORD)pDosHeader + pDosHeader->e_lfanew + 4);
	PIMAGE_OPTIONAL_HEADER32 pOptionalHeader = (PIMAGE_OPTIONAL_HEADER32)((DWORD)pFileHeader + sizeof(IMAGE_FILE_HEADER));

	//2����ȡ��Դ��ĵ�ַ
	DWORD ResourceDirectory_RVAAdd = pOptionalHeader->DataDirectory[2].VirtualAddress;
	DWORD ResourceDirectory_FOAAdd = 0;
	//	(1)���ж���Դ���Ƿ����
	if (ResourceDirectory_RVAAdd == 0)
	{
		printf("ResourceDirectory ������!\n");
		return ret;
	}
	//	(2)����ȡ��Դ���FOA��ַ
	ret = RVA_TO_FOA(FileAddress, ResourceDirectory_RVAAdd, &ResourceDirectory_FOAAdd);
	if (ret != 0)
	{
		printf("func RVA_TO_FOA() Error!\n");
		return ret;
	}

	//3��ָ����Դ��
	PIMAGE_RESOURCE_DIRECTORY ResourceDirectory = (PIMAGE_RESOURCE_DIRECTORY)((DWORD)FileAddress + ResourceDirectory_FOAAdd);

	//4����ӡ��Դ����Ϣ(һ��Ŀ¼)
	printf("|==================================================\n");
	printf("|��Դ��һ��Ŀ¼��Ϣ:\n");
	printf("|Characteristics        :%08X\n", ResourceDirectory->Characteristics);
	printf("|TimeDateStamp          :%08X\n", ResourceDirectory->TimeDateStamp);
	printf("|MajorVersion           :%04X\n", ResourceDirectory->MajorVersion);
	printf("|MinorVersion           :%04X\n", ResourceDirectory->MinorVersion);
	printf("|NumberOfNamedEntries   :%04X\n", ResourceDirectory->NumberOfNamedEntries);
	printf("|NumberOfIdEntries      :%04X\n", ResourceDirectory->NumberOfIdEntries);
	printf("|==================================================\n");

	//4��ѭ����ӡ������Դ����Ϣ
	//	(1)ָ��һ��Ŀ¼�е���ԴĿ¼��(һ��Ŀ¼)	��Դ����
	PIMAGE_RESOURCE_DIRECTORY_ENTRY ResourceDirectoryEntry = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)((DWORD)ResourceDirectory + sizeof(IMAGE_RESOURCE_DIRECTORY));
	printf("|----------------------------------------\n");

	for (int i = 0; i < (ResourceDirectory->NumberOfIdEntries + ResourceDirectory->NumberOfNamedEntries); i++)
	{
		//	(2)�ж�һ��Ŀ¼�е���ԴĿ¼���������Ƿ����ַ��� 1 = �ַ���(�Ǳ�׼����)�� 0 = ���ַ���(��׼����)
		if (ResourceDirectoryEntry->NameIsString) //�ַ���(�Ǳ�׼����)
		{
			//		1.ָ�����ֽṹ��
			PIMAGE_RESOURCE_DIR_STRING_U pStringName = (PIMAGE_RESOURCE_DIR_STRING_U)((DWORD)ResourceDirectory + ResourceDirectoryEntry->NameOffset);

			//		2.��Unicode�ַ���ת����ASCII�ַ���
			CHAR TypeName[20] = { 0 };
			for (int j = 0; j < pStringName->Length; j++)
			{
				TypeName[j] = (CHAR)pStringName->NameString[j];
			}
			//		3.��ӡ�ַ���
			printf("|ResourceType           :\"%s\"\n", TypeName);
		}
		else //���ַ���(��׼����)
		{
			if (ResourceDirectoryEntry->Id < 0x11) //ֻ��1 - 16�ж���
				printf("|ResourceType           :%d\n", szResType[ResourceDirectoryEntry->Id]);
			else
				printf("|ResourceType           :%04Xh\n", ResourceDirectoryEntry->Id);
		}

		//	(3)�ж�һ��Ŀ¼���ӽڵ������		1 = Ŀ¼�� 0 = ���� (һ��Ŀ¼�Ͷ���Ŀ¼��ֵ��Ϊ1)
		if (ResourceDirectoryEntry->DataIsDirectory)
		{
			//	(4)��ӡĿ¼ƫ��
			printf("|OffsetToDirectory      :%08X\n", ResourceDirectoryEntry->OffsetToDirectory);
			printf("|----------------------------------------\n");

			//	(5)ָ�����Ŀ¼	��Դ���
			PIMAGE_RESOURCE_DIRECTORY ResourceDirectory_Sec = (PIMAGE_RESOURCE_DIRECTORY)((DWORD)ResourceDirectory + ResourceDirectoryEntry->OffsetToDirectory);

			//	(6)��ӡ��Դ����Ϣ(����Ŀ¼)
			printf("    |====================================\n");
			printf("    |��Դ�����Ŀ¼��Ϣ:\n");
			printf("    |Characteristics        :%08X\n", ResourceDirectory_Sec->Characteristics);
			printf("    |TimeDateStamp          :%08X\n", ResourceDirectory_Sec->TimeDateStamp);
			printf("    |MajorVersion           :%04X\n", ResourceDirectory_Sec->MajorVersion);
			printf("    |MinorVersion           :%04X\n", ResourceDirectory_Sec->MinorVersion);
			printf("    |NumberOfNamedEntries   :%04X\n", ResourceDirectory_Sec->NumberOfNamedEntries);
			printf("    |NumberOfIdEntries      :%04X\n", ResourceDirectory_Sec->NumberOfIdEntries);
			printf("    |====================================\n");

			//	(7)ָ�����Ŀ¼�е���ԴĿ¼��
			PIMAGE_RESOURCE_DIRECTORY_ENTRY ResourceDirectoryEntry_Sec = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)((DWORD)ResourceDirectory_Sec + sizeof(IMAGE_RESOURCE_DIRECTORY));

			//	(8)ѭ����ӡ����Ŀ¼
			for (int j = 0; j < (ResourceDirectory_Sec->NumberOfIdEntries + ResourceDirectory_Sec->NumberOfNamedEntries); j++)
			{
				//	(9)�ж϶���Ŀ¼�е���ԴĿ¼���б���Ƿ����ַ���
				if (ResourceDirectoryEntry_Sec->NameIsString) //�ַ���(�Ǳ�׼����)
				{
					//		1.ָ�����ֽṹ��
					PIMAGE_RESOURCE_DIR_STRING_U pStringName = (PIMAGE_RESOURCE_DIR_STRING_U)((DWORD)ResourceDirectory + ResourceDirectoryEntry_Sec->NameOffset);

					//		2.��Unicode�ַ���ת����ASCII�ַ���
					CHAR TypeName[20] = { 0 };
					for (int k = 0; k < pStringName->Length; k++)
					{
						TypeName[k] = (CHAR)pStringName->NameString[k];
					}
					//		3.��ӡ�ַ���
					printf("    |ResourceNumber         :\"%s\"\n", TypeName);
				}
				else //���ַ���(��׼����)
				{
					printf("    |ResourceNumber         :%04Xh\n", ResourceDirectoryEntry_Sec->Id);
				}
				//	(10)�ж϶���Ŀ¼���ӽڵ������
				if (ResourceDirectoryEntry_Sec->DataIsDirectory)
				{
					//	(11)��ӡĿ¼ƫ��
					printf("    |OffsetToDirectory      :%08X\n", ResourceDirectoryEntry_Sec->OffsetToDirectory);
					printf("    |------------------------------------\n");

					//	(12)ָ������Ŀ¼	����ҳ
					PIMAGE_RESOURCE_DIRECTORY ResourceDirectory_Thir = (PIMAGE_RESOURCE_DIRECTORY)((DWORD)ResourceDirectory + ResourceDirectoryEntry_Sec->OffsetToDirectory);

					//	(13)��ӡ��Դ����Ϣ(����Ŀ¼)
					printf("        |================================\n");
					printf("        |��Դ������Ŀ¼��Ϣ:\n");
					printf("        |Characteristics        :%08X\n", ResourceDirectory_Thir->Characteristics);
					printf("        |TimeDateStamp          :%08X\n", ResourceDirectory_Thir->TimeDateStamp);
					printf("        |MajorVersion           :%04X\n", ResourceDirectory_Thir->MajorVersion);
					printf("        |MinorVersion           :%04X\n", ResourceDirectory_Thir->MinorVersion);
					printf("        |NumberOfNamedEntries   :%04X\n", ResourceDirectory_Thir->NumberOfNamedEntries);
					printf("        |NumberOfIdEntries      :%04X\n", ResourceDirectory_Thir->NumberOfIdEntries);
					printf("        |================================\n");

					//	(14)ָ������Ŀ¼�е���ԴĿ¼��
					PIMAGE_RESOURCE_DIRECTORY_ENTRY ResourceDirectoryEntry_Thir = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)((DWORD)ResourceDirectory_Thir + sizeof(IMAGE_RESOURCE_DIRECTORY));

					//	(15)ѭ����ӡ����Ŀ¼��
					for (int k = 0; k < (ResourceDirectory_Thir->NumberOfNamedEntries + ResourceDirectory_Thir->NumberOfIdEntries); k++)
					{
						//	(16)�ж�����Ŀ¼�е���ԴĿ¼���б���Ƿ����ַ���
						if (ResourceDirectoryEntry_Thir->NameIsString) //�ַ���(�Ǳ�׼����)
						{
							//		1.ָ�����ֽṹ��
							PIMAGE_RESOURCE_DIR_STRING_U pStringName = (PIMAGE_RESOURCE_DIR_STRING_U)((DWORD)ResourceDirectory + ResourceDirectoryEntry_Thir->NameOffset);

							//		2.��Unicode�ַ���ת����ASCII�ַ���
							CHAR TypeName[20] = { 0 };
							for (int k = 0; k < pStringName->Length; k++)
							{
								TypeName[k] = (CHAR)pStringName->NameString[k];
							}
							//		3.��ӡ�ַ���
							printf("        |CodePageNumber         :\"%s\"\n", TypeName);
						}
						else //���ַ���(��׼����)
						{
							printf("        |CodePageNumber         :%04Xh\n", ResourceDirectoryEntry_Thir->Id);
						}
						//	(17)�ж�����Ŀ¼���ӽڵ������		(����Ŀ¼�ӽڵ㶼�����ݣ��������ʡȥ�ж�)
						if (ResourceDirectoryEntry_Thir->DataIsDirectory)
						{
							//	(18)��ӡƫ��
							printf("        |OffsetToDirectory      :%08X\n", ResourceDirectoryEntry_Thir->OffsetToDirectory);
							printf("        |------------------------------------\n");
						}
						else
						{
							//	(18)��ӡƫ��
							printf("        |OffsetToData           :%08X\n", ResourceDirectoryEntry_Thir->OffsetToData);
							printf("        |------------------------------------\n");

							//	(19)ָ����������	(��Դ���FOA + OffsetToData)
							PIMAGE_RESOURCE_DATA_ENTRY ResourceDataEntry = (PIMAGE_RESOURCE_DATA_ENTRY)((DWORD)ResourceDirectory + ResourceDirectoryEntry_Thir->OffsetToData);

							//	(20)��ӡ������Ϣ
							printf("            |================================\n");
							printf("            |��Դ���������Ϣ\n");
							printf("            |OffsetToData(RVA)      :%08X\n", ResourceDataEntry->OffsetToData);
							printf("            |Size                   :%08X\n", ResourceDataEntry->Size);
							printf("            |CodePage               :%08X\n", ResourceDataEntry->CodePage);
							printf("            |================================\n");
						}

						ResourceDirectoryEntry_Thir++;
					}
				}
				//	(21)Ŀ¼�����
				ResourceDirectoryEntry_Sec++;
			}
		}
		printf("|----------------------------------------\n");
		//	(22)Ŀ¼�����
		ResourceDirectoryEntry++;
	}
	return ret;
}


int main()
{
	int ret = 0;
	PVOID FileAddress = NULL;

	//1�����ļ����뵽�ڴ�   
	ret = MyReadFile(&FileAddress);
	if (ret != 0)
	{
		if (FileAddress != NULL)
			free(FileAddress);
		return ret;
	}

	//2����ӡ�������Ϣ64λ
	ret = PrintImportTable_64(FileAddress);
	if (ret != 0)
	{
		if (FileAddress != NULL)
			free(FileAddress);
		return ret;
	}
	qDebug() << "print done\n";
	//2����ӡ��������Ϣ64λ
	/*ret = PrintExportTable_64(FileAddress);
	if (ret != 0)
	{
		if (FileAddress != NULL)
			free(FileAddress);
		return ret;
	}*/
	//2����ӡ�������Ϣ
	/*ret = PrintImportTable(FileAddress);
	if (ret != 0)
	{
		if (FileAddress != NULL)
			free(FileAddress);
		return ret;
	}*/

	//3�������
	/*ret = PrintExportTable(FileAddress);
	if (ret != 0)
	{
		if (FileAddress != NULL)
			free(FileAddress);
		return ret;
	}*/

	//4����ӡ�ض�λ����Ϣ
	/*ret = PrintReloactionTable(FileAddress);
	if (ret != 0)
	{
		if (FileAddress != NULL)
			free(FileAddress);
		return ret;
	}*/

	//5����ӡ��Դ�����Ϣ
	/*ret = PrintResourceTable(FileAddress);
	if (ret != 0)
	{
		if (FileAddress != NULL)
			free(FileAddress);
		return ret;
	}*/
	//checkFile
	ret = checkFile(FileAddress);
	if (ret != 0)
	{
		if (FileAddress != NULL)
			free(FileAddress);
		return ret;
	}
	qDebug() << "check done\n";
	if (FileAddress != NULL)
		free(FileAddress);

	system("pause");
	return ret;
}
//�쳣��[3]
