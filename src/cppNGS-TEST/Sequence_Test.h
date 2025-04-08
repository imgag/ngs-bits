#include "TestFramework.h"
#include "Sequence.h"

TEST_CLASS(Sequence_Test)
{
Q_OBJECT
private slots:

	void reverse()
	{
		Sequence seq = "";
		seq.reverse();
		S_EQUAL(seq, QByteArray(""));
		seq = "ACGTN";
		seq.reverse();
		S_EQUAL(seq, QByteArray("NTGCA"));
	}

	void complement()
	{
		Sequence seq = "";
		seq.complement();
		S_EQUAL(seq, QByteArray(""));
		seq = "ACGTN";
		seq.complement();
		S_EQUAL(seq, QByteArray("TGCAN"));
	}

	void reverseComplement()
	{
		Sequence seq = "";
		seq.reverseComplement();
		S_EQUAL(seq, QByteArray(""));

		seq = "ACGTA";
		seq.reverseComplement();
		S_EQUAL(seq, QByteArray("TACGT"));

		seq = "ACGT";
		seq.reverseComplement();
		S_EQUAL(seq, QByteArray("ACGT"));

		seq = "ACGTN";
		seq.reverseComplement();
		S_EQUAL(seq, QByteArray("NACGT"));

		seq = "ACNT";
		seq.reverseComplement();
		S_EQUAL(seq, QByteArray("ANGT"));
	}

	void toReverseComplement()
	{
		Sequence seq = "ACGTN";
		S_EQUAL(seq.toReverseComplement(), QByteArray("NACGT"));
	}

	void addNoise()
	{
		Sequence original = "ACGTACGTACGTACGT";
		Sequence seq = original;

		unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
		std::mt19937 gen(seed);
		int errors = 0;
		do
		{
			errors = seq.addNoise(0.3, gen);
		}
		while(errors==0);

		int errors_found = 0;
        for(int i=0; i<seq.size(); ++i)
		{
			if (seq[i]!=original[i])
			{
				++errors_found;
			}
		}

		IS_TRUE(original!=seq);
		I_EQUAL(errors, errors_found);
	}


	void onlyACGT()
	{
		Sequence seq = "ACGTACGTACGTACGT";
		IS_TRUE(seq.onlyACGT());
		seq = "A";
		IS_TRUE(seq.onlyACGT());

		seq = "ACGTN";
		IS_FALSE(seq.onlyACGT());
		seq = "N";
		IS_FALSE(seq.onlyACGT());

		seq = "ACGTB";
		IS_FALSE(seq.onlyACGT());
		seq = "B";
		IS_FALSE(seq.onlyACGT());
	}
};


