#ifndef PILEUP_H
#define PILEUP_H

#include "Sequence.h"
#include <QVector>
#include <QStringList>
#include "cppNGS_global.h"

///Pileup representation, similar to SAMTOOLS pileup.
class CPPNGSSHARED_EXPORT Pileup
{
public:
    ///Default constructor.
    Pileup();

    ///Returns the count of 'A'.
    long long a() const
    {
        return a_;
    }
    ///Returns the count of 'C'
    long long c() const
    {
        return c_;
    }
    ///Returns the count of 'G'
    long long g() const
    {
        return g_;
    }
    ///Returns the count of 'T'
    long long t() const
    {
        return t_;
    }
    ///Returns the count of 'N'
    long long n() const
    {
        return n_;
    }
    ///Returns the base allele frequency in relation to the overall depth (but ignorning 'N').
    long long countOf(QChar base) const;
	
    ///Increases the base count of geht given base. Valid bases are 'A','C','T','G','N' both upper-case and lower-case and '-' for deletions.
    void inc(char base);
	
	///Increase count of A
    void incA()
	{
		++a_;
	}
	///Increase count of A
    void incC()
	{
		++c_;
	}
	///Increase count of A
    void incG()
	{
		++g_;
	}
	///Increase count of A
    void incT()
	{
		++t_;
	}
	///Increase count of A
    void incN()
	{
		++n_;
	}
	///Increase count of deletions
    void incDel()
	{
		++del_;
	}

	///Clears all counts and indels.
    void clear();
    ///Returns the overall depth of the based 'A','C','G' and 'T'. 'N' and '-' are only included on demand.
    long long depth(bool count_del, bool count_n=false) const;

    ///Returns the maximum base count of the bases 'A','C','G' and 'T'.
    long long max() const
    {
        return std::max( std::max(a_, c_), std::max(g_, t_));
    }
    ///Returns the allele frequency of a mutant base compared to a wild-type base (ignoring all other bases and deletions). @note If neither mutant base nor wild-type base are found NAN is returned!
    double frequency(QChar wt, QChar mut) const;

    /**
	  @brief Returns the indels at the pileup position.
      @note Insersions are inserted just before the position, deletions start at the position.
      @note Insersions start with '+', deletions start with  '-'.
    */
	const QList<Sequence>& indels() const
    {
        return indels_;
    }
	///Adds indel to the list.
	void addIndel(const Sequence& indel)
    {
        indels_.append(indel);
    }
	///Adds indels to the list.
	void addIndels(const QList<Sequence>& indels)
    {
        indels_ << indels;
    }

	///Sets the fraction of reads with mapping quality 0
	void setMapq0Frac(double mapq0_frac)
	{
		mapq0_frac_ = mapq0_frac;
	}
	///Returns the fraction of reads with mapping quality 0
	double mapq0Frac() const
	{
		return mapq0_frac_;
	}

protected:
    long long a_;
    long long c_;
    long long g_;
    long long t_;
    long long n_;
    long long del_;
	QList<Sequence> indels_;
	double mapq0_frac_;
};

#endif // PILEUP_H
