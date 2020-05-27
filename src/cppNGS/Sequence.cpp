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
	//TODO make this faster?!
	reverse();
	complement();
}

Sequence Sequence::toReverseComplement() const
{
	Sequence output = *this;
	output.reverseComplement();
	return output;
}


char Sequence::complement(char base)
{
	switch(base)
	{
		case 'A':
			return 'T';
		case 'C':
			return 'G';
		case 'T':
			return 'A';
		case 'G':
			return 'C';
		case 'N':
			return 'N';
		default:
			THROW(ProgrammingException, "Could not convert base " + QString(base) + " to complement!");
	}
}
