#include "ToolBase.h"
#include "Settings.h"
#include "FastaFileIndex.h"
#include "Exceptions.h"
#include "Helper.h"
#include "OntologyTermCollection.h"

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
		setDescription("Checks a VCF file for errors.");
		setExtendedDescription(QStringList() << "Checks the input VCF file with SNVs and small InDels for errors and warnings." << "If the VEP-based CSQ annotation is present, it also checks that the Miso terms in the consequence field are valid.");
		//optional
		addInfile("in", "Input VCF file. If unset, reads from STDIN.", true, true);
		addOutfile("out", "Output file. If unset, writes to STDOUT.", true, true);
		addInt("lines", "Number of lines to check in the VCF file (unlimited if 0)", true, 1000);
		addInfile("ref", "Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.", true, false);
		addFlag("info", "Add general information about the input file to the output.");
		changeLog(2018, 12, 3, "Initial implementation.");
	}

	void printInfo(QSharedPointer<QFile> out, QByteArray message)
	{
		if (getFlag("info"))
		{
			out->write(message.trimmed() + "\n");
		}
	}

	void printWarning(QSharedPointer<QFile> out, QByteArray message, int l, const QByteArray& line)
	{
		out->write("WARNING: " + message.trimmed() + " - in line " + QByteArray::number(l+1) + ":\n" + line + "\n");
	}

	void printError(QSharedPointer<QFile> out, QByteArray message, int l, const QByteArray& line)
	{
		out->write("ERROR: " + message.trimmed() + " - in line " + QByteArray::number(l+1) + ":\n" + line + "\n");
		exit(1);
	}

	struct DefinitionLine
	{
		QByteArray id;
		QByteArray description;
		QByteArray type;
		QByteArray number;
		int used = 0;

		QByteArray toString() const
		{
			QByteArray output;
			output += "ID="+id + " ("+QByteArray::number(used)+"x used)";
			if (!type.isEmpty()) output += " Type="+type;
			if (!number.isEmpty()) output += " Number="+number;
			output += " Description="+description;

			return output;
		}
	};

	DefinitionLine parseDefinitionLine(QSharedPointer<QFile> out, int l, QByteArray line)
	{
		if (!line.endsWith(">"))
		{
			printError(out, "Character '>' at end missing!", l ,line);
		}

		int start = line.indexOf('<');
		if (start==-1)
		{
			printError(out, "Character '<' at beginning missing!", l ,line);
		}

		QByteArray def_type = line.mid(2, start-3);

		DefinitionLine output;
		QByteArrayList parts = line.mid(start+1, line.length()-start-2).split(',');
		foreach(const QByteArray& entry, parts)
		{
			int sep = entry.indexOf('=');
			if (sep==-1)
			{
				output.description += entry;
			}
			else
			{
				QByteArray name = entry.left(sep).trimmed();
				QByteArray value = entry.mid(sep+1).trimmed();
				if (name=="ID")
				{
					output.id = value;
				}
				else if (name=="Description")
				{
					output.description = value;
				}
				else if (name=="Number")
				{
					output.number = value;
				}
				else if (name=="Type")
				{
					output.type = value;
				}
			}
		}

		if (output.id.isEmpty())
		{
			printError(out, "Entry 'ID' missing!", l, line);
		}

		if (output.description.isEmpty())
		{
			printError(out, "Entry 'Description' missing!", l, line);
		}

		if (!output.number.isEmpty())
		{
			if (def_type!="FORMAT" && def_type!="INFO")
			{
				printError(out, def_type+" definition cannot have a 'Number' entry!", l, line);
			}

			if (output.number!="." && output.number!="G" && output.number!="A" && output.number!="R" && output.number.toInt()<=0)
			{
				printError(out, def_type+" definition has invalid 'Number' field ", l, line);
			}
		}

		if (!output.type.isEmpty())
		{
			if (def_type!="FORMAT" && def_type!="INFO")
			{
				printError(out, def_type+" definition cannot have a 'Number' entry!", l, line);
			}

			if (output.type!="Integer" && output.type!="Float" && output.type!="Character" && output.type!="String")
			{
				if (output.type!="Flag" || def_type!="INFO")
				{
					printError(out, def_type+" definition cannot have a 'Type' entry of '" + output.type + "'!", l, line);
				}
			}
		}


		return output;
	}

	void checkValues(const DefinitionLine& def, const QByteArrayList& values, int alt_count, const QByteArray& sample, QSharedPointer<QFile> out_p, int l, const QByteArray& line)
	{
		//check number of values
		int expected = -1;
		if (def.number=="A")
		{
			expected = alt_count;
		}
		else if (def.number=="R")
		{
			expected = alt_count + 1;
		}
		else if (def.number.toInt()>0)
		{
			expected = def.number.toInt();
		}
		else
		{
			//"G" and "." are not checked
		}
		if (expected!=-1 && expected!=values.count())
		{
			QByteArray where = sample.isEmpty() ? "INFO" : "sample '" + sample + " / annotation";
			printWarning(out_p, "For " + where + " '" + def.id + "' (number=" + def.number + "), the number of values is " + QByteArray::number(values.count()) + ", but should be " + QByteArray::number(expected) + "!", l, line);
		}

		//check value type
		foreach(const QByteArray& value, values)
		{
			bool value_valid = true;
			if (def.type=="Integer")
			{
				if (value!=".")
				{
					value.toInt(&value_valid);
				}
			}
			else if (def.type=="Float")
			{
				if (value!=".")
				{
					value.toFloat(&value_valid);
				}
			}
			else if (def.type=="Character")
			{
				value_valid = value.length()==1;
			}
			else if (def.type=="String")
			{
				//nothing to check
			}
			if (!value_valid)
			{
				QByteArray where = sample.isEmpty() ? "INFO" : "sample '" + sample + " / annotation";
				printWarning(out_p, "For " + where + " '" + def.id + "', the value '" + value + "' is not a '" + def.type + "'!", l, line);
			}
		}
	}

	virtual void main()
	{
		//init
		QString in = getInfile("in");
		if (in.endsWith(".gz") || in.endsWith(".bgz")) THROW(ArgumentException, "VcfCheck does not support zipped VCFs!");
		QSharedPointer<QFile> in_p = Helper::openFileForReading(in, true);
		QString out = getOutfile("out");
		QSharedPointer<QFile> out_p = Helper::openFileForWriting(out, true);
		int lines = getInt("lines");
		if (lines<=0) lines = std::numeric_limits<int>::max();

		//open refererence genome file (to check reference bases)
		QString ref_file = getInfile("ref");
		if (ref_file=="") ref_file = Settings::string("reference_genome");
		if (ref_file=="") THROW(CommandLineParsingException, "Reference genome FASTA unset in both command-line and settings.ini file!");
		FastaFileIndex reference(ref_file);

		//load MISO terms
		OntologyTermCollection obo_terms("://Resources/so-xp_3_0_0.obo", true);

		//perform checks
		QMap<QByteArray, DefinitionLine> defined_filters;
		QMap<QByteArray, DefinitionLine> defined_formats;
		QMap<QByteArray, DefinitionLine> defined_infos;
		QByteArrayList defined_samples;
		int expected_parts = 8;
		bool in_header = true;
		int c_data = 0;
		int l = 1;
		while(!in_p->atEnd() && l<lines)
		{
			QByteArray line = in_p->readLine().trimmed();

			//skip empty lines
			if (line.isEmpty()) continue;

			//check first line (VCF format)
			if (l==1)
			{
				if (!line.startsWith("##fileformat=VCFv"))
				{
					printError(out_p, "First line must be 'fileformat' line!", l, line);
				}
				printInfo(out_p, "VCF version: " + line.mid(17));
			}

			if (line.startsWith("#"))
			{
				//check all header lines are at the beginning of the file
				if (!in_header)
				{
					printError(out_p, "Header lines are not allowed in VCF body!", l, line);
				}

				//##INFO=<ID=NS,Number=1,Type=Integer,Description="Number of samples with data">
				if (line.startsWith("##INFO=<"))
				{
					DefinitionLine data = parseDefinitionLine(out_p, l, line);

					//check for duplicates
					if (defined_infos.contains(data.id))
					{
						printError(out_p, "INFO '" + data.id + "' defined twice!", l, line);
					}

					defined_infos[data.id] = data;
				}

				//##FORMAT=<ID=GT,Number=1,Type=String,Description="Genotype">
				else if (line.startsWith("##FORMAT=<"))
				{
					DefinitionLine data = parseDefinitionLine(out_p, l, line);

					//check for duplicates
					if (defined_formats.contains(data.id))
					{
						printError(out_p, "FORMAT '" + data.id + "' defined twice!", l, line);
					}

					defined_formats[data.id] = data;
				}

				//##FILTER=<ID=off-target,Description="Variant marked as 'off-target'.">
				else if (line.startsWith("##FILTER=<"))
				{
					DefinitionLine data = parseDefinitionLine(out_p, l, line);

					//check for duplicates
					if (defined_filters.contains(data.id))
					{
						printError(out_p, "FILTER '" + data.id + "' defined twice!", l, line);
					}

					defined_filters[data.id] = data;
				}
				//other ## header lines
				else if (line.startsWith("##"))
				{
					//not much to check here
				}
				else //main header line
				{
					QByteArrayList parts = line.split('\t');

					if (parts.count()<8)
					{
						printError(out_p, "Header line with less than 8 fields!", l, line);
					}
					if (parts.count()==9)
					{
						printError(out_p, "Header line with FORMAT, but without samples!", l, line);
					}
					if (parts.count()>9)
					{
						defined_samples = parts.mid(9);
						expected_parts = 9 + defined_samples.count();
					}

					//flag as not in header anymore
					in_header = false;
				}
			}
			//data line: chr1	62732421	rs11207949	T	C	13777.5	off-target	AB=0.515753;ABP=4.85745;AC=1;AF=0.5;AN=2;AO=442;CIGAR=1X;DP=857;DPB=857;DPRA=0;EPP=3.50158;EPPR=3.89459;GTI=0;LEN=1;MEANALT=1;MQM=60;MQMR=60;NS=1;NUMALT=1;ODDS=3022.34;PAIRED=0.995475;PAIREDR=1;PAO=0;PQA=0;PQR=0;PRO=0;QA=16752;QR=15862;RO=415;RPL=437;RPP=919.863;RPPR=827.694;RPR=5;RUN=1;SAF=221;SAP=3.0103;SAR=221;SRF=210;SRP=3.14111;SRR=205;TYPE=snp;technology.ILLUMINA=1;CSQ=C|missense_variant|MODERATE|KANK4|KANK4|transcript|NM_181712.4|Coding|6/10|c.2302A>G|p.Thr768Ala|2679/5477|2302/2988|768/995||;dbNSFP_Polyphen2_HDIV_pred=B,B;dbNSFP_phyloP100way_vertebrate=2.380000;dbNSFP_MutationTaster_pred=P;dbNSFP_Polyphen2_HVAR_pred=B,B;dbNSFP_SIFT_pred=T;ESP6500EA_AF=0.2449;ESP6500AA_AF=0.2229;1000G_AF=0.305511;EXAC_AF=0.284	GT:GL:DP:RO:QR:AO:QA	0/1:-1506.31,0,-1426.21:857:415:15862:442:16752
			else
			{
				++c_data;

				//split line
				QByteArrayList parts = line.split('\t');

				//check that the number of elements is correct
				if (parts.count()<expected_parts)
				{
					printError(out_p, "Data line with " + QByteArray::number(parts.count()) + " elements, expected " + QByteArray::number(expected_parts) + "!", l, line);
				}

				//chromosome
				Chromosome chr(parts[0]);
				if (chr.str().contains(":"))
				{
					printError(out_p, "Chromosome '" + parts[0] + "' is not valid!", l, line);
				}

				//position
				bool pos_is_valid;
				int pos = parts[1].toInt(&pos_is_valid);
				if(!pos_is_valid)
				{
					printError(out_p, "Chromosomal position '" + parts[1] + "' is not a number!", l, line);
				}

				//reference base
				QByteArray ref = parts[3].toUpper();
				if (pos_is_valid)
				{
					Sequence ref_exp = reference.seq(chr, pos, ref.length());
					if (ref!=ref_exp)
					{
						printError(out_p, "Reference base(s) not correct. Is '" + ref + "', should be '" + ref_exp + "'!", l, line);
					}
				}

				//alternate base(s)
				QByteArrayList alts = parts[4].split(',');

				//quality
				const QByteArray& qual = parts[5];
				if (qual!=".")
				{
					bool ok = false;
					qual.toDouble(&ok);
					if (!ok)
					{
						printError(out_p, "Invalid quality value '" + qual + "'!", l, line);
					}
				}

				//filter
				const QByteArray& filter = parts[6];
				if (filter!="." && filter!="PASS")
				{
					QByteArrayList filters = filter.split(';');
					foreach(const QByteArray& name, filters)
					{
						if (!defined_filters.contains(name))
						{
							printWarning(out_p, "FILTER '" + name + "' used but not defined!", l, line);
						}
						else
						{
							defined_filters[name].used +=1;
						}
					}
				}

				//info
				QByteArrayList info = parts[7].split(';');
				foreach(const QByteArray& entry, info)
				{
					int sep = entry.indexOf('=');
					bool has_value = sep!=-1;
					QByteArray name = has_value ? entry.left(sep) : entry;
					QByteArray value = has_value ? entry.mid(sep+1).trimmed() : "";

					bool is_defined = defined_infos.contains(name);
					if (is_defined)
					{
						defined_infos[name].used +=1;
					}
					else
					{
						printWarning(out_p, "INFO '" + name + "' used but not defined!", l, line);
					}

					//check flags
					if (is_defined)
					{
						if (defined_infos[name].type!="Flag" && !has_value)
						{
							printError(out_p, "Non-flag INFO '" + name + "' has no value!", l, line);
						}
						if (defined_infos[name].type=="Flag" && has_value)
						{
							printError(out_p, "Flag INFO '" + name + "' has a value (" + value + ")!", l, line);
						}
					}

					//check value (number, type)
					if (is_defined && has_value)
					{
						const DefinitionLine& current_info = defined_infos[name];
						QByteArrayList values = value.split(',');
						checkValues(current_info, values, alts.count(), QByteArray(), out_p, l, line);
					}

					//check MISO ontology entries in CSQ:IMPACT (split by &)
					if (name=="CSQ")
					{
						QByteArrayList csq_defs = defined_infos[name].description.split('|');
						QByteArrayList csq_transcripts = value.split(',');
						int i_consequence = csq_defs.indexOf("Consequence");
						foreach(const QByteArray& csq_transcript, csq_transcripts)
						{
							QByteArrayList csq_parts = csq_transcript.split('|');
							if (csq_parts.count()!=csq_defs.count())
							{
								printError(out_p, "VEP-based CSQ annoation has " + QByteArray::number(csq_parts.count()) + " entries, expected " + QByteArray::number(csq_defs.count()) + " according to definition in header!", l, line);
							}

							QByteArrayList terms = csq_parts[i_consequence].split('&');
							foreach(const QByteArray& term, terms)
							{
								if(!obo_terms.containsByName(term))
								{
									printWarning(out_p, "Unknown MISO term '" + term + "' used!", l, line);
								}
							}

						}
					}
				}

				//format
				if (parts.count()==8) continue;
				QByteArrayList format_names = parts[8].split(':');
				foreach(const QByteArray& name, format_names)
				{
					if (!defined_formats.contains(name))
					{
						printWarning(out_p, "FORMAT '" + name + "' used but not defined!", l, line);
					}
					else
					{
						defined_formats[name].used +=1;
					}
				}

				//samples
				for (int s=0; s<defined_samples.count(); ++s)
				{
					QByteArrayList sample_data = parts[9+s].split(':');

					//check the number of entries
					if (format_names.count()!=sample_data.count())
					{
						printError(out_p, "Sample " + defined_samples[s] + " has " + QByteArray::number(sample_data.count()) + " entries, expected " + QByteArray::number(format_names.count()) + " according to FORMAT entry!", l, line);
					}

					//check values (number, type)

					for (int i=0; i<format_names.count(); ++i)
					{
						const DefinitionLine& current_format = defined_formats[format_names[i]];
						QByteArrayList values = sample_data[i].split(',');
						checkValues(current_format, values, alts.count(), defined_samples[s], out_p, l, line);
					}
				}
			}

			++l;
		}


		//output infos
		foreach(const DefinitionLine& filter, defined_filters)
		{
			printInfo(out_p, "FILTER: " + filter.toString());
		}
		foreach(const DefinitionLine& filter, defined_infos)
		{
			printInfo(out_p, "INFO: " + filter.toString());
		}
		foreach(const DefinitionLine& filter, defined_formats)
		{
			printInfo(out_p, "FORMAT: " + filter.toString());
		}
		foreach(const QByteArray& sample, defined_samples)
		{
			printInfo(out_p, "SAMPLE: " + sample);
		}
		printInfo(out_p, "Finished - checked " + QByteArray::number(l) + " lines - " + QByteArray::number(c_data) + " data lines.");
    }
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

