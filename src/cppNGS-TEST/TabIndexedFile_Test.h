#ifndef TABINDEXEDFILE_TEST_H
#define TABINDEXEDFILE_TEST_H

#include "TestFramework.h"
#include "TestFrameworkNGS.h"
#include "Settings.h"
#include "TabixIndexedFile.h"

TEST_CLASS(TabixIndexedFile_Test)
{
private:

	void working_index()
	{
		TabixIndexedFile file;
		file.load(TESTDATA("data_in/TabixIndexedFile_in1.vcf.gz"));

		Chromosome chr("chr1");
		QByteArrayList lines = file.getMatchingLines(chr, 17384, 17386);
		I_EQUAL(lines.count(), 1);
		S_EQUAL(lines[0], "chr1	17385	.	G	A	111	.	MQM=26;SAP=42;ABP=24	GT:DP:AO:GQ	0/1:60:18:110");

		lines = file.getMatchingLines(chr, 3831039, 3836572);
		I_EQUAL(lines.count(), 3);
		S_EQUAL(lines[0], "chr1	3831039	.	T	C	1286	.	MQM=60;SAP=88;ABP=0	GT:DP:AO:GQ	1/1:43:43:148");
		S_EQUAL(lines[1], "chr1	3836468	.	G	GT	7	off-target	MQM=60;SAP=10;ABP=15	GT:DP:AO:GQ	0/1:15:3:6");
		S_EQUAL(lines[2], "chr1	3836572	.	A	T	7952	.	MQM=60;SAP=19;ABP=0	GT:DP:AO:GQ	1/1:247:247:160");

		lines = file.getMatchingLines(chr, 6554355, 6554355);
		I_EQUAL(lines.count(), 1);
		S_EQUAL(lines[0], "chr1	6554355	.	A	G	3086	.	MQM=60;SAP=10;ABP=0	GT:DP:AO:GQ	1/1:95:95:160");

		lines = file.getMatchingLines(chr, 17380, 17384);
		I_EQUAL(lines.count(), 0);

		lines = file.getMatchingLines(chr, 6554331, 6554360);
		I_EQUAL(lines.count(), 2);

		lines = file.getMatchingLines(chr, 6554356, 6554360);
		I_EQUAL(lines.count(), 0);

		lines = file.getMatchingLines(chr, 3752608, 5888617);
		I_EQUAL(lines.count(), 42);
	}

	void broken_index()
	{
		TabixIndexedFile file;
		file.load(TESTDATA("data_in/TabixIndexedFile_in2.vcf.gz"));

		Chromosome chr("chr1");
		IS_THROWN(FileParseException, file.getMatchingLines(chr, 953259, 961945)); // [E::get_intv] Failed to parse TBX_VCF, was wrong -p [type] used?
	}
};
#endif // TABINDEXEDFILE_TEST_H
