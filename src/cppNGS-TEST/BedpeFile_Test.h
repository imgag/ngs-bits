#include "TestFramework.h"
#include "BedpeFile.h"

TEST_CLASS(BedpeFile_Test)
{
Q_OBJECT
private slots:
	void load()
	{
		BedpeFile test_file_germl;
		test_file_germl.load( TESTDATA("data_in/SV_Manta_germline.bedpe") );
		IS_FALSE(test_file_germl.isSomatic());
		I_EQUAL(test_file_germl.count(), 87);
		//check line 4 as exmaple
		S_EQUAL( test_file_germl[14].chr1().strNormalized(true), "chr1");
		S_EQUAL( test_file_germl[14].chr2().strNormalized(true), "chr1");
		I_EQUAL( test_file_germl[14].start1(), 1588290 );
		I_EQUAL( test_file_germl[14].end1(), 1588661 );
		I_EQUAL( test_file_germl[14].start2(), 1653313 );
		I_EQUAL( test_file_germl[14].end2(), 1654249 );


		BedpeFile test_file_som;
		test_file_som.load( TESTDATA("data_in/SV_Manta_somatic.bedpe") );

		IS_TRUE( test_file_som.isSomatic() );
		I_EQUAL( test_file_som.count(), 8 );
		S_EQUAL( test_file_som[0].toTsv(),"chr1\t9780838\t9780841\tchr1\t9781143\t9781143\tINV\tPASS\t50\t.\t108\t478\t3\t16\t325\t0\t964\t0\tINV5;SOMATIC\tT\t<INV>\t.\t.\tSVTYPE=INV;POS=9780838;SVLEN=305;END=9781143;CIPOS=0,3;CIEND=-3,0;HOMLEN=3;HOMSEQ=GGG;INV5;CONTIG=ACGGGCAGCTCCGGCCAGGAGCACAGCAGGTAGAGCATCTGGGGGGAGCCGAGGTCAGGCTTGGGGGCGGCCGGGGTCAGGGGAGCTGTATGAGCACGAGAAGGACCTGGTGTGGAAGCTGCGG;SOMATIC;SOMATICSCORE=50\t.\tMantaINV:89:0:0:7:0:0\t.");

	}

	void findMatch()
	{
		// load 2 identical files and find the SVs in the second 2
		BedpeFile file1, file2;
		file1.load(TESTDATA("data_in/SV_Manta_germline.bedpe"));
		file2.load(TESTDATA("data_in/SV_Manta_germline.bedpe"));


		QVector<int> indices;
		for (int i = 0; i < file1.count(); ++i)
		{
			indices << file2.findMatch(file1[i], true, true);
		}

		for (int i = 0; i < file2.count(); ++i)
		{
			I_EQUAL(indices[i], i);
		}
	}

	void isSomatic()
	{
		BedpeFile file_germline;
		file_germline.load( TESTDATA("data_in/SV_Manta_germline.bedpe") );
		IS_FALSE(file_germline.isSomatic());

		BedpeFile file_somatic;
		file_somatic.load( TESTDATA("data_in/SV_Manta_somatic.bedpe") );
		IS_TRUE(file_somatic.isSomatic());
	}

};
