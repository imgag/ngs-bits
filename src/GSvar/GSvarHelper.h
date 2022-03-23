#ifndef GSVARHELPER_H
#define GSVARHELPER_H

#include "GeneSet.h"
#include "BedFile.h"
#include "VariantList.h"
#include "GenomeBuild.h"
#include "NGSD.h"
#include <QTableWidgetItem>
#include <QLabel>


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
	//Returns the genome build used by GSvar.
	static GenomeBuild build();

	//colors imprinting and non-haploinsufficiency genes.
	static void colorGeneItem(QTableWidgetItem* item, const GeneSet& genes);
	//limit QLabel to certain number of lines
	static void limitLines(QLabel* label, QString text, QString sep="\n", int max_lines=15);

	//Lift-over region from GRCh37 to GRCh38 (or the other way). Throws ArgumentException if conversion not possible.
	static BedLine liftOver(const Chromosome& chr, int start, int end, bool hg19_to_hg38 = true);
	//Lift-over variant from GRCh37 to GRCh38 (or the other way). Throws ArgumentException if conversion not possible.
	static Variant liftOverVariant(const Variant& v, bool hg19_to_hg38 = true);

	//Returns gnomAD link for a variant
	static QString gnomADLink(Variant v, GenomeBuild build);

	///Returns a the local target region folder where tempory target regions and gene lists can be stored for IGV.
	static QString localRoiFolder();

	//Queue the analysis of samples
	static bool queueSampleAnalysis(AnalysisType type, const QList<AnalysisJobSample>& samples, QWidget* parent = 0);

	//returns if the change of MaxEntScan is large enough to color it in the VariantTable, also provides percent- and abs-changes of MaxEntScan.
	static bool colorMaxEntScan(QString anno, QList<double>& percentages, QList<double>& absValues);

protected:
	GSvarHelper() = delete;
};

#endif // GSVARHELPER_H
