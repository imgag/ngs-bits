#ifndef NGSHELPER_H
#define NGSHELPER_H

#include "cppNGS_global.h"
#include "BamReader.h"

///Sample header struct for samples in variant lists (GSvar).
struct CPPNGSSHARED_EXPORT SampleInfo
{
	QString column_name;
	QMap<QString, QString> properties;

	///Returns if the sample has state 'affected'.
	bool isAffected() const;
};

///Sample header information from GSvar files.
class CPPNGSSHARED_EXPORT SampleHeaderInfo
	: public QMap<QString, SampleInfo>
{
	public:
		///Returns all sample genotype column names of all samples.
		 QStringList sampleColumns() const;
		///Returns all sample genotype column names of affected/unaffected samples.
		QStringList sampleColumns(bool affected) const;
};

///Helper class for NGS-specific stuff.
class CPPNGSSHARED_EXPORT NGSHelper
{
public:
	///Returns known SNV and indels of the GRCh37 genome (AF>=1%).
	static VariantList getKnownVariants(bool only_snvs, double min_af=0.0, double max_af=1.0, const BedFile* roi=nullptr);

	///Returns the complement of a base
	static char complement(char base);
	///Changes a DNA sequence to reverse/complementary order.
	static QByteArray changeSeq(const QByteArray& seq, bool rev, bool comp);

	///Soft-clip alignment from the beginning or end (positions are 1-based)
	static void softClipAlignment(BamAlignment& al, int start_ref_pos, int end_ref_pos);

	///Create sample overview file
	static void createSampleOverview(QStringList in, QString out, int indel_window=100, bool cols_auto=true, QStringList cols = QStringList());

	///Parses and returns sample data from variant list header. The @p filename argument is needed to support old germline variant lists without sample header.
	static SampleHeaderInfo getSampleHeader(const VariantList& vl, QString gsvar_file);

	///Expands a Amino acid notation with 1 letter to 3 letters
	static QByteArray expandAminoAcidAbbreviation(QChar amino_acid_change_in);


private:
	///Constructor declared away
	NGSHelper() = delete;
};

#endif // NGSHELPER_H
