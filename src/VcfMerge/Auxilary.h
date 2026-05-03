#ifndef AUXILARY_H
#define AUXILARY_H

#include "Chromosome.h"

struct VariantDefinition
{
	Chromosome chr;
	int pos;
	QByteArray ref;
	QByteArray alt;
	bool is_snv;

	QByteArray tag; //tag that identifies this variant

	bool operator<(const VariantDefinition& rhs) const
	{
		if (chr<rhs.chr) return true; //compare chromosome
		else if (chr>rhs.chr) return false;
		else if (pos<rhs.pos) return true; //compare start position
		else if (pos>rhs.pos) return false;
		else if (ref<rhs.ref) return true; //compare ref sequence
		else if (ref>rhs.ref) return false;
		else if (alt<rhs.alt) return true; //compare obs sequence
		else if (alt>rhs.alt) return false;
		return false;
	}
};

struct FormatData
{
	QByteArray gt = "0/0";
	QByteArray dp = ".";
	QByteArray af = ".";
	QByteArray gq = ".";
	QByteArray ps = "."; //Phase set
	QByteArray ct = "."; //Call type
};

struct VcfData
{
	//general data
	QByteArray filename; //filename without path
	QByteArray sample; //sample name
	QByteArray sample_desc; //megSAP-specific sample line '##SAMPLE=...'

	//hash from tag to FORMAT data
	QHash<QByteArray, FormatData> tag_to_format;

	//statistics data
	double chrx_het_perc = -1; //heteroyzgous chrX SNVs used to estimate the gender
	int c_snv = 0; //SNV count
	int c_indel = 0; //INDEL count
	int c_mosaic = 0; //mosaic variant count
	int c_low_mappability = 0; //low_mappability variant count
	int c_skipped_wt = 0; //variants skipped because they are wild-type
	int c_skipped_qual = 0; //variants skipped because they have too low quality
	int c_skipped_special = 0; //variants skipped because they are special calls
	int c_recall_no_wt = 0; //variants added during re-calling
};

#endif // AUXILARY_H
