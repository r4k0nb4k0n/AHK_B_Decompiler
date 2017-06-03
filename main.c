//-L "C:\Program Files (x86)\Dev-Cpp\MinGW64\x86_64-w64-mingw32\lib32" -l comdlg32
#include<stdio.h>
#include<windows.h>

BYTE INT3 = 0xCC;
BYTE JMP = 0xE9;
BYTE LEA = 0x8D;
BYTE CALL = 0xE8;
BYTE PUSH_EBP = 0x55;
BYTE CALL2 = 0xFF;
BYTE PUSH = 0x68;
BYTE PUSHAD = 0x60;
BYTE MOV = 0x8B;

BYTE Sig1[] = { 0x8B, 0x45, 0x64, 0xFF, 0x05 };
BYTE Sig2[] = { 0x8D, 0x86, 0x10, 0x01, 0x00, 0x00 };
BYTE UPX_Sig[] = { 0x83, 0xEC, 0x80, 0xE9 };
BYTE MoleBoxSig[] = {0x55, 0x8b, 0xec};
BYTE EP_Sig[] = {0x60,0xEB};
BOOL isMolebox = FALSE;

VOID Err_Chk(char Msg[], DWORD Err_Code)
{
	if (Err_Code == 0)
	{
		printf("%s Fail!\n", Msg);
		printf("GetLastError : 0x%X", GetLastError());
		system("pause");
		exit(0);
	}
}
BOOL isBP(LPDEBUG_EVENT DE, DWORD RVA)
{
	PEXCEPTION_RECORD per = &DE->u.Exception.ExceptionRecord;
	if ((DWORD)per->ExceptionAddress == RVA)
	{
		printf("Break Point : %X\n", per->ExceptionAddress);
		return TRUE;
	}
	return FALSE;
}

DWORD RAW2RVA(DWORD dwRAW, PIMAGE_NT_HEADERS pNtHeader, PIMAGE_SECTION_HEADER pSecHeader)
{
	int i;
	for (i = 0; i < pNtHeader->FileHeader.NumberOfSections; i++)
	{
		if ((dwRAW >= pSecHeader[i].PointerToRawData) && (dwRAW < (pSecHeader[i].PointerToRawData + pSecHeader[i].SizeOfRawData)))
			return (dwRAW - pSecHeader[i].PointerToRawData) + pSecHeader[i].VirtualAddress;
	}
	return 0;
}

