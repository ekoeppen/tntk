// ***** BEGIN LICENSE BLOCK *****
// Version: MPL 1.1
//
// The contents of this file are subject to the Mozilla Public License Version
// 1.1 (the "License"); you may not use this file except in compliance with
// the License. You may obtain a copy of the License at
// http://www.mozilla.org/MPL/
//
// Software distributed under the License is distributed on an "AS IS" basis,
// WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
// for the specific language governing rights and limitations under the
// License.
//
// The Original Code is part.cpp.
//
// The Initial Developer of the Original Code is Eckhart Köppen.
// Copyright (C) 2004 the Initial Developer. All Rights Reserved.
//
// ***** END LICENSE BLOCK *****

// ANSI C & POSIX
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <limits.h>

#ifdef _WIN32
#include <io.h>
#define strdup _strdup
#else
#include <libgen.h>
#include <unistd.h>
#endif

// NEWT/0
extern "C" {
#include <NewtCore.h>
#include <NewtVM.h>
#include <NewtParser.h>
#include <NewtNSOF.h>
#include <NewtBC.h>
#include <NewtGC.h>
#include <NewtPkg.h>
#include <NewtPrint.h>
}

#include "part.h"
#include "NewtType.h"

#ifndef PATH_MAX
#ifdef _MAX_PATH
#define PATH_MAX _MAX_PATH
#else
#define PATH_MAX 4096
#endif
#endif

static char* ResolvePath(const char* baseDir, const char* path)
{
    if (path == NULL) return NULL;
    if (path[0] == '/'
#ifdef _WIN32
        || (isalpha((unsigned char)path[0]) && path[1] == ':') || path[0] == '\\'
#endif
    ) {
        return strdup(path);
    }
    if (baseDir == NULL || strlen(baseDir) == 0 || strcmp(baseDir, ".") == 0) {
        return strdup(path);
    }
    char buf[PATH_MAX];
    snprintf(buf, sizeof(buf), "%s/%s", baseDir, path);
    return strdup(buf);
}

const char *helper = " __tntk := func() begin end;\nDefGlobalVar('theBase, __tntk.argFrame);\n";

TPart::TPart (newtRefVar partInfo, const char* projectDir):
    fType (kForm),
    fMainFile (NULL),
    fConstants (kNewtRefNIL),
    fGlobalFunctions (kNewtRefNIL),
    fFiles (NULL),
    fNativeModules (NULL),
    fNumNativeModules (0),
    fNumFiles (0),
    fRelocations(kNewtRefNIL),
    fBaseAddress(0)
{
    newtRefVar files, mainSlot;
    int i;

    mainSlot = NcGetSlot (partInfo, NewtMakeSymbol ("main"));
    if (mainSlot != kNewtRefUnbind && NewtRefIsString (mainSlot)) {
        fMainFile = ResolvePath (projectDir, NewtRefToString (mainSlot));
    }
    files = NcGetSlot (partInfo, NewtMakeSymbol ("files"));
    if (files != kNewtRefUnbind) {
        fNumFiles = NewtArrayLength (files);
        fFiles = (char **) malloc (fNumFiles * sizeof (char *));
        for (i = 0; i < fNumFiles; i++) {
            fFiles[i] = ResolvePath (projectDir, NewtRefToString (NewtGetArraySlot (files, i)));
        }
    }
    files = NcGetSlot (partInfo, NewtMakeSymbol ("modules"));
    if (files != kNewtRefUnbind) {
        fNumNativeModules = NewtArrayLength (files);
        fNativeModules = (char **) malloc (fNumNativeModules * sizeof (char *));
        for (i = 0; i < fNumNativeModules; i++) {
            fNativeModules[i] = ResolvePath (projectDir, NewtRefToString (NewtGetArraySlot (files, i)));
        }
    }
    if (NcGetSlot (partInfo, NewtMakeSymbol ("type")) == NewtMakeSymbol ("auto")) {
        fType = kAuto;
    }
}

TPart::~TPart ()
{
    while (fNumFiles-- > 0) {
        free (fFiles[fNumFiles]);
    }
    while (fNumNativeModules-- > 0) {
        free (fNativeModules[fNumNativeModules]);
    }
    free (fFiles);
    free (fNativeModules);
    free (fMainFile);
}

