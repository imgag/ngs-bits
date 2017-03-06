#include "ToolBase.h"
#include "api/BamReader.h"
#include "api/BamWriter.h"
#include "api/BamAlgorithms.h"
#include "NGSHelper.h"
#include "Helper.h"
#include "cppNGS_global.h"
#include "Settings.h"
#include "ChromosomeInfo.h"
#include "FastqFileStream.h"
#include <QString>
#include <QDataStream>
#include <algorithm>

using namespace BamTools;

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
		setDescription("Annotates strand and family information to variants in a vcf file.");
		setExtendedDescription(QStringList()	<< "Annotates strand information to variants in a vcf file."
												<< "BAM file has to be annotated using BamDeduplicateByBarcode."
							   );
		addInfile("vcf", "Input VCF file.", false);
		addInfile("bam", "Input BAM file.", false);
		addInfile("mip", "Input MIP file.", true);
		addInfile("hpHS", "Input HaloPlex HS file.", true);
		addOutfile("out", "Output VCF file.", false);

		//changelog
		changeLog(2017,01,17,"Initial commit.");
	}

	virtual void main()
	{
		//init
		QString ref_file = Settings::string("reference_genome");
		VariantList variants;
		variants.load(getInfile("vcf"));

		BamReader reader;
		NGSHelper::openBAM(reader, getInfile("bam"));

		//remove all columns
		QList<VariantAnnotationHeader> headers = variants.annotations();
		foreach(const VariantAnnotationHeader& header, headers)
		{
			if (header.name().startsWith("fs"))	variants.removeAnnotationByName(header.name(), true);
		}

		QString hpHS = getInfile("hpHS");
		QString mip = getInfile("mip");
		for (int i=0; i<variants.count(); ++i)
		{
			Variant& variant = variants[i];

			// check if +/- strand amplicons are availabe for this region in a MIP file
			int st_plus = 0;
			int st_minus = 0;
			if(mip!="" && hpHS!="")	THROW(ToolFailedException, "MIP or HaloPlex HS file can not be given in parallel.");
			if(mip!="")
			{
				QSharedPointer<QFile> file = Helper::openFileForReading(mip, true);
				while(!file->atEnd())
				{
					QByteArray line = file->readLine();

					while (line.endsWith('\n') || line.endsWith('\r')) line.chop(1);
					if (line.startsWith(">"))	continue;	// skip comment / header line

					// check that variant is completely covered by amplicons (n.b. deletions)
					QList<QByteArray> fields = line.split('\t');
					if(!variant.overlapsWith(Chromosome("chr"+fields[2]),fields[11].toInt(),fields[12].toInt()))	continue;	//TODO

					QString strand = fields[17];	//TODO
					if(strand=="+")	++st_plus;
					else if(strand=="-")	++st_minus;
					else	THROW(ToolFailedException, "Could not identify if amplicon is lying on + or - strand. Unknown strand " + strand);
				}
//				qDebug() << variant.chr().str() << QString::number(variant.start()) << QString::number(st_plus) << QString::number(st_minus);
			}
			if(hpHS!="")
			{
				QSharedPointer<QFile> file = Helper::openFileForReading(hpHS, true);
				while(!file->atEnd())
				{
					QByteArray line = file->readLine();

					while (line.endsWith('\n') || line.endsWith('\r')) line.chop(1);
					if (line.startsWith(";"))	continue;	// skip comment / header line

					// check that variant is completely covered by amplicons (n.b. deletions)
					QList<QByteArray> fields = line.split('\t');
					if(!variant.overlapsWith(Chromosome(fields[0]),fields[1].toInt()+1,fields[2].toInt()))	continue;

					QString strand = fields[5];	//TODO
					if(strand=="+")	++st_plus;
					else if(strand=="-")	++st_minus;
					else	THROW(ToolFailedException, "Could not identify if amplicon is lying on + or - strand. Unknown strand " + strand);
				}
			}

			// count variants on +/- strand
			int st_mu_plus = 0;
			int st_mu_minus = 0;
			int st_mu_unknown = 0;
			int st_wt_plus = 0;
			int st_wt_minus = 0;
			int st_wt_unknown = 0;
			if (variant.isSNV()) //SVN
			{
				int ref_id = ChromosomeInfo::refID(reader, variant.chr());
				bool jump_ok = reader.SetRegion(ref_id, variant.start()-1, ref_id, variant.start());
				if (!jump_ok) THROW(FileAccessException, QString::fromStdString(reader.GetErrorString()));

				//iterate through all alignments
				BamAlignment al;
				while (reader.GetNextAlignment(al))
				{
					//if (!al.IsProperPair()) continue;
					if (!al.IsPrimaryAlignment()) continue;
					if (al.IsDuplicate()) continue;
					if (!al.IsMapped()) continue;
					if (!al.HasTag("fs"))	THROW(ToolFailedException, "No fs tag available for read " + QString::fromStdString(al.Name));

					//snps
					int read_pos = 0;
					int genome_pos = al.Position;
					bool no_count = false;
					for (unsigned int i=0; i<al.CigarData.size(); ++i)
					{
						const CigarOp& op = al.CigarData[i];

						//update positions
						if (op.Type=='M')
						{
							genome_pos += op.Length;
							read_pos += op.Length;
						}
						else if(op.Type=='I')
						{
							read_pos += op.Length;
						}
						else if(op.Type=='D')
						{
							genome_pos += op.Length;

							//base is deleted
							if (genome_pos>=variant.start())	no_count = true;
						}
						else if(op.Type=='N') //skipped reference bases (for RNA)
						{
							genome_pos += op.Length;

							//base is skipped
							if (genome_pos>=variant.start())	no_count = true;
						}
						else if(op.Type=='S') //soft-clipped (only at the beginning/end)
						{
							read_pos += op.Length;

							//base is soft-clipped
							//Nb: reads that are mapped in paired end mode and completely soft-clipped (e.g. 7I64S, 71S) keep their original left-most (genomic) position
							if(read_pos>=al.Length)	no_count = true;
						}
						else if(op.Type=='H') //hard-clipped (only at the beginning/end)
						{
							//can be irgnored as hard-clipped bases are not considered in the position or sequence
						}
						else
						{
							THROW(Exception, "Unknown CIGAR operation " + QString(QChar(op.Type)) + "!");
						}

//						if(variant.start()==21968199)	qDebug() << QString::number(variant.start()) << QString::fromStdString(al.Name) << QString::number(al.Position) << QString::number(genome_pos) << QString::number(read_pos) << NGSHelper::Cigar2QString(al.CigarData);
						if (genome_pos>=variant.start() && !no_count)
						{
							if(!no_count)
							{

								int actual_pos = read_pos - (genome_pos + 1 - variant.start());
								QString base = QString(al.QueryBases[actual_pos]);

								std::string strand;
								al.GetTag("fs", strand);

								if(base==variant.ref())	//wildtype
								{
									if(strand.compare("+")==0)	++st_wt_plus;
									else if(strand.compare("-")==0)	++st_wt_minus;
									else if(strand.compare(".")==0)	++st_wt_unknown;
									else	THROW(Exception, "Found unknown strand '" + QString::fromStdString(strand) + "' for read " + QString::fromStdString(al.Name) + "!");
								}
								if(base==variant.obs())
								{
									if(strand.compare("+")==0)	++st_mu_plus;
									else if(strand.compare("-")==0)	++st_mu_minus;
									else if(strand.compare(".")==0)	++st_mu_unknown;
									else	THROW(Exception, "Found unknown strand '" + QString::fromStdString(strand) + "' for read " + QString::fromStdString(al.Name) + "!");
								}

								no_count = true;	//found and should not be counted anymore
							}
						}
					}
				}
			}
			else //indel
			{
				//determine region of interest for indel (important for repeat regions where more than one alignment is possible)
				QPair<int, int> reg = Variant::indelRegion(variant.chr(), variant.start(), variant.end(), variant.ref(), variant.obs(), ref_file);

				//get indels from region
				QVector<Sequence> indels;

				//restrict region
				int ref_id = ChromosomeInfo::refID(reader, variant.chr());
				bool jump_ok = reader.SetRegion(ref_id, variant.start()-1, ref_id, (int)variant.end());
				if (!jump_ok)	THROW(FileAccessException, QString::fromStdString(reader.GetErrorString()));

				//iterate through all alignments and create counts
				BamAlignment al;
				while (reader.GetNextAlignment(al))
				{
					bool found = false;

					//skip low-quality reads
					//if (!al.IsProperPair()) continue;
					if (al.IsDuplicate()) continue;
					if (!al.IsPrimaryAlignment()) continue;
					if (!al.IsMapped()) continue;
					if (!al.HasTag("fs"))	continue;

					//skip reads that do not span the whole region
					if (al.Position+1>reg.first || al.GetEndPosition()<reg.second ) continue;

					//load string data
					al.BuildCharData();

					//look up indels
					int read_pos = 0;
					int genome_pos = al.Position+1; //convert to 1-based position
					for (unsigned int i=0; i<al.CigarData.size(); ++i)
					{
						const CigarOp& op = al.CigarData[i];

						//update positions
						if (op.Type=='M')
						{
							genome_pos += op.Length;
							read_pos += op.Length;
						}
						else if(op.Type=='I')
						{
							if (genome_pos>=reg.first && genome_pos<=reg.second)	found = true;
							read_pos += op.Length;
						}
						else if(op.Type=='D')
						{
							if (genome_pos>=reg.first && genome_pos<=reg.second)	found = true;
							genome_pos += op.Length;
						}
						else if(op.Type=='N') //skipped reference bases (for RNA)
						{
							genome_pos += op.Length;
						}
						else if(op.Type=='S') //soft-clipped (only at the beginning/end)
						{
							read_pos += op.Length;
						}
						else if(op.Type=='H') //hard-clipped (only at the beginning/end)
						{
							//can be irgnored as hard-clipped bases are not considered in the position or sequence
						}
						else
						{
							THROW(Exception, "Unknown CIGAR operation " + QString(QChar(op.Type)) + "!");
						}
					}

					if(found)
					{
						std::string strand;
						al.GetTag("fs", strand);
						if(strand.compare("+")==0)	++st_mu_plus;
						else if(strand.compare("-")==0)	++st_mu_minus;
						else if(strand.compare(".")==0)	++st_mu_unknown;
						else	THROW(Exception, "Found unknown strand '" + QString::fromStdString(strand) + "' for read " + QString::fromStdString(al.Name) + "!");
					}
					else
					{
						std::string strand;
						al.GetTag("fs", strand);
						if(strand.compare("+")==0)	++st_wt_plus;
						else if(strand.compare("-")==0)	++st_wt_minus;
						else if(strand.compare(".")==0)	++st_wt_unknown;
						else	THROW(Exception, "Found unknown strand '" + QString::fromStdString(strand) + "' for read " + QString::fromStdString(al.Name) + "!");
					}
				}
			}

			QString field = QString::number(st_mu_plus) + "|" + QString::number(st_mu_minus) + "|" + QString::number(st_mu_unknown) + "," + QString::number(st_wt_plus) + "|" + QString::number(st_wt_minus) +  "|" + QString::number(st_wt_unknown) + ((mip!="" || hpHS!="")?"," + QString::number(st_plus) + "|" + QString::number(st_minus):"");
			variant.annotations().append(field.toLatin1());
		}
		variants.annotations().append(VariantAnnotationHeader("fs"));
		variants.annotationDescriptions().append(VariantAnnotationDescription("fs", "Counts for strand information. Format: [mutation_plus]|[mutation_minus]|[mutation_unknown],[wildtype_plus]|[wildtype_minus]|[wildtype_unknown]" + ((mip!="" || hpHS!="")?QString(",[amplicon_plus_in_design]|[amplicon_minus_in_design]"):"")+".", VariantAnnotationDescription::STRING, false, QString::number(2), true));
		variants.store(getOutfile("out"));
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
