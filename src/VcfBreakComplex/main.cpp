#include <algorithm>
#include <cmath>
#include <vector>
#include <QFile>
#include <QRegularExpression>
#include "ToolBase.h"
#include "Exceptions.h"
#include "Helper.h"
#include "VcfFile.h"
#include "NeedlemanWunsch.hpp"

using namespace std;

class ConcreteTool: public ToolBase
{
	Q_OBJECT

public:
	ConcreteTool(int& argc, char *argv[])
		: ToolBase(argc, argv)
	{
	}

	virtual void setup()
	{
		setDescription("Breaks complex variants into primitives preserving INFO/SAMPLE fields.");
		setExtendedDescription(QStringList() << "Multi-allelic variants are ignored, since we assume they have already been split, e.g. with VcfBreakMulti."
											 << "Complex variants that are decomposed, are flagged with a BBC (before break-complex) info entry.");
		addInfile("in", "Input VCF file. If unset, reads from STDIN.", true, true);
		addOutfile("out", "Output VCF list. If unset, writes to STDOUT.", true, true);
		addOutfile("stats", "Ouptuts statistics. If unset, writes to STDERR.", true, true);
		addFlag("keep_mnps", "Write out MNPs unchanged.");
		addFlag("no_tag", "Skip annotation of decomposed variants with BBC in INFO field.");

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
		QString stats = getOutfile("stats");
		bool keep_mnps = getFlag("keep_mnps");
		bool no_tag = getFlag("no_tag");
		if(in!="" && in==out)
		{
			THROW(ArgumentException, "Input and output files must be different when streaming!");
		}
		if(stats!="" && stats==out)
		{
			THROW(ArgumentException, "Error and output files must be different when streaming!");
		}

		QSharedPointer<QFile> in_p = Helper::openFileForReading(in, true);
		QSharedPointer<QFile> out_p = Helper::openFileForWriting(out, true);

		// Statistics
		bool inserted_info = false;
		unsigned int number_of_additional_snps = 0;
		unsigned int number_of_biallelic_block_substitutions = 0;
		unsigned int number_of_new_variants = 0;
		unsigned int number_of_variants = 0;

		while(!in_p->atEnd())
		{
			QByteArray line = in_p->readLine();
			// Skip empty lines and headers
			if (line.trimmed().isEmpty() || line.startsWith("#"))
			{
				out_p->write(line);
				if (!inserted_info) // Insert right after version line
				{
					if (!no_tag) out_p->write("##INFO=<ID=BBC,Number=1,Type=String,Description=\"Original chr:pos:ref|alt encoding\">\n");
					inserted_info = true;
				}
				continue;
			}


			// Not a header line so increment the variant
			++number_of_variants;

			QByteArray ref = VcfFile::getPartByColumn(line, VcfFile::REF).trimmed();
			QByteArray alt = VcfFile::getPartByColumn(line, VcfFile::ALT).trimmed();

			// Skip alternating allele pairs
			if (alt.contains(","))
			{
				out_p->write(line);
				continue;
			}

			VariantType variant_type = VcfFile::classifyVariant(ref, alt);

			if (variant_type != SNP) // Align with Needleman-Wunsch
			{
				auto nw = NeedlemanWunsch<QByteArray>(ref, alt);
				nw.compute_matrix();
				nw.trace_back();
				auto aligment = nw.get_aligments();
				auto reference = get<ALIGMENT_REFERENCE>(aligment), query = get<ALIGMENT_QUERY>(aligment);

				// Iterate through the reference and compare it with the query
				using AligmentPair = pair<QByteArray, QByteArray>;
				vector<AligmentPair> aligments;

				if (ref.length() == 1 || alt.length() == 1) // SNP
				{
					aligments.push_back(AligmentPair(query.replace("-", ""), reference.replace("-", "")));
					++number_of_additional_snps;
				}
				else // MNP or COMPLEX
				{
					if (keep_mnps) // Keep more complex variants
					{
						out_p->write(line);
					}
					else // Break up more complex variants
					{
						for (auto i = 0; i < query.size(); ++i)
						{
							if (query.at(i) != reference.at(i)) // transition from REF -> ALT
							{
								aligments.push_back(AligmentPair(query.mid(i, 1), reference.mid(i, 1)));
								++i;
								++number_of_additional_snps;
							}
							else if (i + 1 < query.size() && query.at(i + 1) == '-') // transition from REF,- to ALT
							{
								int gap_start = static_cast<int> (i + 1);
								int gap_end = gap_start;
								while ((i + 1) < query.size() && query.at(i + 1) == '-')
								{
									++i;
									gap_end++;
								}
								aligments.push_back(AligmentPair(query.mid(gap_start - 1, 1), reference.mid(gap_start - 1, gap_end)));
								++number_of_biallelic_block_substitutions; // new biallelic block substitutios
							}
						}
					}
				}

				for (size_t i = 0; i < aligments.size(); ++i) // write out the new sequences
				{
					if (i > 0) ++number_of_new_variants;
					auto parts = line.split('\t');
					// Append INFO entry in format BBC=chr:pos:ref:alt
					if (!no_tag) parts[VcfFile::INFO] = parts[VcfFile::INFO].trimmed() + ";BBC=" + parts[VcfFile::CHROM] + ":" + parts[VcfFile::POS] + ":" + parts[VcfFile::REF] + "|" + parts[VcfFile::ALT];

					// Modify alt and ref with new aligments
					parts[VcfFile::REF] = aligments[i].first;
					parts[VcfFile::ALT] = aligments[i].second;

					// Write to output
					out_p->write(parts.join("\t"));

				}
			}
			else // We don't look at SNP's
			{
				out_p->write(line);
			}
		}

		// After processing print statistics to error stream
		QSharedPointer<QFile> stats_p = QSharedPointer<QFile>(new QFile(stats));
		stats_p->open(stderr, QFile::WriteOnly | QIODevice::Text);
		double new_variants_in_percent = (number_of_variants>0) ? 100.0 * number_of_new_variants / number_of_variants : 0.0;
		stats_p->write(QByteArray("Processed ") + QByteArray::number(number_of_variants) + " variant(s) of which " + QByteArray::number(number_of_new_variants) + " (" + QByteArray::number(new_variants_in_percent, 'f', 2) + "%) were decomposed.\n");
		stats_p->write(QByteArray::number(number_of_additional_snps - number_of_new_variants) + " of these are additional SNPs and " + QByteArray::number(number_of_biallelic_block_substitutions - number_of_variants) + " of these are biallelic substitutions.\n");
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
