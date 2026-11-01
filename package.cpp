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
// The Original Code is newtpackage.cpp.
//
// The Initial Developer of the Original Code is Eckhart Köppen.
// Copyright (C) 2004 the Initial Developer. All Rights Reserved.
//
// ***** END LICENSE BLOCK *****

// ANSI C & POSIX
#include <cstdio>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <ctype.h>

#ifdef _WIN32
#include <io.h>
#define strdup _strdup
#ifndef PATH_MAX
#define PATH_MAX _MAX_PATH
#endif
#else
#include <libgen.h>
#include <unistd.h>
#endif

// ISO C++
#include <stdexcept>

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

#include "package.h"
#include "part.h"

// Constants
const char *kPackageNameSuffix = ".pkg";
const char *kPackageCopyrightString = "All rights reserved";
const char *kDefaultName = "Test";
const char *kDefaultOutput = "test.pkg";
const char *kOptions = "m:o:n:p:";

static char* GetDirName(const char* path)
{
    if (path == NULL) return NULL;
    const char* lastSlash = strrchr(path, '/');
#ifdef _WIN32
    const char* lastBackslash = strrchr(path, '\\');
    if (lastBackslash != NULL && (lastSlash == NULL || lastBackslash > lastSlash)) {
        lastSlash = lastBackslash;
    }
#endif
    if (lastSlash == NULL) {
        return strdup(".");
    }
    if (lastSlash == path) {
        return strdup("/");
    }
    size_t len = lastSlash - path;
    char* dir = (char*) malloc(len + 1);
    strncpy(dir, path, len);
    dir[len] = '\0';
    return dir;
}

TPackage::TPackage (TPreferences* preferences):
    fPreferences(preferences),
    fPackageName (NULL),
    fOutputFileName (NULL),
    fPlatformFileName (NULL),
    fCopyright(NULL),
    fProjectDir(NULL),
    fPackageData (NULL),
    fPackageDataLen (0),
    fParts (NULL),
    fNumParts (0)
{
}

TPackage::~TPackage ()
{
    MReset ();
}

void TPackage::MReset ()
{
    free (fPackageName);
    free (fOutputFileName);
    free (fPlatformFileName);
    free (fPackageData);
    free (fCopyright);
    free (fProjectDir);

    fPackageName = NULL;
    fOutputFileName = NULL;
    fPlatformFileName = NULL;
    fPackageData = NULL;
    fCopyright = NULL;
    fProjectDir = NULL;
    fPackageDataLen = 0;
    while (fNumParts-- > 0) {
        delete fParts[fNumParts];
    }
    free (fParts);
    fParts = NULL;
    fNumParts = 0;
}

newtRef TPackage::MInterpretFile (const char *f)
{
    nps_syntax_node_t *stree = NULL;
    uint32_t numStree = 0;
    newtRefVar fn = kNewtRefNIL;
    newtErr err;
    struct stat st;

    if (f == NULL) {
        fprintf (stderr, "Error: no file specified\n");
        exit (1);
    }
    if (stat (f, &st) != 0) {
        fprintf (stderr, "Error: file %s not found\n", f);
        exit (1);
    }

    err = NPSParseFile (f, &stree, &numStree);
    if (err != kNErrNone) {
        fprintf (stderr, "Error parsing file %s\n", f);
        exit (1);
    }
    NBCInit ();
    fn = NBCGenBC (stree, numStree, true);
    NBCCleanup ();
    fn = NVMCall (fn, 0, &err);
    NPSCleanup ();
    return fn;
}

