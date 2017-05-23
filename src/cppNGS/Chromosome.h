#ifndef CHROMOSOME_H
#define CHROMOSOME_H

#include "cppNGS_global.h"
#include <QString>

///Chromosome class that assigns a numeric value to each chromosome string.
///The numeric representation is unique for each normalized string representation!
class CPPNGSSHARED_EXPORT Chromosome
{
	public:
		///Default constructor (creates an invalid instance - needed for containers only).
		Chromosome();
		///Constructor.
		Chromosome(const QString& chr);
		///Constructor.
		Chromosome(const QByteArray& chr);
		///Constructor.
		Chromosome(const char* chr);
		///Constructor.
		Chromosome(const std::string& chr);

		///Less-than operator.
		bool operator<(const Chromosome& rhs) const
		{
			return num_ < rhs.num_;
		}
		///Greater-than operator.
		bool operator>(const Chromosome& rhs) const
		{
			return num_ > rhs.num_;
		}
		///Equal operator.
		bool operator==(const Chromosome& rhs) const
		{
			return num_ == rhs.num_;
		}
		///Not-equal operator.
		bool operator!=(const Chromosome& rhs) const
		{
			return num_ != rhs.num_;
		}

		///Returns if the chromosome is valid (empty string, numeric value 0).
		bool isValid() const
		{
			return num_>0;
		}
		///Returns if the chromosome is an autosome.
		bool isAutosome() const
		{
			return num_>0 && num_<1001;
		}
        ///Returns if the chromosome is a gonosome, i.e. chrX or chrY.
        bool isGonosome() const
        {
            return num_==1001 || num_==1002;
        }
        ///Returns if the chromosome is chrX.
        bool isX() const
        {
            return num_==1001;
        }
        ///Returns if the chromosome is chrY.
        bool isY() const
        {
            return num_==1002;
        }
        ///Returns if the chromosome is mitochondria.
        bool isM() const
        {
            return num_==1003;
        }
        ///Returns if the chromosome is a non-special chromosome, i.e. an autosome, a gonosome or mitochondria.
        bool isNonSpecial() const
        {
            return num_>0 && num_<1004;
        }

		///Returns the unchanged string representation.
		const QByteArray& str() const
		{
			return str_;
		}

		///Returns the normalized string representation (upper-case letters, 'chr' on demand only).
		QByteArray strNormalized(bool prepend_chr) const
		{
			return (prepend_chr ? "chr" : "") + normalizedStringRepresentation();
		}

        //numeric representation (0=invalid, 1-1000=autosome, 1001=X, 1002=Y, 1003=M, 1004+=other non-numeric chromosomes)
		int num() const
		{
			return num_;
		}

	private:
		QByteArray str_;
		int num_;

		///Returns the normalized string representation (no 'chr', upper-case)
		QByteArray normalizedStringRepresentation() const;
		///Numeric representation.
		int numericRepresentation() const;
};

//Required to make Chromosome hashable by Qt, e.g. to use it in QSet or QHash
inline uint qHash(const Chromosome& key)
{
	int num = key.num();
	if (num<0) return -num;
	return num;
}

#endif // CHROMOSOME_H
