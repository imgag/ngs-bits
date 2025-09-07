#include "ToolBase.h"
#include "BamWriter.h"
#include "TabixIndexedFile.h"

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
		setDescription("Removes reads which contain the provided variants");
		addInfile("in", "Input BAM/CRAM file.", false);
		addOutfile("out", "Output BAM/CRAM file.", false);
		addInfile("vcf", "Input indexed VCF.GZ file.", false);

		//optional
		addInfile("ref", "Reference genome for CRAM support (mandatory if CRAM is used).", true);
		addFlag("mask", "Replace variant bases with reference instead of removing the read (SNV only)");
		addFlag("single_end", "Input file is from single-end sequencing (e.g. lrGS).");
		addFlag("keep_indels", "Do not remove InDels in mask mode.");

		changeLog(2024, 7, 24, "Inital commit.");
		changeLog(2025, 1, 17, "Added mask option.");
		changeLog(2025, 1, 20, "Added single-end mode.");
	}

	bool alignment_pass(BamAlignment& al, const BamReader& reader) const
	{
		//get variants
		QByteArrayList matches = vcf.getMatchingLines(reader.chromosome(al.chromosomeID()), al.start(), al.end(), true);

		if (matches.count() == 0) return true; //no overlapping variants

		foreach (const QByteArray& match, matches)
		{
			QByteArrayList columns = match.split('\t');
			VcfLine vcf_line(Chromosome(columns.at(0)), columns.at(1).toInt(), columns.at(3), QList<Sequence>() << columns.at(4));
			Variant var(vcf_line);

			if (var.isSNV())
			{
				QPair<char, int> base = al.extractBaseByCIGAR(var.start());
				Sequence read_base;
				read_base.append(base.first);

				if (read_base == var.obs()) return false;
				//special handling of undefined ID observed base:
				if ((var.obs() == "<NON_REF>") && (read_base != var.ref())) return false;
			}
			else //InDel
			{
				//remove all reads which has an InDel at this position.
				QList<Sequence> indels = al.extractIndelsByCIGAR(var.start(), 50);
				if (indels.size() > 0) return false;
			}
		}

		return true;
	}

	QPair<bool,BamAlignment> mask_alignment(BamAlignment& al, const BamReader& reader) const
	{
		//get variants
		QByteArrayList matches = vcf.getMatchingLines(reader.chromosome(al.chromosomeID()), al.start(), al.end(), true);

		if (matches.count() == 0) return QPair<bool,BamAlignment>(true, al); //no overlapping variants

		foreach (const QByteArray& match, matches)
		{
			QByteArrayList columns = match.split('\t');
			VcfLine vcf_line(Chromosome(columns.at(0)), columns.at(1).toInt(), columns.at(3), QList<Sequence>() << columns.at(4));
			Variant var(vcf_line);

			if (var.isSNV())
			{
				int read_pos = -1;
                QPair<char, int> base = al.extractBaseByCIGAR(var.start(), &read_pos);
				Sequence read_base;
				read_base.append(base.first);

				//if read contains observed variant -> change sequence
				if ((read_base == var.obs()) || ((var.obs() == "<NON_REF>") && (read_base != var.ref())))
				{
					//replace sequence:
					Sequence read_sequence = al.bases();
					if ((read_pos < 0) || (read_pos >= read_sequence.size())) THROW(ArgumentException, "Invalid read position " + QString::number(read_pos) + "!");
					read_sequence[read_pos] = var.ref().at(0);
					al.setBases(read_sequence);
				}
			}
			else //InDel
			{
				//remove all reads which has an InDel at this position. (InDels cannot be fixed easily)
				QList<Sequence> indels = al.extractIndelsByCIGAR(var.start(), 50);
				if (indels.size() > 0)
				{
					return QPair<bool,BamAlignment>(getFlag("keep_indels"), al);
				}
			}
		}

		return QPair<bool,BamAlignment>(true, al);
	}

	virtual void main()
	{
		//init
		QTextStream out(stdout);

		int count_pass = 0;
		int count_modified = 0;
		int count_fail = 0;
		int count_skipped = 0;


		BamReader reader(getInfile("in"), getInfile("ref"));
		BamWriter writer(getOutfile("out"), getInfile("ref"));
		writer.writeHeader(reader);

		//read VCF file:
		vcf.load(getInfile("vcf").toUtf8());

		//get flags
		bool mask = getFlag("mask");
		bool single_end = getFlag("single_end");

		//process alignments
		BamAlignment al;
		QHash<QByteArray, BamAlignment> cache; //tracks alignments until mate is seen
		QHash<QByteArray, bool> cache_pass; //tracks pass status of alignments until mate is seen
		while (reader.getNextAlignment(al))
		{
			//skip secondary/supplementary alignments
			if(al.isSecondaryAlignment() || al.isSupplementaryAlignment())
			{
				++count_skipped;
				continue;
			}

			//single-end mode:
			if (single_end)
			{
				if (mask)
				{
					Sequence prev_seq = al.bases();
					QPair<bool,BamAlignment> return_value = mask_alignment(al, reader);

					if (return_value.first)
					{
						writer.writeAlignment(return_value.second);
						++count_pass;
						if (prev_seq != return_value.second.bases()) ++count_modified;
					}
					else
					{
						++count_fail;
					}
				}
				else
				{
					if (alignment_pass(al, reader))
					{
						writer.writeAlignment(al);
						++count_pass;
					}
					else
					{
						++count_fail;
					}
				}
			}
			else
			{
				//paired-end mode:
				QByteArray name = al.name();

				if (!cache.contains(name))
				{
					//mate not seen

					if (mask)
					{
						Sequence prev_seq = al.bases();
						QPair<bool,BamAlignment> return_value = mask_alignment(al, reader);

						//determine pass status
						cache_pass.insert(name, return_value.first);

						//add alignment to cache
						cache.insert(name, return_value.second);

						if (prev_seq != return_value.second.bases()) ++count_modified;

					}
					else
					{
						//determine pass status
						cache_pass.insert(name, alignment_pass(al, reader));

						//add alignment to cache
						cache.insert(name, al);
					}


				}
				else
				{
					//mate seen
					if (mask)
					{
						if (cache_pass.value(name))
						{
							Sequence prev_seq = al.bases();
							QPair<bool,BamAlignment> return_value = mask_alignment(al, reader);

							if (return_value.first)
							{
								//mate passed, this alignment passes, keep alignments
								writer.writeAlignment(cache.take(name));
								writer.writeAlignment(al);
								cache_pass.remove(name);
								++count_pass;
							}
							else
							{
								//this alignment does not pass
								cache.remove(name);
								cache_pass.remove(name);
								++count_fail;
							}

							if (prev_seq != return_value.second.bases()) ++count_modified;
						}
						else
						{
							//mate does not pass
							cache.remove(name);
							cache_pass.remove(name);
							++count_fail;
						}
					}
					else
					{
						if (cache_pass.value(name) && alignment_pass(al, reader))
						{
							//mate passed, this alignment passes, keep alignments
							writer.writeAlignment(cache.take(name));
							writer.writeAlignment(al);
							cache_pass.remove(name);
							++count_pass;
						}
						else
						{
							//mate and/or this alignment does not pass
							cache.remove(name);
							cache_pass.remove(name);
							++count_fail;
						}
					}
				}


			}

		}

        out << "pairs passed: " << count_pass << QT_ENDL;
        out << "pairs dropped: " << count_fail << QT_ENDL;
        out << "reads modified: " << count_modified << QT_ENDL;
        out << "skipped reads: " << count_skipped << QT_ENDL;
	}

private:
	TabixIndexedFile vcf;

};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
