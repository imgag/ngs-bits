#include "TestFramework.h"
#include "BedpeFile.h"

TEST_CLASS(BedpeLine_Test)
{
Q_OBJECT
private slots:

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

};
