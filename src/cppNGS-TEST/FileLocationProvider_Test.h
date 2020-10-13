#include "TestFramework.h"
#include "FileLocationProvider.h"
#include "FileLocationProviderFileSystem.h"

TEST_CLASS(FileLocationProvider_Test)
{
Q_OBJECT
private slots:
	void getfiles()
	{
		QString filename = "../src/cppNGS-TEST/data_in/VariantFilter_in.GSvar";

		VariantList vl;
		vl.load(TESTDATA("data_in/VariantFilter_in.GSvar"));

		FileLocationProviderFileSystem fp = FileLocationProviderFileSystem(filename, vl.getSampleHeader(), vl.type());
		QList<FileLocation> bam_files = fp.getBamFiles();
		I_EQUAL(bam_files.length(), 1);
		IS_TRUE(bam_files[0].filename.endsWith("NA12878_03.bam"));

		QList<FileLocation> cnv_files = fp.getSegFilesCnv();
		I_EQUAL(cnv_files.length(), 0);

		QList<FileLocation> igv_files = fp.getIgvFilesBaf();
		I_EQUAL(igv_files.length(), 1);

		QList<FileLocation> manta_files = fp.getMantaEvidenceFiles();
		I_EQUAL(manta_files.length(), 1);
	}
};
