#ifndef __TPACKAGE_H
#define __TPACKAGE_H

#include <NewtCore.h>
#include "part.h"

class TPackage
{
public:
    int                         fPackageDataLen;
    char*                       fPackageData;

    char*                       fPackageName;
    char*                       fOutputFileName;
    char*                       fPlatformFileName;
    char*                       fProjectFileName;
    
    TPart**                     fParts;
    int                         fNumParts;

                                TPackage (const char* projectFileName);
                                ~TPackage ();

    void                        MBuildPackage ();
    void                        MSavePackage ();

protected:
    void                        MReset ();
    void                        MReadProjectFile ();
    newtRef                     MInterpretFile (const char* fileName);

    int                         MDCLPartType (TPartType type);
    
};

#endif
