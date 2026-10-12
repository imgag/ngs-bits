#ifndef CHROMOSOME_H
#define CHROMOSOME_H

#include "cppNGS_global.h"
#include <QString>
#include <QHash>

///Chromosome class that assigns a numeric value to each chromosome string.
///The numeric representation is unique for each normalized string representation!
///Instances share immutable, process-lifetime records; construction and const access are thread-safe.
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
		///Constructor.
		Chromosome(QByteArrayView chr);

		///Less-than operator.
		bool operator<(const Chromosome& rhs) const
		{
			return data_->num < rhs.data_->num;
		}
		///Greater-than operator.
		bool operator>(const Chromosome& rhs) const
		{
			return data_->num > rhs.data_->num;
		}
		///Equal operator.
		bool operator==(const Chromosome& rhs) const
		{
			return data_->num == rhs.data_->num;
		}
		///Not-equal operator.
		bool operator!=(const Chromosome& rhs) const
		{
			return data_->num != rhs.data_->num;
		}

		///Returns if the chromosome is valid (empty string, numeric value 0).
		bool isValid() const
		{
			return data_->num>0;
		}
		///Returns if the chromosome is an autosome.
		bool isAutosome() const
		{
			return data_->num>0 && data_->num<1001;
		}
        ///Returns if the chromosome is a gonosome, i.e. chrX or chrY.
        bool isGonosome() const
        {
            return data_->num==1001 || data_->num==1002;
        }
        ///Returns if the chromosome is chrX.
        bool isX() const
        {
            return data_->num==1001;
        }
        ///Returns if the chromosome is chrY.
        bool isY() const
        {
            return data_->num==1002;
        }
        ///Returns if the chromosome is mitochondria.
        bool isM() const
        {
            return data_->num==1003;
        }
        ///Returns if the chromosome is a non-special chromosome, i.e. an autosome, a gonosome or mitochondria.
        bool isNonSpecial() const
        {
            return data_->num>0 && data_->num<1004;
        }

		///Returns the unchanged string representation.
		const QByteArray& str() const
		{
			return data_->original;
		}

		///Returns the normalized string representation (upper-case letters, 'chr' on demand only).
		QByteArray strNormalized(bool prepend_chr) const
		{
			return prepend_chr ? "chr" + data_->normalized : data_->normalized;
		}

        //numeric representation (0=invalid, 1-1000=autosome, 1001=X, 1002=Y, 1003=M, 1004+=other non-numeric chromosomes)
		int num() const
		{
			return data_->num;
		}

	private:
		//Immutable interned record. Instances contain only a pointer to this data.
		struct Data
		{
			QByteArray original;
			QByteArray normalized;
			int num;
		};
		struct Pool;
		const Data* data_;

		///Returns a stable, process-lifetime record for the trimmed spelling.
		static const Data* getDataPointer(QByteArrayView chr);
};

//Required to make Chromosome hashable by Qt, e.g. to use it in QSet or QHash
inline uint qHash(const Chromosome& key)
{
	int num = key.num();
	if (num<0) return -num;
	return num;
}

#endif // CHROMOSOME_H
