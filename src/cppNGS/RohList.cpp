#include "RohList.h"
#include "Exceptions.h"
#include "TSVFileStream.h"
#include "BasicStatistics.h"

RunOfHomozygosity::RunOfHomozygosity()
	: chr_()
	, start_(0)
	, end_(0)
	, markers_(-1)
	, markers_het_(-1)
	, q_score_(-1.0)
	, genes_()
	, annotations_()
{
}

RunOfHomozygosity::RunOfHomozygosity(const Chromosome& chr, int start, int end, int markers, int markers_het, double q_score, GeneSet genes, QByteArrayList annotations)
	: chr_(chr)
	, start_(start)
	, end_(end)
	, markers_(markers)
	, markers_het_(markers_het)
	, q_score_(q_score)
	, genes_(genes)
	, annotations_(annotations)
{
}

RohList::RohList()
	: comments_()
	, rohs_()
	, annotation_headers_()
{
}

void RohList::clear()
{
	comments_.clear();
	rohs_.clear();
	annotation_headers_.clear();
}

void RohList::load(QString filename)
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
	int i_markers = file.colIndex("number of markers", true);
	annotation_indices.removeAll(i_markers);
	int i_markers_het = file.colIndex("het markers", true);
	annotation_indices.removeAll(i_markers_het);
	int i_size_kb = file.colIndex("size [Kb]", true);
	annotation_indices.removeAll(i_size_kb);
	int i_q_score = file.colIndex("Q score", true);
	annotation_indices.removeAll(i_q_score);
	int i_genes = file.colIndex("genes", false); //optional
	annotation_indices.removeAll(i_genes);

	//parse annotation headers
	foreach(int index, annotation_indices)
	{
		annotation_headers_ << file.header()[index];
	}

	//parse input file
	while (!file.atEnd())
	{
		QByteArrayList parts = file.readLine();
		if (parts.count()<6) THROW(FileParseException, "Invalid RohHunter file line: " + parts.join('\t'));

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

		rohs_.append(RunOfHomozygosity(parts[i_chr], parts[i_start].toInt(), parts[i_end].toInt(), parts[i_markers].toInt(), parts[i_markers_het].toInt(), parts[i_q_score].toDouble(), genes, annos));
	}
}

