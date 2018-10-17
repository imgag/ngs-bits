#ifndef VCFFILE_H
#define VCFFILE_H

#include "cppNGS_global.h"

class CPPNGSSHARED_EXPORT VcfFile {
public:
    static const int CHROM  = 0;
    static const int POS    = 1;
    static const int ID     = 2;
    static const int REF    = 3;
    static const int ALT    = 4;
    static const int QUAL   = 5;
    static const int FILTER = 6;
    static const int INFO   = 7;
    static const int FORMAT = 8;
    static const int REQUIRED_LENGTH = FORMAT; // because we start from a 0 index
};

#endif // VCFFILE_H
