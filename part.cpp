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
// The Initial Developer of the Original Code is Eckhart K�ppen.
// Copyright (C) 2004 the Initial Developer. All Rights Reserved.
//
// ***** END LICENSE BLOCK *****

// ANSI C & POSIX
#include <libgen.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

// NEWT/0
#include <NewtCore.h>
#include <NewtVM.h>
#include <NewtParser.h>
#include <NewtNSOF.h>
#include <NewtBC.h>
#include <NewtGC.h>
#include <NewtPkg.h>
#include <NewtPrint.h>

#include "part.h"

const char *helper = " __tntk := func() begin end;\nDefGlobalVar('theBase, __tntk.argFrame);\n";

TPart::TPart (newtRefVar partInfo):
    fType (kForm),
    fConstants (kNewtRefNIL),
    fGlobalFunctions (kNewtRefNIL),
    fFiles (NULL),
    fNativeModules (NULL),
    fNumNativeModules (0),
    fNumFiles (0),
    fRelocations(kNewtRefNIL)
{
    newtRefVar files, map;
    int i;

    fMainFile = strdup (NewtRefToString (NcGetSlot (partInfo, NewtMakeSymbol ("main"))));
    files = NcGetSlot (partInfo, NewtMakeSymbol ("files"));
    if (files != kNewtRefUnbind) {
        fNumFiles = NewtArrayLength (files);
        fFiles = (char **) malloc (fNumFiles * sizeof (char *));
        for (i = 0; i < fNumFiles; i++) {
            fFiles[i] = strdup (NewtRefToString (NewtGetArraySlot (files, i)));
        }
    }
    files = NcGetSlot (partInfo, NewtMakeSymbol ("modules"));
    if (files != kNewtRefUnbind) {
        fNumNativeModules = NewtArrayLength (files);
        fNativeModules = (char **) malloc (fNumNativeModules * sizeof (char *));
        for (i = 0; i < fNumNativeModules; i++) {
            fNativeModules[i] = strdup (NewtRefToString (NewtGetArraySlot (files, i)));
        }
    }
    if (NcGetSlot (partInfo, NewtMakeSymbol ("type")) == NewtMakeSymbol ("auto")) {
        fType = kAuto;
    }
}

TPart::~TPart ()
{
    while (fNumFiles-- > 0) {
        delete fFiles[fNumFiles];
    }
    while (fNumNativeModules-- > 0) {
        delete fNativeModules[fNumNativeModules];
    }
    delete fFiles;
    delete fNativeModules;
    delete fMainFile;
}

newtRef TPart::MLoadNativeModule (const char *f)
{
    int file;
    struct stat st;
    uint8_t *data;
    newtRef binary, name, code, entryPoints, relocations, nativeFuncs, baseAddress;

    file = open(f, O_RDONLY);
    fstat (file, &st);
    printf ("Reading module %s, size %lld\n", f, st.st_size);
    data = (uint8_t *) malloc (st.st_size);
    read (file, data, st.st_size);
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
    uint32_t len;
    newtRefVar fn = kNewtRefNIL;
    newtRefVar constants;
    newtErr err;
    newtRefVar map, s, v;
    char *script;
    newtRefVar base = kNewtRefNIL;
    newtObjRef obj;
    int i;
    int file;
    struct stat st;

    printf ("Compiling %s\n", f);

    /* Read the script into a string and prepend it with the helper
     * function which exposes the local variable frame */
    file = open(f, O_RDONLY);
    fstat (file, &st);
    script = (char *) malloc (st.st_size + strlen (helper) + 1);
    memset (script, 0, st.st_size + strlen (helper) + 1);
    strcpy (script, helper);
    read (file, script + strlen(helper), st.st_size);
    close (file);

    err = NPSParseStr (script, &stree, &numStree);

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

    /* Remove the local variables of the called function to avoid inifite
     * recursion when writing the package */
    base = NcGetGlobalVar (NSSYM (theBase));
    obj = NewtRefToPointer (base);
    /*
    for (i = NewtObjSlotsLength (obj) - 1; i > 2; i--) {
        size_t index;
        newtRefVar slot = NewtGetMapIndex (obj->as.map, i, &index);
        NcRemoveSlot(base, slot);
    }
    */
    NPSCleanup ();
    return fn;
}

void TPart::MReadPlatformFile (const char *fileName)
{
    FILE *f;
    uint8_t *buffer;
    struct stat st;
    newtRefVar pf;

    f = fopen (fileName, "rb");
    if (f != NULL) {
        printf ("Reading platform file %s\n", fileName);
        fstat (fileno (f), &st);
        buffer = (uint8_t *) malloc (st.st_size);
        fread (buffer, st.st_size, 1, f);
        pf = NewtReadNSOF (buffer, st.st_size);

        fConstants = NcGetSlot (pf, NewtMakeSymbol ("platformConstants"));
        fGlobalFunctions = NcGetSlot (pf, NewtMakeSymbol ("platformFunctions"));

        free (buffer);
        fclose (f);
    }
}

void TPart::MBuildPart (const char *platformFileName)
{
    newtRefVar packageNSOF;
    newtObjRef obj;
    char* data;
    int len;
    int i;

    NVMInit ();
    for (i = 0; i < fNumNativeModules; i++) {
        MLoadNativeModule (fNativeModules[i]);
    }
    for (i = 0; i < fNumFiles; i++) {
        MReadPlatformFile (platformFileName);
        MInterpretFile (fFiles[i]);
    }
    MReadPlatformFile (platformFileName);
    fMainForm = MInterpretFile (fMainFile);

    NVMClean ();
}
