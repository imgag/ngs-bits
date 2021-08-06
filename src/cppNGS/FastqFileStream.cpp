#include "FastqFileStream.h"

void FastqEntry::validate() const
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
		THROW(FileParseException, message + "Differing length of bases and qualities string in sequence '" + header + "'.");
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
        if (value<33 || value>74)
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
	int count = qualities.count();
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
	int count = bases.count();
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

FastqFileStream::FastqFileStream(QString filename, bool auto_validate)
	: filename_(filename)
	, gzfile_(NULL)
	, buffer_(0)
    , is_first_entry_(true)
    , last_output_(NULL)
    , entry_index_(-1)
    , auto_validate_(auto_validate)
{
    gzfile_ = gzopen(filename.toLatin1().data(), "rb"); //read binary: always open in binary mode because windows and mac open in text mode
    if (gzfile_ == NULL)
    {
		THROW(FileAccessException, "Could not open file '" + filename + "' for reading!");
    }
	buffer_ = new char[1024];
}

FastqFileStream::~FastqFileStream()
{
    gzclose(gzfile_);
	delete[] buffer_;
}

void FastqFileStream::readEntry(FastqEntry& entry)
{
    //special cases handling
    if (is_first_entry_)
    {
		last_output_ = gzgets(gzfile_, buffer_, 1024); //TODO try speed-up by reading bigger chunks and extracting the lines from the chunks, like in VcfFile::loadFromVCFGZ
        is_first_entry_ = false;
    }

	//handle errors like truncated GZ file
	if (last_output_==nullptr)
	{
		int error_no = Z_OK;
		QByteArray error_message = gzerror(gzfile_, &error_no);
		if (error_no==Z_OK || error_no==Z_STREAM_END)
		{
			entry.clear();
			return;
		}
		else
		{
			THROW(FileParseException, "Error while reading file '" + filename_ + "': " + error_message);
		}
	}

	//read data
	extractLine(entry.header);
	extractLine(entry.bases);
	extractLine(entry.header2);
	extractLine(entry.qualities);

    //increase index.
    ++entry_index_;

    //validate
	if (auto_validate_) entry.validate();
}

void FastqFileStream::extractLine(QByteArray& line)
{
	line = QByteArray(buffer_);
	while (line.endsWith('\n') || line.endsWith('\r')) line.chop(1);

	last_output_ = gzgets(gzfile_, buffer_, 1024);

	//handle errors like truncated GZ file
	if (last_output_==nullptr)
	{
		int error_no = Z_OK;
		QByteArray error_message = gzerror(gzfile_, &error_no);
		if (error_no!=Z_OK && error_no!=Z_STREAM_END)
		{
			THROW(FileParseException, "Error while reading file '" + filename_ + "': " + error_message);
		}
	}
}


FastqOutfileStream::FastqOutfileStream(QString filename, int compression_level, int compression_strategy)
	: filename_(filename)
    , is_closed_(false)
	, buffer_()
	, buffer_size_(10000)
{
	gzfile_ = gzopen(filename.toLatin1().data(), "wb");
    if (gzfile_ == NULL)
    {
        THROW(FileAccessException, "Could not open file '" + filename + "' for writing!");
	}

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
	//append entry to buffer
	buffer_.append(entry.header);
	buffer_.append('\n');
	buffer_.append(entry.bases);
	buffer_.append('\n');
	buffer_.append(entry.header2);
	buffer_.append('\n');
	buffer_.append(entry.qualities);
	buffer_.append('\n');

	if (buffer_.size()>=buffer_size_)
	{
		int written = gzputs(gzfile_, buffer_.constData());
		if (written==0)
		{
			THROW(FileAccessException, "Could not write to file '" + filename_ + "'!");
		}

		buffer_.clear();
		buffer_.reserve(buffer_size_+1000);
	}
}

void FastqOutfileStream::close()
{
    if (is_closed_) return;


	if (buffer_.size()>0)
	{
		int written = gzputs(gzfile_, buffer_.constData());
		if (written==0)
		{
			THROW(FileAccessException, "Could not write to file '" + filename_ + "'!");
		}

		buffer_.clear();
		buffer_.reserve(buffer_size_+1000);
	}

    gzclose(gzfile_);
	is_closed_ = true;
}
