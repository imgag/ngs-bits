#ifndef VCFFILE_H
#define VCFFILE_H

#include "cppNGS_global.h"
#include <QIODevice>
#include <QString>
#include <QMap>
#include <QTextStream>

//Helper class for VCF file handling
class CPPNGSSHARED_EXPORT VcfFile
{
public:

	static const int MIN_COLS = 8;
	static const int CHROM = 0;
	static const int POS = 1;
	static const int ID = 2;
	static const int REF = 3;
	static const int ALT = 4;
	static const int QUAL = 5;
	static const int FILTER = 6;
	static const int INFO = 7;
	static const int FORMAT = 8;

	//Validates VCF file
	static bool isValid(QString vcf_file, QString ref_file, QTextStream& out_stream, bool print_general_information = false, int max_lines = std::numeric_limits<int>::max());

private:

	//INFO/FORMAT definition line
	struct DefinitionLine
	{
		QByteArray id;
		QByteArray description;
		QByteArray type;
		QByteArray number;
		int used = 0;

		QByteArray toString() const
		{
			QByteArray output;
			output += "ID="+id + " ("+QByteArray::number(used)+"x used)";
			if (!type.isEmpty()) output += " Type="+type;
			if (!number.isEmpty()) output += " Number="+number;
			output += " Description="+description;

			return output;
		}
	};

	//print information line
	static void printInfo(QTextStream& out, QByteArray message)
	{
		out << message.trimmed() << "\n";
	}

	//print warning line
	static void printWarning(QTextStream& out, QByteArray message, int l, const QByteArray& line)
	{
		out << "WARNING: " << message.trimmed() <<  " - in line " << QByteArray::number(l+1) << ":\n" << line << "\n";
	}

	//print error line
	static void printError(QTextStream& out, QByteArray message, int l, const QByteArray& line)
	{
		out << "ERROR: " << message.trimmed() << " - in line " << QByteArray::number(l+1) << ":\n" << line << "\n";
	}

	//parse definition line
	static DefinitionLine parseDefinitionLine(QTextStream& out, int l, QByteArray line);

	//check number of values of a FORMAT/INFO entry
	static void checkValues(const DefinitionLine& def, const QByteArrayList& values, int alt_count, const QByteArray& sample, QTextStream& out, int l, const QByteArray& line);
};

#endif // VCFFILE_H