newtRef TPart::MLoadNativeModule (const char *f)
{
    int file;
    struct stat st;
    uint8_t *data;
    newtRef binary, name, code, entryPoints, relocations, nativeFuncs, baseAddress;

    file = open(f, O_RDONLY);
    if (file < 0) {
        fprintf (stderr, "Error: cannot open module file %s\n", f);
        exit (1);
    }
    if (fstat (file, &st) != 0) {
        fprintf (stderr, "Error: cannot stat module file %s\n", f);
        close (file);
        exit (1);
    }
    printf ("Reading module %s, size %ld\n", f, (long)st.st_size);
    data = (uint8_t *) malloc (st.st_size);
    if (data == NULL && st.st_size > 0) {
        fprintf (stderr, "Error: out of memory reading %s\n", f);
        close (file);
        exit (1);
    }
    if (read (file, data, st.st_size) < 0) {
        fprintf (stderr, "Error reading %s\n", f);
        close (file);
        free (data);
        exit (1);
    }
    binary = NewtReadNSOF (data, st.st_size);
    name = NcGetSlot (binary, NewtMakeSymbol ("name"));
    code = NcGetSlot (binary, NewtMakeSymbol ("code"));
    entryPoints = NcGetSlot (binary, NewtMakeSymbol ("entryPoints"));
    nativeFuncs = NsMakeFrame (kNewtRefNIL);
    for (size_t t = 0; t < NewtArrayLength (entryPoints); t++) {
        newtRef ep (NewtGetArraySlot (entryPoints, t));
        newtRef func = NsMakeFrame (kNewtRefNIL);
        NsSetSlot (kNewtRefNIL, func, NSSYM0(__class), NSSYM(BinCFunction));
        NsSetSlot (kNewtRefNIL, func, NSSYM(numArgs), NsGetSlot (kNewtRefNIL, ep, NSSYM (numArgs)));
        NsSetSlot (kNewtRefNIL, func, NSSYM(offset), NsGetSlot (kNewtRefNIL, ep, NSSYM (offset)));
        NsSetSlot (kNewtRefNIL, func, NSSYM(code), code);
        NsSetSlot (kNewtRefNIL, nativeFuncs, NsGetSlot (kNewtRefNIL, ep, NSSYM (name)), func);
    }
    fRelocations = NcGetSlot (binary, NewtMakeSymbol ("relocations"));
    baseAddress = NcGetSlot (binary, NewtMakeSymbol ("baseAddress"));
    fBaseAddress = NewtRefIsInteger (baseAddress) ? NewtRefToInteger (baseAddress) : 0;
    NsSetSlot (kNewtRefNIL, nativeFuncs, NSSYM(relocations), fRelocations);
    NcSetGlobalVar (name,  nativeFuncs);
    close (file);
    free (data);
    return binary;
}

newtRef TPart::MInterpretFile (const char *f)
{
    nps_syntax_node_t *stree = NULL;
    uint32_t numStree = 0;
    newtRefVar fn = kNewtRefNIL;
    newtRefVar constants;
    newtErr err;
    newtRefVar map, s, v;
    char *script;
    int i;
    int file;
    struct stat st;

    if (f == NULL) {
        fprintf (stderr, "Error: no script file specified\n");
        exit (1);
    }

    printf ("Compiling %s\n", f);

    /* Read the script into a string and prepend it with the helper
     * function which exposes the local variable frame */
    file = open(f, O_RDONLY);
    if (file < 0) {
        fprintf (stderr, "Error: cannot open script file %s\n", f);
        exit (1);
    }
    if (fstat (file, &st) != 0) {
        fprintf (stderr, "Error: cannot stat script file %s\n", f);
        close (file);
        exit (1);
    }
    script = (char *) malloc (st.st_size + strlen (helper) + 1);
    if (script == NULL) {
        fprintf (stderr, "Error: out of memory allocating script buffer for %s\n", f);
        close (file);
        exit (1);
    }
    memset (script, 0, st.st_size + strlen (helper) + 1);
    strcpy (script, helper);
    if (read (file, script + strlen(helper), st.st_size) < 0) {
        fprintf (stderr, "Error reading %s\n", f);
        close (file);
        free (script);
        exit (1);
    }
    close (file);

    err = NPSParseStr (script, &stree, &numStree);
    free (script);
    if (err != kNErrNone) {
        fprintf (stderr, "Error parsing %s\n", f);
        exit (1);
    }

    /* Add constants from the platform file */
    NBCInit ();
    constants = NBCConstantTable ();
    if (fConstants != kNewtRefNIL) {
        map = NewtFrameMap (fConstants);
        for (i = 0; i < NewtMapLength (map); i++) {
            s = NewtGetArraySlot (map, i);
            v = NcGetSlot (fConstants, s);
            NcSetSlot (constants, s, v);
        }
    }

    /* The script will generate a function, which is called here, and
     * which will return the last frame in the script */
    fn = NBCGenBC (stree, numStree, true);
    NBCCleanup ();
    fn = NVMCall (fn, 0, &err);
    NPSCleanup ();
    return fn;
}

void TPart::MReadPlatformFile (const char *fileName)
{
    FILE *f;
    uint8_t *buffer;
    struct stat st;
    newtRefVar pf;

    if (fileName == NULL) {
        return;
    }
    f = fopen (fileName, "rb");
    if (f != NULL) {
        printf ("Reading platform file %s\n", fileName);
        if (fstat (fileno (f), &st) != 0) {
            fprintf (stderr, "Error: cannot stat platform file %s\n", fileName);
            fclose (f);
            return;
        }
        buffer = (uint8_t *) malloc (st.st_size);
        if (buffer == NULL && st.st_size > 0) {
            fprintf (stderr, "Error: out of memory reading platform file %s\n", fileName);
            fclose (f);
            return;
        }
        if (fread (buffer, st.st_size, 1, f) != 1 && st.st_size > 0) {
            fprintf (stderr, "Error reading platform file %s\n", fileName);
            free (buffer);
            fclose (f);
            return;
        }
        pf = NewtReadNSOF (buffer, st.st_size);

        fConstants = NcGetSlot (pf, NewtMakeSymbol ("platformConstants"));
        fGlobalFunctions = NcGetSlot (pf, NewtMakeSymbol ("platformFunctions"));

        free (buffer);
        fclose (f);
    } else {
        fprintf (stderr, "Error: cannot open platform file %s\n", fileName);
    }
}

void TPart::MBuildPart (const char *platformFileName)
{
    int i;

    for (i = 0; i < fNumNativeModules; i++) {
        MLoadNativeModule (fNativeModules[i]);
    }
    for (i = 0; i < fNumFiles; i++) {
        MReadPlatformFile (platformFileName);
        MInterpretFile (fFiles[i]);
    }
    MReadPlatformFile (platformFileName);
    fMainForm = MInterpretFile (fMainFile);
}
