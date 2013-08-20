#pragma strings(readonly)

#define INCL_VIO            // VIO calls
#define INCL_DOSFILEMGR     // File Management
#define INCL_DOSMISC        // Miscellaneous
#define INCL_DOSMODULEMGR   // Module manager
#define INCL_DOSSESMGR      // Session Manager Support
#define INCL_DOSPROCESS     // Process and thread support
#define INCL_DOSQUEUES      // Queues
#define INCL_DOSDATETIME    // Date/Time and Timer support
#define INCL_DOSSEMAPHORES  // Semaphore support
#define INCL_DOSMEMMGR      // Memory Management

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <os2.h>


// #define PRELOAD      // if PRELOAD is defined, preloading is compiled


#ifdef __EMX__

#ifndef __OS2_H__       // only if EMX doesn't use IBM's Toolkit:
#define MY_PSZ PCSZ     // EMX prototype of DosScanEnv uses: const PSZ
#endif

#ifdef PRELOAD
USHORT _THUNK_FUNCTION (DosQProcStat) (PVOID, USHORT);
void DosQProcStatus (PVOID pBuf, ULONG cbBuf)
  {
    _THUNK_PASCAL_PROLOG (6);
    _THUNK_PASCAL_FLAT (pBuf);
    _THUNK_PASCAL_SHORT (cbBuf);
    _THUNK_PASCAL_CALL (DosQProcStat);
  }
#endif

#else                       

#define MY_PSZ PSZ      // other compilers' prototype of DosScanEnv: PSZ

#ifdef PRELOAD
USHORT APIENTRY16 DosQProcStatus(PVOID pBuf, USHORT cbBuf);
#endif

#endif

#ifdef PRELOAD
#define PTR(ptr, ofs)  ((void *) ((char *) (ptr) + (ofs)))
#endif

#define PIPESIZE 256        // size of the unnamed pipe
#define BUFFERSIZE 0x10000  // size of buffered output
#define PRINT_INTERVAL 32   // output every 32 ms
#define BUFFER1k 1024       // length of 1kb buffer
#define BUFFER4k 4096       // length of 4kb buffer
#define TSTACK 16384        // stack size for thread
#define PRINTPRIO 3         // print thread priority 3 (timecritical)

static char *cmdline;       // commandline
static char *ShellPrg;      // command interpreter, i.e. "C:\OS2\CMD.EXE"
static char *ShellArg;      // "/C <commandline>" or parameters from env-var
static BYTE *Buffer;        // output buffer
static BYTE *BufferPos;     // position of next free byte in output buffer
static BYTE *Screen;        // buffer for screen
static int SWidth;          // screen width
static int SHeight;         // screen height
static int SLength;         // length of the screen in bytes (width*height)
static HSEM  OutputSem;     // event semaphore: output screen
static HMTX  BufferSem;     // mutex semaphore: buffer access
static HFILE SaveStdout;    // saved stdout
static HFILE SaveStderr;    // saved stderr
static HFILE hpR;           // read handle of unnamed pipe
static HFILE hpW;           // write handle of unnamed pipe
static HFILE NewStdout;     // new stdout
static HFILE NewStderr;     // new stderr
static HTIMER TimerHandle;  // timer to output screen every x ms
static BOOL ExitThread;     // output thread exits if TRUE
static USHORT row;          // current cursor row
static USHORT col;          // current cursor column
static BOOL NormalMode;     // TRUE: write to stdout; FALSE: jump-scroll


#ifdef PRELOAD
#pragma pack(1)

struct Procstat
  {
  ULONG  summary;       /* global data area                     */
  ULONG  processes;     /* process data area                    */
  ULONG  semaphores;    /* semaphore data area (not used)       */
  ULONG  unknown1;
  ULONG  sharedmemory;  /* shared memory data area (not used)   */
  ULONG  modules;       /* module handle                        */
  ULONG  unknown2;
  ULONG  unknown3;
  };

