#include "FastaFileIndex.h"
#include "Exceptions.h"
#include "Helper.h"
#include "Log.h"
#include <QNetworkProxy>

using namespace std;

FastaFileIndex::FastaFileIndex(QString fasta_file)
	: fasta_name_(fasta_file)
	, index_name_(fasta_file + ".fai")
	, file_(fasta_file)
{
    if (Helper::isHttpUrl(fasta_name_)) THROW(NotImplementedException, "FastaFileIndex does not support HTTP/HTTPS!");

    //open FASTA file handle
	if (!file_.open(QIODevice::ReadOnly))
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

	if (!file_.seek(entry.offset))
	{
		THROW(FileAccessException, "QFile::seek did not work on '" + fasta_name_ + "'!");
	}

	const qint64 line_breaks = (entry.length - 1) / entry.line_blen;
	const qint64 byte_count = entry.length + line_breaks * (entry.line_len - entry.line_blen);

	QByteArray data = file_.read(byte_count);
	if (data.size() != byte_count)
	{
		THROW(FileAccessException, "Unexpected end of FASTA file while reading chromosome '" + chr.str() + "'!");
	}

	//in-place replacement of newlines and upper-case conversion
	char* output = data.data();
	qsizetype output_size = 0;
	for (char base : std::as_const(data))
	{
		if (base == '\n' || base == '\r') continue;

		if (to_upper && base >= 'a' && base <= 'z')
		{
			base -= 'a' - 'A';
		}

		output[output_size++] = base;
	}

	//check that we wrote the right numer of characters into the output
	if (output_size != entry.length)
	{
		THROW(FileParseException, "FASTA index length does not match sequence length for chromosome '" + chr.str() + "'!");
	}

	data.truncate(output_size);
	return data;
}

Sequence FastaFileIndex::seq(const Chromosome& chr, int start, int length, bool to_upper) const
{
	//subtract 1 to make the coordinates 0-based
	const qint64 start_zero_based = static_cast<qint64>(start) - 1;
	if (start_zero_based < 0)
	{
		THROW(ProgrammingException, "FastaFileIndex::seq: Invalid start position (" + QString::number(start_zero_based) + ") for " + chr.strNormalized(true) + ":" + QString::number(start) + "-" + QString::number(start_zero_based+length));
	}
	if (length < 0)
	{
		THROW(ProgrammingException, "FastaFileIndex::seq: Invalid length (" + QString::number(length) + ") for " + chr.strNormalized(true) + ":" + QString::number(start) + "-" + QString::number(start_zero_based+length));
	}
	const FastaIndexEntry& entry = index(chr);
	if (start_zero_based > entry.length)
	{
		THROW(ProgrammingException, "FastaFileIndex::seq: Invalid start position " + chr.strNormalized(true) + ":" + QString::number(start) + " after chromosome end (" + QString::number(entry.length) + ")");
	}

	//restrict to chromosome length
	if (length > entry.length - start_zero_based)
	{
		Log::warn("FastaFileIndex::seq: Sequence length changed to chromosome end for: " + chr.strNormalized(true) + ":" + QString::number(start) + "-" + QString::number(start_zero_based+length));
		length = static_cast<int>(entry.length - start_zero_based);
	}
	if (length==0) return Sequence();
	if (entry.line_blen<=0 || entry.line_len<entry.line_blen)
	{
		THROW(FileParseException, "Invalid line lengths in FASTA index for chromosome '" + chr.str() + "'!");
	}

	//calculate the physical byte range using the FASTA index
	const qint64 end_zero_based = start_zero_based + length - 1;
	const qint64 read_start_pos = entry.offset + (start_zero_based / entry.line_blen) * entry.line_len + (start_zero_based % entry.line_blen);
	const qint64 read_end_pos = entry.offset + (end_zero_based / entry.line_blen) * entry.line_len + (end_zero_based % entry.line_blen);
	const qint64 byte_count = read_end_pos - read_start_pos + 1;

	if (!file_.seek(read_start_pos))
	{
		THROW(FileAccessException, "QFile::seek did not work on '" + fasta_name_ + "'!");
	}

	QByteArray data = file_.read(byte_count);
	if (data.size()!=byte_count)
	{
		THROW(FileAccessException, "Unexpected end of FASTA file while reading " + chr.str() + ":" + QString::number(start) + "-" + QString::number(start_zero_based+length) + "!");
	}
	if (!to_upper && byte_count==length) return data;

	//remove line endings and optionally convert to upper case in one in-place pass
	char* buffer = data.data();
	qsizetype output_size = 0;
	for (qsizetype i=0; i<data.size(); ++i)
	{
		char base = buffer[i];
		if (base=='\n' || base=='\r') continue;
		if (to_upper && base>='a' && base<='z') base -= 'a' - 'A';
		buffer[output_size++] = base;
	}

	if (output_size!=length)
	{
		THROW(FileParseException, "FASTA index length does not match sequence length for " + chr.str() + ":" + QString::number(start) + "-" + QString::number(start_zero_based+length) + "!");
	}

	data.truncate(output_size);
	return data;
}

int FastaFileIndex::n(const Chromosome& chr) const
{
	const FastaIndexEntry& entry = index(chr);
	if (!file_.seek(entry.offset))
	{
		THROW(FileAccessException, "QFile::seek did not work on " + fasta_name_ + "'!");
	}

	const qint64 line_break_count = (entry.length - 1) / entry.line_blen;
	qint64 bytes_remaining = entry.length + line_break_count * (entry.line_len - entry.line_blen);
	int output = 0;
	while (bytes_remaining>0)
	{
		QByteArray chunk = file_.read(qMin(1048576, bytes_remaining)); //1MB max
		if (chunk.isEmpty()) THROW(FileAccessException, "Unexpected end of FASTA file while reading chromosome '" + chr.str() + "'!");

		for (char base : std::as_const(chunk))
		{
			if (base=='N' || base=='n') ++output;
		}
		bytes_remaining -= chunk.size();
	}

	return output;
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
