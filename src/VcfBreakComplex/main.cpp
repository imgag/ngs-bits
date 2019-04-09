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
											 << "Complex variants that are decomposed, are flagged with a BBC (before break-complex) info entry."
											 << "WARNING: THIS IS TOOL IS IN BETA STATE!!!");
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

		using Aligment = tuple<QByteArray, QByteArray, int>; // Describes an aligment of REF, ALT, OFFSET
		enum ClumpType { MNP, DELETION, INSERTION, COMPLEX };

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

			// Save a little bit of memory on multi-records
			QByteArray alt = VcfFile::getPartByColumn(line, VcfFile::ALT).trimmed();

			// Skip alternating allele pairs
			if (alt.contains(","))
			{
				out_p->write(line);
				continue;
			}

			QByteArray ref = VcfFile::getPartByColumn(line, VcfFile::REF).trimmed();
			int pos = VcfFile::getPartByColumn(line, VcfFile::POS).trimmed().toInt();

			VariantType variant_type = VcfFile::classifyVariant(ref, alt);

			if (variant_type != SNP) // Align with Needleman-Wunsch
			{
				auto nw = NeedlemanWunsch<QByteArray>(ref, alt);
				nw.compute_matrix();
				nw.trace_back();
				auto aligment = nw.get_aligments();
				auto reference = get<ALIGMENT_REFERENCE>(aligment), query = get<ALIGMENT_QUERY>(aligment);

				// Iterate through the reference and compare it with the query
				vector<Aligment> aligments;

				if (ref.length() == 1 || alt.length() == 1) // SNP
				{
					aligments.push_back(Aligment(reference.replace("-", ""), query.replace("-", ""), 0));
					++number_of_additional_snps;
				}
				else // MNP or CLUMPED
				{
					if (keep_mnps) // Keep more complex variants
					{
						out_p->write(line);
					}
					else
					{
						// Break up clumped variants
						ClumpType variant_clump = MNP;
						if (query.size() - query.count("-") == reference.size() - reference.count("-")) // TODO: We might not need aligment for cases where |REF|==|ALT|
						{
							variant_clump = MNP;
							query.replace("-", "");
							reference.replace("-", "");
						}
						else if (reference.size() > query.size() - query.count("-"))
						{
							variant_clump = DELETION;
						}
						else if (query.size() > reference.size() - reference.count("-"))
						{
							variant_clump = INSERTION;
						}

						int offset_leading_gap = 0; // kill leading gap aligment
						if (variant_clump == DELETION && query.at(0) == '-')
						{
							while (offset_leading_gap < alt.length() - 1)
							{
								if (ref.at(offset_leading_gap) == alt.at(offset_leading_gap))
								{
									++offset_leading_gap;
								}
								else
								{
									break;
								}
							}

							// Re-calculate shorter aligment
							nw = NeedlemanWunsch<QByteArray>(ref.mid(offset_leading_gap), alt.mid(offset_leading_gap));
							nw.compute_matrix();
							nw.trace_back();

							aligment = nw.get_aligments();
							reference = get<ALIGMENT_REFERENCE>(aligment);
							query = get<ALIGMENT_QUERY>(aligment);
						}
						else if (variant_clump == INSERTION && reference.at(0) == '-') // same as above
						{
							while (offset_leading_gap < ref.length() - 1)
							{
								if (ref.at(offset_leading_gap) == alt.at(offset_leading_gap))
								{
									++offset_leading_gap;
								}
								else
								{
									break;
								}
							}

							// Re-calculate shorter aligment
							nw = NeedlemanWunsch<QByteArray>(ref.mid(offset_leading_gap), alt.mid(offset_leading_gap));
							nw.compute_matrix();
							nw.trace_back();

							aligment = nw.get_aligments();
							reference = get<ALIGMENT_REFERENCE>(aligment);
							query = get<ALIGMENT_QUERY>(aligment);
						}

						int i = 0;
						while (i < reference.length())
						{
							if (variant_clump == DELETION && query.at(i) == '-') // Deletion event
							{
								int gap_start = i;
								while (i < reference.length() && query.at(i) == '-') ++i; // Search for first character that is not a GAP

								// We distinguish two cases
								// CTT / CT- -> CTT / CT
								// CTT / --T -> CTT / T
								int deletion_start = (query.at(max(gap_start - 1, 0)) == '-') ? max(gap_start - 1, 0) : gap_start;

								if (query.at(gap_start) != reference.at(deletion_start)) // Do not break more complex events
								{
									variant_clump = COMPLEX;
									break;
								}

								aligments.push_back(Aligment(reference.mid(deletion_start, (i + 1) - deletion_start), query.mid(i, 1), deletion_start + offset_leading_gap));
								++number_of_biallelic_block_substitutions;
							}
							else if (variant_clump == INSERTION && reference.at(i) == '-') // Insertion event
							{
								int gap_start = i;
								while (i < reference.length() && reference.at(i) == '-') ++i; // Search for first character that is not a GAP

								// We handle two cases
								// C-- / CTT -> C / CTT
								// --C / CTT -> C / CTT
								int ref_start = (reference.at(max(gap_start - 1, 0))  == '-') ? i : max(gap_start - 1, 0);
								int insertion_start = max(gap_start - 1, 0);

								if (reference.at(ref_start) != query.at(insertion_start)) // Do not break more complex events
								{
									variant_clump = COMPLEX;
									break;
								}

								aligments.push_back(Aligment(reference.mid(ref_start, 1), query.mid(insertion_start, (i + 1)- insertion_start), insertion_start + offset_leading_gap));
								++number_of_biallelic_block_substitutions;
							}
							else if (reference.at(i) != query.at(i)) // SNP
							{
								aligments.push_back(Aligment(reference.mid(i, 1), query.mid(i, 1), i + offset_leading_gap));
								++i;
								++number_of_additional_snps;
							}
							else // Simply ignore identical bases
							{
								++i;
							}
						}

						if (variant_clump == COMPLEX) // Write COMPLEX events as they are
						{
							out_p->write(line);
							continue;
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
		//stats_p->write(QByteArray::number(number_of_additional_snps - number_of_new_variants) + " of these are additional SNPs and " + QByteArray::number(number_of_biallelic_block_substitutions - number_of_variants) + " of these are biallelic substitutions.\n");
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
