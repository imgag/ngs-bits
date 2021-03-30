#include "Pileup.h"
#include "Exceptions.h"
#include "limits"

Pileup::Pileup()
	: a_(0)
	, c_(0)
	, g_(0)
	, t_(0)
	, n_()
	, del_()
	, indels_()
	, mapq0_frac_(std::numeric_limits<double>::quiet_NaN())
{
}

void Pileup::inc(char base, long long inc)
{
	if (base=='A') a_ += inc;
	else if (base=='C') c_ += inc;
	else if (base=='G') g_ += inc;
	else if (base=='T') t_ += inc;
	else if (base=='N') n_ += inc;
	else if (base=='-') del_ += inc;
	else if (base=='~') {}
	else if (base=='a') a_ += inc;
	else if (base=='c') c_ += inc;
	else if (base=='g') g_ += inc;
	else if (base=='t') t_ += inc;
	else if (base=='n') n_ += inc;
	else THROW(ArgumentException, "Unknown base '" + QString(QChar(base)) + "' in pileup!");
}

void Pileup::clear()
{
	a_ = 0;
	c_ = 0;
	g_ = 0;
	t_ = 0;
	del_ = 0;
	indels_.clear();
}

long long Pileup::depth(bool count_del, bool count_n) const
{
	long long output = a_ + c_ + g_ + t_;
	if (count_del) output += del_;
	if (count_n) output += n_;
	return output;
}

double Pileup::frequency(QChar wt, QChar mut) const
{
	wt = wt.toUpper();
	mut = mut.toUpper();

	double w;
	if (wt=='A') w = a_;
	else if (wt=='C') w = c_;
	else if (wt=='G') w = g_;
	else if (wt=='T') w = t_;
	else THROW(ArgumentException, "Unknown wild-type base '" + QString(wt) + "' in frequency calculation!");

	double m;
	if (mut=='A') m = a_;
	else if (mut=='C') m = c_;
	else if (mut=='G') m = g_;
	else if (mut=='T') m = t_;
	else THROW(ArgumentException, "Unknown mutant base '" + QString(mut) + "' in frequency calculation!");

	if (w+m==0)
	{
		return std::numeric_limits<double>::quiet_NaN();
	}
	return m / (w + m);
}

long long Pileup::countOf(QChar base) const
{
	if (base=='A') return a_;
	else if (base=='C') return c_;
	else if (base=='G') return g_;
	else if (base=='T') return t_;
	else if (base=='N') return n_;

	THROW(ArgumentException, "Unknown base '" + QString(base) + "' in counting function!");
}
