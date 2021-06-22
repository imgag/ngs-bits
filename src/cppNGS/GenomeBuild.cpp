#include "GenomeBuild.h"

QString buildToString(GenomeBuild build, bool grch)
{
	if (grch)
	{
		return build==GenomeBuild::HG19 ? "GRCh37" : "GRCh38";
	}

	return build==GenomeBuild::HG19 ? "hg19" : "hg38";
}


GenomeBuild stringToBuild(QString build)
{
	build = build.toLower();

	if (build=="hg19" || build=="grch37") return GenomeBuild::HG19;
	if (build=="hg38" || build=="grch38") return GenomeBuild::HG38;

	THROW(ArgumentException, "Invalid genome build '" + build + " cannot be converted to GenomeBuild enum!");
}
