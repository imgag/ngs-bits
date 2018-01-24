#include "CnvList.h"
#include "Exceptions.h"
#include "Helper.h"
#include "TSVFileStream.h"
#include "BasicStatistics.h"

CopyNumberVariant::CopyNumberVariant()
	: chr_()
	, start_(0)
	, end_(0)
	, regions_()
	, cns_()
	, z_scores_()
	, afs_()
	, genes_()
	, annotations_()
{
}

CopyNumberVariant::CopyNumberVariant(const Chromosome& chr, int start, int end, QByteArrayList regions, QList<int> cns, QList<double> z_scores, QList<double> afs, GeneSet genes, QByteArrayList annotations)
	: chr_(chr)
	, start_(start)
	, end_(end)
	, regions_(regions)
	, cns_(cns)
	, z_scores_(z_scores)
	, afs_(afs)
	, genes_(genes)
	, annotations_(annotations)
{
	if (regions_.count()!=cns_.count() || regions_.count()!=z_scores_.count() || regions_.count()!=afs_.count())
	{
		THROW(ProgrammingException, "Error while constructing copy-number variant. Region count mismatch for copy-numbers and/or z-scores!");
	}
}

CnvList::CnvList()
	: comments_()
	, variants_()
	, annotation_headers_()
{
}

void CnvList::clear()
{
	comments_.clear();
	variants_.clear();
	annotation_headers_.clear();
}

void CnvList::load(QString filename)
{
	//clear previous content
	clear();

	//parse file
	TSVFileStream file(filename);
	comments_ = file.comments();

	//handle column indices
	QVector<int> annotation_indices = BasicStatistics::range(file.header().count(), 0, 1);
	int i_chr = file.colIndex("chr", true);
	annotation_indices.removeAll(i_chr);
	int i_start = file.colIndex("start", true);
	annotation_indices.removeAll(i_start);
	int i_end = file.colIndex("end", true);
	annotation_indices.removeAll(i_end);
	int i_sample = file.colIndex("sample", true);
	annotation_indices.removeAll(i_sample);
	int i_cns = file.colIndex("region_copy_numbers", true);
	annotation_indices.removeAll(i_cns);
	int i_zscores = file.colIndex("region_zscores", true);
	annotation_indices.removeAll(i_zscores);
	int i_coords = file.colIndex("region_coordinates", true);
	annotation_indices.removeAll(i_coords);
	int i_afs = file.colIndex("region_cnv_af", false); //optinal - added in a later version
	annotation_indices.removeAll(i_afs);
	int i_genes = file.colIndex("genes", false); //optional
	annotation_indices.removeAll(i_genes);

	//parse annotation headers
	foreach(int index, annotation_indices)
	{
		annotation_headers_ << file.header()[index];
	}

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
		if (i_afs!=-1 && parts[i_afs]!="no_data")
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
		GeneSet genes;
		if (i_genes!=-1)
		{
			genes << GeneSet::createFromText(parts[i_genes], ',');
		}

		//parse annotation headers
		QByteArrayList annos;
		foreach(int index, annotation_indices)
		{
			annos << parts[index];
		}

		variants_.append(CopyNumberVariant(parts[i_chr], parts[i_start].toInt(), parts[i_end].toInt(), parts[i_coords].split(','), cns, zs, afs, genes, annos));
	}
}

void CnvList::copyMetaData(const CnvList& rhs)
{
	annotation_headers_ = rhs.annotation_headers_;
	comments_ = rhs.comments_;
}

