#ifndef __TPREFERENCES_H
#define __TPREFERENCES_H

enum TConnectionType
{
    kSerial,
    kTcp,
    kAppleTalk
};

class TPreferences
{
public:
    char*                       fPort;
    int                         fPortSpeed;
    char**                      fPlatformFileSearchPaths;
    TConnectionType             fConnectionType;
    int                         fTcpPort;
    char*                       fLogFileName;
    char*                       fProjectFileName;
    bool                        fCompileOnly;
    bool                        fDumpPackage;
    
                                TPreferences ();
                                ~TPreferences ();
                                
    void                        MReadRcFile ();
    void                        MSetPlatformFileSeachPaths (char* arg);
    void                        MInitPreferences (int argc, char *argv[]);
};

#endif
