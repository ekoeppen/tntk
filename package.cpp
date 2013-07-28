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
#include <libgen.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>

// ISO C++
#include <stdexcept>

// NEWT/0
#include <NewtCore.h>
#include <NewtVM.h>
#include <NewtParser.h>
#include <NewtNSOF.h>
#include <NewtBC.h>
#include <NewtGC.h>
#include <NewtPkg.h>
#include <NewtPrint.h>

#include "package.h"
#include "part.h"

// Constants
const char *kPackageNameSuffix = ".pkg";
const char *kPackageCopyrightString = "All rights reserved";
const char *kDefaultName = "Test";
const char *kDefaultOutput = "test.pkg";
const char *kOptions = "m:o:n:p:";

TPackage::TPackage (const char *projectFileName):
    fPackageName (NULL),
    fOutputFileName (NULL),
    fPlatformFileName (NULL),
    fProjectFileName (NULL),
    fPackageData (NULL),
    fPackageDataLen (0),
    fParts (NULL),
    fNumParts (0)
{
    fProjectFileName = strdup (projectFileName);
}

TPackage::~TPackage ()
{
    MReset ();
    delete fProjectFileName;
}

void TPackage::MReset ()
{
    delete fPackageName;
    delete fOutputFileName;
    delete fPlatformFileName;
    delete fPackageData;

    fPackageName = NULL;
    fOutputFileName = NULL;
    fPlatformFileName = NULL;
    fPackageData = NULL;
    fPackageDataLen = 0;
    while (fNumParts-- > 0) {
        delete fParts[fNumParts];
    }
    delete fParts;
    fParts = NULL;
    fNumParts = 0;
}

newtRef TPackage::MInterpretFile (const char *f)
{
    nps_syntax_node_t *stree = NULL;
    uint32_t numStree = 0;
    newtRefVar fn = kNewtRefNIL;
    newtErr err;
    newtRefVar map, s, v;
    int i;

    err = NPSParseFile (f, &stree, &numStree);
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

    NVMInit ();
    MReadProjectFile ();
    printf ("Building package %s\n", fPackageName);

    for (i = 0; i < fNumParts; i++) {
        printf ("Building part %i\n", i);
        fParts[i]->MBuildPart (fPlatformFileName);
    }
    parts = NewtMakeArray(kNewtRefNIL, fNumParts);
    for (i = 0; i < fNumParts; i++) {
        part = NcMakeFrame();
        NcSetSlot(part, NSSYM(class), NSSYM(PackagePart));
        NcSetSlot(part, NSSYM(info), NewtMakeBinary(NSSYM(binary), (uint8_t*)"A Newton Toolkit application", /*28*/24, false));
        NcSetSlot(part, NSSYM(flags), NewtMakeInt30(129));
        NcSetSlot(part, NSSYM(type), NewtMakeInt32('form'));
        NcSetSlot(part, NSSYM(data), fParts[i]->fMainForm);
        NewtSetArraySlot(parts, i, part);
    }
    package = NcMakeFrame();
    NcSetSlot(package, NSSYM(class), NSSYM(PackageHeader));
    NcSetSlot(package, NSSYM(type), NewtMakeInt32('xxxx'));
    NcSetSlot(package, NSSYM(pkg_version), NewtMakeInt32(0));
    NcSetSlot(package, NSSYM(version), NewtMakeInt32(1));
    NcSetSlot(package, NSSYM(copyright), NewtMakeString("Eckhart Koeppen", false));
    NcSetSlot(package, NSSYM(name), NewtMakeString(fPackageName, false));
    NcSetSlot(package, NSSYM(flags), NewtMakeInt32(0x10000000));
    NcSetSlot(package, NSSYM(parts), parts);
    packageData = NsMakePkg(kNewtRefNIL, package);
    fPackageDataLen = NewtBinaryLength(packageData);
    fPackageData = NewtRefToBinary(packageData);

    NVMClean ();
}

void TPackage::MReadProjectFile ()
{
    newtRefVar project, p, parts;
    char* c;
    int i, n;

    MReset ();
    printf ("Reading project file %s\n", fProjectFileName);
    project = MInterpretFile (fProjectFileName);
    p = NcGetSlot (project, NewtMakeSymbol ("platform"));
    if (p != kNewtRefNIL) {
        fPlatformFileName = strdup (NewtRefToString (p));
    }
    fPackageName = strdup (NewtRefToString (NcGetSlot (project, NewtMakeSymbol ("name"))));
    fOutputFileName = (char*) malloc (strlen (fProjectFileName) + sizeof (kPackageNameSuffix));
    strcpy (fOutputFileName, fProjectFileName);
    c = strrchr (fOutputFileName, '.');
    if (c != NULL) *c = 0;
    strcat (fOutputFileName, kPackageNameSuffix);
    parts = NcGetSlot (project, NewtMakeSymbol ("parts"));
    fNumParts = NewtArrayLength (parts);
    printf ("Parts: %d\n", fNumParts);
    fParts = (TPart **) malloc (fNumParts * sizeof (TPart *));
    for (i = 0; i < fNumParts; i++) {
        fParts[i] = new TPart (NewtGetArraySlot (parts, i));
    }
}

void TPackage::MSavePackage ()
{
    FILE *f;

    f = fopen (fOutputFileName, "wb+");
    fwrite (fPackageData, fPackageDataLen, 1, f);
    fclose (f);
}
