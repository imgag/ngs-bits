#include "TestFramework.h"
#include "VersatileFile.h"
#include "VersatileOutStream.h"
#include <QTextStream>

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
};
