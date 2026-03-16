#include "TestFramework.h"
#include "BedpeFile.h"

TEST_CLASS(BedpeLine_Test)
{
private:

	TEST_METHOD(load)
	{
		BedpeFile file;
		file.load(TESTDATA("data_in/panel_svs.bedpe"));
		I_EQUAL(file.count(), 1);
		I_EQUAL(file.headers().count(), 131);
		I_EQUAL(file.annotationHeaders().count(), 22);
	}

	TEST_METHOD(loadHeaderOnly)
	{
		BedpeFile file;
		file.loadHeaderOnly(TESTDATA("data_in/panel_svs.bedpe"));
		I_EQUAL(file.count(), 0);
		I_EQUAL(file.headers().count(), 131);
		I_EQUAL(file.annotationHeaders().count(), 22);
	}

	TEST_METHOD(build)
	{
		BedpeFile file;
		file.loadHeaderOnly(TESTDATA("data_in/panel_svs.bedpe"));
		S_EQUAL(file.build(), "GRCh37");

		file.loadHeaderOnly(TESTDATA("data_in/panel_svs_dragen.bedpe"));
		S_EQUAL(file.build(), "GRCh38");

		file.loadHeaderOnly(TESTDATA("data_in/SV_Sniffles_germline.bedpe"));
		S_EQUAL(file.build(), "GRCh38");
	}

	TEST_METHOD(caller)
	{
		BedpeFile file;
		file.loadHeaderOnly(TESTDATA("data_in/panel_svs.bedpe"));
		S_EQUAL(file.caller(), "Manta");

		file.loadHeaderOnly(TESTDATA("data_in/panel_svs_dragen.bedpe"));
		S_EQUAL(file.caller(), "DRAGEN");

		file.loadHeaderOnly(TESTDATA("data_in/SV_Sniffles_germline.bedpe"));
		S_EQUAL(file.caller(), "Sniffles");
	}

	TEST_METHOD(callerVersion)
	{
		BedpeFile file;
		file.loadHeaderOnly(TESTDATA("data_in/panel_svs.bedpe"));
		S_EQUAL(file.callerVersion(), "1.6.0");

		file.loadHeaderOnly(TESTDATA("data_in/panel_svs_dragen.bedpe"));
		S_EQUAL(file.callerVersion(), "4.3.16");

		file.loadHeaderOnly(TESTDATA("data_in/SV_Sniffles_germline.bedpe"));
		S_EQUAL(file.callerVersion(), "2.0.7");
	}

	TEST_METHOD(callingDate)
	{
		BedpeFile file;
		file.loadHeaderOnly(TESTDATA("data_in/panel_svs.bedpe"));
		S_EQUAL(file.callingDate().toString("yyyyMMdd"), "20201021");

		file.loadHeaderOnly(TESTDATA("data_in/panel_svs_dragen.bedpe"));
		S_EQUAL(file.callingDate().toString("yyyyMMdd"), "20250402");

		file.loadHeaderOnly(TESTDATA("data_in/SV_Sniffles_germline.bedpe"));
		S_EQUAL(file.callingDate().toString("yyyyMMdd"), "20240127");
	}

	TEST_METHOD(toTsv)
	{
		QByteArrayList raw = {"BND00001285", ".", ".", ".", "BND", "PASS", "BND00001285", "A", "A[CHR1:144534771[", ".", ".", ".", "CIEND=-619,619;CIPOS=-619,619;CHR2=chr1;END=144534771;PE=11;MAPQ=24;CT=3to5;IMPRECISE;SVTYPE=BND;POS=5765244;SVMETHOD=EMBL.DELLYv0.8.1", ".", "GT:GL:GQ:FT:RC:RCL:RCR:CN:DR:DV:RR:RV","0/1:-15.1235,0,-127.373:151:PASS:153:84:147:1:31:11:0:0"};
		BedpeLine test("chr2", 5764625, 5765863, "chr1", -1, -1, StructuralVariantType::UNKNOWN, raw);
		S_EQUAL("chr2\t5764625\t5765863\tchr1\t.\t.\t" + raw.join("\t"), test.toTsv());
	}

	TEST_METHOD(operator_lessthan)
	{
		BedpeLine first =  BedpeLine("chr3",123,1243,"chr4",41240,1242421, StructuralVariantType::UNKNOWN,{});
		BedpeLine second = BedpeLine("chr6",12,123,"chr4",41240,1242421, StructuralVariantType::UNKNOWN,{});
		IS_TRUE( first < second);
		IS_FALSE( second < first );

		first =  BedpeLine("chr3",12454,1243,"chr4",41240,1242421, StructuralVariantType::UNKNOWN, {});
		second = BedpeLine("chr3",124540,1243,"chr4",41240,1242421, StructuralVariantType::UNKNOWN,{});
		IS_TRUE(first < second);
		IS_FALSE( second < first );

		first =  BedpeLine("chr3",124540,1243,"chr4",41240,1242421, StructuralVariantType::UNKNOWN,{});
		second = BedpeLine("chr3",124540,1243,"chr5",41240,1242421, StructuralVariantType::UNKNOWN,{});
		IS_TRUE(first < second);
		IS_FALSE( second < first );

		first =  BedpeLine("chr3",124540,1243,"chr4",41240,1242421, StructuralVariantType::UNKNOWN,{});
		second = BedpeLine("chr3",124540,1243,"chr4",49240,1242421, StructuralVariantType::UNKNOWN,{});
		IS_TRUE(first < second);
		IS_FALSE( second < first );

		first =  BedpeLine("chr3",124540,1243,"chr4",49240,1242421, StructuralVariantType::UNKNOWN,{});
		second = BedpeLine("chr3",124540,12434501,"chr4",49240,124242457, StructuralVariantType::UNKNOWN,{});
		IS_FALSE(first < second);

		first =  BedpeLine("chr3",124540,1243,"chr4",41240,1242421, StructuralVariantType::UNKNOWN,{});
		second = BedpeLine("chr3",124540,1243,"chr4",41240,1242421, StructuralVariantType::UNKNOWN,{"ANNOTATION1","ANNOTATION2"});
		IS_FALSE(first < second);
		IS_FALSE(second < first);
	}

	TEST_METHOD(genotype)
	{
		BedpeFile file;
		file.load(TESTDATA("data_in/panel_svs.bedpe"));

		QString gt = file[0].genotype(file.annotationHeaders());
		S_EQUAL(gt, "0/1");

		gt = file[0].genotypeHumanReadable(file.annotationHeaders());
		S_EQUAL(gt, "het");
	}

	TEST_METHOD(setGenotype)
	{
		BedpeFile file;
		file.load(TESTDATA("data_in/panel_svs.bedpe"));

		file[0].setGenotype(file.annotationHeaders(), "1/1");

		QString gt = file[0].genotype(file.annotationHeaders());
		S_EQUAL(gt, "1/1");

		gt = file[0].genotypeHumanReadable(file.annotationHeaders());
		S_EQUAL(gt, "hom");
	}

	TEST_METHOD(genes)
	{
		BedpeFile file;
		file.load(TESTDATA("data_in/panel_svs.bedpe"));

		GeneSet genes = file[0].genes(file.annotationHeaders());
		I_EQUAL(genes.count(), 2);
		IS_TRUE(genes.contains("BTBD7"));
		IS_TRUE(genes.contains("SLC2A5"));
	}

	TEST_METHOD(setGenes)
	{
		BedpeFile file;
		file.load(TESTDATA("data_in/panel_svs.bedpe"));

		file[0].setGenes(file.annotationHeaders(), GeneSet() << "A" << "B" << "C");

		GeneSet genes = file[0].genes(file.annotationHeaders());
		I_EQUAL(genes.count(), 3);
		IS_TRUE(genes.contains("A"));
		IS_TRUE(genes.contains("B"));
		IS_TRUE(genes.contains("C"));
	}

	TEST_METHOD(normalize)
	{
		BedpeFile file;
		file.load(TESTDATA("data_in/panel_sniffles.bedpe"));

		int idx_info_a = file.annotationIndexByName("INFO_A");
		int idx_info_b = file.annotationIndexByName("INFO_B");
		int idx_name_a = file.annotationIndexByName("NAME_A");
		int idx_name_b = file.annotationIndexByName("NAME_B");
		int idx_alt_a = file.annotationIndexByName("ALT_A");
		int idx_alt_b = file.annotationIndexByName("ALT_B");

		file[0].normalize(file.annotationHeaders(), false);
		IS_TRUE(file[0].chr1() == Chromosome("chr1"));
		I_EQUAL(file[0].start1(), 4336500);
		I_EQUAL(file[0].end1(), 4336502);
		IS_TRUE(file[0].chr2() == Chromosome("chr1"));
		I_EQUAL(file[0].start2(), 4336501);
		I_EQUAL(file[0].end2(), 4336501);
		S_EQUAL(file[0].annotations().at(idx_info_a), "PRECISE;SVTYPE=INS;POS=4336501;SVLEN=615;END=4336501;SUPPORT=37;RNAMES=00804bcb-f373-4494-a004-d750ecf26263,050250c9-216a-45ec-9b8e-b72bb1017222,0a752a64-d0e6-481d-a4d8-c47f8cc7d863,0e971b4b-4fa0-4d02-8336-1c5fcb9ebdc7,18baa970-fe22-4ee8-a103-ecd6072977eb,1dde3cee-912a-4826-bd60-797d9b2311b1,25b747bd-eaf9-405d-aa29-8a5579b34d7b,25cad1b1-f370-42a5-9568-3b8263a1c0fd,36fea12e-006c-45c5-a2fe-647e30f50978,37fce301-8015-4af9-8592-8c61cb2373ba,3922c81f-40b4-4f93-ba3e-790ca072add1,423bb245-fed7-4381-b3bb-1e622c6dc48d,498e7931-3a28-4577-b5d4-ee3f6a8176ef,4cb94b7a-88de-4233-99d1-178737d992e3,518af3d5-6504-4ccf-905b-7a7d7b9c52eb,51b36af8-a8c0-45d1-925e-75957431af5f,57acd48c-ff1e-48ec-b2b9-9eec31a02694,5b575598-5956-4a83-8bb9-3f2d39337f5f,6b943e09-29a5-42a4-ac82-bdc0fc8ea43c,6bbf8bcb-9996-4d89-af1f-40ee7e310878,707df45f-0fb5-493f-b041-1a086f9e899f,7c7ae8f4-abe3-4e5f-895c-6f5740740aae,8bb39e5f-30f8-4346-960d-259c76b28509,8c701e92-7aca-4cc2-994c-78db79268037,8efa51a0-6de6-421e-a92e-26ffb219d261,9526e3d5-4b39-451d-b295-3a40bff0aad3,9d9b9e33-9fed-45b8-8d93-0f3c35cb7553,a4bbc1aa-4fd1-4432-ac04-4c03350cdf57,a6a1583c-76fe-4a07-9efd-c62bf0992a26,ba0a9525-029a-45fe-a570-f64753e97cb9,ba4014c0-a0b1-41a9-b083-41cfac53dc82,bdc84715-b4d8-4642-8e6e-1e88c1c0233f,d11cd992-192a-491b-a740-576a83b09828,e16f54ef-8a05-45c1-a566-e6d05e0177e9,f390d171-c83f-42c8-b75e-e3a1128b26b1,f92b84b1-2691-44fd-8c75-234deddb55e6,fe131eb9-aca5-4d07-b3b6-5c1b3d327ec8;COVERAGE=45,44,44,44,42;STRAND=+-;NM=0.006;STDEV_LEN=1.433;STDEV_POS=0.000;SUPPORT_LONG=0;VAF=0.841");
		S_EQUAL(file[0].annotations().at(idx_info_b), ".");
		S_EQUAL(file[0].annotations().at(idx_name_a), "Sniffles2.INS.13BS0");
		S_EQUAL(file[0].annotations().at(idx_name_b), ".");
		S_EQUAL(file[0].annotations().at(idx_alt_a), "TGGTGATGATAATGGTGATGGTGATGGTAAACATGATAACGACAATGATAGTGGAAAGCGAATGGTGGTATTGATGGTGATGATGAAGAGGATGGTGATAATAATGATGATGGTGATGATTATGTTGGTGATGATGGTAATGGTGATGATAATGATGGTGATGGTGATGATGATGATTATGTGATGATGGTGGTGATGGTGATGATGGTGATGGTGATGGTGATGGTGGTGATGGTGGTGATGATGATGGTGATGGTGATAATGGTGATGGTGGTGATGATGATGGCGGGGATGTGATGATGATGGTGATGATGATGATGATGGAGATGATGATGATGGTGATGATGATGATGGTGATGATGTGATGACGATGATAGTGATTATGGTGATGGTGATGATGATGATGGTGATGGTGGTGATGGTGATGATGTGATGGTGATGGTGATGATAATAATGATGTGATGGTGATGATGGCGGTGATGTGATGATGATGGTGATGGTGATGATAATGGAGATGATGATGGTGATGATGGTGATGATGTGATGATGACGGTGATGGTGATGATGATGATGGTGATGGTGGTGATGGTAGTGATAATGTGATAATGATGGTGAC");
		S_EQUAL(file[0].annotations().at(idx_alt_b), ".");

		file[3].normalize(file.annotationHeaders(), false);
		IS_TRUE(file[3].chr1() == Chromosome("chr5"));
		I_EQUAL(file[3].start1(), 18223523);
		I_EQUAL(file[3].end1(), 18223523);
		IS_TRUE(file[3].chr2() == Chromosome("chr8"));
		I_EQUAL(file[3].start2(), 132238289);
		I_EQUAL(file[3].end2(), 132238339);
		S_EQUAL(file[3].annotations().at(idx_info_a), "IMPRECISE;SVTYPE=BND;POS=132238314;SUPPORT=24;RNAMES=07ca6253-9af3-4b7d-a312-a14c62d3aca7,1f68fc0e-0c07-4f5a-acfd-d1dc0eed85bf,2036c8a6-2625-4771-9226-836be56ccdd0,28e3f285-cb9f-4ede-ab64-1a9f272cafe0,2d321e61-8e18-4540-b228-28c2dba3898b,3081ae11-a178-46ad-abaf-409e742ca217,31a89744-640b-40a9-980a-e9c265b14d18,32aa0d80-81ab-4da6-bbe8-2200788d3043,35026428-11ba-4ee1-b808-d5907553e66a,3c965c9b-9daf-4faa-ae3f-60205b4ec681,3cd43008-1399-4848-b8a6-328579b3671d,5ce69ee3-4d14-4bf7-b304-95fe5eac0c0d,5f4a7b4d-5008-4ff1-87a5-a4d5401098d1,7a97e8ff-071a-42d0-b758-53ba303b4063,94337b59-6119-4925-b485-3f36a1566344,a6907f15-5df1-40b0-9825-82210f9c75b1,a78d1ce3-e1a5-4462-a6b9-a58d6bf84b36,aff7b79c-319b-4bf9-b472-ddcd1c1899c4,b23ecc3e-1f03-4b5c-b596-afd3b260bbaf,b5032057-bb13-4045-918e-692c5f33152a,da90b82a-06a7-4f78-81cd-b14491891f3b,ede7c8d3-fcac-4aa2-b832-314f46bbe165,f02bcb61-4be2-43b3-8284-2534a3d8430b,f1be675e-5b5a-491f-a828-8aed940ba915;COVERAGE=45,46,60,72,73;STRAND=+-;NM=0.128;CHR2=chr14;STDEV_POS=61.518;VAF=0.407");
		S_EQUAL(file[3].annotations().at(idx_info_b), ".");
		S_EQUAL(file[3].annotations().at(idx_name_a), "Sniffles2.BND.1F32S1");
		S_EQUAL(file[3].annotations().at(idx_name_b), ".");
		S_EQUAL(file[3].annotations().at(idx_alt_a), "]CHR14:18223523]G");
		S_EQUAL(file[3].annotations().at(idx_alt_b), ".");

		file[4].normalize(file.annotationHeaders(), false);
		IS_TRUE(file[4].chr1() == Chromosome("chr11"));
		I_EQUAL(file[4].start1(), 10210154);
		I_EQUAL(file[4].end1(), 10210154);
		IS_TRUE(file[4].chr2() == Chromosome("chr11"));
		I_EQUAL(file[4].start2(), 193218447);
		I_EQUAL(file[4].end2(), 193218447);
		S_EQUAL(file[4].annotations().at(idx_info_a), "PRECISE;MOSAIC;SVTYPE=BND;POS=193218447;SUPPORT=3;RNAMES=3184c3fd-4bae-40bf-a477-b7a66bc048a1,44905c49-4799-4b16-b040-c8bce55da4cd,ff8ddd7f-a799-43fe-a5f7-0ad4845b2904;COVERAGE=60,58,47,32,31;STRAND=+-;NM=0.004;CHR2=chr1;STDEV_POS=0.000;VAF=0.065");
		S_EQUAL(file[4].annotations().at(idx_info_b), ".");
		S_EQUAL(file[4].annotations().at(idx_name_a), "Sniffles2.BND.24A6S1");
		S_EQUAL(file[4].annotations().at(idx_name_b), ".");
		S_EQUAL(file[4].annotations().at(idx_alt_a), "C]CHR1:102101546]");
		S_EQUAL(file[4].annotations().at(idx_alt_b), ".");

		file[5].normalize(file.annotationHeaders(), false);
		IS_TRUE(file[5].chr1() == Chromosome("chr22"));
		I_EQUAL(file[5].start1(), 2212289);
		I_EQUAL(file[5].end1(), 2212289);
		IS_TRUE(file[5].chr2() == Chromosome("chrY"));
		I_EQUAL(file[5].start2(), 5606161);
		I_EQUAL(file[5].end2(), 5606161);
		S_EQUAL(file[5].annotations().at(idx_info_a), "PRECISE;SVTYPE=BND;POS=5606161;SUPPORT=8;RNAMES=2defe44e-e0c1-4056-a954-1ae1a056c7a4,67e000a9-9712-4d90-9b89-509f5e19e49d,7e7e48ef-27a7-46a9-b2f3-58535ae389af,9609ce48-c282-425d-974b-1d5907c4f44d,9b1076be-a955-441a-8510-297e1b9196ac,abc447e0-0950-43ed-90a7-f06ad46db3c9,bfedf3c8-09b5-4a51-a318-4cdcdd3feef1,e30deeb5-e791-4981-ac0e-6acd4a00e2a3;COVERAGE=21,21,35,39,38;STRAND=+-;NM=0.004;CHR2=chr22;STDEV_POS=0.000;VAF=0.250");
		S_EQUAL(file[5].annotations().at(idx_info_b), ".");
		S_EQUAL(file[5].annotations().at(idx_name_a), "Sniffles2.BND.708S17");
		S_EQUAL(file[5].annotations().at(idx_name_b), ".");
		S_EQUAL(file[5].annotations().at(idx_alt_a), "T[CHR22:22122892[");
		S_EQUAL(file[5].annotations().at(idx_alt_b), ".");

		file.load(TESTDATA("data_in/panel_dragen.bedpe"));

		idx_info_a = file.annotationIndexByName("INFO_A");
		idx_info_b = file.annotationIndexByName("INFO_B");
		idx_name_a = file.annotationIndexByName("NAME_A");
		idx_name_b = file.annotationIndexByName("NAME_B");
		idx_alt_a = file.annotationIndexByName("ALT_A");
		idx_alt_b = file.annotationIndexByName("ALT_B");

		file[0].normalize(file.annotationHeaders(), true);
		IS_TRUE(file[0].chr1() == Chromosome("chr1"));
		I_EQUAL(file[0].start1(), 17437673);
		I_EQUAL(file[0].end1(), 17437681);
		IS_TRUE(file[0].chr2() == Chromosome("chr1"));
		I_EQUAL(file[0].start2(), 17437673);
		I_EQUAL(file[0].end2(), 17437673);
		S_EQUAL(file[0].annotations().at(idx_info_a), "END=17437673;SVTYPE=INS;POS=17437673;SVLEN=78;CIGAR=1M78I;CONTIG=TCACTTGGGGGGGCTACAACAGAGCCCTCGCGAGCATCCTGCAGCTTCGGACCACCGGCGGCAAACAAAGCCCGAGCACCGCTCAGCCGGGGGGCTTCCCCGACCTCGGGGGAGGGCTCTGCGGAGCATGCGCCCTCGCACCCCGCCCCGGCGCCCGCCGCCCCCGCCTTCCCGGCGCGCGAGCCGGCGCAGGGCCGCACGGGGCATGCGCGGCGGCCGTCAGGCCCCGCCCCCCCCGGGCGCCGGAGCCGAGGCGGCGGGAACCTCAAAGCCCCGGCGCAAACGGCCGCTCCCCGCAGAGCGCCGGCCGCCCCCTCCCCGCGGCGCCCGGGC;CIPOS=0,8;HOMLEN=8;HOMSEQ=GCATGCGC");
		S_EQUAL(file[0].annotations().at(idx_info_b), ".");
		S_EQUAL(file[0].annotations().at(idx_name_a), "MantaINS:1090:0:0:0:0:0");
		S_EQUAL(file[0].annotations().at(idx_name_b), ".");
		S_EQUAL(file[0].annotations().at(idx_alt_a), "AGCATGCGCCCTCGCACCCCGCCCCGGCGCCCGCCGCCCCCGCCTTCCCGGCGCGCGAGCCGGCGCAGGGCCGCACGGG");
		S_EQUAL(file[0].annotations().at(idx_alt_b), ".");

		file[3].normalize(file.annotationHeaders(), true);
		IS_TRUE(file[3].chr1() == Chromosome("chr1"));
		I_EQUAL(file[3].start1(), 105963329);
		I_EQUAL(file[3].end1(), 105963338);
		IS_TRUE(file[3].chr2() == Chromosome("chr4"));
		I_EQUAL(file[3].start2(), 89640132);
		I_EQUAL(file[3].end2(), 89640141);
		S_EQUAL(file[3].annotations().at(idx_info_a), "SVTYPE=BND;MATEID=MantaBND:5342:0:1:0:2:0:1;CONTIG=CATATATAATAGGTATATTATATATAAAATATGCATAGTATTGCAGGCAGCCTATTGTGGGACATATATATATATGTGTGTATATATACGTATATATATACGTATATATGTGTGTATATATACGTATATATATACGTATATATGTGTGTATATATACGTATATATACGTATATATGTGTGTATATATACGTATATATATACGTATATATGTGTGTATCTATACG;CIPOS=0,9;HOMLEN=9;HOMSEQ=TATATATAT;BND_DEPTH=30;MATE_BND_DEPTH=40");
		S_EQUAL(file[3].annotations().at(idx_info_b), "SVTYPE=BND;MATEID=MantaBND:5342:0:1:0:2:0:0;CONTIG=CATATATAATAGGTATATTATATATAAAATATGCATAGTATTGCAGGCAGCCTATTGTGGGACATATATATATATGTGTGTATATATACGTATATATATACGTATATATGTGTGTATATATACGTATATATATACGTATATATGTGTGTATATATACGTATATATACGTATATATGTGTGTATATATACGTATATATATACGTATATATGTGTGTATCTATACG;CIPOS=0,9;HOMLEN=9;HOMSEQ=ATATATATA;BND_DEPTH=40;MATE_BND_DEPTH=30");
		S_EQUAL(file[3].annotations().at(idx_name_a), "MantaBND:5342:0:1:0:2:0:0");
		S_EQUAL(file[3].annotations().at(idx_name_b), "MantaBND:5342:0:1:0:2:0:1");
		S_EQUAL(file[3].annotations().at(idx_alt_a), "G]CHR4:89640141]");
		S_EQUAL(file[3].annotations().at(idx_alt_b), "T]CHR1:105963338]");

		file[4].normalize(file.annotationHeaders(), true);
		IS_TRUE(file[4].chr1() == Chromosome("chr1"));
		I_EQUAL(file[4].start1(), 226227761);
		I_EQUAL(file[4].end1(), 226227773);
		IS_TRUE(file[4].chr2() == Chromosome("chr19"));
		I_EQUAL(file[4].start2(), 44272062);
		I_EQUAL(file[4].end2(), 44272074);
		S_EQUAL(file[4].annotations().at(idx_info_a), "SVTYPE=BND;MATEID=MantaBND:10014:0:1:0:0:0:0;CONTIG=ACACAGTCTTGCTCTGTTGCCCAGCTCTTGGTTGCTTTAAATGGTGGATTTGCTACCCTGAAAAGTAAAGGCCAGGCGTGGTGGCTCACACCTGTAATCCCAGCACTTTGGGAGGCCAAGGCAGGCAGATGACCTGAGGTCAGGAGCTCGAGACCAGCCTGGCCAACATGGTGAAACCCTGTCTCTACTAAAAATACAAAATTAGCTGGGCGTGGTGACGGGCGCCTGTAATCCCAGCTACTCGGGAGGCTGAGGCAGGAGAATCGCTTGAACCCGGGAGGCAGAGGTTGCAGTGAGCCAAGATCATGCCACTACACTCCAGCCTGGGTGAC;CIPOS=0,12;HOMLEN=12;HOMSEQ=TTTTGTATTTTT;BND_DEPTH=67;MATE_BND_DEPTH=37");
		S_EQUAL(file[4].annotations().at(idx_info_b), "SVTYPE=BND;MATEID=MantaBND:10014:0:1:0:0:0:1;CONTIG=ACACAGTCTTGCTCTGTTGCCCAGCTCTTGGTTGCTTTAAATGGTGGATTTGCTACCCTGAAAAGTAAAGGCCAGGCGTGGTGGCTCACACCTGTAATCCCAGCACTTTGGGAGGCCAAGGCAGGCAGATGACCTGAGGTCAGGAGCTCGAGACCAGCCTGGCCAACATGGTGAAACCCTGTCTCTACTAAAAATACAAAATTAGCTGGGCGTGGTGACGGGCGCCTGTAATCCCAGCTACTCGGGAGGCTGAGGCAGGAGAATCGCTTGAACCCGGGAGGCAGAGGTTGCAGTGAGCCAAGATCATGCCACTACACTCCAGCCTGGGTGAC;CIPOS=0,12;HOMLEN=12;HOMSEQ=AAAAATACAAAA;BND_DEPTH=37;MATE_BND_DEPTH=67");
		S_EQUAL(file[4].annotations().at(idx_name_a), "MantaBND:10014:0:1:0:0:0:1");
		S_EQUAL(file[4].annotations().at(idx_name_b), "MantaBND:10014:0:1:0:0:0:0");
		S_EQUAL(file[4].annotations().at(idx_alt_a), "A]CHR19:44272074]");
		S_EQUAL(file[4].annotations().at(idx_alt_b), "T]CHR1:226227773]");
	}
};
