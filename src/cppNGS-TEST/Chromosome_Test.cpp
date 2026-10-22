#include "TestFramework.h"
#include "Chromosome.h"
#include <array>
#include <atomic>
#include <thread>

TEST_CLASS(Chromosome_Test)
{
private:

	TEST_METHOD(internedStorage)
	{
		I_EQUAL(sizeof(Chromosome), sizeof(void*));
		Chromosome first("chr1");
		Chromosome second(QString(" chr1 "));
		IS_TRUE(&first.str() == &second.str());
		IS_TRUE(first == Chromosome("1"));
		IS_TRUE(&first.str() != &Chromosome("1").str());
		Chromosome copy = first;
		IS_TRUE(&copy.str() == &first.str());
		copy = Chromosome("chrY");
		S_EQUAL(first.str(), "chr1");
		IS_TRUE(copy.isY());
		Chromosome moved = std::move(copy);
		IS_TRUE(moved.isY());
		IS_TRUE(copy.isY()); //Moving a pointer-sized value also leaves its source valid.
		IS_FALSE(Chromosome().isValid());
		IS_TRUE(&Chromosome().str() == &Chromosome("").str());
	}

	TEST_METHOD(inputOwnership)
	{
		char buffer[] = " chrOwnershipTest ";
		Chromosome first(QByteArrayView(buffer, sizeof(buffer) - 1));
		Chromosome second(QByteArray::fromRawData(buffer, sizeof(buffer) - 1));
		buffer[4] = 'Z';
		S_EQUAL(first.str(), "chrOwnershipTest");
		IS_TRUE(&first.str() == &second.str());
		IS_TRUE(&first.str() == &Chromosome(std::string("chrOwnershipTest")).str());
		IS_TRUE(&first.str() == &Chromosome(QString("chrOwnershipTest")).str());
		QByteArray normalized = first.strNormalized(false);
		normalized[0] = 'Z';
		S_EQUAL(first.strNormalized(false), "OWNERSHIPTEST");
		IS_FALSE(Chromosome(static_cast<const char*>(nullptr)).isValid());
	}

	TEST_METHOD(normalizationEdgeCases)
	{
		IS_FALSE(Chromosome("chr").isValid());
		S_EQUAL(Chromosome("chr").str(), "chr");
		S_EQUAL(Chromosome("chr").strNormalized(true), "chr");
		I_EQUAL(Chromosome("chr+1").num(), 1);
		S_EQUAL(Chromosome("chr+1").strNormalized(false), "1");
		I_EQUAL(Chromosome("chr+100").num(), 100);
		S_EQUAL(Chromosome("chr+100").strNormalized(false), "+100");
		I_EQUAL(Chromosome("1000").num(), 1000);
		IS_FALSE(Chromosome("1001").isNonSpecial());
		IS_FALSE(Chromosome("0").isAutosome());
		IS_FALSE(Chromosome("01").isAutosome());
		IS_TRUE(Chromosome("chrCaseTest") == Chromosome("CASETEST"));
		S_EQUAL(Chromosome("chrCaseTest").str(), "chrCaseTest");
		S_EQUAL(Chromosome("chrCaseTest").strNormalized(true), "chrCASETEST");
		IS_TRUE(Chromosome("chrM") == Chromosome("mt"));
	}

	TEST_METHOD(stableReferences)
	{
		const QByteArray* original = &Chromosome("chrStableReferenceTest").str();
		for (int i = 0; i < 4096; ++i)
		{
			Chromosome chr(QByteArray("chrGrowthTest_") + QByteArray::number(i));
			IS_TRUE(chr.isValid());
		}
		S_EQUAL(*original, "chrStableReferenceTest");
		IS_TRUE(original == &Chromosome("chrStableReferenceTest").str());
	}

	TEST_METHOD(concurrentConstruction)
	{
		constexpr int thread_count = 8;
		constexpr int name_count = 256;
		std::array<std::thread, thread_count> threads;
		std::array<std::array<const QByteArray*, name_count>, thread_count> strings{};
		std::array<std::array<int, name_count>, thread_count> numbers{};
		std::array<bool, thread_count> success{};
		std::atomic<int> ready{0};
		for (int t = 0; t < thread_count; ++t)
		{
			threads[t] = std::thread([&, t]()
			{
				++ready;
				while (ready.load() != thread_count) std::this_thread::yield();
				success[t] = true;
				for (int i = 0; i < name_count; ++i)
				{
					const QByteArray name = "chrConcurrentTest_" + QByteArray::number(i);
					const Chromosome chr(name);
					strings[t][i] = &chr.str();
					numbers[t][i] = chr.num();
					const Chromosome alias(name.toUpper().mid(3));
					const Chromosome unique(name + "_thread_" + QByteArray::number(t));
					const Chromosome common(i % 2 == 0 ? "chr1" : "X");
					success[t] = success[t] && alias == chr && unique != chr
						&& common.num() == (i % 2 == 0 ? 1 : 1001)
						&& chr.strNormalized(false) == name.toUpper().mid(3)
						&& &Chromosome(name).str() == strings[t][i];
				}
			});
		}
		for (auto& thread : threads) thread.join();
		for (int t = 0; t < thread_count; ++t)
		{
			IS_TRUE(success[t]);
			for (int i = 0; i < name_count; ++i)
			{
				IS_TRUE(strings[t][i] == strings[0][i]);
				I_EQUAL(numbers[t][i], numbers[0][i]);
				IS_TRUE(numbers[t][i] >= 1004);
				for (int j = 0; j < i; ++j) IS_TRUE(numbers[t][i] != numbers[t][j]);
			}
		}
	}

	TEST_METHOD(str)
	{
		S_EQUAL(Chromosome("1").str(), QByteArray("1"));
		S_EQUAL(Chromosome("chr1").str(), QByteArray("chr1"));
		S_EQUAL(Chromosome("chr1").str(), QByteArray("chr1"));
		S_EQUAL(Chromosome("CHRX").str(), QByteArray("CHRX"));
		S_EQUAL(Chromosome("chrY").str(), QByteArray("chrY"));
		S_EQUAL(Chromosome("M").str(), QByteArray("M"));
		S_EQUAL(Chromosome("MT").str(), QByteArray("MT"));

		S_EQUAL(Chromosome(QString("1")).str(), QByteArray("1"));
		S_EQUAL(Chromosome(QByteArray("1")).str(), QByteArray("1"));
	}

	TEST_METHOD(strNormalized)
	{
		S_EQUAL(Chromosome("1").strNormalized(false), QByteArray("1"));
		S_EQUAL(Chromosome("chr1").strNormalized(false), QByteArray("1"));
		S_EQUAL(Chromosome("chr01").strNormalized(false), QByteArray("01"));
		S_EQUAL(Chromosome("CHRX").strNormalized(false), QByteArray("X"));
		S_EQUAL(Chromosome("chrY").strNormalized(false), QByteArray("Y"));
		S_EQUAL(Chromosome("M").strNormalized(false), QByteArray("MT"));
        S_EQUAL(Chromosome("MT").strNormalized(false), QByteArray("MT"));

		S_EQUAL(Chromosome("1").strNormalized(true),  QByteArray("chr1"));
		S_EQUAL(Chromosome("chr1").strNormalized(true), QByteArray("chr1"));
		S_EQUAL(Chromosome("CHRX").strNormalized(true), QByteArray("chrX"));
		S_EQUAL(Chromosome("chrY").strNormalized(true), QByteArray("chrY"));
		S_EQUAL(Chromosome("M").strNormalized(true), QByteArray("chrMT"));
        S_EQUAL(Chromosome("MT").strNormalized(true), QByteArray("chrMT"));
	}

	TEST_METHOD(num)
	{
		I_EQUAL(Chromosome("1").num(), 1);
		I_EQUAL(Chromosome("chr1").num(),1);
		I_EQUAL(Chromosome("X").num(),1001);
		I_EQUAL(Chromosome("CHRX").num(),1001);
		I_EQUAL(Chromosome("Y").num(),1002);
		I_EQUAL(Chromosome("chrY").num(),1002);
		I_EQUAL(Chromosome("M").num(),1003);
		I_EQUAL(Chromosome("MT").num(),1003);

		//chromosomes without fixed numbers
		int base = Chromosome("chrBLA").num();
		IS_TRUE(base >= 1004);
		I_EQUAL(Chromosome("chrBLA2").num(),base+1);
		I_EQUAL(Chromosome("BLA").num(),base);
		I_EQUAL(Chromosome("BLA2").num(),base+1);
		IS_TRUE(Chromosome("chr01").num()>=1004);
	}

	TEST_METHOD(isNonSpecial)
	{
		IS_TRUE(Chromosome("1").isNonSpecial());
		IS_TRUE(Chromosome("chr1").isNonSpecial());
		IS_TRUE(Chromosome("X").isNonSpecial());
		IS_TRUE(Chromosome("CHRX").isNonSpecial());
		IS_TRUE(Chromosome("Y").isNonSpecial());
		IS_TRUE(Chromosome("chrY").isNonSpecial());
		IS_TRUE(Chromosome("M").isNonSpecial());
		IS_TRUE(Chromosome("MT").isNonSpecial());

		IS_FALSE(Chromosome("chrBLA2").isNonSpecial());
		IS_FALSE(Chromosome("BLA").isNonSpecial());
		IS_FALSE(Chromosome("BLA2").isNonSpecial());
		IS_FALSE(Chromosome("chr01").isNonSpecial());
	}

	TEST_METHOD(isAutosome)
	{
		IS_TRUE(!Chromosome("").isAutosome());
		IS_TRUE(!Chromosome("X").isAutosome());
		IS_TRUE(!Chromosome("Y").isAutosome());
		IS_TRUE(!Chromosome("M").isAutosome());
		IS_TRUE(!Chromosome("BLA").isAutosome());

		IS_TRUE(Chromosome("1").isAutosome());
		IS_TRUE(Chromosome("2").isAutosome());
		IS_TRUE(Chromosome("10").isAutosome());
		IS_TRUE(Chromosome("20").isAutosome());
		IS_TRUE(Chromosome("22").isAutosome());
		IS_TRUE(Chromosome("100").isAutosome());
	}

	TEST_METHOD(isGonosome)
	{
		IS_TRUE(Chromosome("X").isGonosome());
		IS_TRUE(Chromosome("Y").isGonosome());

		IS_TRUE(!Chromosome("").isGonosome());
		IS_TRUE(!Chromosome("M").isGonosome());
		IS_TRUE(!Chromosome("BLA").isGonosome());
		IS_TRUE(!Chromosome("1").isGonosome());
	}

	TEST_METHOD(isX)
	{
		IS_TRUE(Chromosome("X").isX());

		IS_TRUE(!Chromosome("Y").isX());
		IS_TRUE(!Chromosome("").isX());
		IS_TRUE(!Chromosome("M").isX());
		IS_TRUE(!Chromosome("BLA").isX());
		IS_TRUE(!Chromosome("1").isX());
	}


	TEST_METHOD(isY)
	{
		IS_TRUE(Chromosome("y").isY());

		IS_TRUE(!Chromosome("X").isY());
		IS_TRUE(!Chromosome("").isY());
		IS_TRUE(!Chromosome("M").isY());
		IS_TRUE(!Chromosome("BLA").isY());
		IS_TRUE(!Chromosome("1").isY());
	}


	TEST_METHOD(isM)
	{
		IS_TRUE(Chromosome("M").isM());

		IS_TRUE(!Chromosome("X").isM());
		IS_TRUE(!Chromosome("Y").isM());
		IS_TRUE(!Chromosome("").isM());
		IS_TRUE(!Chromosome("BLA").isM());
		IS_TRUE(!Chromosome("1").isM());
	}

};
