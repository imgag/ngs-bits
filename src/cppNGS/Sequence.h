#ifndef SEQUENCE_H
#define SEQUENCE_H

#include "cppNGS_global.h"
#include <QByteArray>
#include <QRegularExpression>
#include <random>

///DNA sequence class
class CPPNGSSHARED_EXPORT Sequence
	: public QByteArray
{
public:

	//Constructors
	Sequence();
	Sequence(const char* rhs);
    Sequence(const Sequence& rhs);
    Sequence(const QByteArray& rhs);

    //Declare assignment operator as explicitly defaulted (implicit generation is not allowed because of explicit copy constructor)
    Sequence& operator=(const Sequence& rhs) = default;

	//Returns the left n bases of the sequence
	Sequence left(int n) const;

	//Split seqence by separator
	QList<Sequence> split(char c) const;

	///Changes the sequence to reverse order.
	void reverse();
	///Changes the sequence to the complement.
	void complement();
	///Changes the sequence to the reverse complement.
	void reverseComplement();

	///Returns the reverse complement of the sequence.
	Sequence toReverseComplement() const;

	///Returns the G/C fraction [0,1]. If not valid base is contained in the sequence, nan is resturned.
	double gcContent() const;

	///Returns the complementary base of the given base.
	static char complement(char base);

	///Adds random noise to the sequence. Returns how many errors have been added.
	int addNoise(double error_probability, std::mt19937& gen);

	//Returns if the sequence consists only of A,C,G and T bases.
	bool onlyACGT() const;
};

#endif // SEQUENCE_H
