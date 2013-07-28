#include <stdio.h>
#ifdef HAVE_LIBREADLINE
#include <readline/readline.h>
#endif

#include <NewtCore.h>
#include <NewtVM.h>
#include <NewtBC.h>
#include <NewtParser.h>
#include <NewtNSOF.h>
#include <NewtPrint.h>

#include "tntk.h"
#include "package.h"
#include "preferences.h"

#define LOG(...) {printf(__VA_ARGS__); putchar(10);}
#define LOGFN() puts(__PRETTY_FUNCTION__);

extern "C" void yyerror(char * s)
{
    if (s[0] && s[1] == ':') {
        NPSErrorStr(*s, s + 2);
    } else {
        NPSErrorStr('E', s);
    }
}

TTntk::TTntk (int argc, char* argv[]):
    fPackage (NULL),
    fPreferences (NULL),
    fDisconnect (false)
{
    NewtInit (argc, (const char **) argv, argc);
    fPreferences = new TPreferences ();
    fPreferences->MInitPreferences (argc, argv);
    if (fPreferences->fProjectFileName != NULL) {
        fPackage = new TPackage (fPreferences->fProjectFileName);
    }
}

TTntk::~TTntk ()
{
    delete fPackage;
    delete fPreferences;
}

void TTntk::MRun ()
{
    fPackage->MBuildPackage ();
    fPackage->MSavePackage ();
    printf ("Package %s created.\n", fPackage->fOutputFileName);
}

int main (int argc, char *argv[])
{
    TTntk* app;

    app = new TTntk (argc, argv);
    app->MRun ();

}
