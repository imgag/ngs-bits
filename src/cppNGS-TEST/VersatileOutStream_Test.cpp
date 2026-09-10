#include "TestFramework.h"
#include "VersatileFile.h"
#include "VersatileOutStream.h"
#include <QTextStream>
#include <htslib/bgzf.h>

TEST_CLASS(VersatileOutStream_Test)
{
private:
	TEST_METHOD(plain_file)
	{
		QString filename = "out/VersatileOutStream_plain.txt";

		VersatileOutStream output(filename);
		QTextStream stream(&output);
		stream.setEncoding(QStringConverter::Utf8);
		stream << "first line\nsecond line: äöü\n";
		stream.flush();
		I_EQUAL(stream.status(), QTextStream::Ok);
		output.close();

		VersatileFile input(filename);
		input.open();
		S_EQUAL(input.readAll(), QByteArray("first line\nsecond line: äöü\n"));
	}

	TEST_METHOD(gzip_file)
	{
		QString filename = "out/VersatileOutStream_gzip.txt.gz";

		VersatileOutStream output(filename, false, 6);
		QTextStream stream(&output);
		stream.setEncoding(QStringConverter::Utf8);
		stream << "first line\nsecond line: äöü\n";
		stream.flush();
		I_EQUAL(stream.status(), QTextStream::Ok);
		output.close();

		VersatileFile input(filename);
		input.open();
		S_EQUAL(input.readAll(), QByteArray("first line\nsecond line: äöü\n"));
	}

	TEST_METHOD(bgzf_file)
	{
		QString filename = "out/VersatileOutStream_bgzf.txt.gz";
		QByteArray expected = "first line\nsecond line: äöü\n";

		VersatileOutStream output(filename, false, 6, true);
		I_EQUAL(output.write(expected), expected.size());
		output.close();

		BGZF* input = bgzf_open(filename.toUtf8().constData(), "r");
		IS_TRUE(input!=nullptr);
		I_EQUAL(bgzf_compression(input), 2);
		QByteArray actual(expected.size(), '\0');
		I_EQUAL(bgzf_read(input, actual.data(), actual.size()), actual.size());
		I_EQUAL(bgzf_check_EOF(input), 1);
		I_EQUAL(bgzf_close(input), 0);
		S_EQUAL(actual, expected);
	}
};
