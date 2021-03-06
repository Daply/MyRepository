#include <windows.h>
#include "common_header.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <conio.h>

using namespace std;
CRITICAL_SECTION cs;
enum States {MAKE,START_CLIENT,PRINT, SET_PENDING,BAD_INPUT,EMPTY, SHUTDOWN, HELP};
struct Arguments 
{
	HANDLE hWrite;
	HANDLE hRead;
	int ID;	
	FILE* file;
};


bool die = false;


States parseCmd ()
{
	printf (">");
	rewind (stdin);
	char* input = new char [100];
	States state = BAD_INPUT;
	cin.getline(input,100);
	printf ("\'%s\'\n",input);
	if (strcmp ("make",input)==0) state = MAKE;
	if (strcmp ("start clients", input) == 0) state = START_CLIENT;
	if (strcmp ("print", input)==0) state = PRINT;
	if (strcmp ("set pending", input)==0) state = SET_PENDING;
	if (strcmp ("die",input)==0) state = SHUTDOWN;
	if (strcmp ("help",input)==0) state = HELP;
	if (strcmp ("", input)==0) state = EMPTY;	
	delete [] input;
	return state;
}

// creating binary file
void make ()
{
	printf ("Output file: ");
	char* filename = new char [100];
	scanf ("%s",filename);
	FILE* fout = fopen (filename,"wb");
	if (!fout)
	{
		printf ("ERR:Can't write\n");
		return;
	}
	delete [] filename;
	printf ("Record count: ");
	int count =0;
	scanf ("%d",&count);
	for (int i=0;i<count;i++)
	{
		Order  order; 
		printf ("\nRecord #%d:\n",i+1);
		printf ("\t\tName: ");
		scanf ("%s",&order.name);
		printf ("\t\tAmount: ");
		scanf ("%d",&order.amount);
		printf ("\t\tPrice: ");
		scanf ("%lf",&order.price);
		fwrite(&order,sizeof(Order),1,fout);
	}
	fclose (fout);
}

void help ()
{
	printf ("make \t\t-\t make binary file\n");
	printf ("start clients \t-\t start clients\n");
	printf ("print \t\t-\t print binary file\n");
	printf ("set pending \t-\t set active file\n");
	printf ("die \t\t-\t kill server\n");
	printf ("help \t\t-\t prints this message\n");
}

void kill(HANDLE* hServerThread, HANDLE* hClients, Arguments* args,DWORD* serverThreadID, int clientCount)
{
	{
		for (int i=0;i<clientCount;i++)
		{			 
			TerminateProcess(hClients[i],0);
			CloseHandle(hServerThread);
			CloseHandle(hClients);
		}
		delete [] hServerThread;
		delete [] args;
		delete [] serverThreadID;
		delete [] hClients;

	}
}


