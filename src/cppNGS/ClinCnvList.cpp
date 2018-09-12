#include "ClinCnvList.h"
#include "Exceptions.h"
#include "Helper.h"
#include "TSVFileStream.h"
#include "BasicStatistics.h"


ClinCopyNumberVariant::ClinCopyNumberVariant()
	: chr_()
	, start_(0)
	, end_(0)
	, copy_number_()
	, log_likelihood_()
	, genes_()
	, annotations_()
{
}

ClinCopyNumberVariant::ClinCopyNumberVariant(const Chromosome& chr, int start, int end, double copy_number, double log_likelihood, GeneSet genes, QByteArrayList annotations)
	: chr_(chr)
	, start_(start)
	, end_(end)
	, copy_number_(copy_number)
	, log_likelihood_(log_likelihood)
	, genes_(genes)
	, annotations_(annotations)
{
}


ClinCnvList::ClinCnvList()
	: comments_()
	, variants_()
	, annotation_headers_()
{
}

void ClinCnvList::clear()
{
	comments_.clear();
	variants_.clear();
	annotation_headers_.clear();
}

void ClinCnvList::load(QString filename)
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
	int i_copy_number = file.colIndex("CN_change",true);
	annotation_indices.removeAll(i_copy_number);
	int i_log_likelihood = file.colIndex("loglikelihood",true);
	annotation_indices.removeAll(i_log_likelihood);
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
		if (parts.count()<7) THROW(FileParseException, "Invalid ClinCNV file line: " + parts.join('\t'));

		//sample
		if (!sample.isEmpty() && sample!=parts[i_sample])
		{
			THROW(FileParseException, "Multi-sample CNV files are currently not supported! Found data for " + sample + "'  and '" + parts[i_sample] + "'!");
		}
		sample = parts[i_sample];

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
		variants_.append(ClinCopyNumberVariant(parts[i_chr], parts[i_start].toInt(), parts[i_end].toInt(), parts[i_copy_number].toDouble(), parts[i_log_likelihood].toDouble(), genes, annos));
	}
}

void ClinCnvList::copyMetaData(const ClinCnvList& rhs)
{
	annotation_headers_ = rhs.annotation_headers_;
	comments_ = rhs.comments_;
}

