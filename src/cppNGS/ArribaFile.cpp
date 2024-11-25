#include "ArribaFile.h"


Fusion::Fusion():
  fully_initialized_(false)
{
}

Fusion::Fusion(GenomePosition breakpoint1, GenomePosition breakpoint2):
	breakpoint1_(breakpoint1)
  , breakpoint2_(breakpoint2)
  , fully_initialized_(false)
{
}

Fusion::Fusion(GenomePosition breakpoint1, GenomePosition breakpoint2, QString symbol1, QString transcript1, QString symbol2, QString transcript2, QString type, QString reading_frame, QStringList annotations):
	breakpoint1_(breakpoint1)
  , breakpoint2_(breakpoint2)
  , symbol1_(symbol1)
  , transcript1_(transcript1)
  , symbol2_(symbol2)
  , transcript2_(transcript2)
  , type_(type)
  , reading_frame_(reading_frame)
  , annotations_(annotations)
  , fully_initialized_(true)
{
}

QString Fusion::getAnnotation(int column_idx) const
{
	if (column_idx < 0 || column_idx >= annotations_.count()) THROW(ArgumentException, "Invalid index: " + QByteArray::number(column_idx) +". Annotation size is " + QByteArray::number(annotations_.count()));
	return annotations_[column_idx];
}

QString Fusion::toString() const
{
	return symbol1_ + " " + breakpoint1_.toByteArray() + "::" + symbol2_ + " " + breakpoint2_.toByteArray();
}


GenomePosition Fusion::breakpoint1() const
{
	return breakpoint1_;
}

GenomePosition Fusion::breakpoint2() const
{
	return breakpoint2_;
}

QString Fusion::symbol1() const
{
	return symbol1_;
}
QString Fusion::transcript1() const
{
	return transcript1_;
}
QString Fusion::symbol2() const
{
	return symbol2_;
}
QString Fusion::transcript2() const
{
	return transcript2_;
}
QString Fusion::type() const
{
	return type_;
}
QString Fusion::reading_frame() const
{
	return reading_frame_;
}

QStringList Fusion::annotations()
{
	return annotations_;
}

bool Fusion::fully_initialized() const
{
	return fully_initialized_;
}

ArribaFile::ArribaFile()
{
}

void ArribaFile::load(QString filename)
{
	TsvFile::load(filename);
}

Fusion ArribaFile::getFusion(int idx) const
{
	QStringList annotations = this->row(idx);

	int idx_breakpoint1 = this->columnIndex("breakpoint1");
	int idx_breakpoint2 = this->columnIndex("breakpoint2");
	int idx_gene1 = this->columnIndex("gene1");
	int idx_transcript1 = this->columnIndex("transcript_id1");
	int idx_gene2 = this->columnIndex("gene2");
	int idx_transcript2 = this->columnIndex("transcript_id2");
	int idx_type = this->columnIndex("type");
	int idx_reading_frame = this->columnIndex("reading_frame");

	GenomePosition breakpoint1(annotations[idx_breakpoint1]);
	GenomePosition breakpoint2(annotations[idx_breakpoint2]);

	return Fusion(breakpoint1, breakpoint2, annotations[idx_gene1], annotations[idx_transcript1], annotations[idx_gene2], annotations[idx_transcript2], annotations[idx_type], annotations[idx_reading_frame], annotations);
}

int ArribaFile::count() const
{
	return TsvFile::rowCount();
}

QByteArray ArribaFile::getCallerVersion() const
{
	foreach (const QString comment, comments())
	{
		if (comment.contains("##source"))
		{
			//##source=Arriba v2.4.0 (set in vc_arriba)
			return comment.split("=")[1].split(" ")[1].toUtf8();
		}
	}
	return "";
}

QByteArray ArribaFile::getCaller() const
{
	foreach (const QString comment, comments())
	{
		if (comment.contains("##source"))
		{
			//##source=Arriba v2.4.0 (set in vc_arriba)
			return comment.split("=")[1].split(" ")[0].toUtf8();
		}
	}
	return "";
}

QByteArray ArribaFile::getCallDate() const
{
	foreach (const QString comment, comments())
	{
		if (comment.contains("##AnalysisDate"))
		{
			//##AnalysisDate=2023-04-20 18:06:00
			return comment.split("=")[1].toUtf8();
		}
	}
	return "";
}

