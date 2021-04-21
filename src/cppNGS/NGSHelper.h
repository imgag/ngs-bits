#ifndef NGSHELPER_H
#define NGSHELPER_H

#include "cppNGS_global.h"
#include "BamReader.h"
#include "FilterCascade.h"

//Helper datastructure for gene impringing info.
struct ImprintingInfo
{
	QByteArray source_allele;
	QByteArray status;
};

//Target region information.
struct TargetRegionInfo
{
	QString name;
	BedFile regions;
	GeneSet genes;

	//Returns if a target region is set
	bool isValid() const
	{
		return !regions.isEmpty();
	}

	//Clears all data
	void clear()
	{
		name.clear();
		regions.clear();
		genes.clear();
	}

};

///Helper class for NGS-specific stuff.
class CPPNGSSHARED_EXPORT NGSHelper
{
public:
	///Returns known SNPs and indels from gnomAD (AF>=1%, AN>=5000).
	static VcfFile getKnownVariants(QString build, bool only_snvs, const BedFile& roi, double min_af=0.0, double max_af=1.0);
	static VcfFile getKnownVariants(QString build, bool only_snvs, double min_af=0.0, double max_af=1.0);

	///Soft-clip alignment from the beginning or end (positions are 1-based)
	static void softClipAlignment(BamAlignment& al, int start_ref_pos, int end_ref_pos);

	///Create sample overview file
	static void createSampleOverview(QStringList in, QString out, int indel_window=100, bool cols_auto=true, QStringList cols = QStringList());

	///Expands a Amino acid notation with 1 letter to 3 letters
	static QByteArray expandAminoAcidAbbreviation(QChar amino_acid_change_in);

	///Returns the pseudoautomal regions on gnosomes.
	static const BedFile& pseudoAutosomalRegion(const QString& build);

	///Returns the cytogenetic band for to chromosomal position
	static QByteArray cytoBand(Chromosome chr, int pos);
	///Returns the chromosomal range of a cytoband or cytoband range.
	static BedLine cytoBandToRange(QByteArray cytoband);

	///Returns a map if imprinted genes and inherited allele.
	static const QMap<QByteArray, ImprintingInfo>& imprintingGenes();

	///Parses a chromosomal region from the given text. Throws an error, if the region is not valid.
	static void parseRegion(const QString& text, Chromosome& chr, int& start, int& end);

private:
	///Constructor declared away
	NGSHelper() = delete;
};

#endif // NGSHELPER_H
