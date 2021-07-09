#ifndef GENOMEBUILD_H
#define GENOMEBUILD_H

#include "cppNGS_global.h"
#include "Exceptions.h"
#include <QString>

//Genome build
enum class GenomeBuild
{
	HG19,
	HG38
};

//Convert genome build to string.
QString CPPNGSSHARED_EXPORT buildToString(GenomeBuild build, bool grch=false);

//Convert string to genome build. If invalid build, a ArgumentException is thrown.
GenomeBuild CPPNGSSHARED_EXPORT stringToBuild(QString build);


#endif // GENOMEBUILD_H