struct Process
  {
  ULONG  type;          /* process: 1                           */
  ULONG  threadlist;    /* -> to 1st thread struct              */
  USHORT pid;           /* process ID                           */
  USHORT ppid;          /* parent process ID                    */
  ULONG  styp;          /* session type                         */
  ULONG  status;        /* process status                       */
  ULONG  sgroup;        /* screen group                         */
  USHORT handle;        /* module handle                        */
  USHORT threads;       /* # of thread control blocks           */
  ULONG  time;
  ULONG  reserved2;
  USHORT sems;          /* # of semaphores used                 */
  USHORT libs;          /* # of libraries                       */
  USHORT mem;           /* # of shared memory handles           */
  USHORT reserved3;
  ULONG  psem;
  ULONG  plib;
  ULONG  pmem;
  };

struct Thread
  {
  ULONG  type;          /* Thread: 100                          */
  USHORT tid;           /* Thread ID                            */
  USHORT tsysid;        /* other Thread ID (?)                  */
  ULONG  blockid;       /* id thread is "sleeping" on           */
  ULONG  priority;      /* Thread priority                      */
  ULONG  systime;       /* thread system time                   */
  ULONG  usertime;      /* thread user time                     */
  UCHAR  status;
  UCHAR  unused1;
  USHORT unused2;
  };

struct Procstat *procstat;

#endif


//  Error - abort program with an error message
//
//  code:    display specific message if != 0 (input)
//  apicode: display system error message if != 0 (input)

static void Error (int code, int apicode)
  {
    switch(code)
      {
        case  0 : break;
        case  1 : printf("JUMP_SHELL, OS2_SHELL and COMSPEC not set\n"); break;
        case  2 : printf("don't redirect JUMP's help!\n"); break;
        case  3 : printf("QueryModuleName (%i): ",apicode); break;
        case  4 : printf("ExecPgm (%i): ",apicode); break;
        case  5 : printf("CreatePipe (%i): ",apicode); break;
        case  6 : printf("Read (%i): ",apicode); break;
        case  7 : printf("StartTimer (%i): ",apicode); break;
        case  8 : printf("CreateEventSem (%i): ",apicode); break;
        case  9 : printf("CreateThread (%i): ",apicode); break;
        case 10 : printf("CreateMutexSem (%i): ",apicode); break;
        case 11 : printf("malloc: not enough memory\n"); break;
        case 12 : printf("RequestMutexSem (%i): ",apicode); break;
        case 13 : printf("ReleaseMutexSem (%i): ",apicode); break;
        case 14 : printf("AllocMem (%i): ",apicode); break;
        case 15 : printf("Close (%i): ",apicode); break;
        case 16 : printf("DupHandle (%i): ",apicode); break;
        case 17 : printf("Write (%i): ",apicode); break;
        case 18 : printf("Close (%i): ",apicode); break;
        default : printf("(unknown code: %i)\n",code);
      }

    if (apicode)
      {
        char Msg[BUFFER1k];
        ULONG Msg_Len=0;
        DosGetMessage(NULL,0,Msg,sizeof(Msg),apicode,"OSO001.MSG",&Msg_Len);
        Msg[Msg_Len]='\0';
        printf("%s\n",Msg);
      }
    exit(code);
  }


//  GetOs2Shell - search environment variables and command line for
//                command interpreter file name and arguments
//
//  direct: parameters from command line (input)
//  *prg:   command interpreter exe file name (output)
//  *arg:   shell arguments (output)

static void GetOs2Shell (char *direct, char **prg, char **arg)
  {
    APIRET rc;
    PSZ Os2Shell;
    char *p;

    // get environment variable: i.e. "C:\OS2\CMD.EXE /k set prompt="

    if (DosScanEnv("JUMP_SHELL",(MY_PSZ *)&Os2Shell))
      if (DosScanEnv("OS2_SHELL",(MY_PSZ *)&Os2Shell))
        if ((rc=DosScanEnv("COMSPEC",(MY_PSZ *)&Os2Shell)))
          Error(1,rc);

    // divide EXE file name and parameters:
    // i.e. *prg="C:\OS2\CMD.EXE", *arg="/k set prompt="

    if ((*prg=malloc(strlen(Os2Shell)+1))==NULL)
      Error(11,0);
    strcpy(*prg,Os2Shell);
    if ((p=strchr(*prg,' ')))
      {
        *p++='\0';  // terminate EXE file name
        *arg=p;     // next byte: parameters
      }
    else
      *arg=Os2Shell+strlen(Os2Shell);   // no parameters, ptr to "\0"

    // if command line parameters exist, use them instead, prepend "/C "

    if (direct && direct[0]!='\0')
      {
        if ((*arg=malloc(strlen(direct)+4))==NULL)
          Error(11,0);
        strcpy(*arg,"/C ");
        strcat(*arg,direct);
      }
  }


