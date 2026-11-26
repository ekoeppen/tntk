#ifndef __TTNTK_H
#define __TTNTK_H

// POSIX
#include <stdio.h>

// C++
#ifdef HAVE_DCL
#include <atomic>
#include <condition_variable>
#include <mutex>
#endif

// DCL
#ifdef HAVE_DCL
#include <DCL/Interfaces/Common/TDCLLogApplication.h>
#include <DCL/Interfaces/POSIX/TDCLPOSIXFiles.h>
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

    virtual int                 MRun ();
};

#ifdef HAVE_DCL
class TTntk: public TDCLLogApplication, public TTntkBase
{
public:
    std::atomic<bool>           fDisconnect;
    bool                        fConnected;
    std::mutex                  fConnectedMutex;
    std::condition_variable     fConnectedCV;

                                TTntk (int argc, char* argv[]);
                                ~TTntk ();

    int                         MRun ();
    void                        MCommandLoop (TDCLServer *server, TDCLLink *link);
    void                        MSendCode (char *command, TDCLLink *link);
    void                        MHandleCommand (char *command, TDCLLink *link);
    void                        MCompileString (char *s, void**out, int* outlen);

    virtual void                ConnectedToNewtonDevice(TDCLLink* inLink, const KUInt16* inName);
    virtual void                Disconnected(TDCLLink* inLink);
    IDCLFiles*                  CreateFilesIntf ();
};
#endif

#endif
