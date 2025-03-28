#ifndef GSVARHELPER_H
#define GSVARHELPER_H

#include "GeneSet.h"
#include "BedFile.h"
#include "VariantList.h"
#include "GenomeBuild.h"
#include "NGSD.h"
#include "TsvFile.h"
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
	//Returns gene to preferred transcripts map (note: transcript names do not contain version numbers)
    static const QMap<QByteArray, QByteArrayList>& preferredTranscripts(bool reload=false);
    //Returns a list of special regions that are to be added to sub-panel designs for a given gene
	static const QMap<QByteArray, QList<BedLine>>& specialRegions();

	//Returns the application base name - path and filename
	static QString applicationBaseName();
	//Returns the genome build used by GSvar.
	static GenomeBuild build();

	//colors imprinting and non-haploinsufficiency genes.
	static void colorGeneItem(QTableWidgetItem* item, const GeneSet& genes);
	//colors QC metric item background. Returns if the item was assigned a background color.
	static bool colorQcItem(QTableWidgetItem* item, const QString& accession, const QString& sys_type, const QString& gender);

	//limit QLabel to certain number of lines
	static void limitLines(QLabel* label, QString text, int max_lines=15);

	//Lift-over region from GRCh37 to GRCh38 (or the other way). Throws ArgumentException if conversion not possible.
	static BedLine liftOver(const Chromosome& chr, int start, int end, bool hg19_to_hg38);
	//Lift-over variant from GRCh37 to GRCh38 (or the other way). Throws ArgumentException if conversion not possible.
	static Variant liftOverVariant(const Variant& v, bool hg19_to_hg38);

	//Returns gnomAD link for a variant
	static QString gnomADLink(const Variant& v, bool open_in_v4=true);
	//Returns All of Us link for a variant
	static QString allOfUsLink(const Variant& v);
	//Returns ClinVar search link for a variant
	static QString clinVarSearchLink(const Variant& v, GenomeBuild build);

	///Returns a the local target region folder where tempory target regions and gene lists can be stored for IGV.
	static QString localRoiFolder();
	///Returns a the local log folder where tempory log files can be stored for opening in a text editor.
	static QString localLogFolder();
	///Returns a the local log folder where tempory qcML files can be stored for opening in the browser.
	static QString localQcFolder();

	//Queue the analysis of samples
	static bool queueSampleAnalysis(AnalysisType type, const QList<AnalysisJobSample>& samples, QWidget* parent = 0);

	//returns a warning message if genes with 'indikationsspezifische Abrechnung' are contained
	static QString specialGenes(const GeneSet& genes);

	//Returns a table struct containing all related cfDNA variant info for a given tumor sample
	static CfdnaDiseaseCourseTable cfdnaTable(const QString& tumor_ps_name, QStringList& errors, bool throw_if_fails=true);

	//Returns the coding and splicing entry (and genes through GeneSet reference) for a given variant
	static QList<QStringList> annotateCodingAndSplicing(const VcfLine& variant, GeneSet& genes, bool add_flags=true, int offset=5000);

    //Returns a path for the settings file (Windows or Unix format), by substituting a placeholder with the current app path
    static QString appPathForTemplate(QString path);

protected:
	GSvarHelper() = delete;
};

#endif // GSVARHELPER_H
