#ifndef __TPACKAGE_H
#define __TPACKAGE_H

#include <NewtCore.h>
#include "part.h"

class TPackage
{
public:
    int                         fPackageDataLen;
    unsigned char*              fPackageData;

    char*                       fPackageName;
    char*                       fOutputFileName;
    char*                       fPlatformFileName;
    char*                       fProjectFileName;
    char*                       fCopyright;
    
    TPart**                     fParts;
    int                         fNumParts;

                                TPackage (const char* projectFileName);
                                ~TPackage ();

    void                        MBuildPackage ();
    void                        MSavePackage ();
    void                        MDumpPackage ();

protected:
    void                        MReset ();
    void                        MReadProjectFile ();
    newtRef                     MInterpretFile (const char* fileName);

};

#endif
