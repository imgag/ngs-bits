#include "SvList.h"
#include "TSVFileStream.h"
#include "BasicStatistics.h"
#include "Log.h"

StructuralVariant::StructuralVariant()
	: type_()
	, chr_()
	, start_()
	, end_()
	, score_()
	, filter_()
	, mate_chr_()
	, mate_pos_()
	, mate_filter_()
	, annotations_()
{
}

StructuralVariant::StructuralVariant(const QByteArray type,const Chromosome& chr,int start,int end,int score, const QByteArray& filter, const Chromosome& mate_chr,int mate_pos,const QByteArray& mate_filter,const QByteArrayList& annotations)
	: type_(type)
	, chr_(chr)
	, start_(start)
	, end_(end)
	, score_(score)
	, filter_(filter)
	, mate_chr_(mate_chr)
	, mate_pos_(mate_pos)
	, mate_filter_(mate_filter)
	, annotations_(annotations)
{
}


SvList::SvList()
	: variants_()
	, annotation_headers_()
	, comments_()
{
}

void SvList::clear()
{
	variants_.clear();
	annotation_headers_.clear();
	comments_.clear();
}

void SvList::load(QString filename)
{
	//remove old content
	clear();

	TSVFileStream file(filename);
	comments_ = file.comments();

	QVector<int> annotation_indices = BasicStatistics::range(file.header().count(),0,1);


	//determine indices of each column which has an entry in StructuralVariant class
	int i_chr = file.colIndex("chr",true);
	annotation_indices.removeAll(i_chr);
	int i_start = file.colIndex("start",true);
	annotation_indices.removeAll(i_start);
	int i_end = file.colIndex("end",true);
	annotation_indices.removeAll(i_end);
	int i_score = file.colIndex("score",true);
	annotation_indices.removeAll(i_score);
	int i_filter = file.colIndex("filter",true);
	annotation_indices.removeAll(i_filter);
	int i_type = file.colIndex("type",true);
	annotation_indices.removeAll(i_type);
	int i_mate_chr = file.colIndex("mate_chr",true);
	annotation_indices.removeAll(i_mate_chr);
	int i_mate_pos = file.colIndex("mate_pos",true);
	annotation_indices.removeAll(i_mate_pos);
	int i_mate_filter = file.colIndex("mate_filter",true);
	annotation_indices.removeAll(i_mate_filter);

	//remaining annotations
	foreach(int index,annotation_indices)
	{
		annotation_headers_ << file.header()[index];
	}

	while(!file.atEnd())
	{
		QByteArrayList parts = file.readLine();

		QByteArrayList annotations;
		foreach(int i,annotation_indices)
		{
			annotations << parts[i];
		}

		if(parts[i_end] == ".")
		{
			parts[i_end] = "-1";
		}
		if(parts[i_mate_pos] == ".")
		{
			parts[i_mate_pos] = "-1";
		}

		variants_.append(StructuralVariant(parts[i_type],parts[i_chr],parts[i_start].toInt(),parts[i_end].toInt(),parts[i_score].toInt(),parts[i_filter],parts[i_mate_chr],parts[i_mate_pos].toInt(),parts[i_mate_filter],annotations));
	}

}

int SvList::annotationIndexByName(const QByteArray &name, bool error_on_mismatch)
{

	QList<int> matches = {};

	for(int i=0;i<annotation_headers_.count();i++)
	{
		if(annotation_headers_[i] == name)
		{
			matches.append(i);
		}
	}

	if(matches.count() > 1)
	{
		if(error_on_mismatch)
		{
			THROW(ArgumentException, "Found multiple column annotations for '" + name + "' in " + "structural variant list!");
		}
		else
		{
			Log::warn("Found multiple columns for '" + name + "' in structural variant list!");
			return -2;
		}
	}


	if(matches.count() < 1)
	{
		if(error_on_mismatch)
		{
			THROW(ArgumentException, "Could not find column '" + name + "' in structural variant list!");
		}
		else
		{
			return -1;
		}
	}

	return matches.at(0);
}

void SvList::checkValid() const
{
	foreach(const StructuralVariant& variant, variants_)
	{
		if (!variant.chr().isValid())
		{
			THROW(ArgumentException, "Invalid variant chromosome string in structural variant '" + variant.chr().str());
		}

		if (variant.start()<1 || (variant.end()<1 && variant.end() >= 0) || (variant.start()>variant.end() && variant.end()>=0) )
		{
			THROW(ArgumentException, "Invalid variant position range in structural variant '" + variant.chr().str() + ":" + QString::number(variant.start()) + "-" + QString::number(variant.end()) );
		}

		if (variant.annotations().count()!=annotation_headers_.count())
		{
			THROW(ArgumentException, "Invalid structural variant annotation data: Expected " + QString::number(annotation_headers_.count()) + " values, but " + QString::number(variant.annotations().count()) + " values found");
		}
	}
}

