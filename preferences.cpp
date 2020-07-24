#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include "preferences.h"

static const char kOptions[] = "p:s:t:P:l:cd";
static const char kRcFile[] = ".tntkrc";
static const char kDefaultPort[] = "/dev/ttyS0";
static const char kDefaultLogFileName[] = "/dev/null";
const int kDefaultSpeed = 38400;

TPreferences::TPreferences ():
    fPort (NULL),
    fPortSpeed (0),
    fPlatformFileSearchPaths (NULL),
    fConnectionType (kSerial),
    fLogFileName (NULL),
    fProjectFileName (NULL),
    fCompileOnly (false),
    fDumpPackage (false)
{
    fPort = strdup (kDefaultPort);
    fLogFileName = strdup (kDefaultLogFileName);
}

TPreferences::~TPreferences ()
{
    int i;

    delete fPort;
    if (fPlatformFileSearchPaths != NULL) {
        for (i = 0; fPlatformFileSearchPaths[i] != NULL; i++) {
            delete fPlatformFileSearchPaths[i];
        }
        delete fPlatformFileSearchPaths;
    }
    delete fLogFileName;
    delete fProjectFileName;
}

void TPreferences::MReadRcFile ()
{
}

void TPreferences::MSetPlatformFileSeachPaths (char* arg)
{
    char* p;
    int i;

    if (fPlatformFileSearchPaths != NULL) {
        for (i = 0; fPlatformFileSearchPaths[i] != NULL; i++) {
            delete fPlatformFileSearchPaths[i];
        }
        delete fPlatformFileSearchPaths;
    }

    i = 0;
    p = strtok (arg, ":");
    while (p != NULL)  {
        fPlatformFileSearchPaths = (char**) realloc (fPlatformFileSearchPaths, sizeof (char*) * (i + 1));
        fPlatformFileSearchPaths[i] = strdup (p);
        p = strtok (NULL, ":");
        i++;
    }
    fPlatformFileSearchPaths = (char**) realloc (fPlatformFileSearchPaths, sizeof (char*) * (i + 1));
    fPlatformFileSearchPaths[i] = NULL;
}

void TPreferences::MInitPreferences (int argc, char *argv[])
{
    char opt;

    MReadRcFile ();
    while ((opt = getopt (argc, argv, kOptions)) != -1) {
        switch (opt) {
            case 'p':
                delete fPort;
                fPort = strdup (optarg);
                fConnectionType = kSerial;
                break;
            case 's':
                fPortSpeed = atoi (optarg);
                break;
            case 't':
                fTcpPort = atoi (optarg);
                fConnectionType = kTcp;
                break;
            case 'P':
                MSetPlatformFileSeachPaths (optarg);
                break;
            case 'l':
                delete fLogFileName;
                fLogFileName = strdup (optarg);
                break;
            case 'c':
                fCompileOnly = true;
                break;
            case 'd':
                fDumpPackage = true;
                break;
        }
    }
    if (optind < argc) {
        fProjectFileName = strdup (argv[optind]);
    }
}

