#include "ToolBase.h"
#include "TabixIndexedFile.h"
#include "Helper.h"
#include "VcfFile.h"

#include <BamReader.h>
#include <QFile>
#include <QTextStream>

class ConcreteTool
		: public ToolBase
{
	Q_OBJECT

public:
	ConcreteTool(int& argc, char *argv[])
		: ToolBase(argc, argv)
	{
	}

	virtual void setup()
	{
		setDescription("Calculates the Polgenic Risk Score(s) for a sample.");
		setExtendedDescription(QStringList() << "The PRS VCF files have to contain WEIGHT and POP_AF fields in the INFO column." << "Additionally some information about the PRS score is required in the VCF header." << "An example VCF file can be found at https://github.com/imgag/ngs-bits/blob/master/src/tools-TEST/data_in/VcfCalculatePRS_prs2.vcf");
		addInfile("in", "Tabix indexed VCF.GZ file of a sample.", false);
		addInfileList("prs", "List of PRS VCFs.", false);
		addInfile("bam", "BAM file corresponding to the VCF.", false);
		addOutfile("out", "Output TSV file containing Scores and PRS details", false);
		addOutfile("details", "Output TSV containing each variant with weight, allele count and population AF.", true);

		//optional
		addInfile("ref", "Reference genome FASTA file. If unset, 'reference_genome' from the 'settings.ini' file is used.", true, false);
		addInt("min_depth", "Depth cutoff below which uncalled SNPs are considered not callable and POP_AF is used instead of genotype.", true, 10);

		changeLog(2020,  7, 22, "Initial version of this tool.");
		changeLog(2022, 12, 15, "Added BAM depth check and population AF.");
		changeLog(2024,  4, 22, "Added output of factors and support for wt variants.");
		changeLog(2024,  6,  5, "Added support for imputed variants.");
	}

	virtual void main()
	{
		//init
		QTextStream out(stdout);
		int min_depth = getInt("min_depth");

		//load sample VCF
		TabixIndexedFile sample_vcf;
		sample_vcf.load(getInfile("in").toUtf8());

		//open BAM file
		BamReader bam_file(getInfile("bam"));

		//open refererence genome file
		QString ref_file = getInfile("ref");
		if (ref_file=="") ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") THROW(CommandLineParsingException, "Reference genome FASTA unset in both command-line and settings.ini file!");
		FastaFileIndex reference(ref_file);

		//open output file and write header
		QSharedPointer<QFile> output_tsv = Helper::openFileForWriting(getOutfile("out"), true);
		QByteArrayList column_headers = QByteArrayList() << "pgs_id" << "trait" << "score" << "percentile" << "build" << "variants_in_prs" << "variants_low_depth" << "variants_imputed" << "pgp_id" << "citation";
		output_tsv->write("#" + column_headers.join("\t") + "\n");

		//create optional output file containing all the factors used to calculate the PRS
		bool detail_output = !getOutfile("details").isEmpty();
		QSharedPointer<QFile> detail_tsv;
		if (detail_output)
		{
			detail_tsv = Helper::openFileForWriting(getOutfile("details"), false);
			detail_tsv->write("## allele_count: A '.' in the count_effect_allele column means insufficient depth and the tool uses the population_af as fallback allele count\n");
			QByteArrayList detail_tsv_headers = QByteArrayList() << "chr" << "start" << "end" << "ref_allele" << "effect_allele" << "other_allele" << "patient_allele1" << "patient_allele2"  << "count_effect_allele"
																 << "weight" << "population_af" << "pgs_id" << "comment";
			detail_tsv->write("#" + detail_tsv_headers.join("\t") + "\n");
		}

		//iterate over all given PRS files
		foreach (const QString& prs_file_path, getInfileList("prs"))
		{
			//load PRS file
			VcfFile prs_variant_list;
			prs_variant_list.load(prs_file_path);

			//does not support multi sample
			if(prs_variant_list.sampleIDs().count() > 1)
			{
				THROW(FileParseException, "PRS VCF file must not contain more than one sample: " + prs_file_path);
			}

			//define PRS
			double prs = 0;
			QVector<double> percentiles;
			QHash<QByteArray,QByteArray> column_entries;

			//parse comment lines
			foreach(const VcfHeaderLine& comment_line, prs_variant_list.vcfHeader().comments())
			{

				foreach(const QByteArray& column_name, column_headers)
				{
					if(comment_line.key.startsWith(column_name))
					{
						if(column_entries.contains(column_name))
						{
							THROW(FileParseException, "Comment section of PRS VCF  file contains more than one entry for '" +column_name + "': " + prs_file_path);
						}
						column_entries[column_name] = comment_line.value.trimmed();
						break;
					}
				}

				if(comment_line.key.startsWith("percentiles"))
				{
					if (percentiles.size() != 0) THROW(FileParseException, "Percentiles in PRS VCF file given twice: " + prs_file_path);
					QByteArrayList percentile_string = comment_line.value.trimmed().split(',');
					if (percentile_string.size() != 100) THROW(FileParseException, "Invalid number of percentiles given (required: 100, given: "  + QByteArray::number(percentile_string.size()) + ": " + prs_file_path);
					foreach (const QByteArray& value_string, percentile_string)
					{
						percentiles.append(Helper::toDouble(value_string, "Percentile"));
					}
				}
			}

			//check if all required comment lines are present
			QByteArrayList col_entries_not_in_header = QByteArrayList() << "score" << "percentile" << "variants_in_prs" << "variants_low_depth"<< "variants_imputed";
			foreach (const QByteArray& key, column_headers)
			{
				if(col_entries_not_in_header.contains(key)) continue;
				if(!column_entries.contains(key))
				{
					THROW(FileParseException, "Comment section of PRS VCFs does not contain an entry for '" + key + "': " + prs_file_path);
				}
			}

			//iterate over all variants in PRS
			int c_found = 0;
			int c_low_depth = 0;
			int c_imputed = 0;
			for(int i = 0; i < prs_variant_list.count(); ++i)
			{	
				VcfLine& prs_variant = prs_variant_list[i];

				//does not support multi-allelic variants
				if(prs_variant.isMultiAllelic())
				{
					THROW(FileParseException, "Multi-allelic variants in PRS VCF files are not supported: " + prs_variant.toString());
				}

				// get weight/pop_af from PRS VCF
				double weight = Helper::toDouble(prs_variant.info("WEIGHT"), "PRS weight");
				double pop_af = Helper::toDouble(prs_variant.info("POP_AF"), "PRS population allele frequency");
				bool imputed = prs_variant.infoKeys().contains("IMPUTED");
				QByteArray other_allele = prs_variant.info("OTHER_ALLELE");
				int allele_count = -1;
				QByteArrayList patient_alleles;
				QByteArrayList comment;
				bool prs_var_is_wildtype = ((prs_variant.altString() == ".") || (prs_variant.altString() == prs_variant.ref()) || prs_variant.infoKeys().contains("REF_IS_EFFECT_ALLELE"));
				//replace '.' in wildtype var with ref
				if (prs_variant.ref() == ".") prs_variant.setSingleAlt(prs_variant.ref());

				if (imputed)
				{
					//variant is not reliable -> use POP_AF
					allele_count = -1;
					prs += 2.0 * weight * pop_af;
					++c_imputed;
					comment << "Variant imputed";
				}
				else if (bam_file.getVariantDetails(reference, prs_variant).depth < min_depth)
				{
					//coverage too low --> use POP_AF
					allele_count = -1;
					prs += 2.0 * weight * pop_af;
					++c_low_depth;
					comment << "Variant has insufficient depth";
				}
				else
				{
					//QC is ok, check for called variants

					//get all matching variants at this position
					QByteArrayList matching_lines = sample_vcf.getMatchingLines(prs_variant.chr(), prs_variant.start(), prs_variant.end(), true);
					QByteArrayList matching_variants;

					if (prs_var_is_wildtype)
					{
						if (matching_lines.size() > 0)
						{
							allele_count = 2;
							//each variant at this position reduces the allele count by 1 (het) or 2 (hom)
							foreach(const QByteArray& line, matching_lines)
							{
								//get genotype
								QByteArrayList split_line = line.split('\t');
								QByteArrayList format_header_items = split_line[8].split(':');
								QByteArrayList format_value_items = split_line[9].split(':');
								int genotype_idx = format_header_items.indexOf("GT");
								if(genotype_idx < 0) THROW(FileParseException, "Genotype information is missing for sample variant: " + matching_variants[0]);

								int var_allele_count = format_value_items[genotype_idx].count('1');

								if (var_allele_count > 2) THROW(FileParseException, "Invalid genotype '" + format_value_items[genotype_idx].trimmed() + "' in sample variant: " + matching_variants[0]);

								allele_count -= var_allele_count;

								//prevent allele count to drop below 0
								allele_count = std::max(allele_count, 0);

								if (detail_output)
								{
									//get patient allele
									Sequence ref = split_line[3].trimmed();
									Sequence alt = split_line[4].trimmed();
									for (int j = 0; j < var_allele_count; j++)
									{
										patient_alleles << ref + ">" + alt;
									}
								}
							}

							if (allele_count > 0)
							{
								//calculate PRS part
								prs += weight * allele_count;

								++c_found;

								if (detail_output)
								{
									//get patient allele
									for (int j = 0; j < allele_count; j++)
									{
										patient_alleles << prs_variant.ref() + ">" + prs_variant.ref();
									}
								}
							}
							//else: both alleles contain (non wt) variants

							if (patient_alleles.size() > 2)
							{
								THROW(ArgumentException, "More than 2 alleles found at position " + prs_variant.chr().strNormalized(true) + ":" + QByteArray::number(prs_variant.start()) + "!");
							}

						}
						else
						{
							//sufficient depth (checked previously) & no call => both alleles wildtype
							allele_count = 2;
							++c_found;

							//both allele are ref
							if (detail_output) patient_alleles << prs_variant.ref() + ">" + prs_variant.ref() << prs_variant.ref() + ">" + prs_variant.ref();
						}

					}
					else
					{
						foreach(const QByteArray& line, matching_lines)
						{
							// check if overlapping variant is actually the one we are looking for
							QByteArrayList parts = line.split('\t');
							if(parts[1].toInt()==prs_variant.start() && parts[3]==prs_variant.ref() && parts[4]==prs_variant.alt(0))
							{
								matching_variants.append(line);
							}
							else if(detail_output) //get alt allele of patient
							{
								//get genotype
								QByteArrayList split_line = line.split('\t');
								QByteArrayList format_header_items = split_line[8].split(':');
								QByteArrayList format_value_items = split_line[9].split(':');
								int genotype_idx = format_header_items.indexOf("GT");
								if(genotype_idx < 0) THROW(FileParseException, "Genotype information is missing for sample variant: " + matching_variants[0]);

								int var_allele_count = format_value_items[genotype_idx].count('1');

								if (var_allele_count > 2) THROW(FileParseException, "Invalid genotype '" + format_value_items[genotype_idx].trimmed() + "' in sample variant: " + matching_variants[0]);

								//get patient allele
								Sequence ref = split_line[3].trimmed();
								Sequence alt = split_line[4].trimmed();
								for (int j = 0; j < var_allele_count; j++)
								{
									patient_alleles << ref + ">" + alt;
								}
							}
						}

						if(matching_variants.size() > 1)
						{
							THROW(FileParseException, "Variant occurs multiple times in sample VCF: " +  prs_variant.toString());
						}

						if(matching_variants.size() == 1)
						{
							//get genotype
							QByteArrayList split_line = matching_variants[0].split('\t');
							QByteArrayList format_header_items = split_line[8].split(':');
							QByteArrayList format_value_items = split_line[9].split(':');
							int genotype_idx = format_header_items.indexOf("GT");
							if(genotype_idx < 0) THROW(FileParseException, "Genotype information is missing for sample variant: " + matching_variants[0]);

							allele_count = format_value_items[genotype_idx].count('1');

							if (allele_count > 2) THROW(FileParseException, "Invalid genotype '" + format_value_items[genotype_idx].trimmed() + "' in sample variant: " + matching_variants[0]);

							//calculate PRS part
							prs += weight * allele_count;

							++c_found;

							if (detail_output)
							{
								for (int j = 0; j < allele_count; j++)
								{
									patient_alleles << prs_variant.ref() + ">" + prs_variant.altString();
								}
								//fill up with ref calls
								if (patient_alleles.size() == 1) patient_alleles << prs_variant.ref() + ">" + prs_variant.ref();

							}
						}
						else //0 matching variants
						{
							//sufficient depth (checked previously) & no call => both alleles wildtype
							allele_count = 0;
							if (detail_output)
							{
								//fill up with ref calls
								if (patient_alleles.size() == 0) patient_alleles << prs_variant.ref() + ">" + prs_variant.ref() << prs_variant.ref() + ">" + prs_variant.ref();
								if (patient_alleles.size() == 1) patient_alleles << prs_variant.ref() + ">" + prs_variant.ref();

							}
						}

						if (patient_alleles.size() > 2)
						{
							qDebug() << patient_alleles;
							THROW(ArgumentException, "More than 2 alleles found at position " + prs_variant.chr().strNormalized(true) + ":" + QByteArray::number(prs_variant.start()) + "!");
						}
					}

				}

				if (detail_output)
				{
					QByteArrayList detail_tsv_line = QByteArrayList() << prs_variant.chr().strNormalized(true) << QByteArray::number(prs_variant.start()) << QByteArray::number(prs_variant.end())
																	  << prs_variant.ref() << ((prs_var_is_wildtype)?prs_variant.ref():prs_variant.altString()) << other_allele
																	  << ((patient_alleles.size() > 0)?patient_alleles[0]:".") << ((patient_alleles.size() > 1)?patient_alleles[1]:".")
																	  << ((allele_count < 0)?".":QByteArray::number(allele_count)) << QByteArray::number(weight)
																	  << QByteArray::number(pop_af) <<  column_entries["pgs_id"] << comment.join(";");
					detail_tsv->write(detail_tsv_line.join("\t") + "\n");
				}
			}

			// compute percentile
			int percentile = -1;
			QByteArray percentile_string = ".";
			if (percentiles.size() == 100)
			{
				for (int i = 0; i < percentiles.size(); ++i)
				{
					if (prs < percentiles[i])
					{
						percentile = i + 1;
						break;
					}
				}
				percentile_string = (percentile == -1) ? "100" : QByteArray::number(percentile);
			}
			else if (percentiles.size() != 0)
			{
				THROW(FileParseException, "Invalid number of percentiles given (required: 100 or 0, given: "  + QByteArray::number(percentiles.size()) + ")!");
			}

			QByteArrayList prs_line = QByteArrayList() << column_entries["pgs_id"] << column_entries["trait"] << QByteArray::number(prs)
													   << percentile_string << column_entries["build"] << QByteArray::number(prs_variant_list.count()) << QByteArray::number(c_low_depth)
													   << QByteArray::number(c_imputed) << column_entries["pgp_id"] << column_entries["citation"];
			output_tsv->write(prs_line.join("\t") + "\n");


			//print final PRS
			out << column_entries["pgs_id"] << ": variants_found=" << c_found << " prs=" << prs << " percentile=" << percentile_string << " low_depth_variants=" << c_low_depth
				<< " variants_imputed=" << c_imputed << endl;
		}

		output_tsv->flush();
		output_tsv->close();

		if (detail_output)
		{
			detail_tsv->flush();
			detail_tsv->close();
		}

	}
};


#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
