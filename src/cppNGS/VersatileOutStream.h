#ifndef VERSATILEOUTSTREAM_H
#define VERSATILEOUTSTREAM_H

#include "cppNGS_global.h"
#include <QFile>
#include <QIODevice>
#include <htslib/bgzf.h>
#include <zlib.h>

///Streaming output device for plain, gzip-compressed and BGZF-compressed files.
class CPPNGSSHARED_EXPORT VersatileOutStream
	: public QIODevice
{
public:
	///When compression level is 1 to 9, gzipped output is written. When additionally bgz is true, block-zipped output is written.
	VersatileOutStream(QString filename, bool stdout_if_empty=false, int compression_level=Z_NO_COMPRESSION, bool bgz=false);
	~VersatileOutStream() override;

	///Flushes and closes the output. Throws FileAccessException on failure.
	void close() override;

	QString filename() const
	{
		return filename_;
	}

	bool isCompressed() const
	{
		return compression_level_!=Z_NO_COMPRESSION;
	}

	bool isBGZ()
	{
		return bgz_;
	}

protected:
	qint64 readData(char* data, qint64 max_size) override;
	qint64 writeData(const char* data, qint64 size) override;

private:
	void closeInternal(bool throw_on_error);

	QString filename_;
	int compression_level_;
	bool bgz_;
	QFile file_;
	gzFile gz_file_ = nullptr;
	BGZF* bgzf_file_ = nullptr;

	VersatileOutStream(const VersatileOutStream&) = delete;
	VersatileOutStream& operator=(const VersatileOutStream&) = delete;
};

#endif // VERSATILEOUTSTREAM_H
