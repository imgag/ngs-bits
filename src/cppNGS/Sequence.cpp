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
	int i_left = 0;
	int i_right = count()-1;

	while(i_left<=i_right)
	{
		char base_left = operator[](i_left);
		char base_right = operator[](i_right);

		if (base_left=='A') operator[](i_right) = 'T';
		else if (base_left=='C') operator[](i_right) = 'G';
		else if (base_left=='T') operator[](i_right) = 'A';
		else if (base_left=='G') operator[](i_right) = 'C';
		else if (base_left=='N') operator[](i_right) = 'N';
		else THROW(ProgrammingException, "Could not convert base '" + QString(base_left) + "' to complement!");

		if (base_right=='A') operator[](i_left) = 'T';
		else if (base_right=='C') operator[](i_left) = 'G';
		else if (base_right=='T') operator[](i_left) = 'A';
		else if (base_right=='G') operator[](i_left) = 'C';
		else if (base_right=='N') operator[](i_left) = 'N';
		else THROW(ProgrammingException, "Could not convert base '" + QString(base_right) + "' to complement!");

		++i_left;
		--i_right;
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

int Sequence::addNoise(double error_probabilty, std::mt19937& gen)
{
	int ec = 0;

	//uniform distribution
	std::uniform_real_distribution<double> error_dist(0, 1);

	//bases vector
	QByteArray bases = "ACGT";
	for(int i=0; i<count(); ++i)
	{
		//base error?
		bool error = error_dist(gen) < error_probabilty;

		//replace base at random
		if (error)
		{
			do
			{
				std::random_shuffle(bases.begin(), bases.end());
			}
			while (at(i)==bases[0]);
			operator[](i) = bases[0];

			++ec;
		}
	}

	return ec;
}
