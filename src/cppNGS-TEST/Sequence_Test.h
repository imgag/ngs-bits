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
		seq = "ACGTN";
		seq.reverseComplement();
		S_EQUAL(seq, QByteArray("NACGT"));
	}
};