// serving clients
DWORD WINAPI serverThread (LPVOID arg)
{

	Arguments args = *((Arguments*) arg);
	HANDLE hPut, hGet;
	char lpszPut[20];
	char lpszGet[20];  
	wsprintf(lpszGet, "GET%d ",args.ID);
	wsprintf(lpszPut, "PUT%d ",args.ID);
	hPut = CreateEvent(NULL,FALSE,FALSE,lpszPut);
	hGet = CreateEvent(NULL,FALSE,FALSE,lpszGet);
	while(!die)
	{
		DWORD dwBytesRead;
		DWORD dwBytesWritten;
		WaitForSingleObject(hGet,INFINITE);
		printf ("[] Got connetcion\n");
		int request = 0;
		if (!ReadFile(args.hRead,&request,	sizeof(request),&dwBytesRead,	NULL))
		{
			EnterCriticalSection(&cs);
			_cputs("Read from the pipe failed.\n");
			_cputs("Press any key to finish.\n");
			LeaveCriticalSection(&cs);
			_getch();		
			ExitThread( GetLastError());
		}
		if (request == READ)
		{
			SetEvent(hPut);
			WaitForSingleObject(hGet,INFINITE);
			char key [10];
			if (!ReadFile(args.hRead,&key,	sizeof(key),&dwBytesRead,	NULL))
			{
				EnterCriticalSection(&cs);
				_cputs("Read from the pipe failed.\n");
				_cputs("Press any key to finish.\n");
				LeaveCriticalSection(&cs);
				_getch();		
				ExitThread( GetLastError());
			}			
			EnterCriticalSection(&cs);
			Order order;
			rewind(args.file);
			bool notFound = true;
			while (!feof(args.file))
			{				
				fread (&order,sizeof(Order),1,args.file);
				if (!strcmp(order.name,key))
				{
					notFound = false;
					break;
				}
			}
			LeaveCriticalSection(&cs);
			if (notFound)
			{
				if (!WriteFile(args.hWrite,&NOT_FOUND,sizeof(NOT_FOUND),&dwBytesWritten,NULL))
				{
					EnterCriticalSection(&cs);
					_cputs("Write to file failed.\n");
					_cputs("Press any key to finish.\n");
					_getch();
					LeaveCriticalSection(&cs);
					return GetLastError();
				}
				SetEvent(hPut);
				continue;
			}
			if (!WriteFile(args.hWrite,&OK,sizeof(OK),&dwBytesWritten,NULL))
			{
				EnterCriticalSection(&cs);
				_cputs("Write to file failed.\n");
				_cputs("Press any key to finish.\n");
				_getch();
				LeaveCriticalSection(&cs);
				return GetLastError();
			}
			SetEvent(hPut);
			WaitForSingleObject(hGet,INFINITE);
			if (!WriteFile(args.hWrite,&order,sizeof(order),&dwBytesWritten,NULL))
			{
				EnterCriticalSection(&cs);
				_cputs("Write to file failed.\n");
				_cputs("Press any key to finish.\n");
				_getch();
				LeaveCriticalSection(&cs);
				return GetLastError();
			}
			SetEvent(hPut);
			continue;
		}
		if (request == MODIFY)
		{
			SetEvent(hPut);
			WaitForSingleObject(hGet,INFINITE);
			char key [10];
			if (!ReadFile(args.hRead,&key,	sizeof(key),&dwBytesRead,	NULL))
			{
				EnterCriticalSection(&cs);
				_cputs("Read from the pipe failed.\n");
				_cputs("Press any key to finish.\n");
				LeaveCriticalSection(&cs);
				_getch();		
				ExitThread( GetLastError());
			}			
			EnterCriticalSection(&cs);
			Order order;
			rewind(args.file);
			bool notFound = true;
			while (!feof(args.file))
			{				
				fread (&order,sizeof(Order),1,args.file);
				if (!strcmp(order.name,key))
				{
					notFound = false;
					break;
				}
			}
			if (notFound)
			{
				if (!WriteFile(args.hWrite,&NOT_FOUND,sizeof(NOT_FOUND),&dwBytesWritten,NULL))
				{
					EnterCriticalSection(&cs);
					_cputs("Write to file failed.\n");
					_cputs("Press any key to finish.\n");
					_getch();
					LeaveCriticalSection(&cs);
					return GetLastError();
				}
				SetEvent(hPut);
				continue;
			}
			if (!WriteFile(args.hWrite,&OK,sizeof(OK),&dwBytesWritten,NULL))
			{
				EnterCriticalSection(&cs);
				_cputs("Write to file failed.\n");
				_cputs("Press any key to finish.\n");
				_getch();
				LeaveCriticalSection(&cs);
				return GetLastError();
			}
			SetEvent(hPut);
			WaitForSingleObject(hGet,INFINITE);
			if (!WriteFile(args.hWrite,&order,sizeof(order),&dwBytesWritten,NULL))
			{
				EnterCriticalSection(&cs);
				_cputs("Write to file failed.\n");
				_cputs("Press any key to finish.\n");
				_getch();
				LeaveCriticalSection(&cs);
				return GetLastError();
			}
			SetEvent(hPut);
			WaitForSingleObject(hGet,INFINITE);
			if (!ReadFile(args.hRead,&order,	sizeof(order),&dwBytesRead,	NULL))
			{
				EnterCriticalSection(&cs);
				_cputs("Read from the pipe failed.\n");
				_cputs("Press any key to finish.\n");
				LeaveCriticalSection(&cs);
				_getch();		
				ExitThread( GetLastError());
			}
			fseek(args.file,sizeof(Order),SEEK_CUR);
			fwrite(&order,sizeof(order),1,args.file);
			LeaveCriticalSection(&cs);
			continue;
		}		
	}
	ExitThread (0);
}

