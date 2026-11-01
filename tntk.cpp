#include <stdio.h>
#ifdef _WIN32
#define strcasecmp _stricmp
#else
#include <strings.h>
#include <unistd.h>
#endif
#ifdef HAVE_LIBREADLINE
#include <readline/readline.h>
#endif

#ifdef HAVE_DCL
#include <DCL/Communication_Layers/POSIX/TDCLFDSerialPort.h>
#include <DCL/Communication_Layers/TDCLSyncCommLayer.h>
#include <DCL/Link/TDCLFullDockLink.h>
#include <DCL/Link/Inspector_Commands/TDCLInspectorCmdCode.h>
#include <DCL/Link/Inspector_Commands/TDCLInspectorCmdCodeBlock.h>
#include <DCL/Link/Inspector_Commands/TDCLInspectorCmdFuncObj.h>
#include <DCL/Link/Inspector_Commands/TDCLInspectorCmdLoadPkg.h>
#include <DCL/Link/Inspector_Commands/TDCLInspectorCmdDeletePkg.h>
#endif

extern "C" {
#include <NewtCore.h>
#include <NewtVM.h>
#include <NewtBC.h>
#include <NewtParser.h>
#include <NewtNSOF.h>
#include <NewtPrint.h>
}

#include "tntk.h"
#include "package.h"
#include "preferences.h"

#define LOG(...) {printf(__VA_ARGS__); putchar(10);}
#define LOGFN() puts(__PRETTY_FUNCTION__);

TTntkBase::TTntkBase (int argc, char* argv[]):
    fPackage (NULL),
    fPreferences (NULL)
{
    NewtInit (argc, (const char **) argv, argc);
    fPreferences = new TPreferences ();
    fPreferences->MInitPreferences (argc, argv);
    if (!fPreferences->fHelp && fPreferences->fProjectFileName != NULL) {
        fPackage = new TPackage (fPreferences);
    }
}

TTntkBase::~TTntkBase ()
{
    delete fPackage;
    delete fPreferences;
}

int TTntkBase::MRun ()
{
    if (fPreferences->fHelp) {
        fPreferences->MPrintUsage ("tntk");
        return 0;
    }
    if (fPreferences->fCompileOnly) {
        if (fPreferences->fProjectFileName == NULL) {
            fprintf (stderr, "Error: no project file specified for compilation.\n\n");
            fPreferences->MPrintUsage ("tntk");
            return 1;
        }
        fPackage->MBuildPackage ();
        fPackage->MSavePackage ();
        printf ("Package %s created.\n", fPackage->fOutputFileName);
        return 0;
    } else if (fPreferences->fDumpPackage) {
        if (fPreferences->fProjectFileName == NULL) {
            fprintf (stderr, "Error: no package file specified to dump.\n\n");
            fPreferences->MPrintUsage ("tntk");
            return 1;
        }
        fPackage->MDumpPackage ();
        return 0;
    } else {
        fPreferences->MPrintUsage ("tntk");
        return 1;
    }
}

#ifdef HAVE_DCL
TTntk::TTntk (int argc, char* argv[]):
    TTntkBase (argc, argv),
    TDCLLogApplication (NULL),
    fDisconnect (false),
    fConnected (NULL)
{
    if (!fPreferences->fHelp && !fPreferences->fCompileOnly && !fPreferences->fDumpPackage && fPreferences->fProjectFileName != NULL) {
        fConnected = GetThreadsIntf()->CreateSemaphore ();
        fConnected->Acquire ();
        mLogFile = fopen (fPreferences->fLogFileName, "w+");
    }
}

TTntk::~TTntk ()
{
    if (mLogFile != NULL) {
        fclose (mLogFile);
    }
    if (fConnected != NULL) {
        delete fConnected;
    }
}

IDCLFiles* TTntk::CreateFilesIntf ()
{
    return new TDCLPOSIXFiles ();
}

IDCLThreads* TTntk::CreateThreadsIntf ()
{
    return new TDCLPThreads ();
}

void TTntk::ConnectedToNewtonDevice(TDCLLink* inLink, const KUInt16* inName)
{
    puts ("----> Connected");
    if (fConnected != NULL) {
        fConnected->Release ();
    }
}

void TTntk::Disconnected (TDCLLink* aLink)
{
    TDCLLogApplication::Disconnected (aLink);
    if (fConnected != NULL) {
        fConnected->Release ();
    }
}

