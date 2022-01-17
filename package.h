#ifndef __TPACKAGE_H
#define __TPACKAGE_H

#include <NewtCore.h>
#include "part.h"
#include "preferences.h"

class TPackage
{
public:
    int                         fPackageDataLen;
    unsigned char*              fPackageData;

    TPreferences*               fPreferences;
    char*                       fPackageName;
    char*                       fOutputFileName;
    char*                       fPlatformFileName;
    char*                       fCopyright;
    
    TPart**                     fParts;
    int                         fNumParts;

                                TPackage (TPreferences* preferences);
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
