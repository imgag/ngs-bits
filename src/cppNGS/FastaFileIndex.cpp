#include "FastaFileIndex.h"
#include "Exceptions.h"
#include "Helper.h"
#include "Log.h"
#include <QRegExp>
#include <QStringList>

//large file support definitions
#ifdef WIN32
#define ftell64(a)     _ftelli64(a)
#define fseek64(a,b,c) _fseeki64(a,b,c)
typedef __int64 off_type;
#elif defined(__APPLE__)
#define ftell64(a)     ftello(a)
#define fseek64(a,b,c) fseeko(a,b,c)
typedef off_t off_type;
#else
#define ftell64(a)     ftello(a)
#define fseek64(a,b,c) fseeko(a,b,c)
typedef __off64_t off_type;
#endif

using namespace std;

FastaFileIndex::FastaFileIndex(QString fasta_file)
	: fasta_name_(fasta_file)
	, index_name_(fasta_file + ".fai")
{
	//open FASTA file handle
	file_ = fopen(fasta_name_.toStdString().c_str(), "r");
	if (file_==NULL)
	{
		THROW(FileAccessException, "Could not open FASTA file '" + fasta_name_ + "' for reading!");
	}

	//load index file
	int linenum = 0;
	QScopedPointer<QFile> file(Helper::openFileForReading(index_name_));
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
	fclose(file_);
}

QByteArray FastaFileIndex::seq(const Chromosome& chr, bool to_upper) const
{
	const FastaIndexEntry& entry = index(chr);
	int newlines_in_sequence = entry.length / entry.line_blen;
	int seqlen = newlines_in_sequence  + entry.length;
	char* seq = (char*) calloc (seqlen + 1, sizeof(char));
	fseek64(file_, entry.offset, SEEK_SET);
	fread(seq, sizeof(char), seqlen, file_);
	seq[seqlen] = '\0';
	char* pbegin = seq;
	char* pend = seq + (seqlen/sizeof(char));
	pend = remove(pbegin, pend, '\n');
	pend = remove(pbegin, pend, '\0');

	QByteArray output(seq);
	free(seq);
	output.resize((pend - pbegin)/sizeof(char));
	if (to_upper) output = output.toUpper();
	return output;
}

QByteArray FastaFileIndex::seq(const Chromosome& chr, int start, int length, bool to_upper) const
{
	//subtract 1 to make the coordinates 0-based
	start -= 1;

	//check if coords are valid
	const FastaIndexEntry& entry = index(chr);
	length = min(length, entry.length - start);
	if (start < 0 || length < 1) return "";

	// we have to handle newlines
	// approach: count newlines before start
	//           count newlines by end of read
	//             subtracting newlines before start find count of embedded newlines
	int newlines_before = start > 0 ? (start - 1) / entry.line_blen : 0;
	int newlines_by_end = (start + length - 1) / entry.line_blen;
	int newlines_inside = newlines_by_end - newlines_before;
	int seqlen = length + newlines_inside;
	char* seq = (char*) calloc (seqlen + 1, sizeof(char));
	fseek64(file_, (off_type) (entry.offset + newlines_before + start), SEEK_SET);
	fread(seq, sizeof(char), (off_type) seqlen, file_);
	seq[seqlen] = '\0';
	char* pbegin = seq;
	char* pend = seq + (seqlen/sizeof(char));
	pend = remove(pbegin, pend, '\n');
	pend = remove(pbegin, pend, '\0');

	QByteArray output(seq);
	free(seq);
	output.resize((pend - pbegin)/sizeof(char));
	if (to_upper) output = output.toUpper();
	//qDebug() << "  SEQ: " << chr.strNormalized(true) << start << (start+length) << output;
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
