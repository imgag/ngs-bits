#include "Sequence.h"
#include "Exceptions.h"

Sequence::Sequence()
	: QByteArray()
{
}

Sequence::Sequence(const char* rhs)
	: QByteArray(rhs)
{
}

Sequence::Sequence(const Sequence& rhs)
	: QByteArray(rhs)
{
}

Sequence::Sequence(const QByteArray& rhs)
	: QByteArray(rhs)
{
}

Sequence Sequence::left(int n) const
{
	return QByteArray::left(n);
}

void Sequence::reverse()
{
	std::reverse(begin(), end());
}

void Sequence::complement()
{
	for (int i=0; i<count(); ++i)
	{
		switch(at(i))
		{
			case 'A':
				operator[](i) = 'T';
				break;
			case 'C':
				operator[](i) = 'G';
				break;
			case 'T':
				operator[](i) = 'A';
				break;
			case 'G':
				operator[](i) = 'C';
				break;
			case 'N':
				operator[](i) = 'N';
				break;
			default:
				THROW(ProgrammingException, "Could not convert base '" + QString(at(i)) + "' to complement!");
		}
	}
}

void Sequence::reverseComplement()
{
	int from = 0;
	int to = count()-1;

	while(from<count())
	{
		char base = operator[](to);

		if (base=='A') operator[](to) = 'T';
		else if (base=='C') operator[](to) = 'G';
		else if (base=='T') operator[](to) = 'A';
		else if (base=='G') operator[](to) = 'C';
		else if (base!='N') THROW(ProgrammingException, "Could not convert base '" + QString(base) + "' to complement!");

		++from;
		--to;
	}
}

Sequence Sequence::toReverseComplement() const
{
	Sequence output = *this;
	output.reverseComplement();
	return output;
}


char Sequence::complement(char base)
{
	if (base=='A') return 'T';
	else if (base=='C') return 'G';
	else if (base=='T') return 'A';
	else if (base=='G') return 'C';
	else if (base=='N') return 'N';

	THROW(ProgrammingException, "Could not convert base '" + QString(base) + "' to complement!");
}