//  RedirectOs2Shell - save stdout/stderr, redirect to unnamed pipe
//

static void RedirectOs2Shell (void)
  {
    APIRET rc;

    SaveStdout=-1;                          // allocate new handle
    if ((rc=DosDupHandle(1,&SaveStdout)))   // SaveStdout: dup of stdout
      Error(16,rc);
    SaveStderr=-1;                          // allocate new handle
    if ((rc=DosDupHandle(2,&SaveStderr)))   // SaveStderr: dup of stderr
      Error(16,rc);

    if ((rc=DosCreatePipe(&hpR,&hpW,PIPESIZE)))
      Error(5,rc);

    NewStdout=1;                            // stdout handle
    if ((rc=DosDupHandle(hpW,&NewStdout)))  // Write-Pipe is alias of stdout
      Error(16,rc);
    NewStderr=2;                            // stderr handle
    if ((rc=DosDupHandle(hpW,&NewStderr)))  // Write-Pipe is alias of stderr
      Error(16,rc);
  }


//  RestoreRedirection stdout/stderr are restored

static void RestoreRedirection (void)
  {
    APIRET rc;
    if ((rc=DosClose(hpW)))
      Error(15,rc);
    if ((rc=DosDupHandle(SaveStdout,&NewStdout)))   // stdout alias of old one
      Error(16,rc);
    if ((rc=DosDupHandle(SaveStderr,&NewStderr)))   // stderr alias of old one
      Error(16,rc);
  }


//  EndRedirection - close unneeded file handles

static void EndRedirection (void)
  {
    APIRET rc;

    if ((rc=DosClose(hpR)))
      Error(18,rc);
    if ((rc=DosClose(SaveStdout)))
      Error(18,rc);
    if ((rc=DosClose(SaveStderr)))
      Error(18,rc);
  }


//  StartOs2Shell - start command interpreter

static void StartOs2Shell (char *prg, char *arg, ULONG exeflags)
  {
    CHAR ObjNameBuf[CCHMAXPATH];
    CHAR ArgPointer[BUFFER4k];
    RESULTCODES ReturnCodes;
    APIRET rc;

    // create argument buffer:
    // i.e.  C:\OS2\CMD.EXE\0/C dir\0\0

    strcpy(ArgPointer,prg);
    strcpy(ArgPointer+strlen(prg)+1,arg);
    ArgPointer[strlen(prg)+1+strlen(arg)+1]='\0';

    if ((rc=DosExecPgm((PUCHAR)&ObjNameBuf,sizeof(ObjNameBuf),exeflags,
                       (PUCHAR)&ArgPointer,NULL,&ReturnCodes,prg)))
      {
        RestoreRedirection();
        EndRedirection();
        Error(4,rc);
      }
  }


//  ReadScreen - read cursor position and screen contents

static void ReadScreen (void)
  {
    USHORT Length=SLength;
    VioReadCharStr((PCH)Screen,&Length,0,0,0);
    VioGetCurPos(&row,&col,0);
  }


//  WriteScreen - set cursor position and write screen

static void WriteScreen (void)
  {
    VioSetCurPos(row,col,0);
    VioWrtCharStr(Screen,SLength,0,0,0);
  }


