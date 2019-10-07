#ifndef GSVARHELPER_H
#define GSVARHELPER_H

#include "GeneSet.h"
#include "BedFile.h"

///Helper class for GSvar
class GSvarHelper
{
public:
	//Returns imprinting genes.
	static const GeneSet& impritingGenes();
	//Returns gene to preferred transcripts map
    static const QMap<QByteArray, QByteArrayList>& preferredTranscripts(bool reload=false);
    //Returns a list of special regions that are to be added to sub-panel designs for a given gene
    static const QMap<QByteArray, QList<BedLine> > &specialRegions();

    //Resturns the application base name - path and filename
    static QString applicationBaseName();

protected:
	GSvarHelper() = delete;

	static GeneSet imprinting_genes_;
    static QMap<QByteArray, QByteArrayList> preferred_transcripts_;
    static QMap<QByteArray, QList<BedLine>> special_regions_;
};

#endif // GSVARHELPER_H
