#ifndef __TTNKT_H
#define __TTNTK_H

// POSIX
#include <stdio.h>

class TPackage;
class TPreferences;

class TTntk
{
public:
    bool                        fDisconnect;
    TPackage*                   fPackage;
    TPreferences*               fPreferences;

                                TTntk (int argc, char* argv[]);
                                ~TTntk ();

    void                        MRun ();
};

#endif
