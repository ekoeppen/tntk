#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
#include "win32/getopt.h"
#define strcasecmp _stricmp
#define strdup _strdup
#else
#include <getopt.h>
#include <strings.h>
#endif

#include "preferences.h"

static const char kOptions[] = "p:s:t:P:l:cdh";
static const char kRcFile[] = ".tntkrc";
static const char kDefaultPort[] = "/dev/ttyS0";
static const char kDefaultLogFileName[] = "/dev/null";
const int kDefaultSpeed = 38400;

TPreferences::TPreferences ():
    fPort (NULL),
    fPortSpeed (0),
    fPlatformFileSearchPaths (NULL),
    fConnectionType (kSerial),
    fTcpPort (0),
    fLogFileName (NULL),
    fProjectFileName (NULL),
    fCompileOnly (false),
    fDumpPackage (false),
    fHelp (false)
{
    fPort = strdup (kDefaultPort);
    fLogFileName = strdup (kDefaultLogFileName);
}

TPreferences::~TPreferences ()
{
    int i;

    free (fPort);
    if (fPlatformFileSearchPaths != NULL) {
        for (i = 0; fPlatformFileSearchPaths[i] != NULL; i++) {
            free (fPlatformFileSearchPaths[i]);
        }
        free (fPlatformFileSearchPaths);
    }
    free (fLogFileName);
    free (fProjectFileName);
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
            free (fPlatformFileSearchPaths[i]);
        }
        free (fPlatformFileSearchPaths);
        fPlatformFileSearchPaths = NULL;
    }

    char* copy = strdup (arg);
    i = 0;
    p = strtok (copy, ":");
    while (p != NULL)  {
        fPlatformFileSearchPaths = (char**) realloc (fPlatformFileSearchPaths, sizeof (char*) * (i + 1));
        fPlatformFileSearchPaths[i] = strdup (p);
        p = strtok (NULL, ":");
        i++;
    }
    fPlatformFileSearchPaths = (char**) realloc (fPlatformFileSearchPaths, sizeof (char*) * (i + 1));
    fPlatformFileSearchPaths[i] = NULL;
    free (copy);
}

void TPreferences::MPrintUsage (const char* progName)
{
    printf ("Usage: %s [options] <file>\n", progName);
    printf ("Options:\n");
    printf ("  -c         compile-only\n");
    printf ("  -d         dump contents of a .pkg file\n");
    printf ("  -h, --help print this help message\n");
    printf ("  -l <file>  log file name (default: %s)\n", kDefaultLogFileName);
    printf ("  -p <dev>   serial port device path (default: %s)\n", kDefaultPort);
    printf ("  -P <path>  platform files search path (colon-separated)\n");
    printf ("  -s <speed> serial port speed (default: %d)\n", kDefaultSpeed);
    printf ("  -t <port>  tcp connection port\n");
}

void TPreferences::MInitPreferences (int argc, char *argv[])
{
    char opt;

    for (int i = 1; i < argc; i++) {
        if (strcmp (argv[i], "--help") == 0) {
            fHelp = true;
            return;
        }
    }

    MReadRcFile ();
    while ((opt = getopt (argc, argv, kOptions)) != -1) {
        switch (opt) {
            case 'p':
                free (fPort);
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
                free (fLogFileName);
                fLogFileName = strdup (optarg);
                break;
            case 'c':
                fCompileOnly = true;
                break;
            case 'd':
                fDumpPackage = true;
                break;
            case 'h':
                fHelp = true;
                return;
            default:
                fHelp = true;
                return;
        }
    }
    if (optind < argc) {
        fProjectFileName = strdup (argv[optind]);
    }
}

