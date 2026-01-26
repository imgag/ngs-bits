#include "FastqFileStream.h"

void FastqEntry::validate(bool long_read) const
{
	QString message = "Invalid Fastq file entry: ";

	if (header.length()==0 || header[0]!='@')
    {
		THROW(FileParseException, message + "First header line does not start with '@': '" + header + "'.");
    }
	if (header2.length()==0 || header2[0]!='+')
    {
		THROW(FileParseException, message + "Second header line does not start with '+': '" + header2 + "'.");
    }
	if (bases.length()!=qualities.length())
    {
		THROW(FileParseException, message + "Differing length of bases ("+QString::number(bases.length())+") and qualities string ("+QString::number(bases.length())+") in sequence '" + header + "'.");
    }
	foreach(char c, bases)
    {
        if (c!='A' && c!='C' && c!='G' && c!='T' && c!='N')
        {
			THROW(FileParseException, message + "Invalid base '" + c + "' encountered in sequence '" + header + "'.");
        }
    }
	foreach(char c, qualities)
    {
        int value = c;
		//Nanopore quality range: 33-126 (offset 33) (https://en.wikipedia.org/wiki/FASTQ_format#Encoding)
		if (long_read)
		{
			if(value<33 || value>126)
			{
				THROW(FileParseException, message + "Invalid nanopore quality character '" + c + "' with value '" + QString::number(value) + "' encountered in sequence '" + header + "'.");
			}
		}
		else if (value<33 || value>74)
		{
			THROW(FileParseException, message + "Invalid quality character '" + c + "' with value '" + QString::number(value) + "' encountered in sequence '" + header + "'.");
		}
    }
}

void FastqEntry::clear()
{
	header.clear();
	bases.clear();
	header2.clear();
	qualities.clear();
}

int FastqEntry::trimQuality(int cutoff, int window, int offset)
{
    //too small read => abort
    int count = qualities.size();
    if (count<window) return 0;

    //init
    double sum = 0;
    for (int i=count-1; i>count-window; --i)
    {
        sum += quality(i, offset);
    }

    //search
    for (int i=count-window; i>=0; --i)
    {
        sum += quality(i, offset);
        if (sum/window >= cutoff) //window below cutoff
        {
            int count_new = i+window;
            while(count_new>0 && quality(count_new-1, offset)<cutoff) //remove bases at the end of the window until the cutoff is reached.
            {
                --count_new;
            }
			bases.resize(count_new);
			qualities.resize(count_new);
            return count-count_new;
        }
        sum -= quality(i+window-1, offset);
    }

    //no windows above cutoff => remove all bases
	bases.clear();
	qualities.clear();
	return count;
}

int FastqEntry::trimN(int num_n)
{
    //too small read => abort
    int count = bases.size();
    if (count<num_n) return 0;

    //init
    int sum = 0;
    for (int i=0; i<num_n-1; ++i)
    {
		sum += (bases[i]=='N');
    }

    //search
    for (int i=num_n-1; i<count; ++i)
    {
		sum += (bases[i]=='N');
        if (sum==num_n)
        {
            int count_new = i-num_n+1;
			bases.resize(count_new);
			qualities.resize(count_new);
            return count-count_new;
        }
		sum -= (bases[i-num_n+1]=='N');
    }

    return 0;
}

FastqFileStream::FastqFileStream(QString filename, bool auto_validate, bool long_read)
	: filename_(filename)
	, gzfile_(filename)
	, auto_validate_(auto_validate)
	, long_read_(long_read)
{
	int buffer_size = long_read_ ? 8388608 : 1024;
	gzfile_.setGzBufferSize(buffer_size);
	gzfile_.setGzBufferSizeInternal(128 * buffer_size);
	gzfile_.open();
}

FastqFileStream::~FastqFileStream()
{
}

void FastqFileStream::readEntry(FastqEntry& entry)
{
	//special cases handling
	if (is_first_entry_)
	{
		last_output_ = gzfile_.readLine(true);
		is_first_entry_ = false;
	}

	//read data
	entry.header = last_output_;
	entry.bases = gzfile_.readLine(true);
	entry.header2 = gzfile_.readLine(true);
	entry.qualities = gzfile_.readLine(true);
	last_output_ = gzfile_.readLine(true);

	if (entry.header.isEmpty() && entry.header.isEmpty() && entry.header.isEmpty() && entry.header.isEmpty()) return;

    //increase index.
    ++entry_index_;

    //validate
	if (auto_validate_) entry.validate(long_read_);
}

FastqOutfileStream::FastqOutfileStream(QString filename, int compression_level, int compression_strategy)
	: filename_(filename)
	, is_closed_(false)
{
	gzfile_ = gzopen(filename.toUtf8().data(), "wb");
    if (gzfile_ == NULL)
    {
        THROW(FileAccessException, "Could not open file '" + filename + "' for writing!");
	}

	gzbuffer(gzfile_, 131072);

	if (compression_level<0 || compression_level>9) THROW(ArgumentException, "Invalid gzip compression level '" + QString::number(compression_level) +"' given for FASTQ file '" + filename + "'!");
	if (compression_strategy<0 || compression_strategy>4) THROW(ArgumentException, "Invalid gzip compression strategy '" + QString::number(compression_strategy) +"' given for FASTQ file '" + filename + "'!");
	gzsetparams(gzfile_, compression_level, compression_strategy);
}

FastqOutfileStream::~FastqOutfileStream()
{
    close();
}

void FastqOutfileStream::write(const FastqEntry& entry)
{
	static QByteArray newline = "\n";
	if (gzputs(gzfile_, entry.header.constData())==-1
		|| gzputs(gzfile_, newline)==-1
		|| gzputs(gzfile_, entry.bases.constData())==-1
		|| gzputs(gzfile_, newline)==-1
		|| gzputs(gzfile_, entry.header2.constData())==-1
		|| gzputs(gzfile_, newline)==-1
		|| gzputs(gzfile_, entry.qualities.constData())==-1
		|| gzputs(gzfile_, newline)==-1)
	{
		THROW(FileAccessException, "Could not write to file '" + filename_ + "'!");
	}
}

void FastqOutfileStream::close()
{
    if (is_closed_) return;

	gzclose(gzfile_);
	is_closed_ = true;
}
