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
			int pos = VcfFile::getPartByColumn(line, VcfFile::POS).trimmed().toInt();

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
				vector<Aligment> aligments;

				if (ref.length() == 1 || alt.length() == 1) // SNP
				{
					aligments.push_back(Aligment(query.replace("-", ""), reference.replace("-", ""), 0));
					++number_of_additional_snps;
				}
				else // MNP or CLUMPED
				{
					if (keep_mnps) // Keep more complex variants
					{
						out_p->write(line);
					}
					else // Break up clumped variants
					{
						while (query.startsWith("-")) // Kill leading gaps
						{
							query = query.remove(0, 1);
						}

						for (auto i = 0; i < query.size(); ++i)
						{
							if (i + 1 < query.size() && query.at(i + 1) == '-') // transition from REF,- to ALT
							{
								int gap_start = static_cast<int> (i + 1);
								int gap_end = gap_start;
								while ((i + 1) < query.size() && query.at(i + 1) == '-')
								{
									++i;
									gap_end++;
								}
								aligments.push_back(Aligment(query.mid(gap_start - 1, 1), reference.mid(gap_start - 1, gap_end).replace("-", ""), gap_start));
								++number_of_biallelic_block_substitutions; // new biallelic block substitutios
							}
							else if (i + 1 < reference.size() && reference.at(i + 1) == '-') // do the same for the reference
							{
								int gap_start = static_cast<int> (i +1);
								int gap_end = gap_start;
								while ((i + 1) < reference.size() && reference.at(i + 1) == '-')
								{
									++i;
									++gap_end;
								}
								aligments.push_back(Aligment(query.mid(gap_start - 1, 2), reference.mid(gap_start - 1, gap_end).replace("-", ""), gap_start));
								++number_of_biallelic_block_substitutions; // new biallelic block substitutios
							}
							else if (reference.at(i) == '-') {
								int gap_start = static_cast<int> (i);
								int gap_end = gap_start;
								while ((i + 1) < reference.size() && reference.at(i) == '-')
								{
									++i;
									++gap_end;
								}
								aligments.push_back(Aligment(query.mid(gap_start, gap_end - gap_start), reference.mid(gap_end, 1), gap_start));
								++number_of_biallelic_block_substitutions;
							}
							else if (query.at(i) != reference.at(i)) // transition from REF -> ALT
							{
								aligments.push_back(Aligment(query.mid(i, 1), reference.mid(i, 1), i));
								++i;
								++number_of_additional_snps;
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
					parts[VcfFile::REF] = get<0>(aligments[i]);
					parts[VcfFile::ALT] = get<1>(aligments[i]);
					parts[VcfFile::POS] = QByteArray::number(pos + get<2>(aligments[i]));

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
