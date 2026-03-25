#include "TestFramework.h"
#include "BedpeFile.h"

TEST_CLASS(BedpeFile_Test)
{
private:
	BedpeFile test_file_germl_;
	BedpeFile test_file_som_;
	BedpeFile test_file_lrgs_;
	BedpeFile test_file_normalize_manta_;
	BedpeFile test_file_normalize_sniffles_;
	BedpeFile test_file_validate_manta_;
	BedpeFile test_file_validate_sniffles_;



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

	TEST_METHOD(normalize)
	{

		test_file_normalize_manta_.load(TESTDATA("data_in/BedpeFile_normalize_manta.bedpe"));
		IS_FALSE(test_file_normalize_manta_.validate(false));
		test_file_normalize_manta_.normalize();
		IS_TRUE(test_file_normalize_manta_.validate());

		//TODO: check single SVs
		int idx_name_a = test_file_normalize_manta_.annotationIndexByName("NAME_A");
		int idx_name_b = test_file_normalize_manta_.annotationIndexByName("NAME_B");
		int idx_ref_a = test_file_normalize_manta_.annotationIndexByName("REF_A");
		int idx_ref_b = test_file_normalize_manta_.annotationIndexByName("REF_B");
		int idx_alt_a = test_file_normalize_manta_.annotationIndexByName("ALT_A");
		int idx_alt_b = test_file_normalize_manta_.annotationIndexByName("ALT_B");
		int idx_info_a = test_file_normalize_manta_.annotationIndexByName("INFO_A");
		int idx_info_b = test_file_normalize_manta_.annotationIndexByName("INFO_B");

		//already normalized
		BedpeLine sv = test_file_normalize_manta_[2];
		S_EQUAL(sv.chr1().str(), "chr1");
		S_EQUAL(sv.chr2().str(), "chr3");
		I_EQUAL(sv.start1(), 151769287);
		I_EQUAL(sv.start2(), 56584016);
		I_EQUAL(sv.end1(), 151769287);
		I_EQUAL(sv.end2(), 56584016);
		S_EQUAL(sv.annotations().at(idx_name_a), "MantaBND:1:78021:78022:1:0:0:0");
		S_EQUAL(sv.annotations().at(idx_name_b), "MantaBND:1:78021:78022:1:0:0:1");
		S_EQUAL(sv.annotations().at(idx_ref_a), "C");
		S_EQUAL(sv.annotations().at(idx_ref_b), "G");
		S_EQUAL(sv.annotations().at(idx_alt_a), "]CHR3:56584016]C");
		S_EQUAL(sv.annotations().at(idx_alt_b), "G[CHR1:151769287[");
		S_EQUAL(sv.annotations().at(idx_info_a), "SVTYPE=BND;MATEID=MantaBND:1:78021:78022:1:0:0:1;CONTIG=AAATCCGGCTTCGTGATCATGGCGCTTTCTCACTGAGCTCTTGGGTACACCTCCCAGACGGGGTGGCGGCCGGGCAGAGGGGCTCAGTTCCCAGAAGGGGCGGCCGGGCAGACGCACCCCCCACCTCCCGGAGGGGGCGGTGGCCGGGTGGGGGCTGCCCCCCACCTCCCTCCCGGAGGGGGCGGTGGCCGGGCGGGGGCTGCCCCCCACCTCCCTCCCGGACGGGGCGGCTGACCGGGCGGGGGCTGCCCGCCACCTCCCTCCCAGACGGGGCGGCTGACCGGGCGGGGGCTGCCCCCCACCTCCCGGACGGGGCAGCTGCCGGGCGGAGACGCTCCTCACTTCCCAGACGAAGCGGCTGCCGGGCGGAGGGGCTCCTCACTTCTCAGACAGGGCGGACGGGCAGAGATGCTCCTCACCTCCCAGATGGGGTCGCGGCTGGGCAGAGACACTCCTCAGTTCCCAGACAGGGTCGTGGCCGGGCAGAGGCGCTCCCCACATCCCAGACGATGGGCGGCTGGGCGGAGACGCTCCTCACTTCCTAGACGGGATGACGGCCGGGCAGAGGCGCTCCTCACATCCCAGACAATGGGCGGCCAGGCAGAGACGCTCCTCACTTCCCAGACGGGGTGGTGGCCGGGCAGAGGCTGCAATCTCGGCACTTTGGGAGGCCAAGGCAGGCGGCTGGGAGGTGGAGGTTGTAGCGAGCCGAGATCACGCCACTGCACTCCAGCC;BND_DEPTH=109;MATE_BND_DEPTH=158");
		S_EQUAL(sv.annotations().at(idx_info_b), "SVTYPE=BND;MATEID=MantaBND:1:78021:78022:1:0:0:0;CONTIG=AAATCCGGCTTCGTGATCATGGCGCTTTCTCACTGAGCTCTTGGGTACACCTCCCAGACGGGGTGGCGGCCGGGCAGAGGGGCTCAGTTCCCAGAAGGGGCGGCCGGGCAGACGCACCCCCCACCTCCCGGAGGGGGCGGTGGCCGGGTGGGGGCTGCCCCCCACCTCCCTCCCGGAGGGGGCGGTGGCCGGGCGGGGGCTGCCCCCCACCTCCCTCCCGGACGGGGCGGCTGACCGGGCGGGGGCTGCCCGCCACCTCCCTCCCAGACGGGGCGGCTGACCGGGCGGGGGCTGCCCCCCACCTCCCGGACGGGGCAGCTGCCGGGCGGAGACGCTCCTCACTTCCCAGACGAAGCGGCTGCCGGGCGGAGGGGCTCCTCACTTCTCAGACAGGGCGGACGGGCAGAGATGCTCCTCACCTCCCAGATGGGGTCGCGGCTGGGCAGAGACACTCCTCAGTTCCCAGACAGGGTCGTGGCCGGGCAGAGGCGCTCCCCACATCCCAGACGATGGGCGGCTGGGCGGAGACGCTCCTCACTTCCTAGACGGGATGACGGCCGGGCAGAGGCGCTCCTCACATCCCAGACAATGGGCGGCCAGGCAGAGACGCTCCTCACTTCCCAGACGGGGTGGTGGCCGGGCAGAGGCTGCAATCTCGGCACTTTGGGAGGCCAAGGCAGGCGGCTGGGAGGTGGAGGTTGTAGCGAGCCGAGATCACGCCACTGCACTCCAGCC;BND_DEPTH=158;MATE_BND_DEPTH=109");

		//should now be normalized
		sv = test_file_normalize_manta_[22];
		S_EQUAL(sv.chr1().str(), "chr3");
		S_EQUAL(sv.chr2().str(), "chr6");
		I_EQUAL(sv.start1(), 26428472);
		I_EQUAL(sv.start2(), 77012442);
		I_EQUAL(sv.end1(), 26428844);
		I_EQUAL(sv.end2(), 77013004);
		S_EQUAL(sv.annotations().at(idx_name_a), "MantaBND:36254:0:1:0:0:0:1");
		S_EQUAL(sv.annotations().at(idx_name_b), "MantaBND:36254:0:1:0:0:0:0");
		S_EQUAL(sv.annotations().at(idx_ref_a), "C");
		S_EQUAL(sv.annotations().at(idx_ref_b), "A");
		S_EQUAL(sv.annotations().at(idx_alt_a), "[CHR6:77012723[C");
		S_EQUAL(sv.annotations().at(idx_alt_b), "[CHR3:26428658[A");
		S_EQUAL(sv.annotations().at(idx_info_a), "SVTYPE=BND;MATEID=MantaBND:36254:0:1:0:0:0:0;IMPRECISE;CIPOS=-186,186;BND_DEPTH=115;MATE_BND_DEPTH=108");
		S_EQUAL(sv.annotations().at(idx_info_b), "SVTYPE=BND;MATEID=MantaBND:36254:0:1:0:0:0:1;IMPRECISE;CIPOS=-281,281;BND_DEPTH=108;MATE_BND_DEPTH=115");

		sv = test_file_normalize_manta_[47];
		S_EQUAL(sv.chr1().str(), "chr3");
		S_EQUAL(sv.chr2().str(), "chr16");
		I_EQUAL(sv.start1(), 25897584);
		I_EQUAL(sv.start2(), 19321304);
		I_EQUAL(sv.end1(), 25898021);
		I_EQUAL(sv.end2(), 19322082);
		S_EQUAL(sv.annotations().at(idx_name_a), "MantaBND:16017:1:12:0:0:0:1");
		S_EQUAL(sv.annotations().at(idx_name_b), "MantaBND:16017:1:12:0:0:0:0");
		S_EQUAL(sv.annotations().at(idx_ref_a), "A");
		S_EQUAL(sv.annotations().at(idx_ref_b), "A");
		S_EQUAL(sv.annotations().at(idx_alt_a), "A[CHR16:19321693[");
		S_EQUAL(sv.annotations().at(idx_alt_b), "]CHR3:25897802]A");
		S_EQUAL(sv.annotations().at(idx_info_a), "SVTYPE=BND;MATEID=MantaBND:16017:1:12:0:0:0:0;IMPRECISE;CIPOS=-218,219;BND_DEPTH=98;MATE_BND_DEPTH=176");
		S_EQUAL(sv.annotations().at(idx_info_b), "SVTYPE=BND;MATEID=MantaBND:16017:1:12:0:0:0:1;IMPRECISE;CIPOS=-389,389;BND_DEPTH=176;MATE_BND_DEPTH=98");

		test_file_normalize_manta_.store("out/BedpeFile_normalize_out1_manta.bedpe");
		COMPARE_FILES("out/BedpeFile_normalize_out1_manta.bedpe", TESTDATA("data_out/BedpeFile_normalize_out1_manta.bedpe"));

		test_file_normalize_sniffles_.load(TESTDATA("data_in/BedpeFile_normalize_sniffles.bedpe"));
		IS_FALSE(test_file_normalize_sniffles_.validate(false));
		test_file_normalize_sniffles_.normalize();
		IS_TRUE(test_file_normalize_sniffles_.validate());

		test_file_normalize_sniffles_.store("out/BedpeFile_normalize_out2_sniffles.bedpe");
		COMPARE_FILES("out/BedpeFile_normalize_out1_manta.bedpe", TESTDATA("data_out/BedpeFile_normalize_out1_manta.bedpe"));

	}

	TEST_METHOD(validate)
	{
		IS_FALSE(test_file_germl_.validate(false));
		IS_TRUE(test_file_som_.validate(false));
		test_file_lrgs_.load(TESTDATA("data_in/SV_Sniffles_germline.bedpe"));
		IS_FALSE(test_file_lrgs_.validate(false));


		//test single (invalid) SVs
		test_file_validate_manta_.load(TESTDATA("data_in/BedpeFile_validate_manta.bedpe"));
		IS_TRUE(test_file_validate_manta_[0].isValid()); //valid
		IS_TRUE(test_file_validate_manta_[1].isValid()); //valid
		IS_FALSE(test_file_validate_manta_[2].isValid(false)); //wrong order (same chr)
		IS_FALSE(test_file_validate_manta_[3].isValid(false)); //wrong order
		IS_TRUE(test_file_validate_manta_[4].isValid()); //valid
		IS_TRUE(test_file_validate_manta_[5].isValid()); //valid
		IS_FALSE(test_file_validate_manta_[6].isValid(false)); //pos2 > 1
		IS_FALSE(test_file_validate_manta_[7].isValid(false)); //start1 > end1
		IS_FALSE(test_file_validate_manta_[8].isValid(false)); //pos outside CI
		IS_TRUE(test_file_validate_manta_[9].isValid()); //valid
		IS_FALSE(test_file_validate_manta_[10].isValid(false)); //start2 > end2
		IS_FALSE(test_file_validate_manta_[11].isValid(false)); //pos1 > pos2
		IS_TRUE(test_file_validate_manta_[12].isValid()); //valid
		IS_TRUE(test_file_validate_manta_[13].isValid()); //valid

		test_file_validate_sniffles_.load(TESTDATA("data_in/BedpeFile_validate_sniffles.bedpe"));
		IS_FALSE(test_file_validate_sniffles_[0].isValid(false)); //2nd intervall >1
		IS_TRUE(test_file_validate_sniffles_[1].isValid(false)); //valid
		IS_FALSE(test_file_validate_sniffles_[2].isValid(false)); //2nd pos outside CI♣
		IS_FALSE(test_file_validate_sniffles_[3].isValid(false)); //break points not ordered
		IS_FALSE(test_file_validate_sniffles_[4].isValid(false)); //break points not ordered
		IS_FALSE(test_file_validate_sniffles_[5].isValid(false)); //break points not ordered
		IS_TRUE(test_file_validate_sniffles_[6].isValid(false)); //valid
		IS_TRUE(test_file_validate_sniffles_[7].isValid(false)); //valid
		IS_FALSE(test_file_validate_sniffles_[8].isValid(false)); //DEL with 2 chrs
		IS_FALSE(test_file_validate_sniffles_[9].isValid(false)); //pos1 > pos2

	}

};
