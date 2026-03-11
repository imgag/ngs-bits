#include "FastaFileIndex.h"
#include "Exceptions.h"
#include "Helper.h"
#include "Log.h"
#include <QNetworkProxy>
#include "HttpRequestHandler.h"

using namespace std;

FastaFileIndex::FastaFileIndex(QString fasta_file)
	: fasta_name_(fasta_file)
	, index_name_(fasta_file + ".fai")
	, file_(fasta_file)
{
    if (Helper::isHttpUrl(fasta_name_)) THROW(NotImplementedException, "FastaFileIndex does not support HTTP/HTTPS!");

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
        saveEntryToIndex(fields);
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
	Sequence output;

    output = file_.read(seqlen).replace('\n', "");

	//output
	if (to_upper) output = output.toUpper();
	return output;
}

Sequence FastaFileIndex::seq(const Chromosome& chr, int start, int length, bool to_upper) const
{
	//subtract 1 to make the coordinates 0-based
	start -= 1;
	if (start < 0)
	{
		THROW(ProgrammingException, "FastaFileIndex::seq: Invalid start position (" + QString::number(start) + ") for " + chr.strNormalized(true) + ":" + QString::number(start+1) + "-" + QString::number(start+length));
	}
	if (length < 0)
	{
		THROW(ProgrammingException, "FastaFileIndex::seq: Invalid length (" + QString::number(length) + ") for " + chr.strNormalized(true) + ":" + QString::number(start+1) + "-" + QString::number(start+length));
	}
	const FastaIndexEntry& entry = index(chr);
	if (start > entry.length)
	{
		THROW(ProgrammingException, "FastaFileIndex::seq: Invalid start position " + chr.strNormalized(true) + ":" + QString::number(start) + " after chromosome end (" + QString::number(entry.length) + ")");
	}

	//restrict to chromosome length
	if((start+length) > entry.length)
	{
		Log::warn("FastaFileIndex::seq: Sequence length changed to chromosome end for: " + chr.strNormalized(true) + ":" + QString::number(start+1) + "-" + QString::number(start+length));
		length = min(length, entry.length - start);
	}

	//jump to postion
	int newlines_before = start > 0 ? (start - 1) / entry.line_blen : 0;
	qint64 read_start_pos = entry.offset + newlines_before + start;
    if (!file_.seek(read_start_pos))
    {
        THROW(FileAccessException, "QFile::seek did not work on " + fasta_name_ + "'!");
    }

	//read data
	int newlines_by_end = (start + length - 1) / entry.line_blen;
	int newlines_inside = newlines_by_end - newlines_before;
	int seqlen = length + newlines_inside;
	Sequence output {};

    output = file_.read(seqlen).replace('\n', "");

	//output
	if (to_upper) output = output.toUpper();
	return output;
}

int FastaFileIndex::n(const Chromosome& chr) const
{
	if (!n_.contains(chr))
	{
		int output = 0;
		Sequence sequence = seq(chr, false);
		for (int i=0; i<sequence.length(); ++i)
		{
			if (sequence[i]=='N' || sequence[i]=='n') ++output;
		}
		n_[chr] = output;
	}

	return n_[chr];
}

const FastaFileIndex::FastaIndexEntry& FastaFileIndex::index(const Chromosome& chr) const
{
	QHash<Chromosome, FastaIndexEntry>::const_iterator it = index_.find(chr);
	if(it==index_.cend())
	{
		THROW(ArgumentException, "Unknown FASTA index chromosome '" + chr.str() + "' requested!");
	}
	return it.value();
}

void FastaFileIndex::saveEntryToIndex(const QList<QByteArray>& fields)
{
	FastaIndexEntry entry;
	entry.name = fields[0];
	entry.length = fields[1].toInt();
	entry.offset = fields[2].toLongLong();
	entry.line_blen = fields[3].toInt();
	entry.line_len = fields[4].toInt();
	QString name_norm = Chromosome(fields[0]).strNormalized(true);
	index_[name_norm] = entry;
	chrs_ << name_norm;
}