// launch clients
bool startClients (HANDLE* hServerThread, HANDLE* hClients, Arguments* args,DWORD* serverThreadID, int &clientCount,FILE* file)
{
	bool noErr = true;
	// turning off already created clients
	{
		for (int i=0;i<clientCount;i++)
		{		
			TerminateThread(hServerThread[i],0);
			TerminateProcess(hClients[i],0);
			CloseHandle(hServerThread);
			CloseHandle(hClients);
		}
		delete [] hServerThread;
		delete [] args;
		delete [] serverThreadID;
		delete [] hClients;

	}
	printf ("Client count:");
	scanf("%d",&clientCount);
	hServerThread = new HANDLE[clientCount];
	hClients = new HANDLE [clientCount];
	args = new Arguments [clientCount];
	serverThreadID = new DWORD [clientCount];

	printf ("Process ID \t\t[Path\t\tID\thWrite\thRead\t]\n" );
	for (int i=0;i<clientCount;i++)
	{
		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		HANDLE hWritePipe, hReadPipe;

		// creating anonymous channel
		// setting the channel security attributes
		SECURITY_ATTRIBUTES sa;

		sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		sa.lpSecurityDescriptor = NULL;
		sa.bInheritHandle = TRUE;

		if(!CreatePipe(	&hReadPipe,	&hWritePipe, &sa, 0))			
		{
			_cputs("Create pipe failed.\n");
			_cputs("Press any key to finish.\n");
			_getch();
			return false;
		}


		char lpszComLine[80];
		wsprintf(lpszComLine, "client.exe %d %d %d",i,(int) hReadPipe,(int) hWritePipe);
		ZeroMemory(&si, sizeof(STARTUPINFO));
		si.cb = sizeof(STARTUPINFO);

		// launch client
		if (!CreateProcess(NULL,lpszComLine,NULL,NULL,TRUE,CREATE_NEW_CONSOLE,NULL,NULL,&si,&pi))
		{
			_cputs("Create process failed.\n");
			_cputs("Press any key to finish.\n");
			_getch();
			return false;
		}
		else
		{
			printf ("Process %d \t\t[%s\t%d\t%d\t%d\t]\t [OK]\n",i+1,"client.exe",i,(int) hReadPipe,(int) hWritePipe);
		}
		hClients [i]= pi.hProcess;

		args[i].file = file;
		args[i].hRead = hReadPipe;
		args[i].hWrite = hWritePipe;
		args[i].ID = i;		
		// ��������� �����, ��������� � ���� ��������
		hServerThread[i] = CreateThread(NULL,NULL,serverThread,(void*)&args[i],NULL,&serverThreadID[i]);
		if (!hServerThread[i]) 
		{
			_cputs("Create server threa failed.\n");
			_cputs("Press any key to finish.\n");
			_getch();
			return false;
		}
	}
}

// choosing which file should be processed
void setPending (FILE* &file)
{

	printf ("Set filename: ");
	char filename[100];
	scanf ("%s",&filename);
	file = fopen (filename,"r+b");
}

// print file
void print ()
{
	char* filename = new char [100];
	printf ("Print what: ");
	scanf ("%s",filename);
	FILE* fin = fopen (filename,"rb");
	if (!fin) 
	{
		printf ("ERR:No such file\n");
		return;
	}
	int i=0;
	while (!feof(fin))
	{
		Order order;
		fread (&order,sizeof(Order),1,fin);
		printf ("Record %d:\n\t\tName: %s\n\t\tAmount: \%d\n\t\tPrice: %3.3lf\n",i+1,order.name,order.amount,order.price);
	}
	fclose (fin);
}
int main()
{
	FILE* activeFile = NULL;
	HANDLE* hServerThread=NULL;
	HANDLE* hClients=NULL;
	DWORD* serverThreadID=NULL;
	Arguments* args=NULL;
	int clientCount=0;
	InitializeCriticalSection(&cs);

	while (!die)
	{
		switch (parseCmd ())
		{
		case EMPTY: break;
			// turn off
		case SHUTDOWN: die = true; kill (hServerThread,hClients,args,serverThreadID,clientCount); break;
			// print file
		case PRINT: print (); break;
			// create file with db
		case MAKE: make();break;
			// launch clients
		case START_CLIENT: startClients (hServerThread,hClients,args,serverThreadID,clientCount,activeFile); break;
			// install file with db
		case SET_PENDING: setPending (activeFile); if (!activeFile) printf("ERR:File not exist\n"); break;
		case BAD_INPUT: printf ("ERR:Bad input. Use 'help' for help\n"); break;
		case HELP: help (); break;
		}
	}

	if (activeFile) fclose (activeFile);	
	return 0;
}