static void PrintBuffer (void)
  {
    APIRET rc;
    BYTE *p;
    BYTE *q;
    BYTE c;
    ULONG bw;

    if (BufferPos==Buffer)  // exit if nothing to print
      return;

    if ((rc=DosRequestMutexSem(BufferSem,SEM_INDEFINITE_WAIT)))
      Error(12,rc);

    ReadScreen();   // read the screen
    p=Buffer;       // point to beginning of output buffer

    while (p<BufferPos)     // while there are character to process
      switch ((c=*p++))     // c: next character
        {
          case  7 : DosBeep(440,50);                    // BEL: just beep
                    break;

          case  8 : if (col>0)                          // BS: backspace
                      Screen[row*SWidth+(--col)]=' ';
                    break;

          case  9 : WriteScreen();                      // TAB
                    if ((rc=DosWrite(1,"\t",1,&bw)) || bw!=1)
                      Error(17,rc);
                    ReadScreen();
                    break;

          case 10 : row++;                              // LF: line feed
                    if (row>=SHeight)
                      {
                        row--;
                        memcpy(Screen,Screen+SWidth,SLength-SWidth);
                        memset(&Screen[SLength-SWidth],' ',SWidth);
                      }
                    break;

          case 13 : col=0;                              // CR: carriage return
                    break;

          case 27 :                                     // ESC: ANSI codes

                    if (p>=BufferPos)   // premature end of buffer?
                      break;

                    if (*p != '[')      // no ESC[ ANSI sequence?
                      break;

                    p++;    // p ptr behind ESC[
                    q=p-2;  // q ptr to ESC-sequence

                    while (p<BufferPos) // as long as there are more chars
                      {
                        c=*p++;

                        if (c=='"' || c=='`' || c=='\'')
                          {
                            while (p<BufferPos && *p!=c)
                              p++;
                            if (p<BufferPos)  // matching '"' found
                              p++;
                          }
                        else
                          if ((c<'0' || c>'9') && c!=';') // command char?
                            break;
                      }

                    WriteScreen();
                    if ((rc=DosWrite(1,q,p-q,&bw)) || bw!=p-q)
                      Error(17,rc);
                    ReadScreen();
                    break;

          default : Screen[row*SWidth+col]=c;           // other character
                    if (++col>=SWidth)
                      {
                        col=0;
                        row++;
                        if (row>=SHeight)
                          {
                            row--;
                            memcpy(Screen,Screen+SWidth,SLength-SWidth);
                            memset(&Screen[SLength-SWidth],' ',SWidth);
                          }
                      }
        }

    WriteScreen();
    BufferPos=Buffer;

    if ((rc=DosReleaseMutexSem(BufferSem)))
      Error(13,rc);
  }


static void InitBuffers (void)
  {
    APIRET rc;

    if (NormalMode)
      return;
    if ((Buffer=malloc(BUFFERSIZE))==NULL)
      Error(11,0);
    BufferPos=Buffer;
    if ((rc=DosCreateMutexSem(NULL,&BufferSem,0,FALSE)))
      Error(10,rc);
  }


static void FlushBuffers (void)
  {
    if (NormalMode)
      return;
    DosStopTimer(TimerHandle);
    ExitThread=TRUE;
    DosPostEventSem((HEV)OutputSem);
    PrintBuffer();
  }


static void ProcessRedirection (void)
  {
    ULONG br;

    do
      {
        APIRET rc;
        char PipeBuffer[PIPESIZE];

        if ((rc=DosRead(hpR,(PVOID)&PipeBuffer,PIPESIZE,&br)))
          Error(6,rc);
        if (br)
          if (!NormalMode)
            {
              if ((rc=DosRequestMutexSem(BufferSem,SEM_INDEFINITE_WAIT)))
                Error(12,rc);
              memcpy(BufferPos,PipeBuffer,br);
              BufferPos+=br;
              if ((rc=DosReleaseMutexSem(BufferSem)))
                Error(13,rc);
              if (BufferPos-Buffer>BUFFERSIZE-2*PIPESIZE)
                PrintBuffer();
            }
          else
            {
              ULONG bw;
              if ((rc=DosWrite(1,(PVOID)&PipeBuffer,br,&bw)) || bw!=br)
                Error(17,rc);
            }
      }
    while (br);
  }


static void PrintThread (void *dummy)
  {
    ULONG PostCount;

    DosSetPriority(PRTYS_THREAD,PRINTPRIO,0,0);

    while (!ExitThread)
      {
        DosWaitEventSem((HEV)OutputSem,SEM_INDEFINITE_WAIT);
        DosResetEventSem((HEV)OutputSem,&PostCount);
        PrintBuffer();
      }
  }


static void InitTimers (void)
  {
    APIRET rc;
    TID tid;

    if (NormalMode)
      return;
    if ((rc=DosCreateEventSem(NULL,(PHEV)&OutputSem,DC_SEM_SHARED,FALSE)))
      Error(8,rc);
    if ((rc=DosStartTimer(PRINT_INTERVAL,OutputSem,&TimerHandle)))
      Error(7,rc);
    ExitThread=FALSE;
    if ((rc=DosCreateThread(&tid,(PFNTHREAD)&PrintThread,0,0,TSTACK)))
      Error(9,rc);
  }


