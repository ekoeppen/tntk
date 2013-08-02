#include <stdio.h>
#include <strings.h>
#ifdef HAVE_LIBREADLINE
#include <readline/readline.h>
#endif

#ifdef HAVE_CDCL
#include <DCL/Communication_Layers/POSIX/TDCLFDSerialPort.h>
#include <DCL/Communication_Layers/TDCLSyncCommLayer.h>
#include <DCL/Link/TDCLFullDockLink.h>
#include <DCL/Link/Inspector_Commands/TDCLInspectorCmdCode.h>
#include <DCL/Link/Inspector_Commands/TDCLInspectorCmdCodeBlock.h>
#include <DCL/Link/Inspector_Commands/TDCLInspectorCmdFuncObj.h>
#include <DCL/Link/Inspector_Commands/TDCLInspectorCmdLoadPkg.h>
#include <DCL/Link/Inspector_Commands/TDCLInspectorCmdDeletePkg.h>
#endif

#include <NewtCore.h>
#include <NewtVM.h>
#include <NewtBC.h>
#include <NewtParser.h>
#include <NewtNSOF.h>
#include <NewtPrint.h>

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
    if (fPreferences->fProjectFileName != NULL) {
        fPackage = new TPackage (fPreferences->fProjectFileName);
    }
}

TTntkBase::~TTntkBase ()
{
    delete fPackage;
    delete fPreferences;
}

void TTntkBase::MRun ()
{
    fPackage->MBuildPackage ();
    fPackage->MSavePackage ();
    printf ("Package %s created.\n", fPackage->fOutputFileName);
}

#ifdef HAVE_CDCL
TTntk::TTntk (int argc, char* argv[]):
    TTntkBase (argc, argv),
    TDCLLogApplication (NULL),
    fDisconnect (false),
    fConnected (NULL)
{
    fConnected = GetThreadsIntf()->CreateSemaphore ();
    fConnected->Acquire ();
    mLogFile = fopen (fPreferences->fLogFileName, "w+");
}

TTntk::~TTntk ()
{
    fclose (mLogFile);
    delete fConnected;
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
    fConnected->Release ();
}

void TTntk::Disconnected (TDCLLink* aLink)
{
    TDCLLogApplication::Disconnected (aLink);
    fConnected->Release ();
}

void TTntk::MRun ()
{
    if (fPreferences->fCompileOnly) {
        TTntkBase::MRun();
    } else {
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
            fPackage->MBuildPackage ();
            fPackage->MSavePackage ();
            } break;
        case 'd': {
            printf ("Removing package %s.\n", fPackage->fPackageName);
            TDCLInspectorCmdDeletePkg remove (fPackage->fPackageName);
            remove.SendCommand (link->GetPipe ());
            sleep (1);
            printf ("Downloading package %s.\n", fPackage->fPackageName);
            TDCLInspectorCmdLoadPkg load (fPackage->fPackageDataLen, fPackage->fPackageData);
            load.SendCommand (link->GetPipe ());
            } break;
        case 'x': {
            printf ("Removing package %s.\n", fPackage->fPackageName);
            TDCLInspectorCmdDeletePkg remove (fPackage->fPackageName);
            remove.SendCommand (link->GetPipe ());
            } break;
    }
}

void TTntk::MCommandLoop (TDCLServer *server, TDCLLink *link)
{
    char *command;
    char *s;

    fDisconnect = false;
    fConnected->Acquire ();
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
    fConnected->Release ();
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

#ifdef HAVE_CDCL
    app = new TTntk (argc, argv);
#else
    app = new TTntkBase (argc, argv);
#endif
    app->MRun ();

}
