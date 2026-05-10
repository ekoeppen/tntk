/* Getopt for Microsoft C.
   This code is public domain and provided as-is.
*/

#include <string.h>
#include <stdio.h>
#include "getopt.h"

char *optarg = NULL;
int optind = 1;
int opterr = 1;
int optopt = '?';

int getopt(int argc, char *const argv[], const char *optstring) {
    static char *nextchar = NULL;

    if (optind >= argc || argv[optind] == NULL || argv[optind][0] != '-' || argv[optind][1] == '\0') {
        return -1;
    }

    if (nextchar == NULL || *nextchar == '\0') {
        nextchar = &argv[optind][1];
    }

    char c = *nextchar++;
    char *p = strchr(optstring, c);

    if (p == NULL || c == ':') {
        if (opterr && *optstring != ':') {
            fprintf(stderr, "%s: illegal option -- %c\n", argv[0], c);
        }
        optopt = c;
        if (*nextchar == '\0') optind++;
        return '?';
    }

    if (p[1] == ':') {
        if (*nextchar != '\0') {
            optarg = nextchar;
            optind++;
        } else if (++optind >= argc) {
            if (opterr && *optstring != ':') {
                fprintf(stderr, "%s: option requires an argument -- %c\n", argv[0], c);
            }
            optopt = c;
            nextchar = NULL;
            return (*optstring == ':') ? ':' : '?';
        } else {
            optarg = argv[optind];
            optind++;
        }
        nextchar = NULL;
    } else {
        if (*nextchar == '\0') {
            optind++;
            nextchar = NULL;
        }
        optarg = NULL;
    }

    return c;
}
