#pragma once

#include "VcfFileHelper.h"




///class handling vcf and vcf.gz files
class CPPNGSSHARED_EXPORT VcfFileHandler
{

public:

	//loads a vcf or vcf.gz file
	void load(const QString& filename);
	//stores the data of VCFFileHandler in vcf file
	void store(const QString& filename) const;

	///return a QVector of VCFLines
	QVector<VCFLineType> lineVector() const
	{
		return VcfLineVector_;
	}
	///return a struct storing header information
	VCFHeaderType header() const
	{
		return VcfHeader_;
	}

private:

	void parseVcfHeader(const int line_number, QByteArray& line);
	void parseHeaderFields(QByteArray& line);
	void parseVcfEntry(const int line_number, QByteArray& line);
	void processVcfLine(int& line_number, QByteArray line);
	void loadFromVCF(const QString& filename);
	void loadFromVCFGZ(const QString& filename);

	QVector<VCFLineType> VcfLineVector_;
	VCFHeaderType VcfHeader_;

};
