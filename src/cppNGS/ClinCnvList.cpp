#include "ClinCnvList.h"
#include "Exceptions.h"
#include "Helper.h"
#include "TSVFileStream.h"
#include "BasicStatistics.h"


ClinCnvVariant::ClinCnvVariant()
	: chr_()
	, start_(0)
	, end_(0)
	, copy_number_()
	, log_likelihoods_()
	, qvalues_()
	, genes_()
	, annotations_()
{
}

ClinCnvVariant::ClinCnvVariant(const Chromosome& chr, int start, int end, double copy_number, const QList<double>& log_likelihoods, const QList<double>& qvalues, GeneSet genes, QByteArrayList annotations)
	: chr_(chr)
	, start_(start)
	, end_(end)
	, copy_number_(copy_number)
	, log_likelihoods_(log_likelihoods)
	, qvalues_(qvalues)
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
	int i_sample = file.colIndex("sample", false); //optional
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
		if(file.header()[index] == "size") continue; //Skip size annotations (is calculated in GSvar already)
		annotation_headers_ << file.header()[index];
	}

	//parse input file
	QByteArray sample;
	while (!file.atEnd())
	{
		QByteArrayList parts = file.readLine();
		//if(parts.empty()) continue;
		if(i_sample != -1 &&parts.count()<7)
		{
			THROW(FileParseException, "Invalid ClinCNV file line: " + parts.join('\t'));
		}
		else if(i_sample == -1 && parts.count()<6) THROW(FileParseException,"Invalid ClinCNV file line: " + parts.join('\t'));

		//sample
		if (i_sample != -1 && !sample.isEmpty() && sample!=parts[i_sample])
		{
			THROW(FileParseException, "Multi-sample CNV files are currently not supported! Found data for " + sample + "'  and '" + parts[i_sample] + "'!");
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
			if(file.header()[index] == "size") continue;
			annos << parts[index];
		}

		QList<double> loglikelihoods;
		foreach(QByteArray part, parts[i_log_likelihood].split(','))
		{
			loglikelihoods << part.trimmed().toDouble();
		}

		QList<double> qvalues;
		int i_qvalues = file.colIndex("qvalue",false);
		if (i_qvalues!=-1)
		{
			foreach(QByteArray part, parts[i_qvalues].split(','))
			{
				qvalues << part.trimmed().toDouble();
			}
		}
		else //neccessary for backward compatibility
		{
			qvalues << std::numeric_limits<double>::quiet_NaN();
		}
		variants_.append(ClinCnvVariant(parts[i_chr], parts[i_start].toInt(), parts[i_end].toInt(), parts[i_copy_number].toDouble(), loglikelihoods, qvalues, genes, annos));
	}
}

int ClinCnvList::annotationIndexByName(const QByteArray& name, bool error_on_mismatch) const
{
	QList<int> matches;
	for(int i=0; i<annotation_headers_.count(); ++i)
	{
		if (annotation_headers_[i] == name )
		{
			matches.append(i);
		}
	}

	//Error handling
	if (matches.count()<1)
	{
		if (error_on_mismatch)
		{
			THROW(ArgumentException, "Could not find annotation column '" + name + "' in ClinCNV list!");
		}
		else
		{
			return -1;
		}
	}

	if (matches.count()>1)
	{
		if (error_on_mismatch)
		{
			THROW(ArgumentException, "Found multiple annotation columns for '" + name + "' in ClinCNV list!");
		}
		else
		{
			return -2;
		}
	}

	return matches.at(0);
}

void ClinCnvList::copyMetaData(const ClinCnvList& rhs)
{
	annotation_headers_ = rhs.annotation_headers_;
	comments_ = rhs.comments_;
}

ClinCnvAnalysisType ClinCnvList::type() const
{

	foreach(QByteArray comment,comments_)
	{
		if(comment.startsWith("##ANALYSISTYPE="))
		{
			QByteArray type = comment.trimmed().mid(15);

			if(type == "CLINCNV_GERMLINE_MULTI") return CLINCNV_GERMLINE_MULTI;
			else if(type == "CLINCNV_TUMOR_NORMAL_PAIR") return CLINCNV_TUMOR_NORMAL_PAIR;
			else if(type == "CLINCNV_GERMLINE_SINGLE") return CLINCNV_GERMLINE_SINGLE;
			else if(type == "CLINCNV_GERMLINE_TRIO") return CLINCNV_GERMLINE_TRIO;
		}
	}

	//old files do not contain sample header, try to determine type on columns
	foreach(QByteArray annotation_header,annotation_headers_)
	{
		if(annotation_header.contains("tumor_CN_change")) return CLINCNV_TUMOR_NORMAL_PAIR;
	}
	int i_sample = annotationIndexByName("sample",false);
	if(i_sample != -1)
	{
		foreach(ClinCnvVariant v,variants_)
		{
			if(v.annotations().at(i_sample).contains("multi")) return CLINCNV_GERMLINE_MULTI;
		}
	}

	//Fallback to CLINCNV_GERMLINE_SINGLE if could not be determined
	return CLINCNV_GERMLINE_SINGLE;

}
