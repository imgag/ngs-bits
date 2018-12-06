#include <algorithm>
#include <vector>
#include <string>
#include <QFile>
#include "ToolBase.h"
#include "Exceptions.h"
#include "Helper.h"
#include "VcfFile.h"
#include "../../needleman-wunsch/NeedlemanWunsch.hpp" // TODO: This does seem like an odd import path

using namespace std;

enum VariantType {SNP, MNP, INDEL, CLUMPED, SV};

class ConcreteTool: public ToolBase
{
	Q_OBJECT

public:
	ConcreteTool(int& argc, char *argv[])
		: ToolBase(argc, argv)
	{
	}

	/**
	 * @brief classifyVariant
	 * Classifies variants according to https://genome.sph.umich.edu/wiki/Variant_classification
	 * Assumes that both REF and ALT are already trimmed (no whitespaces, tabs)
	 *
	 * @param ref - the reference sequence
	 * @param alt - the alternating sequence
	 * @return VariantType
	 */
	VariantType classifyVariant(const QByteArray& ref, const QByteArray& alt)
	{
		int length = alt.length() - ref.length();

		if (length == 0)
		{
			if (ref.length() == 1 && ref != alt) return SNP;
			auto distance = static_cast<const int> (editDistance(ref, alt));
			return (min(ref.length(), alt.length()) == distance) ? MNP : CLUMPED;
		}
		else
		{
			return INDEL;
		}
	}

	/**
	 * @brief editDistance
	 * Returns the levensthein distance for two sequences
	 * @param a
	 * @param b
	 * @return
	 */
	static unsigned int editDistance(const QByteArray& a, const QByteArray& b)
	{
		// Implementation from https://en.wikibooks.org/wiki/Algorithm_Implementation/Strings/Levenshtein_distance#C++
		size_t len_a = static_cast<size_t> (a.size()),  len_b = static_cast<size_t> (b.size());
		vector<vector<unsigned int>> d(len_a + 1, vector<unsigned int>(len_b + 1));

		d[0][0] = 0;

		for (unsigned int i = 1; i < len_a; ++i) d[i][0] = i;
		for (unsigned int i = 1; i < len_b; ++i) d[0][1] = i;

		for (unsigned int i = 1; i < len_a; ++i)
		{
			for (unsigned int j = 1; j < len_b; ++j)
			{
				d[i][j] = min({
					d[i - 1][j] + 1,
					d[i][j -1 ] + 1,
					d[i - 1][j - 1] + (a[i - 1] == b[j - 1] ? 0 : 1)
				});
			}
		}

		return d[len_a][len_b];
	}

	//Returns the content of a column by index (tab-separated line)
	static QByteArray getPartByColumn(const QByteArray& line, int index)
	{
		int columns_seen = 0;
		int column_start = 0;
		int column_end = -1;

		for (int i = 0; i < line.length(); ++i)
		{
			if (line[i]=='\t')
			{
				++columns_seen;
				if (columns_seen == index)
				{
					column_start = i;
					column_end = line.length() -1; // for last column that is not followed by a tab
				}
				else if (columns_seen == index + 1)
				{
					column_end = i;
					break;
				}
			}
		}

		if (column_end==-1)
		{
			THROW(ProgrammingException, "Cannot find column " + QByteArray::number(index) + " in line: " + line);
		}

		return line.mid(column_start, column_end - column_start);
	}

	virtual void setup()
	{
		setDescription("Breaks complex variants into several lines preserving INFO/SAMPLE fields. This does not handle multi-allelic or structural variants.");
		//optional
		addInfile("in", "Input VCF file. If unset, reads from STDIN.", true, true);
		addOutfile("out", "Output VCF list. If unset, writes to STDOUT.", true, true);
		addOutfile("err", "Ouptuts statistics. If unset, writes to STDERR.", true, true);

		changeLog(2018, 11, 30, "Initial implementation.");
	}

	/**
	 * @brief main
	 * This program breaks complex variants into several lines. It is inspired by vt's decompose_suballelic and vcflib's vcfallelicprimitives.
	 * However this program will only deal with complex variants. It will not consinder multi-allelic variants.
	 * We use the classification procedure as described in the VT wiki here https://genome.sph.umich.edu/wiki/Variant_classification
	 * Local aligment of variants is done using Needleman-Wunsch as implemented in edlib. See https://doi.org/10.1093/bioinformatics/btw753
	 */
	virtual void main()
	{
		QString in = getInfile("in");
		QString out = getOutfile("out");
		QString err = getOutfile("err");
		if(in!="" && in==out)
		{
			THROW(ArgumentException, "Input and output files must be different when streaming!");
		}
		if(err!="" && err==out)
		{
			THROW(ArgumentException, "Error and output files must be different when streaming!");
		}

		QSharedPointer<QFile> in_p = Helper::openFileForReading(in, true);
		QSharedPointer<QFile> out_p = Helper::openFileForWriting(out, true);
		QSharedPointer<QFile> err_p = Helper::openFileForWriting(err, true);

		// Statistics
		int number_of_additional_snps = 0;
		int number_of_biallelic_block_substitutions = 0;
		int number_of_new_variants = 0;
		int number_of_variants = 0;

		while(!in_p->atEnd())
		{
			QByteArray line = in_p->readLine();
			//skip empty lines and headers
			if (line.trimmed().isEmpty() || line.startsWith("#"))
			{
				out_p->write(line);
				continue;
			}


			QByteArray ref = getPartByColumn(line, VcfFile::REF).trimmed();
			QByteArray alt = getPartByColumn(line, VcfFile::ALT).trimmed();

			VariantType variant_type = classifyVariant(ref, alt);
			++number_of_variants;

			if (variant_type != SNP) // Align with Needleman-Wunsch
			{
				auto alt_ = string(alt.data()), ref_ = string(ref.data());
				auto matrix = NeedlemanWunsch<string>::populateMatrix(ref_, alt_);
				auto aligments = NeedlemanWunsch<string>::aligment(get<MATRIX_TRACEBACK>(matrix), ref_, alt_);

				// After obtaining the aligments we will have a look at both optimal aligments
				// If the aligment contains any GAP (marked as -) we will split the sequences into chunks
				// The remaining chunks will be normalized (check which sequences are identical and remove them)
				// After this the sequences are written with a reference to the original sequences
				vector<string> sequences;


			}
			else // We don't look at SNP's
			{
				out_p->write(line);
			}
		}

		// After processing print statistics to error stream
		QByteArray buffer;
		float new_variants_in_percent = number_of_new_variants / (number_of_variants / 100);
		float additional_snps_in_percent = number_of_additional_snps / (number_of_new_variants / 100);
		float biallelic_substitutions_in_percent = number_of_biallelic_block_substitutions / (number_of_new_variants / 100);
		err_p->write("Processed " + buffer.setNum(number_of_variants) + " variants of which " + buffer.setNum(number_of_new_variants) + " (" + buffer.setNum(new_variants_in_percent) + ")% are new.");
		err_p->write(buffer.setNum(number_of_additional_snps) + "( " + buffer.setNum(additional_snps_in_percent) + "%) are additional SNP's while " + buffer.setNum(number_of_biallelic_block_substitutions) + "(" + buffer.setNum(biallelic_substitutions_in_percent) + "%) are biallelic substitutions.");
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
