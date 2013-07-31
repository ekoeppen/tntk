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
// The Initial Developer of the Original Code is Eckhart Kšppen.
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

TPart::TPart (newtRefVar partInfo):
	fType (kForm),
	fConstants (kNewtRefNIL),
	fGlobalFunctions (kNewtRefNIL),
	fFiles (NULL),
	fNumFiles (0)
{
	newtRefVar files, map;
	int i;

	fMainFile = strdup (NewtRefToString (NcGetSlot (partInfo, NewtMakeSymbol ("main"))));
	files = NcGetSlot (partInfo, NewtMakeSymbol ("files"));
	if (files != kNewtRefNIL) {
		fNumFiles = NewtArrayLength (files);
		fFiles = (char **) malloc (fNumFiles * sizeof (char *));
		for (i = 0; i < fNumFiles; i++) {
			fFiles[i] = strdup (NewtRefToString (NewtGetArraySlot (files, i)));
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
	delete fFiles;
	delete fMainFile;
}

newtRef TPart::MInterpretFile (const char *f)
{
	nps_syntax_node_t *stree = NULL;
	uint32_t numStree = 0;
	newtRefVar fn = kNewtRefNIL;
	newtRefVar constants;
	newtErr err;
	newtRefVar map, s, v;
	newtRefVar theBase = kNewtRefNIL;
	int i;

	printf ("Compiling %s\n", f);
	err = NPSParseFile (f, &stree, &numStree);

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

	fn = NBCGenBC (stree, numStree, true);
	NBCCleanup ();
	fn = NVMCall (fn, 0, &err);
	theBase = NcGetGlobalVar(NSSYM(theBase));
	newtObjRef obj = NewtRefToPointer(theBase);
	uint32_t index, len = NewtObjSlotsLength(obj);
	for (i=len-1; i>2; i--) {
	    newtRefVar slot = NewtGetMapIndex(obj->as.map, i, &index);
	    // printf("Slot %d = <%s>\n", i, NewtRefToSymbol(slot)->name);
	    NcRemoveSlot(theBase, slot);
	}
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
	for (i = 0; i < fNumFiles; i++) {
		MReadPlatformFile (platformFileName);
		MInterpretFile (fFiles[i]);
	}
	MReadPlatformFile (platformFileName);
	fMainForm = MInterpretFile (fMainFile);

	NVMClean ();
}
