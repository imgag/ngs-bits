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
	bases.resize(0);
	qualities.resize(0);
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
	: gzfile_(NULL)
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
	delete buffer_;
}

void FastqFileStream::readEntry(FastqEntry& entry)
{
    //special cases handling
    if (is_first_entry_)
    {
		last_output_ = gzgets(gzfile_, buffer_, 1024);
        is_first_entry_ = false;
    }
    if (last_output_==NULL)
    {
		entry.clear();
		return;
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
	int i=0;
	while(i<1023 && buffer_[i]!='\n' && buffer_[i]!='\0' && buffer_[i]!='\n' && buffer_[i]!='\r')
	{
		++i;
	}
	line = QByteArray(buffer_, i);
	last_output_ = gzgets(gzfile_, buffer_, 1024);
}


FastqOutfileStream::FastqOutfileStream(QString filename, bool thread_safe_mode, int level, int strategy)
	: mutex_()
	, thread_safe_(thread_safe_mode)
	, filename_(filename)
    , is_closed_(false)
{
    gzfile_ = gzopen(filename.toLatin1().data(),"wb");
    if (gzfile_ == NULL)
    {
        THROW(FileAccessException, "Could not open file '" + filename + "' for writing!");
    }
    gzsetparams(gzfile_, level, strategy);
}

FastqOutfileStream::~FastqOutfileStream()
{
    close();
}

void FastqOutfileStream::write(const FastqEntry& entry)
{
	if (thread_safe_) mutex_.lock();

    int written = 0;

	written += gzputs(gzfile_, entry.header.data());
    written += gzwrite(gzfile_, "\n", 1);
	written += gzputs(gzfile_, entry.bases.data());
    written += gzwrite(gzfile_, "\n", 1);
	written += gzputs(gzfile_, entry.header2.data());
    written += gzwrite(gzfile_, "\n", 1);
	written += gzputs(gzfile_, entry.qualities.data());
    written += gzwrite(gzfile_, "\n", 1);

    if (written==0)
    {
        THROW(FileAccessException, "Could not write to file '" + filename_ + "'!");
    }

	if (thread_safe_) mutex_.unlock();
}

void FastqOutfileStream::close()
{
    if (is_closed_) return;

    gzclose(gzfile_);
	is_closed_ = true;
}
