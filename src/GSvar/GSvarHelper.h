#ifndef GSVARHELPER_H
#define GSVARHELPER_H

#include "GeneSet.h"

///Helper class for GSvar
class GSvarHelper
{
public:
	//Returns imprinting genes.
	static const GeneSet& impritingGenes();

	//Returns gene to preferred transcripts map
	static const QMap<QString, QStringList>& preferredTranscripts();

protected:
	GSvarHelper() = delete;

	static GeneSet imprinting_genes_;
	static QMap<QString, QStringList> preferred_transcripts_;
};

#endif // GSVARHELPER_H