BOOL MoleBoxUnpack(char sInput[], BYTE *fs, DWORD EP_RAW, PIMAGE_NT_HEADERS pNtHeader, PIMAGE_SECTION_HEADER pSecHeader, char sOutput[])
{
	
	//MoleBox Routine : 
	FILE *fp;
	BYTE UnpackSig1[] = { 0xFF, 0x35 };
	BYTE UnpackSig2[] = { 0xE8 };
	BYTE UnpackSig3[] = { 0xFF, 0x55, 0xF0 };
	BYTE UnpackSig4[] = { 0xFF, 0xE0 };
	BYTE POPAD_Sig[] = { 0x61 };
	BYTE JMP_Sig[] = { 0xE9 };


	BOOL chk = FALSE;
	BYTE NOW = 0xFF, *PW;

	DWORD Buffer, RetAddr;
	DWORD BP_RVA = 0, BP_RVA2 = 0, ScriptSize, ScriptAddress;

	DWORD i, ret;
	DWORD RVA, RVA2, RVA3, RVA4, RVA5, RVA6, RVA7, RVA8, JMP_RVA;
	BYTE Buffer2[0xFFFF + 1], Buffer3[0x100], *Buffer4, Buffer5[0xFFFF + 1];

	PROCESS_INFORMATION processinfo;
	STARTUPINFO startupinfo;
	DEBUG_EVENT DE;
	CONTEXT ctx;



	memset(&processinfo, 0, sizeof(PROCESS_INFORMATION));
	memset(&startupinfo, 0, sizeof(STARTUPINFO));
	memset(&DE, 0, sizeof(DEBUG_EVENT));

	RVA = RAW2RVA(EP_RAW, pNtHeader, pSecHeader);
	RVA += pNtHeader->OptionalHeader.ImageBase;


	for (i = EP_RAW; i<EP_RAW + 200; i++) //대충.. 100정도면 문제 없으려나. 문제있네. 
	{
		if (memcmp(fs + i, UnpackSig1, sizeof(UnpackSig1)) == 0)
			break;
	}
	EP_RAW = i;
	for (i = EP_RAW; i<EP_RAW + 50; i++)
	{
		if (memcmp(fs + i, UnpackSig2, sizeof(UnpackSig2)) == 0)
			break;
	}
	EP_RAW = i;



	RVA = RAW2RVA(EP_RAW, pNtHeader, pSecHeader);
	RVA += pNtHeader->OptionalHeader.ImageBase;

	printf("EP_RAW : %x EP_RVA : %x\n", EP_RAW, RVA);


	startupinfo.cb = sizeof(STARTUPINFO);
	ret = CreateProcessA(sInput, NULL, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, (LPSTARTUPINFOA)&startupinfo, &processinfo);
	Err_Chk("CreateProcess", ret);
	ret = WriteProcessMemory(processinfo.hProcess, (LPVOID)RVA, (LPCVOID)&INT3, 1, NULL);
	Err_Chk("WriteProcessMemory", ret);
	ret = DebugActiveProcess(processinfo.dwProcessId);
	Err_Chk("DebugActiveProcess", ret);
	ret = ResumeThread(processinfo.hThread);
	Err_Chk("ResumeThread", ret + 1);
	printf("Thread Resume\n");
	while (WaitForDebugEvent(&DE, INFINITE))
	{
		if (DE.dwDebugEventCode == EXCEPTION_DEBUG_EVENT)
		{
			if (isBP(&DE, RVA))
			{
				ret = ReadProcessMemory(processinfo.hProcess, (LPCVOID)(RVA + 1), (LPVOID)&Buffer, 0x4, NULL);
				Err_Chk("ReadProcessMemory", ret);
				RVA2 = Buffer + RVA + 5;

				ret = WriteProcessMemory(processinfo.hProcess, (LPVOID)RVA2, (LPCVOID)&INT3, 1, NULL);
				Err_Chk("WriteProcessMemory", ret);
				ret = WriteProcessMemory(processinfo.hProcess, (LPVOID)RVA, (LPCVOID)&CALL, 1, NULL);
				Err_Chk("WriteProcessMemory", ret);
				ctx.ContextFlags = CONTEXT_CONTROL;
				ret = GetThreadContext(processinfo.hThread, &ctx);
				Err_Chk("GetThreadContext", ret);
				ctx.Eip--;
				ret = SetThreadContext(processinfo.hThread, &ctx);
				Err_Chk("SetThreadContext", ret);

			}
			if (isBP(&DE, RVA2))
			{
				ret = ReadProcessMemory(processinfo.hProcess, (LPCVOID)(RVA2), (LPVOID)&Buffer2, 0xFFFF, NULL);
				Err_Chk("ReadProcessMemory", ret);
				for (i = 0; i<0xFFFF - sizeof(UnpackSig3); i++)
				{
					if (memcmp(Buffer2 + i, UnpackSig3, sizeof(UnpackSig3)) == 0)
						break;
				}
				for (i; i<0xFFFF - sizeof(UnpackSig2); i++)
				{
					if (memcmp(Buffer2 + i, UnpackSig2, sizeof(UnpackSig2)) == 0)
						break;
				}
				for (i; i<0xFFFF - sizeof(UnpackSig3); i++)
				{
					if (memcmp(Buffer2 + i, UnpackSig3, sizeof(UnpackSig3)) == 0)
						break;
				}
				for (i; i<0xFFFF - sizeof(UnpackSig2); i++)
				{
					if (memcmp(Buffer2 + i, UnpackSig2, sizeof(UnpackSig2)) == 0)
						break;
				}
				for (i; i<0xFFFF - sizeof(UnpackSig3); i++)
				{
					if (memcmp(Buffer2 + i, UnpackSig3, sizeof(UnpackSig3)) == 0)
					{
						RVA3 = RVA2 + i;
						break;
					}
				}
				ret = WriteProcessMemory(processinfo.hProcess, (LPVOID)RVA3, (LPCVOID)&INT3, 1, NULL);
				Err_Chk("WriteProcessMemory", ret);
				ret = WriteProcessMemory(processinfo.hProcess, (LPVOID)RVA2, (LPCVOID)&PUSH_EBP, 1, NULL);
				Err_Chk("WriteProcessMemory", ret);
				ctx.ContextFlags = CONTEXT_CONTROL;
				ret = GetThreadContext(processinfo.hThread, &ctx);
				Err_Chk("GetThreadContext", ret);
				ctx.Eip--;
				ret = SetThreadContext(processinfo.hThread, &ctx);
				Err_Chk("SetThreadContext", ret);
			}
			if (isBP(&DE, RVA3))
			{
				ctx.ContextFlags = CONTEXT_FULL;
				ret = GetThreadContext(processinfo.hThread, &ctx);
				Err_Chk("GetThreadContext", ret);
				ret = ReadProcessMemory(processinfo.hProcess, (LPCVOID)(ctx.Ebp - 0x10), (LPVOID)&Buffer, 0x4, NULL);
				Err_Chk("ReadProcessMemory", ret);
				RVA4 = Buffer;
				ret = WriteProcessMemory(processinfo.hProcess, (LPVOID)RVA4, (LPCVOID)&INT3, 1, NULL);
				Err_Chk("WriteProcessMemory", ret);
				ret = WriteProcessMemory(processinfo.hProcess, (LPVOID)RVA3, (LPCVOID)&CALL2, 1, NULL);
				Err_Chk("WriteProcessMemory", ret);
				ctx.ContextFlags = CONTEXT_CONTROL;
				ret = GetThreadContext(processinfo.hThread, &ctx);
				Err_Chk("GetThreadContext", ret);
				ctx.Eip--;
				ret = SetThreadContext(processinfo.hThread, &ctx);
				Err_Chk("SetThreadContext", ret);
			}
			if (isBP(&DE, RVA4))
			{
				ret = ReadProcessMemory(processinfo.hProcess, (LPCVOID)(RVA4), (LPVOID)&Buffer2, 0xFFFF, NULL);
				Err_Chk("ReadProcessMemory", ret);
				for (i = 0; i<0xFFFF - sizeof(UnpackSig4); i++)
				{
					if (memcmp(Buffer2 + i, UnpackSig4, sizeof(UnpackSig4)) == 0)
					{
						RVA5 = RVA4 + i;
						break;
					}
				}
				ret = WriteProcessMemory(processinfo.hProcess, (LPVOID)RVA5, (LPCVOID)&INT3, 1, NULL);
				Err_Chk("WriteProcessMemory", ret);
				ret = WriteProcessMemory(processinfo.hProcess, (LPVOID)RVA4, (LPCVOID)&PUSH_EBP, 1, NULL);
				Err_Chk("WriteProcessMemory", ret);
				ctx.ContextFlags = CONTEXT_CONTROL;
				ret = GetThreadContext(processinfo.hThread, &ctx);
				Err_Chk("GetThreadContext", ret);
				ctx.Eip--;
				ret = SetThreadContext(processinfo.hThread, &ctx);
				Err_Chk("SetThreadContext", ret);
			}
			if (isBP(&DE, RVA5))
			{
				ctx.ContextFlags = CONTEXT_FULL;
				ret = GetThreadContext(processinfo.hThread, &ctx);
				Err_Chk("GetThreadContext", ret);
				RVA6 = ctx.Eax;
				ret = WriteProcessMemory(processinfo.hProcess, (LPVOID)RVA6, (LPCVOID)&INT3, 1, NULL);
				Err_Chk("WriteProcessMemory", ret);
				ret = WriteProcessMemory(processinfo.hProcess, (LPVOID)RVA5, (LPCVOID)&CALL2, 1, NULL);
				Err_Chk("WriteProcessMemory", ret);
				ctx.ContextFlags = CONTEXT_CONTROL;
				ret = GetThreadContext(processinfo.hThread, &ctx);
				Err_Chk("GetThreadContext", ret);
				ctx.Eip--;
				ret = SetThreadContext(processinfo.hThread, &ctx);
				Err_Chk("SetThreadContext", ret);
			}

			if (isBP(&DE, RVA6))
			{
				ctx.ContextFlags = CONTEXT_FULL;
				ret = GetThreadContext(processinfo.hThread, &ctx);
				Err_Chk("GetThreadContext", ret);
				ret = ReadProcessMemory(processinfo.hProcess, (LPCVOID)(ctx.Eax), (LPVOID)&Buffer3, 0x100, NULL);
				Err_Chk("ReadProcessMemory", ret);
				for (i = 0; i<0x100 - sizeof(UnpackSig4); i++)
				{
					if (memcmp(Buffer3 + i, UnpackSig4, sizeof(UnpackSig4)) == 0)
					{
						RVA7 = RVA6 + i;
						break;
					}
				}
				ret = WriteProcessMemory(processinfo.hProcess, (LPVOID)RVA7, (LPCVOID)&INT3, 1, NULL);
				Err_Chk("WriteProcessMemory", ret);
				ret = WriteProcessMemory(processinfo.hProcess, (LPVOID)RVA6, (LPCVOID)&PUSH, 1, NULL);
				Err_Chk("WriteProcessMemory", ret);
				ctx.ContextFlags = CONTEXT_CONTROL;
				ret = GetThreadContext(processinfo.hThread, &ctx);
				Err_Chk("GetThreadContext", ret);
				ctx.Eip--;
				ret = SetThreadContext(processinfo.hThread, &ctx);
				Err_Chk("SetThreadContext", ret);
			}
			if (isBP(&DE, RVA7))
			{
				ctx.ContextFlags = CONTEXT_FULL;
				ret = GetThreadContext(processinfo.hThread, &ctx);
				Err_Chk("GetThreadContext", ret);
				RVA8 = ctx.Eax;
				printf("OEP : %x\n", RVA8);

				ret = WriteProcessMemory(processinfo.hProcess, (LPVOID)RVA8, (LPCVOID)&INT3, 1, NULL);
				Err_Chk("WriteProcessMemory", ret);
				ret = WriteProcessMemory(processinfo.hProcess, (LPVOID)RVA7, (LPCVOID)&CALL2, 1, NULL);
				Err_Chk("WriteProcessMemory", ret);
				ctx.ContextFlags = CONTEXT_CONTROL;
				ret = GetThreadContext(processinfo.hThread, &ctx);
				Err_Chk("GetThreadContext", ret);
				ctx.Eip--;
				ret = SetThreadContext(processinfo.hThread, &ctx);
				Err_Chk("SetThreadContext", ret);

			}
			if (isBP(&DE, RVA8))
			{
				ret = WriteProcessMemory(processinfo.hProcess, (LPVOID)&GetProcAddress, (LPCVOID)&INT3, 1, NULL);
				Err_Chk("WriteProcessMemory", ret);
				ret = WriteProcessMemory(processinfo.hProcess, (LPVOID)RVA8, (LPCVOID)&PUSHAD, 1, NULL);
				Err_Chk("WriteProcessMemory", ret);
				ctx.ContextFlags = CONTEXT_CONTROL;
				ret = GetThreadContext(processinfo.hThread, &ctx);
				Err_Chk("GetThreadContext", ret);
				ctx.Eip--;
				ret = SetThreadContext(processinfo.hThread, &ctx);
				Err_Chk("SetThreadContext", ret);
				//&GetProcAddress

			}
			if (isBP(&DE, (DWORD)GetProcAddress))
			{
				ctx.ContextFlags = CONTEXT_FULL;
				ret = GetThreadContext(processinfo.hThread, &ctx);
				Err_Chk("GetThreadContext", ret);
				ret = ReadProcessMemory(processinfo.hProcess, (LPCVOID)(ctx.Esp), (LPVOID)&RetAddr, 4, NULL);
				Err_Chk("ReadProcessMemory", ret);
				printf("RET Addr : %x\n", RetAddr);

				ret = ReadProcessMemory(processinfo.hProcess, (LPCVOID)(RetAddr), (LPVOID)&Buffer3, 0x100, NULL);
				Err_Chk("ReadProcessMemory", ret);

				for (i = 0; i<0x100 - sizeof(POPAD_Sig); i++)
				{
					if (memcmp(Buffer3 + i, POPAD_Sig, sizeof(POPAD_Sig)) == 0)
						break;
				}
				for (i; i<0x100 - sizeof(JMP_Sig); i++)
				{
					if (memcmp(Buffer3 + i, JMP_Sig, sizeof(JMP_Sig)) == 0)
						break;
				}
				JMP_RVA = RetAddr + i;


				ret = WriteProcessMemory(processinfo.hProcess, (LPVOID)JMP_RVA, (LPCVOID)&INT3, 1, NULL);
				Err_Chk("WriteProcessMemory", ret);
				ret = WriteProcessMemory(processinfo.hProcess, (LPVOID)&GetProcAddress, (LPCVOID)&MOV, 1, NULL);
				Err_Chk("WriteProcessMemory", ret);
				ctx.ContextFlags = CONTEXT_CONTROL;
				ret = GetThreadContext(processinfo.hThread, &ctx);
				Err_Chk("GetThreadContext", ret);
				ctx.Eip--;
				ret = SetThreadContext(processinfo.hThread, &ctx);
				Err_Chk("SetThreadContext", ret);

			}
			if (isBP(&DE, JMP_RVA))
			{
				ret = ReadProcessMemory(processinfo.hProcess, (LPCVOID)0x448000, (LPVOID)&Buffer5, 0xFFFF, NULL);
				Err_Chk("ReadProcessMemory", ret);
				for (i = 0; i<0xFFFF - 6; i++)
				{
					if (memcmp(Buffer5 + i, Sig1, sizeof(Sig1)) == 0)
					{
						BP_RVA2 = 0x448000 + i;
						ret = WriteProcessMemory(processinfo.hProcess, (LPVOID)BP_RVA2, (LPCVOID)&INT3, 1, NULL);
						Err_Chk("WriteProcessMemory", ret);
						chk = TRUE;
					}
					if (memcmp(Buffer5 + i, Sig2, sizeof(Sig2)) == 0)
					{
						BP_RVA = 0x448000 + i;
						ret = WriteProcessMemory(processinfo.hProcess, (LPVOID)BP_RVA, (LPCVOID)&INT3, 1, NULL);
						Err_Chk("WriteProcessMemory", ret);
						chk = TRUE;
					}
				}
				if (!chk) Err_Chk("Signature Scan", ret);
				ret = WriteProcessMemory(processinfo.hProcess, (LPVOID)JMP_RVA, (LPCVOID)&JMP, 1, NULL);
				Err_Chk("WriteProcessMemory", ret);
				ctx.ContextFlags = CONTEXT_CONTROL;
				ret = GetThreadContext(processinfo.hThread, &ctx);
				Err_Chk("GetThreadContext", ret);
				ctx.Eip--;
				ret = SetThreadContext(processinfo.hThread, &ctx);
				Err_Chk("SetThreadContext", ret);
			}

			else if (isBP(&DE, BP_RVA))
			{
				ctx.ContextFlags = CONTEXT_FULL;
				ret = GetThreadContext(processinfo.hThread, &ctx);
				Err_Chk("GetThreadContext", ret);
				ret = ReadProcessMemory(processinfo.hProcess, (LPCVOID)(ctx.Ebx), (LPVOID)&NOW, 1, NULL);
				Err_Chk("ReadProcessMemory", ret);
				i = 0;
				PW = (BYTE *)malloc(2);
				PW[i] = NOW;
				while (NOW != 0x00)
				{
					i++;
					ret = ReadProcessMemory(processinfo.hProcess, (LPCVOID)(ctx.Ebx + i), (LPVOID)&NOW, 1, NULL);
					Err_Chk("ReadProcessMemory", ret);
					PW = (BYTE *)realloc(PW, i + 2);
					PW[i] = NOW;
				}
				ret = WriteProcessMemory(processinfo.hProcess, (LPVOID)BP_RVA, (LPCVOID)&LEA, 1, NULL);
				Err_Chk("WriteProcessMemory", ret);
				ctx.ContextFlags = CONTEXT_CONTROL;
				ret = GetThreadContext(processinfo.hThread, &ctx);
				Err_Chk("GetThreadContext", ret);
				ctx.Eip--;
				ret = SetThreadContext(processinfo.hThread, &ctx);
				Err_Chk("SetThreadContext", ret);
			}
			else if (isBP(&DE, BP_RVA2))
			{
				ctx.ContextFlags = CONTEXT_FULL;
				ret = GetThreadContext(processinfo.hThread, &ctx);
				Err_Chk("GetThreadContext", ret);
				ret = ReadProcessMemory(processinfo.hProcess, (LPCVOID)(ctx.Ebp + 0x30), (LPVOID)&ScriptSize, 4, NULL);
				Err_Chk("ReadProcessMemory", ret);
				ret = ReadProcessMemory(processinfo.hProcess, (LPCVOID)(ctx.Ebp + 0x64), (LPVOID)&ScriptAddress, 4, NULL);
				Err_Chk("ReadProcessMemory", ret);
				printf("ScriptAddress : %X\n", ScriptAddress);
				Buffer4 = (PBYTE)malloc(ScriptSize);
				memset(Buffer4, 0, sizeof(Buffer4));
				ret = ReadProcessMemory(processinfo.hProcess, (LPCVOID)ScriptAddress, (LPVOID)Buffer4, ScriptSize, NULL);
				Err_Chk("ReadProcessMemory", ret);
				fp = fopen(sOutput, "wb");
				fwrite(Buffer4, ScriptSize, 1, fp);
				fclose(fp);
				ret = DebugActiveProcessStop(processinfo.dwProcessId);
				if (ret == 0) printf("DebugActiveProcessStop Fail!\n");
				ret = TerminateProcess(processinfo.hProcess, 0);
				if (ret == 0) printf("TerminateProcess Fail!\n");
				printf("----------------------------\n");
				printf("PW : %s\n", PW);
				printf("ScriptSize : %d Bytes\n", ScriptSize);
				printf("----------------------------\n");
				return TRUE;
			}
		}
		else if (DE.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT)
		{
			printf("Exit Process!\n");
			return FALSE;
		}
		ContinueDebugEvent(DE.dwProcessId, DE.dwThreadId, DBG_CONTINUE);
	}
	return FALSE;


}


