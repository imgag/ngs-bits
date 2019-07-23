#include "TestFramework.h"
#include "BedpeFile.h"

TEST_CLASS(BedpeLine_Test)
{
Q_OBJECT
private slots:
	void constructors()
	{
		QByteArrayList raw = {"chr2", "5764625", "5765863", "chr1", ".", ".", "BND00001285", ".", ".", ".", "BND", "PASS", "BND00001285", "A", "A[CHR1:144534771[", ".", ".", ".", "CIEND=-619,619;CIPOS=-619,619;CHR2=chr1;END=144534771;PE=11;MAPQ=24;CT=3to5;IMPRECISE;SVTYPE=BND;POS=5765244;SVMETHOD=EMBL.DELLYv0.8.1", ".", "GT:GL:GQ:FT:RC:RCL:RCR:CN:DR:DV:RR:RV","0/1:-15.1235,0,-127.373:151:PASS:153:84:147:1:31:11:0:0"};

		BedpeLine test(raw);

		//check coordinates
		I_EQUAL(2, test.chr1().num());
		I_EQUAL(5764625, test.start1());
		I_EQUAL(5765863, test.end1());

		I_EQUAL(1, test.chr2().num());
		I_EQUAL(-1, test.start2());
		I_EQUAL(-1, test.end2());

		QByteArrayList expected_annotations = {"BND00001285", ".", ".", ".", "BND", "PASS", "BND00001285", "A", "A[CHR1:144534771[", ".", ".", ".", "CIEND=-619,619;CIPOS=-619,619;CHR2=chr1;END=144534771;PE=11;MAPQ=24;CT=3to5;IMPRECISE;SVTYPE=BND;POS=5765244;SVMETHOD=EMBL.DELLYv0.8.1", ".", "GT:GL:GQ:FT:RC:RCL:RCR:CN:DR:DV:RR:RV","0/1:-15.1235,0,-127.373:151:PASS:153:84:147:1:31:11:0:0"};

		for(int i=0;i<test.annotations().count();++i)
		{
			S_EQUAL(expected_annotations[i],test.annotations()[i]);
		}
	}

	void toTsv()
	{
		QByteArrayList raw = {"chr2", "5764625", "5765863", "chr1", ".", ".", "BND00001285", ".", ".", ".", "BND", "PASS", "BND00001285", "A", "A[CHR1:144534771[", ".", ".", ".", "CIEND=-619,619;CIPOS=-619,619;CHR2=chr1;END=144534771;PE=11;MAPQ=24;CT=3to5;IMPRECISE;SVTYPE=BND;POS=5765244;SVMETHOD=EMBL.DELLYv0.8.1", ".", "GT:GL:GQ:FT:RC:RCL:RCR:CN:DR:DV:RR:RV","0/1:-15.1235,0,-127.373:151:PASS:153:84:147:1:31:11:0:0"};
		BedpeLine test(raw);
		S_EQUAL(raw.join("\t"),test.toTsv());
	}

	void operator_lessthan()
	{
		BedpeLine first =  BedpeLine("chr3",123,1243,"chr4",41240,1242421,{});
		BedpeLine second = BedpeLine("chr6",12,123,"chr4",41240,1242421,{});
		IS_TRUE( first < second);
		IS_FALSE( second < first );

		first =  BedpeLine("chr3",12454,1243,"chr4",41240,1242421,{});
		second = BedpeLine("chr3",124540,1243,"chr4",41240,1242421,{});
		IS_TRUE(first < second);
		IS_FALSE( second < first );

		first =  BedpeLine("chr3",124540,1243,"chr4",41240,1242421,{});
		second = BedpeLine("chr3",124540,1243,"chr5",41240,1242421,{});
		IS_TRUE(first < second);
		IS_FALSE( second < first );

		first =  BedpeLine("chr3",124540,1243,"chr4",41240,1242421,{});
		second = BedpeLine("chr3",124540,1243,"chr4",49240,1242421,{});
		IS_TRUE(first < second);
		IS_FALSE( second < first );

		first =  BedpeLine("chr3",124540,1243,"chr4",49240,1242421,{});
		second = BedpeLine("chr3",124540,12434501,"chr4",49240,124242457,{});
		IS_FALSE(first < second);

		first =  BedpeLine("chr3",124540,1243,"chr4",41240,1242421,{});
		second = BedpeLine("chr3",124540,1243,"chr4",41240,1242421,{"ANNOTATION1","ANNOTATION2"});
		IS_FALSE(first < second);
		IS_FALSE(second < first);
	}

};
