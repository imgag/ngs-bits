#include "FastaFileIndex.h"
#include "Exceptions.h"
#include "Helper.h"
#include "Log.h"
#include <QRegExp>
#include <QStringList>

using namespace std;

FastaFileIndex::FastaFileIndex(QString fasta_file)
	: fasta_name_(fasta_file)
	, index_name_(fasta_file + ".fai")
	, file_(fasta_file)
{
	//open FASTA file handle
	if (!file_.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		THROW(FileAccessException, "Could not open FASTA file '" + fasta_name_ + "' for reading!");
	}

	//load index file
	int linenum = 0;
	QSharedPointer<QFile> file = Helper::openFileForReading(index_name_);
	while(!file->atEnd())
	{
		++linenum;
		QList<QByteArray> fields = file->readLine().split('\t');
		if (fields.size()!=5)
		{
			THROW(FileParseException, "Malformed FASTA index line " + QString::number(linenum) + " in file '" + index_name_ + "'!");
		}

		FastaIndexEntry entry;
		entry.name = fields[0];
		entry.length = fields[1].toInt();
		entry.offset = fields[2].toLongLong();
		entry.line_blen = fields[3].toInt();
		entry.line_len = fields[4].toInt();
		QString name_norm = Chromosome(fields[0]).strNormalized(false);
		index_[name_norm] = entry;
	}

	//throw error upon empty FAI file
	if (index_.count()==0)
	{
		THROW(FileParseException, "Empty FAI file for " + fasta_file + "'!");
	}
}

FastaFileIndex::~FastaFileIndex()
{
	file_.close();
}

Sequence FastaFileIndex::seq(const Chromosome& chr, bool to_upper) const
{
	const FastaIndexEntry& entry = index(chr);

	//jump to postion
	if (!file_.seek(entry.offset))
	{
		THROW(FileAccessException, "QFile::seek did not work on " + fasta_name_ + "'!");
	}

	//read data
	int newlines_in_sequence = entry.length / entry.line_blen;
	int seqlen = newlines_in_sequence  + entry.length;
	Sequence output = file_.read(seqlen).replace("\n", 1, "", 0);

	//output
	if (to_upper) output = output.toUpper();
	return output;
}

Sequence FastaFileIndex::seq(const Chromosome& chr, int start, int length, bool to_upper) const
{
	//subtract 1 to make the coordinates 0-based
	start -= 1;

	//check if coords are valid
	const FastaIndexEntry& entry = index(chr);
	if((start+length) > entry.length)	Log::warn("Sequence length changed to chromosome end, was: " + chr.strNormalized(true) + ":" + QString::number(start+1) + "-" + QString::number(start+length));
	length = min(length, entry.length - start);
	if (start < 0 || length < 1)
	{
		Log::error("Negative start or start bigger than chromosome length, was: " + chr.strNormalized(true) + ":" + QString::number(start+1));
		return "";
	}

	//jump to postion
	int newlines_before = start > 0 ? (start - 1) / entry.line_blen : 0;
	if (!file_.seek(entry.offset + newlines_before + start))
	{
		THROW(FileAccessException, "QFile::seek did not work on " + fasta_name_ + "'!");
	}

	//read data
	int newlines_by_end = (start + length - 1) / entry.line_blen;
	int newlines_inside = newlines_by_end - newlines_before;
	int seqlen = length + newlines_inside;
	Sequence output = file_.read(seqlen).replace("\n", 1, "", 0);

	//output
	if (to_upper) output = output.toUpper();
	return output;
}

const FastaFileIndex::FastaIndexEntry& FastaFileIndex::index(const Chromosome& chr) const
{
	QMap<QString, FastaIndexEntry>::const_iterator it = index_.find(chr.strNormalized(false));
	if(it==index_.cend())
	{
		THROW(ArgumentException, "Unknown FASTA index chromosome '" + chr.strNormalized(false) + "' requested!");
	}

	return it.value();
}
