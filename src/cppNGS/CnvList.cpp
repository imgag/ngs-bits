#include "CnvList.h"
#include "Exceptions.h"
#include "Helper.h"
#include "TSVFileStream.h"

CopyNumberVariant::CopyNumberVariant()
	: chr_()
	, start_(0)
	, end_(0)
	, regions_()
	, cns_()
	, z_scores_()
	, afs_()
	, genes_()
{
}

CopyNumberVariant::CopyNumberVariant(const Chromosome& chr, int start, int end, QByteArrayList regions, QList<int> cns, QList<double> z_scores, QList<double> afs, QByteArrayList genes)
	: chr_(chr)
	, start_(start)
	, end_(end)
	, regions_(regions)
	, cns_(cns)
	, z_scores_(z_scores)
	, afs_(afs)
	, genes_(genes)
{
	if (regions_.count()!=cns_.count() || regions_.count()!=z_scores_.count() || regions_.count()!=afs_.count())
	{
		THROW(ProgrammingException, "Error while constructing copy-number variant. Region count mismatch for copy-numbers and/or z-scores!");
	}
}

CnvList::CnvList()
	: comments_()
	, variants_()
{
}

void CnvList::clear()
{
	comments_.clear();
	variants_.clear();
}

void CnvList::load(QString filename)
{
	//clear previous content
	clear();

	//get column indices
	TSVFileStream file(filename);
	comments_ = file.comments();
	int i_chr = file.colIndex("chr", true);
	int i_start = file.colIndex("start", true);
	int i_end = file.colIndex("end", true);
	int i_sample = file.colIndex("sample", true);
	int i_cns = file.colIndex("region_copy_numbers", true);
	int i_zscores = file.colIndex("region_zscores", true);
	int i_coords = file.colIndex("region_coordinates", true);
	int i_afs = file.colIndex("region_cnv_af", false);
	int i_genes = file.colIndex("genes", false);

	//parse input file
	QByteArray sample;
	while (!file.atEnd())
	{
		QByteArrayList parts = file.readLine();
		if (parts.count()<9) THROW(FileParseException, "Invalid CnvHunter file line: " + parts.join('\t'));

		QString line = parts.join('\t');

		//sample
		if (!sample.isEmpty() && sample!=parts[i_sample])
		{
			THROW(FileParseException, "Multi-sample CNV files are currently not supported! Found data for " + sample + "'  and '" + parts[3] + "'!");
		}
		sample = parts[i_sample];

		//copy-numbers
		QList<int> cns;
		QByteArrayList tmp = parts[i_cns].split(',');
		foreach(const QByteArray& t, tmp)
		{
			cns.append(t.toInt());
		}

		//z-scores
		QList<double> zs;
		tmp = parts[i_zscores].split(',');
		foreach(const QByteArray& t, tmp)
		{
			zs.append(Helper::toDouble(t, "z-score value", line));
		}

		//allele frequencies (optional)
		QList<double> afs;
		if (i_afs!=-1)
		{
			tmp = parts[i_afs].split(',');
			foreach(const QByteArray& t, tmp)
			{
				afs.append(Helper::toDouble(t, "allele frequency value", line));
			}
		}
		while(afs.count()<zs.count())
		{
			afs.append(0.0);
		}

		//genes (optional)
		if (i_genes!=-1)
		{
			tmp = parts[i_genes].split(',');
		}

		variants_.append(CopyNumberVariant(parts[i_chr], parts[i_start].toInt(), parts[i_end].toInt(), parts[i_coords].split(','), cns, zs, afs, tmp));
	}
}

