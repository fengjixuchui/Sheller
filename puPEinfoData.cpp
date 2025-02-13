#include "puPEinfoData.h"


void* PuPEInfo::m_pFileBase = nullptr;

void* PuPEInfo::m_pNtHeader = nullptr;

void* PuPEInfo::m_SectionHeader = nullptr;

DWORD PuPEInfo::m_FileSize = 0;

CString PuPEInfo::m_strNamePath;

HANDLE PuPEInfo::m_hFileHandle = nullptr;

DWORD PuPEInfo::m_OldOEP = 0;

int	PuPEInfo::m_SectionCount = 0;

BOOL PuPEInfo::OepFlag = FALSE;

PuPEInfo::PuPEInfo()
{

}

PuPEInfo::~PuPEInfo()
{

}

// 判断文件是否可执行
BOOL PuPEInfo::IsPEFile()
{
	if (IMAGE_DOS_SIGNATURE != ((PIMAGE_DOS_HEADER)PuPEInfo::m_pFileBase)->e_magic) return FALSE;
	
	if (IMAGE_NT_SIGNATURE != ((PIMAGE_NT_HEADERS)PuPEInfo::m_pNtHeader)->Signature) return FALSE;

	return TRUE;
}

// 加载文件到内存
BOOL PuPEInfo::prOpenFile(const CString & PathName)
{

	m_strNamePath = PathName;

	HANDLE hFile = CreateFile(PathName, GENERIC_READ | GENERIC_WRITE, FALSE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	
	if ((int)hFile <= 0){ AfxMessageBox(L"当前进程有可能被占用或者意外错误"); return FALSE; }

	m_hFileHandle = hFile;

	DWORD dwSize = GetFileSize(hFile, NULL);

	PuPEInfo::m_FileSize = dwSize;
	
	//HeapCreate(, );//HeapAlloc(, );

	PuPEInfo::m_pFileBase = (void *)malloc(dwSize);
	
	memset(PuPEInfo::m_pFileBase, 0, dwSize);
	
	DWORD dwRead = 0;
	
	OVERLAPPED OverLapped = { 0 };
	
	int nRetCode = 	ReadFile(hFile, PuPEInfo::m_pFileBase, dwSize, &dwRead, &OverLapped);
	
	PIMAGE_DOS_HEADER pDosHander = (PIMAGE_DOS_HEADER)PuPEInfo::m_pFileBase;

	PIMAGE_NT_HEADERS pHeadres = (PIMAGE_NT_HEADERS)(pDosHander->e_lfanew + (LONG)m_pFileBase);

	PuPEInfo::m_pNtHeader = (void *)pHeadres;

	if (PuPEInfo::OepFlag == FALSE)
	{
		m_OldOEP = pHeadres->OptionalHeader.AddressOfEntryPoint;
		PuPEInfo::OepFlag = TRUE;
	}

	m_SectionCount = pHeadres->FileHeader.NumberOfSections;
	
	if (!IsPEFile()){ free(m_pFileBase); PuPEInfo::m_pFileBase = nullptr; AfxMessageBox(L"不是一个有效的PE文件"); return FALSE; }

	PIMAGE_SECTION_HEADER pSection = IMAGE_FIRST_SECTION((PIMAGE_NT_HEADERS)PuPEInfo::m_pNtHeader);

	PuPEInfo::m_SectionHeader = (void *)pSection;

	return TRUE;
}

// RVAofFOA
DWORD PuPEInfo::RVAofFOA(const DWORD Rva)
{
	DWORD dwSectionCount = (PIMAGE_NT_HEADERS(PuPEInfo::m_pNtHeader))->FileHeader.NumberOfSections;

	PIMAGE_SECTION_HEADER pSection = IMAGE_FIRST_SECTION((PIMAGE_NT_HEADERS)PuPEInfo::m_pNtHeader);

	for (DWORD i = 0; i < dwSectionCount; ++i)
	{
		if ((Rva >= (pSection->VirtualAddress)) && (Rva < ((pSection->VirtualAddress) + (pSection->SizeOfRawData)))) {
			return (pSection->VirtualAddress + pSection->PointerToRawData);
		}
		++pSection;
	}
	return 0;
}

// 根据区段名称获取区段首地址
PIMAGE_SECTION_HEADER PuPEInfo::GetSectionAddress(const char* Base, const BYTE* SectionName)
{
	PIMAGE_NT_HEADERS pNt = (PIMAGE_NT_HEADERS)(((PIMAGE_DOS_HEADER)Base)->e_lfanew + Base);

	PIMAGE_SECTION_HEADER pSect = IMAGE_FIRST_SECTION(pNt);

	for (int i = 0; i < m_SectionCount; ++i) { 
		if (0 == _mbscmp(pSect->Name, SectionName))
			return (PIMAGE_SECTION_HEADER)pSect; 
		++pSect; 
	}
	
	return 0;
}

// 设置文件偏移以及文件大小
BOOL PuPEInfo::SetFileoffsetAndFileSize(const void* Base, const DWORD & offset, const DWORD size, const BYTE* Name)
{
	 PIMAGE_SECTION_HEADER Address = GetSectionAddress((char*)Base, Name);

	 Address->PointerToRawData = offset;

	 Address->SizeOfRawData = size;

	 return TRUE;
}