BOOL Decompile(char sInput[], char sOutput[], DWORD JMP_RVA)
{
	FILE *fp;
	DWORD i, ret;
	DWORD BP_RVA = 0, BP_RVA2 = 0, ScriptSize, ScriptAddress;
	BYTE NOW = 0xFF, *PW;
	BYTE *Buffer2;
	BYTE Buffer[0xFFFF + 1];
	BOOL chk = FALSE;
	PROCESS_INFORMATION processinfo;
	STARTUPINFO startupinfo;
	DEBUG_EVENT DE;
	CONTEXT ctx;
	memset(&processinfo, 0, sizeof(PROCESS_INFORMATION));
	memset(&startupinfo, 0, sizeof(STARTUPINFO));
	memset(&DE, 0, sizeof(DEBUG_EVENT));
	startupinfo.cb = sizeof(STARTUPINFO);
	ret = CreateProcessA(sInput, NULL, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, (LPSTARTUPINFOA)&startupinfo, &processinfo);
	Err_Chk("CreateProcess", ret);
	ret = WriteProcessMemory(processinfo.hProcess, (LPVOID)JMP_RVA, (LPCVOID)&INT3, 1, NULL);
	Err_Chk("WriteProcessMemory", ret);
	ret = DebugActiveProcess(processinfo.dwProcessId);
	Err_Chk("DebugActiveProcess", ret);
	ret = ResumeThread(processinfo.hThread);
	Err_Chk("ResumeThread", ret + 1);
	printf("Thread Resume\n");


	while (WaitForDebugEvent(&DE, INFINITE))
	{
		if (DE.dwDebugEventCode == EXCEPTION_DEBUG_EVENT)
		{
			if (BP_RVA == 0)
			{
				if (isBP(&DE, JMP_RVA))
				{
					ret = ReadProcessMemory(processinfo.hProcess, (LPCVOID)0x448000, (LPVOID)&Buffer, 0xFFFF, NULL);
					Err_Chk("ReadProcessMemory", ret);
					for (i = 0; i<0xFFFF - 6; i++)
					{
						if (memcmp(Buffer + i, Sig1, sizeof(Sig1)) == 0)
						{
							BP_RVA2 = 0x448000 + i;
							ret = WriteProcessMemory(processinfo.hProcess, (LPVOID)BP_RVA2, (LPCVOID)&INT3, 1, NULL);
							Err_Chk("WriteProcessMemory", ret);
							chk = TRUE;
						}
						if (memcmp(Buffer + i, Sig2, sizeof(Sig2)) == 0)
						{
							BP_RVA = 0x448000 + i;
							ret = WriteProcessMemory(processinfo.hProcess, (LPVOID)BP_RVA, (LPCVOID)&INT3, 1, NULL);
							Err_Chk("WriteProcessMemory", ret);
							chk = TRUE;
						}
					}
					if (!chk) Err_Chk("Signature Scan", ret);
					ret = WriteProcessMemory(processinfo.hProcess, (LPVOID)JMP_RVA, (LPCVOID)&JMP, 1, NULL);
					Err_Chk("WriteProcessMemory", ret);
					ctx.ContextFlags = CONTEXT_CONTROL;
					ret = GetThreadContext(processinfo.hThread, &ctx);
					Err_Chk("GetThreadContext", ret);
					ctx.Eip--;
					ret = SetThreadContext(processinfo.hThread, &ctx);
					Err_Chk("SetThreadContext", ret);
				}
			}
			else if (isBP(&DE, BP_RVA))
			{
				ctx.ContextFlags = CONTEXT_FULL;
				ret = GetThreadContext(processinfo.hThread, &ctx);
				Err_Chk("GetThreadContext", ret);
				ret = ReadProcessMemory(processinfo.hProcess, (LPCVOID)(ctx.Ebx), (LPVOID)&NOW, 1, NULL);
				Err_Chk("ReadProcessMemory", ret);
				i = 0;
				PW = (BYTE *)malloc(2);
				PW[i] = NOW;
				while (NOW != 0x00)
				{
					i++;
					ret = ReadProcessMemory(processinfo.hProcess, (LPCVOID)(ctx.Ebx + i), (LPVOID)&NOW, 1, NULL);
					Err_Chk("ReadProcessMemory", ret);
					PW = (BYTE *)realloc(PW, i + 2);
					PW[i] = NOW;
				}
				ret = WriteProcessMemory(processinfo.hProcess, (LPVOID)BP_RVA, (LPCVOID)&LEA, 1, NULL);
				Err_Chk("WriteProcessMemory", ret);
				ctx.ContextFlags = CONTEXT_CONTROL;
				ret = GetThreadContext(processinfo.hThread, &ctx);
				Err_Chk("GetThreadContext", ret);
				ctx.Eip--;
				ret = SetThreadContext(processinfo.hThread, &ctx);
				Err_Chk("SetThreadContext", ret);
			}
			else if (isBP(&DE, BP_RVA2))
			{
				ctx.ContextFlags = CONTEXT_FULL;
				ret = GetThreadContext(processinfo.hThread, &ctx);
				Err_Chk("GetThreadContext", ret);
				ret = ReadProcessMemory(processinfo.hProcess, (LPCVOID)(ctx.Ebp + 0x30), (LPVOID)&ScriptSize, 4, NULL);
				Err_Chk("ReadProcessMemory", ret);
				ret = ReadProcessMemory(processinfo.hProcess, (LPCVOID)(ctx.Ebp + 0x64), (LPVOID)&ScriptAddress, 4, NULL);
				Err_Chk("ReadProcessMemory", ret);
				printf("ScriptAddress : %X\n", ScriptAddress);
				Buffer2 = (PBYTE)malloc(ScriptSize);
				memset(Buffer2, 0, sizeof(Buffer2));
				ret = ReadProcessMemory(processinfo.hProcess, (LPCVOID)ScriptAddress, (LPVOID)Buffer2, ScriptSize, NULL);
				Err_Chk("ReadProcessMemory", ret);
				fp = fopen(sOutput, "wb");
				fwrite(Buffer2, ScriptSize, 1, fp);
				fclose(fp);
				ret = DebugActiveProcessStop(processinfo.dwProcessId);
				if (ret == 0) printf("DebugActiveProcessStop Fail!\n");
				ret = TerminateProcess(processinfo.hProcess, 0);
				if (ret == 0) printf("TerminateProcess Fail!\n");
				printf("----------------------------\n");
				printf("PW : %s\n", PW);
				printf("ScriptSize : %d Bytes\n", ScriptSize);
				printf("----------------------------\n");
				return TRUE;
			}
		}
		else if (DE.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT)
		{
			printf("Exit Process!\n");
			return FALSE;
		}
		ContinueDebugEvent(DE.dwProcessId, DE.dwThreadId, DBG_CONTINUE);
	}
	return FALSE;
}
int main(int argc, char* argv[])
{
	FILE *fp;
	OPENFILENAME OFN;
	IMAGE_DOS_HEADER IDH;
	IMAGE_NT_HEADERS INH;
	IMAGE_SECTION_HEADER ISH;
	DWORD EP_RVA, EP_RAW, RAW, RAWSize, RVA, RVASize;
	DWORD OEP_RVA, JMP_RVA, Relative_Addr, i = 0, Len;
	BYTE *fs, *Tmp, *Path;
	WORD NumberOfSections;
	BOOL chk = TRUE;
	char Input;

	char *PATH, lpstrFile[MAX_PATH] = { 0, };
	SetLastError(0);
	if (argc == 1)
	{
		memset(&OFN, 0, sizeof(OPENFILENAME));
		OFN.lStructSize = sizeof(OPENFILENAME);
		OFN.hwndOwner = 0;
		OFN.lpstrFilter = "Exe File(*.exe)\0*.exe\0";
		OFN.lpstrFile = lpstrFile;
		OFN.nMaxFile = 256;
		if (GetOpenFileName(&OFN) != 0)
		{
			PATH = (char *)malloc(strlen(OFN.lpstrFile));
			memset(PATH, 0, strlen(OFN.lpstrFile));
			CopyMemory(PATH, OFN.lpstrFile, strlen(OFN.lpstrFile));
			printf("%s\n", OFN.lpstrFile);
		}
		else return EXIT_SUCCESS;
	}
	else
	{
		PATH = (char *)malloc(strlen(argv[1]) + 1);
		memset(PATH, 0, strlen(argv[1]) + 1);
		CopyMemory(PATH, argv[1], strlen(argv[1]));
	}
	printf("AutohotKey B Version Decompiler v1.4 Beta!\n");
	fp = fopen(PATH, "rb");
	fseek(fp, 0, SEEK_END);
	Len = ftell(fp);
	fs = (BYTE *)malloc(Len);
	memset(fs, 0, Len);
	fseek(fp, 0, SEEK_SET);
	fread(fs, 1, Len, fp);
	fseek(fp, 0, SEEK_SET);
	Tmp = (BYTE *)malloc(sizeof(IMAGE_DOS_HEADER));
	fread(Tmp, 1, sizeof(IMAGE_DOS_HEADER), fp);
	CopyMemory(&IDH, Tmp, sizeof(IMAGE_DOS_HEADER));
	printf("IDH.e_lfanew : %X\n", IDH.e_lfanew);
	fseek(fp, IDH.e_lfanew, SEEK_SET);
	Tmp = (BYTE *)malloc(sizeof(IMAGE_NT_HEADERS));
	fread(Tmp, 1, sizeof(IMAGE_NT_HEADERS), fp);
	CopyMemory(&INH, Tmp, sizeof(IMAGE_NT_HEADERS));
	NumberOfSections = INH.FileHeader.NumberOfSections;
	EP_RVA = INH.OptionalHeader.AddressOfEntryPoint;
	printf("EP_RVA : %X\n", EP_RVA);
	Tmp = (BYTE *)malloc(sizeof(IMAGE_SECTION_HEADER));
	while (chk)
	{
		fseek(fp, IDH.e_lfanew + sizeof(IMAGE_NT_HEADERS)+(sizeof(IMAGE_SECTION_HEADER)* i), SEEK_SET);
		i++;
		fread(Tmp, 1, sizeof(IMAGE_SECTION_HEADER), fp);
		CopyMemory(&ISH, Tmp, sizeof(IMAGE_SECTION_HEADER));
		RAW = ISH.PointerToRawData;
		RAWSize = ISH.SizeOfRawData;
		RVA = ISH.VirtualAddress;
		RVASize = ISH.Misc.VirtualSize;
		EP_RAW = EP_RVA - RVA + RAW;
		if (RVA < EP_RVA && RAW + RAWSize > EP_RAW) chk = FALSE;
		if (NumberOfSections < (WORD)i)
		{
			printf("EP Section Not Found!\n");
			fclose(fp);
			system("pause");
			return EXIT_SUCCESS;
		}
	}
	fclose(fp);
	printf("RAW : %X\nRAWSize : %X\nRVA : %X\nRVASize : %X\nEP_RAW : %X\n", RAW, RAWSize, RVA, RVASize, EP_RAW);

	if(memcmp(fs+EP_RAW,MoleBoxSig,sizeof(MoleBoxSig)) == 0)
	{
		printf("Are You Convinced That This File is Packed by Molebox? (Y/N) ");
		scanf("%c", &Input);
		if (Input == 'Y' || Input == 'y')
		{
			isMolebox = TRUE;
		}
		else
		{
			return EXIT_SUCCESS;
		}
	}

	else if (memcmp(fs+EP_RAW,EP_Sig,sizeof(EP_Sig)) == 0) //memcmp...
	{
		printf("Fail!\n");
		system("pause");
		return EXIT_SUCCESS;
	}

	Path = (BYTE *)malloc(Len + 4);
	sprintf((char *)Path, "%s.ahk", (char *)PATH);

	if (isMolebox)
	{
		if(MoleBoxUnpack(PATH, fs, EP_RAW, &INH, &ISH, Path)) printf("Successfully Write File.\n");
		else printf("Fail!!!\n");
	}
	else
	{
		for (i = EP_RAW; i<Len; i++)
		{
			if (memcmp(fs + i, UPX_Sig, sizeof(UPX_Sig)) == 0)
			{
				CopyMemory(&Relative_Addr, fs + i + 4, 4);
				JMP_RVA = i + 3 + RVA - RAW;
				JMP_RVA = JMP_RVA + INH.OptionalHeader.ImageBase;
				OEP_RVA = Relative_Addr + JMP_RVA + 5;
				printf("Relative_Addr : %X\nJMP Addr : %X\nOEP Addr : %X\n", Relative_Addr, JMP_RVA, OEP_RVA);
				continue;
			}
		}

		if (Decompile(PATH, (char *)Path, JMP_RVA)) printf("Successfully Write File.\n");
		else printf("Fail!!\n");
	}

	system("pause");
	return EXIT_SUCCESS;
}
