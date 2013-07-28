#ifndef __TPART_H
#define __TPART_H

#include <NewtCore.h>

enum TPartType
{
    kForm = 0x666f726d,
    kAuto = 0x6175746f,
    kBook = 0x626f6f6b,
    kFont = 0x666f6e74,
    kDict = 0x64696374,
    kStore = 0x73746f72
};

class TPart
{
public:
    TPartType                   fType;

    char*                       fMainFile;
    char**                      fFiles;
    int                         fNumFiles;

    newtRefVar                  fMainForm;
    newtRefVar                  fConstants;
    newtRefVar                  fGlobalFunctions;

                                TPart (newtRefVar partInfo);
                                ~TPart ();

    void                        MBuildPart (const char* platformFileName);
    void                        MDump ();

protected:
    newtRef                     MInterpretFile (const char* fileName);
    void                        MReadPlatformFile (const char* platformFileName);
};

#endif
