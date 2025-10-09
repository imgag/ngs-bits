#ifndef FASTQFILESTREAM_H
#define FASTQFILESTREAM_H

#include "cppNGS_global.h"
#include "Exceptions.h"
#include "Sequence.h"
#include <QString>
#include "VersatileFile.h"

///Representation of a FASTQ entry.
struct CPPNGSSHARED_EXPORT FastqEntry
{
	///Main header line.
	QByteArray header;
	///Bases string.
	Sequence bases;
	///Second header line.
	QByteArray header2;
	///Qualities string.
	QByteArray qualities;

    ///Returns the quality of the base with the index @p i.
    int quality(int i, int offset=33) const
    {
		return (int)(qualities[i]) - offset;
    }
    ///Checks if the entry is valid. If not, a FileParseException is thrown.
	void validate(bool long_read = false) const;
    ///Resets the entry
    void clear();
    ///Trims the read by quality from the end using a sliding window approach. Returns the number of trimmed bases.
    int trimQuality(int cutoff, int window=5, int offset=33);
    ///Trims the read by non-determined bases using a sliding window approach. Returns the number of trimmed bases.
    int trimN(int num_n);
};

/**
  @brief FASTQ file input stream (gzipped or plain).

  @note The base/quality lines must not be wrapped.
  @note Fastq lines longer that 1024 characters are not supported!
*/
class CPPNGSSHARED_EXPORT FastqFileStream
{
public:
    ///Constructor.
	FastqFileStream(QString filename, bool auto_validate=true, bool long_read=false);
    ///Destructor.
    ~FastqFileStream();

    ///Checks if the end of the file is reached.
    bool atEnd() const
    {
		return gzfile_.atEnd();
	}
	///Reads a line (or until the buffer is full).
	void readEntry(FastqEntry& entry);
    ///Returns the 0-based index of the current entry, or -1 if no entry has been loaded.
    int index() const
    {
        return entry_index_;
    }

	///Returns the file name
	QString filename() const
	{
		return filename_;
	}

protected:
	QString filename_;
	bool is_first_entry_ = true;
	VersatileFile gzfile_;
	QByteArray last_output_;
	int entry_index_  = -1;
    bool auto_validate_;
	bool long_read_;

    //declared away methods
	FastqFileStream(const FastqFileStream& ) = delete;
	FastqFileStream& operator=(const FastqFileStream&) = delete;
	FastqFileStream() = delete;
};



/**
  @brief FASTQ file output stream (gzipped).
*/
class CPPNGSSHARED_EXPORT FastqOutfileStream
{
public:
    ///Constructor.
	FastqOutfileStream(QString filename, int compression_level = Z_BEST_SPEED, int compression_strategy = Z_DEFAULT_STRATEGY);
    ///Destructor - closes the stream if not already done.
    ~FastqOutfileStream();

    ///Writes an entry to the stream.
	void write(const FastqEntry& entry);
    ///Closes the stream.
    void close();

	///Returns the filename the stream writes to.
	QString filename() const
	{
		return filename_;
	}

protected:
    QString filename_;
	gzFile gzfile_;
	bool is_closed_;

    //declared away methods
	FastqOutfileStream(const FastqOutfileStream& ) = delete;
	FastqOutfileStream& operator=(const FastqOutfileStream&) = delete;
	FastqOutfileStream() = delete;
};

#endif