int TTntk::MRun ()
{
    if (fPreferences->fHelp || fPreferences->fCompileOnly || fPreferences->fDumpPackage) {
        return TTntkBase::MRun();
    } else {
        if (fPreferences->fProjectFileName == NULL) {
            fPreferences->MPrintUsage ("tntk");
            return 1;
        }
        TDCLSimpleServer* server;
        TDCLSyncCommLayer* comms;
        TDCLInspectorLink *link;

        comms = new TDCLFDSerialPort (GetThreadsIntf (), fPreferences->fPort);
        link = new TDCLInspectorLink (this);
        server = new TDCLSimpleServer (this, comms);
        server->SetLink (link);
        server->Start ();
        puts ("Waiting for connection...");
        MCommandLoop (server, link);
        puts ("Quitting...");
        server->Stop ();
        link->Disconnect ();
//    delete link;
//    delete comms;
//    delete server;
        return 0;
    }
}

void TTntk::MCompileString (char *s, void**out, int* outlen)
{
    newtRefVar fn = NBCCompileStr(s, true);
    newtRefVar nsof = NsMakeNSOF (0, fn, NewtMakeInt30 (2));
    newtObjRef obj = NewtRefToPointer (nsof);
    *out = NewtObjData (obj);
    *outlen = NewtObjSize (obj);
}

void TTntk::MSendCode (char *command, TDCLLink *link)
{
    void *compiledCmd;
    int compiledCmdLen;

    MCompileString (command, &compiledCmd, &compiledCmdLen);
    TDCLInspectorCmdCodeBlock cmd (compiledCmdLen, compiledCmd);
    cmd.SendCommand(link->GetPipe());
}

void TTntk::MHandleCommand (char *command, TDCLLink *link)
{
    switch (command[1]) {
        case 'q': {
            fDisconnect = true;
            TDCLInspectorCommand cmd ('term');
            cmd.SendCommand (link->GetPipe ());
            } break;
        case 'c': {
            if (fPackage != NULL) {
                fPackage->MBuildPackage ();
                fPackage->MSavePackage ();
            }
            } break;
        case 'd': {
            if (fPackage != NULL) {
                printf ("Removing package %s.\n", fPackage->fPackageName);
                TDCLInspectorCmdDeletePkg remove (fPackage->fPackageName);
                remove.SendCommand (link->GetPipe ());
                sleep (1);
                printf ("Downloading package %s.\n", fPackage->fPackageName);
                TDCLInspectorCmdLoadPkg load (fPackage->fPackageDataLen, fPackage->fPackageData);
                load.SendCommand (link->GetPipe ());
            }
            } break;
        case 'x': {
            if (fPackage != NULL) {
                printf ("Removing package %s.\n", fPackage->fPackageName);
                TDCLInspectorCmdDeletePkg remove (fPackage->fPackageName);
                remove.SendCommand (link->GetPipe ());
            }
            } break;
    }
}

void TTntk::MCommandLoop (TDCLServer *server, TDCLLink *link)
{
    char *command;
    char *s;

    fDisconnect = false;
    if (fConnected != NULL) {
        fConnected->Acquire ();
    }
    while (!fDisconnect) {
#ifdef HAVE_LIBREADLINE
        command = readline ("> ");
        add_history (command);
#else
        putchar ('>'); putchar (' ');
        command = (char *) malloc (FILENAME_MAX);
        memset (command, 0, FILENAME_MAX);
        fgets (command, FILENAME_MAX, stdin);
#endif
        for (s = command; *s <= ' ' && *s != 0; s++);
        if (*s == ':') {
            MHandleCommand (s, link);
        } else {
            MSendCode (s, link);
        }
        free (command);
    }
    if (fConnected != NULL) {
        fConnected->Release ();
    }
}
#endif

extern "C" void yyerror(char * s)
{
    if (s[0] && s[1] == ':') {
        NPSErrorStr(*s, s + 2);
    } else {
        NPSErrorStr('E', s);
    }
}

int main (int argc, char *argv[])
{
    TTntkBase* app;

#ifdef HAVE_DCL
    app = new TTntk (argc, argv);
#else
    app = new TTntkBase (argc, argv);
#endif
    int result = app->MRun ();
    delete app;
    return result;
}
