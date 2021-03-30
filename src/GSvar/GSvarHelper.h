#ifndef GSVARHELPER_H
#define GSVARHELPER_H

#include "GeneSet.h"
#include "BedFile.h"
#include "VariantList.h"
#include <QTableWidgetItem>

///Helper class for GSvar
class GSvarHelper
{
public:
	//Returns imprinting genes.
	static const GeneSet& impritingGenes();
	//Returns genes with no evidence of haploinsufficiency.
	static const GeneSet& hi0Genes();
	//Returns genes that have one or more pseudogenes.
	static const GeneSet& genesWithPseudogene();
	//Returns gene to preferred transcripts map
    static const QMap<QByteArray, QByteArrayList>& preferredTranscripts(bool reload=false);
    //Returns a list of special regions that are to be added to sub-panel designs for a given gene
	static const QMap<QByteArray, QList<BedLine>>& specialRegions();
	//Returns a list of matching regions for ENST transcripts (no version numbers!)
	static const QMap<QByteArray, QByteArrayList>& transcriptMatches();

	//Returns the application base name - path and filename
    static QString applicationBaseName();

	//colors imprinting and non-haploinsufficiency genes.
	static void colorGeneItem(QTableWidgetItem* item, const GeneSet& genes);

	//Lift-over from GRCh37 to GRCh38
	static BedLine liftOver(const Chromosome& chr, int start, int end);

	//Returns gnomAD link for a variant
	static QString gnomaADLink(const Variant& v);

protected:
	GSvarHelper() = delete;

	static GeneSet imprinting_genes_;
	static GeneSet hi0_genes_;
	static GeneSet pseudogene_genes_;
    static QMap<QByteArray, QByteArrayList> preferred_transcripts_;
    static QMap<QByteArray, QList<BedLine>> special_regions_;
	static QMap<QByteArray, QByteArrayList> transcript_matches_;
};

#endif // GSVARHELPER_H
