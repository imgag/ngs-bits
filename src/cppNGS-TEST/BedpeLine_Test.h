#include "TestFramework.h"
#include "BedpeFile.h"

TEST_CLASS(BedpeLine_Test)
{
private:

	void load()
	{
		BedpeFile file;
		file.load(TESTDATA("data_in/panel_svs.bedpe"));
		I_EQUAL(file.count(), 1);
		I_EQUAL(file.headers().count(), 131);
		I_EQUAL(file.annotationHeaders().count(), 22);
	}

	void loadHeaderOnly()
	{
		BedpeFile file;
		file.loadHeaderOnly(TESTDATA("data_in/panel_svs.bedpe"));
		I_EQUAL(file.count(), 0);
		I_EQUAL(file.headers().count(), 131);
		I_EQUAL(file.annotationHeaders().count(), 22);
	}

	void build()
	{
		BedpeFile file;
		file.loadHeaderOnly(TESTDATA("data_in/panel_svs.bedpe"));
		S_EQUAL(file.build(), "GRCh37");

		file.loadHeaderOnly(TESTDATA("data_in/panel_svs_dragen.bedpe"));
		S_EQUAL(file.build(), "GRCh38");

		file.loadHeaderOnly(TESTDATA("data_in/SV_Sniffles_germline.bedpe"));
		S_EQUAL(file.build(), "GRCh38");
	}

	void caller()
	{
		BedpeFile file;
		file.loadHeaderOnly(TESTDATA("data_in/panel_svs.bedpe"));
		S_EQUAL(file.caller(), "Manta");

		file.loadHeaderOnly(TESTDATA("data_in/panel_svs_dragen.bedpe"));
		S_EQUAL(file.caller(), "DRAGEN");

		file.loadHeaderOnly(TESTDATA("data_in/SV_Sniffles_germline.bedpe"));
		S_EQUAL(file.caller(), "Sniffles");
	}

	void callerVersion()
	{
		BedpeFile file;
		file.loadHeaderOnly(TESTDATA("data_in/panel_svs.bedpe"));
		S_EQUAL(file.callerVersion(), "1.6.0");

		file.loadHeaderOnly(TESTDATA("data_in/panel_svs_dragen.bedpe"));
		S_EQUAL(file.callerVersion(), "4.3.16");

		file.loadHeaderOnly(TESTDATA("data_in/SV_Sniffles_germline.bedpe"));
		S_EQUAL(file.callerVersion(), "2.0.7");
	}

	void callingDate()
	{
		BedpeFile file;
		file.loadHeaderOnly(TESTDATA("data_in/panel_svs.bedpe"));
		S_EQUAL(file.callingDate().toString("yyyyMMdd"), "20201021");

		file.loadHeaderOnly(TESTDATA("data_in/panel_svs_dragen.bedpe"));
		S_EQUAL(file.callingDate().toString("yyyyMMdd"), "20250402");

		file.loadHeaderOnly(TESTDATA("data_in/SV_Sniffles_germline.bedpe"));
		S_EQUAL(file.callingDate().toString("yyyyMMdd"), "20240127");
	}

	void toTsv()
	{
		QByteArrayList raw = {"BND00001285", ".", ".", ".", "BND", "PASS", "BND00001285", "A", "A[CHR1:144534771[", ".", ".", ".", "CIEND=-619,619;CIPOS=-619,619;CHR2=chr1;END=144534771;PE=11;MAPQ=24;CT=3to5;IMPRECISE;SVTYPE=BND;POS=5765244;SVMETHOD=EMBL.DELLYv0.8.1", ".", "GT:GL:GQ:FT:RC:RCL:RCR:CN:DR:DV:RR:RV","0/1:-15.1235,0,-127.373:151:PASS:153:84:147:1:31:11:0:0"};
		BedpeLine test("chr2", 5764625, 5765863, "chr1", -1, -1, StructuralVariantType::UNKNOWN, raw);
		S_EQUAL("chr2\t5764625\t5765863\tchr1\t.\t.\t" + raw.join("\t"), test.toTsv());
	}

	void operator_lessthan()
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

	void genotype()
	{
		BedpeFile file;
		file.load(TESTDATA("data_in/panel_svs.bedpe"));

		QString gt = file[0].genotype(file.annotationHeaders());
		S_EQUAL(gt, "0/1");

		gt = file[0].genotypeHumanReadable(file.annotationHeaders());
		S_EQUAL(gt, "het");
	}

	void setGenotype()
	{
		BedpeFile file;
		file.load(TESTDATA("data_in/panel_svs.bedpe"));

		file[0].setGenotype(file.annotationHeaders(), "1/1");

		QString gt = file[0].genotype(file.annotationHeaders());
		S_EQUAL(gt, "1/1");

		gt = file[0].genotypeHumanReadable(file.annotationHeaders());
		S_EQUAL(gt, "hom");
	}

	void genes()
	{
		BedpeFile file;
		file.load(TESTDATA("data_in/panel_svs.bedpe"));

		GeneSet genes = file[0].genes(file.annotationHeaders());
		I_EQUAL(genes.count(), 2);
		IS_TRUE(genes.contains("BTBD7"));
		IS_TRUE(genes.contains("SLC2A5"));
	}

	void setGenes()
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
};
