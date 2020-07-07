#pragma once

#include "VcfFileHelper.h"

namespace VcfFormat
{

///class handling vcf and vcf.gz files
class CPPNGSSHARED_EXPORT VcfFileHandler
{

public:

	//loads a vcf or vcf.gz file
	void load(const QString& filename);
	//stores the data of VCFFileHandler in vcf file
	void store(const QString& filename) const;

	void checkValid() const;
	void sort(bool use_quality);
	void removeDuplicates(bool sort_by_quality);
	int count() const
	{
		return VcfLines_.count();
	}
	QByteArrayList sampleNames() const;
	QByteArrayList informations() const;
	QByteArrayList filters() const;
	QByteArrayList formats() const;


	///return a QVector of VCFLines
	QVector<VCFLine> vcfLines() const
	{
		return VcfLines_;
	}
	///return a struct storing header information
	VCFHeader vcfHeader() const
	{
		return VcfHeader_;
	}

private:

	void parseVcfHeader(const int line_number, QByteArray& line);
	void parseHeaderFields(QByteArray& line);
	void parseVcfEntry(const int line_number, QByteArray& line, QSet<QByteArray> info_ids, QSet<QByteArray> format_ids);
	void processVcfLine(int& line_number, QByteArray line, QSet<QByteArray> info_ids, QSet<QByteArray> format_ids);
	void loadFromVCF(const QString& filename);
	void loadFromVCFGZ(const QString& filename);

	QVector<VCFLine> VcfLines_;
	VCFHeader VcfHeader_;
	QVector<QByteArray> column_headers_;

};

} //end namespace VcfFormat