void TPackage::MBuildPackage ()
{
    int i;
    newtRef part;
    newtRef parts;
    newtRef package;
    newtRefVar packageData;
    unsigned int flags = 0x10000000;

    NVMInit ();
    MReadProjectFile ();
    printf ("Building package %s\n", fPackageName);

    for (i = 0; i < fNumParts; i++) {
        printf ("Building part %i\n", i);
        fParts[i]->MBuildPart (fPlatformFileName);
    }
    parts = NewtMakeArray(kNewtRefNIL, fNumParts);
    package = NcMakeFrame();
    for (i = 0; i < fNumParts; i++) {
        printf("Encoding Part %d %lx...\n", i, (unsigned long)fParts[i]->fMainForm);
        NewtPrintObject(stdout, fParts[i]->fMainForm);
        part = NcMakeFrame();
        NcSetSlot(part, NSSYM(class), NSSYM(PackagePart));
        NcSetSlot(part, NSSYM(info), NewtMakeBinary(NSSYM(binary), (uint8_t*)"A Newton Toolkit application", /*28*/24, false));
        NcSetSlot(part, NSSYM(flags), NewtMakeInt30(129));
        NcSetSlot(part, NSSYM(type), NewtMakeInt32(fParts[i]->fType));
        NcSetSlot(part, NSSYM(data), fParts[i]->fMainForm);
        NewtSetArraySlot(parts, i, part);
        if (fParts[i]->fRelocations != kNewtRefUnbind && fParts[i]->fRelocations != kNewtRefNIL) {
            NcSetSlot(package, NSSYM(relocations), fParts[i]->fRelocations);
            NcSetSlot(package, NSSYM(nativeBaseAddress), NewtMakeInteger(fParts[i]->fBaseAddress));
            flags |= 0x04000000;
        }
    }
    NcSetSlot(package, NSSYM(class), NSSYM(PackageHeader));
    NcSetSlot(package, NSSYM(type), NewtMakeInt32('xxxx'));
    NcSetSlot(package, NSSYM(pkg_version), NewtMakeInt32(flags & 0x04000000 ? 1 : 0));
    NcSetSlot(package, NSSYM(version), NewtMakeInt32(1));
    NcSetSlot(package, NSSYM(copyright), NewtMakeString(fCopyright, false));
    NcSetSlot(package, NSSYM(name), NewtMakeString(fPackageName, false));
    NcSetSlot(package, NSSYM(flags), NewtMakeInt32(flags));
    NcSetSlot(package, NSSYM(parts), parts);
    packageData = NsMakePkg(kNewtRefNIL, package);
    fPackageDataLen = NewtBinaryLength(packageData);
    fPackageData = NewtRefToBinary(packageData);

    NVMClean ();
}

