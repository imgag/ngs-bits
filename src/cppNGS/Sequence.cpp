#include "Sequence.h"
#include "Exceptions.h"

#include <QRegularExpression>

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

QList<Sequence> Sequence::split(char c) const
{
	QList<Sequence> output;
	foreach(const QByteArray& part, QByteArray::split(c))
	{
		output << part;
	}
	return output;
}

void Sequence::reverse()
{
	std::reverse(begin(), end());
}

void Sequence::complement()
{
    for (int i=0; i<size(); ++i)
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
	reverse();
	complement();
}

Sequence Sequence::toReverseComplement() const
{
	Sequence output = *this;
	output.reverseComplement();
	return output;
}

double Sequence::gcContent() const
{
	int gc = 0;
	int at = 0;
    for (int i=0; i<size(); ++i)
	{
		char base = operator[](i);
		if (base=='G' ||base=='C') ++gc;
		else if (base=='A' ||base=='T') ++at;
	}

	if (gc+at==0) return std::numeric_limits<double>::quiet_NaN();


	return (double)(gc)/(gc+at);
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

int Sequence::addNoise(double error_probability, std::mt19937& gen)
{
	int ec = 0;

	//uniform distribution
	std::uniform_real_distribution<double> error_dist(0, 1);

	//bases vector
	QByteArray bases = "ACGT";
    for(int i=0; i<size(); ++i)
	{
		//base error?
		bool error = error_dist(gen) < error_probability;

		//replace base at random
		if (error)
		{
			do
			{
				// we need to provide a random number generator explicitly
				std::random_device rd;
				std::mt19937 g(rd());
				std::shuffle(bases.begin(), bases.end(), g);
			}
			while (at(i)==bases[0]);
			operator[](i) = bases[0];

			++ec;
		}
	}

	return ec;
}

bool Sequence::onlyACGT() const
{
    return QRegularExpression("^[ACGT]+$").match(*this).hasMatch();
}
