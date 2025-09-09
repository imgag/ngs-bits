#include "TestFramework.h"
#include "BedpeFile.h"

TEST_CLASS(BedpeFile_Test)
{
private:
	BedpeFile test_file_germl_;
	BedpeFile test_file_som_;


private:
	TEST_METHOD(load)
	{
		test_file_germl_.load( TESTDATA("data_in/SV_Manta_germline.bedpe") );
		IS_FALSE(test_file_germl_.isSomatic());
		I_EQUAL(test_file_germl_.count(), 87);
		//check line 4 as exmaple
		S_EQUAL( test_file_germl_[14].chr1().strNormalized(true), "chr1");
		S_EQUAL( test_file_germl_[14].chr2().strNormalized(true), "chr1");
		I_EQUAL( test_file_germl_[14].start1(), 1588290 );
		I_EQUAL( test_file_germl_[14].end1(), 1588661 );
		I_EQUAL( test_file_germl_[14].start2(), 1653313 );
		I_EQUAL( test_file_germl_[14].end2(), 1654249 );
		S_EQUAL(test_file_germl_[14].genes(test_file_germl_.annotationHeaders())[0], "CDK11A");
		S_EQUAL(test_file_germl_[14].genes(test_file_germl_.annotationHeaders())[4], "SLC35E2B");



		test_file_som_.load( TESTDATA("data_in/SV_Manta_somatic.bedpe") );
		IS_TRUE( test_file_som_.isSomatic() );
		I_EQUAL( test_file_som_.count(), 8 );
		S_EQUAL( test_file_som_[0].toTsv(),"chr1\t9780838\t9780841\tchr1\t9781143\t9781143\tINV\tPASS\t50\t.\t108\t478\t3\t16\t325\t0\t964\t0\tINV5;SOMATIC\tT\t<INV>\t.\t.\tSVTYPE=INV;POS=9780838;SVLEN=305;END=9781143;CIPOS=0,3;CIEND=-3,0;HOMLEN=3;HOMSEQ=GGG;INV5;CONTIG=ACGGGCAGCTCCGGCCAGGAGCACAGCAGGTAGAGCATCTGGGGGGAGCCGAGGTCAGGCTTGGGGGCGGCCGGGGTCAGGGGAGCTGTATGAGCACGAGAAGGACCTGGTGTGGAAGCTGCGG;SOMATIC;SOMATICSCORE=50\t.\tMantaINV:89:0:0:7:0:0\t.");

	}

	TEST_METHOD(findMatch)
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

	TEST_METHOD(isSomatic)
	{
		IS_FALSE(test_file_germl_.isSomatic());
		IS_TRUE(test_file_som_.isSomatic());
	}

	TEST_METHOD(annotationIndexByName)
	{
		IS_THROWN(ArgumentException, test_file_som_.annotationIndexByName("NOT_EXISTING") );
		I_EQUAL(test_file_som_.annotationIndexByName("NOT_EXISTING", false), -1);
		I_EQUAL(test_file_som_.annotationIndexByName("JUNCTION_SOMATICSCORE"), 3);
		I_EQUAL(test_file_som_.annotationIndexByName("REF_B"), 15);
	}

	TEST_METHOD(metaInfoDescriptionByID)
	{
		QMap<QByteArray,QByteArray> meta_filter_description = test_file_som_.metaInfoDescriptionByID("FILTER");
		S_EQUAL(meta_filter_description.value("MinSomaticScore"), "Somatic score is less than 30");
		S_EQUAL(meta_filter_description.value("off-target"), "Variant marked as 'off-target'.");
	}

	TEST_METHOD(annotationDescriptionByName)
	{
		S_EQUAL(test_file_som_.annotationDescriptionByName("SOMATICSCORE"), "Somatic variant quality score");
		S_EQUAL(test_file_som_.annotationDescriptionByName("TUM_PR_ALT"), "Spanning paired-read support for the alt alleles in DX000002_01.");
		S_EQUAL(test_file_som_.annotationDescriptionByName("DOES_NOT_EXIST"), "");
	}

	TEST_METHOD(estimatedSvSize)
	{
		//deletion
		I_EQUAL( test_file_germl_.estimatedSvSize(0), 56);
		//insertion
		I_EQUAL( test_file_germl_.estimatedSvSize(43), 514);
		//BND
		I_EQUAL( test_file_germl_.estimatedSvSize(12), -1);
	}

};