void TPackage::MReadProjectFile ()
{
    newtRefVar project, p, copyright, parts;
    char* c;
    char* platformFileName = NULL;
    int i;

    MReset ();
    printf ("Reading project file %s\n", fPreferences->fProjectFileName);
    project = MInterpretFile (fPreferences->fProjectFileName);
    fProjectDir = GetDirName (fPreferences->fProjectFileName);
    p = NcGetSlot (project, NewtMakeSymbol ("platform"));
    if (p != kNewtRefNIL) {
        platformFileName = strdup (NewtRefToString (p));
        printf ("Reading platform file %s\n", platformFileName);
        fPlatformFileName = (char *) malloc(PATH_MAX);
        if (platformFileName[0] == '/'
#ifdef _WIN32
            || (isalpha((unsigned char)platformFileName[0]) && platformFileName[1] == ':') || platformFileName[0] == '\\'
#endif
        ) {
            struct stat s;
            if (stat (platformFileName, &s) == 0) {
                strncpy (fPlatformFileName, platformFileName, PATH_MAX);
                fPlatformFileName[PATH_MAX - 1] = '\0';
            } else {
                fprintf (stderr, "Platform file %s not found, aborting\n", platformFileName);
                free (platformFileName);
                exit (1);
            }
        } else {
            bool found = false;
            if (fPreferences->fPlatformFileSearchPaths != NULL) {
                for (char** path = fPreferences->fPlatformFileSearchPaths; *path != NULL; path++) {
                    struct stat s;
                    snprintf (fPlatformFileName, PATH_MAX, "%s/%s", *path, platformFileName);
                    if (stat (fPlatformFileName, &s) == 0) {
                        found = true;
                        break;
                    }
                }
            }
            if (!found && fProjectDir != NULL && strlen (fProjectDir) > 0) {
                struct stat s;
                snprintf (fPlatformFileName, PATH_MAX, "%s/%s", fProjectDir, platformFileName);
                if (stat (fPlatformFileName, &s) == 0) {
                    found = true;
                }
            }
            if (!found) {
                struct stat s;
                if (stat (platformFileName, &s) == 0) {
                    strncpy (fPlatformFileName, platformFileName, PATH_MAX);
                    fPlatformFileName[PATH_MAX - 1] = '\0';
                    found = true;
                }
            }
            if (!found) {
                fprintf (stderr, "Platform file %s not found, aborting\n", platformFileName);
                free (platformFileName);
                exit (1);
            }
        }
        free (platformFileName);
    }
    copyright = NcGetSlot (project, NewtMakeSymbol ("copyright"));
    if (NewtRefIsString (copyright)) {
        fCopyright = strdup (NewtRefToString (copyright));
    } else {
        fCopyright = strdup ("");
    }
    fPackageName = strdup (NewtRefToString (NcGetSlot (project, NewtMakeSymbol ("name"))));
    fOutputFileName = (char*) malloc (strlen (fPreferences->fProjectFileName) + sizeof (kPackageNameSuffix));
    strcpy (fOutputFileName, fPreferences->fProjectFileName);
    c = strrchr (fOutputFileName, '.');
    if (c != NULL) *c = 0;
    strcat (fOutputFileName, kPackageNameSuffix);
    parts = NcGetSlot (project, NewtMakeSymbol ("parts"));
    fNumParts = NewtArrayLength (parts);
    printf ("Parts: %d\n", fNumParts);
    fParts = (TPart **) malloc (fNumParts * sizeof (TPart *));
    for (i = 0; i < fNumParts; i++) {
        fParts[i] = new TPart (NewtGetArraySlot (parts, i), fProjectDir);
    }
}

void TPackage::MSavePackage ()
{
    FILE *f;
    
    f = fopen (fOutputFileName, "wb+");
    if (f == NULL) {
        fprintf (stderr, "Error: cannot open output file %s for writing\n", fOutputFileName);
        exit (1);
    }
    fwrite (fPackageData, fPackageDataLen, 1, f);
    fclose (f);
}

void TPackage::MDumpPackage ()
{
    FILE *f;
    struct stat st;
    newtRef r;

    if (fPreferences->fProjectFileName == NULL) {
        fprintf (stderr, "Error: no package file specified\n");
        exit (1);
    }
    if (stat (fPreferences->fProjectFileName, &st) != 0) {
        fprintf (stderr, "Error: package file %s not found\n", fPreferences->fProjectFileName);
        exit (1);
    }
    f = fopen (fPreferences->fProjectFileName, "rb");
    if (f == NULL) {
        fprintf (stderr, "Error: cannot open package file %s\n", fPreferences->fProjectFileName);
        exit (1);
    }
    fPackageDataLen = st.st_size;
    fPackageData = (unsigned char *) malloc (fPackageDataLen);
    if (fPackageData == NULL && fPackageDataLen > 0) {
        fprintf (stderr, "Error: out of memory reading %s\n", fPreferences->fProjectFileName);
        fclose (f);
        exit (1);
    }
    if (fread (fPackageData, fPackageDataLen, 1, f) != 1 && fPackageDataLen > 0) {
        fprintf (stderr, "Error reading package data from %s\n", fPreferences->fProjectFileName);
        fclose (f);
        free (fPackageData);
        exit (1);
    }
    fclose (f);

    NVMInit ();
    NcSetGlobalVar (NSSYM0 (printDepth), NewtMakeInt32(9999));
    NcSetGlobalVar (NSSYM0 (printLength), NewtMakeInt32(9999));
    r = NewtReadPkg (fPackageData, fPackageDataLen);
    NewtPrintObj (stdout, r);
    free (fPackageData);
    fPackageData = NULL;
}
