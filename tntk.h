#ifndef __TTNTK_H
#define __TTNTK_H

// POSIX
#include <stdio.h>

// DCL
#ifdef HAVE_DCL
#include <DCL/Interfaces/Common/TDCLLogApplication.h>
#include <DCL/Interfaces/POSIX/TDCLPOSIXFiles.h>
#include <DCL/Interfaces/POSIX/TDCLPThreads.h>
#include <DCL/Interfaces/IDCLThreads.h>
#include <DCL/Server/TDCLSimpleServer.h>
#include <DCL/Link/TDCLInspectorLink.h>
#endif

class TPackage;
class TPreferences;

class TTntkBase
{
public:
    TPackage*                   fPackage;
    TPreferences*               fPreferences;

                                TTntkBase (int argc, char* argv[]);
    virtual                     ~TTntkBase ();

    virtual void                MRun ();
};

#ifdef HAVE_DCL
class TTntk: public TDCLLogApplication, public TTntkBase
{
public:
    bool                        fDisconnect;
    IDCLThreads::ISemaphore*    fConnected;

                                TTntk (int argc, char* argv[]);
                                ~TTntk ();

    void                        MRun ();
    void                        MCommandLoop (TDCLServer *server, TDCLLink *link);
    void                        MSendCode (char *command, TDCLLink *link);
    void                        MHandleCommand (char *command, TDCLLink *link);
    void                        MCompileString (char *s, void**out, int* outlen);

    virtual void                ConnectedToNewtonDevice(TDCLLink* inLink, const KUInt16* inName);
    virtual void                Disconnected(TDCLLink* inLink);
    IDCLFiles*                  CreateFilesIntf ();
    IDCLThreads*                CreateThreadsIntf ();
};
#endif

#endif
