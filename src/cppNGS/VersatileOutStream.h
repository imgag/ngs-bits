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
	///Compression level NO_COMPRESSION selects plain output;
	static constexpr int NO_COMPRESSION = 10;

	///When bgz is true, BGZF output is written using htslib.
	VersatileOutStream(QString filename, bool stdout_if_empty=false, int compression_level=NO_COMPRESSION, bool bgz=false);
	~VersatileOutStream() override;

	///Flushes and closes the output. Throws FileAccessException on failure.
	void close() override;

	QString filename() const
	{
		return filename_;
	}

	bool isCompressed() const
	{
		return compression_level_!=NO_COMPRESSION;
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