static void InitVideo (void)
  {
    VIOMODEINFO mi;

    if (NormalMode)
      return;
    mi.cb=sizeof(VIOMODEINFO);
    VioGetMode(&mi,0);
    SWidth=mi.col;
    SHeight=mi.row;
    SLength=SWidth*SHeight;
    if ((Screen=malloc(SLength))==NULL)
      Error(11,0);
  }


#ifdef PRELOAD
static BOOL Preloaded (void)
  {
    APIRET rc;
    struct Process *proc;
    UCHAR name[CCHMAXPATH];

    if ((rc=DosAllocMem((PPVOID)&procstat,0xF000,
            PAG_COMMIT|OBJ_TILE|PAG_READ|PAG_WRITE)))
      Error(14,rc);
    DosQProcStatus(procstat,0xF000);

    for (proc=PTR(procstat->processes,0);  proc->type!=3;
         proc=PTR(proc->threadlist, proc->threads*sizeof(struct Thread)))
      if ( proc->type == 1 &&
           proc->ppid == 0 &&
           proc->sems == 0 &&
           proc->threads == 1 &&
           DosQueryModuleName(proc->handle, sizeof(name), name) == 0 &&
           strlen(name) >= 8 )
        {
          UCHAR *p = &name[strlen(name)-8];
          int i;
          for (i=0; i<=7; i++)
            if (p[i]>='a' && p[i]<='z')
              p[i]-='a'-'A';
          if (strcmp("JUMP.EXE",p)==0)
            return TRUE;
        }
    return FALSE;
  }


static char *GetLoadPath (void)
  {
    PTIB pptib;
    PPIB pppib;
    char *szPath;

    DosGetInfoBlocks(&pptib, &pppib);
    szPath = pppib -> pib_pchenv;
    while (*szPath)
       szPath = (char *) strchr(szPath, 0) + 1;
    return szPath + 1;
  }
#endif


static void GetCmdLine (int argc, char *argv[], char **cline)
  {
    int i;

    if ((*cline=malloc(BUFFER1k))==NULL)
      Error(11,0);
    *cline[0]='\0';

    for (i=1; i<argc; i++)
      {
        strcat(*cline,argv[i]);
        strcat(*cline," ");
      }

    #ifdef PRELOAD
    if (strcmp(*cline,"/PRELOAD ")==0)
      {
        StartOs2Shell(GetLoadPath(),"",EXEC_LOAD);
        exit(0);
      }
    #endif

    if ( strcmp(*cline,"/? ")==0 ||
         strcmp(*cline,"/h ")==0 ||
         strcmp(*cline,"/H ")==0 )
      {
        if (NormalMode)
          Error(2,0);
        InitBuffers();
        InitTimers();
        InitVideo();
        sprintf(Buffer,
          "JUMP 1.0 (c) 1994 by Thomas Opheys, all rights reserved\r\n"
          "jump-scroll output of programs using stdout and stderr\r\n"
          "\r\n"
          "usage: %s <command>\r\n"
          "\r\n"
          "JUMP_SHELL, OS2_SHELL, COMSPEC environment variables are\r\n"
          "used to determine the command interpreter to be started.\r\n"
          "\r\n"
          "please send email to opheys@kirk.fmi.uni-passau.de !\r\n"
        , argv[0]);
        BufferPos=Buffer+strlen(Buffer);
        FlushBuffers();
        exit(0);
      }
  }


int main (int argc, char *argv[])
  {
    NormalMode=(!isatty(1));
    GetCmdLine(argc,argv,&cmdline);
    #ifdef PRELOAD
    if (!Preloaded())
      StartOs2Shell(GetLoadPath(),"/PRELOAD",EXEC_BACKGROUND);
    #endif
    InitBuffers();
    InitTimers();
    InitVideo();
    GetOs2Shell(cmdline, &ShellPrg, &ShellArg);
    RedirectOs2Shell();
    StartOs2Shell(ShellPrg,ShellArg,EXEC_ASYNC);
    RestoreRedirection();
    ProcessRedirection();
    EndRedirection();
    FlushBuffers();
    return 0;
  }

