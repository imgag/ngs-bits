#include "ToolBase.h"
#include "api/BamReader.h"
#include "api/BamWriter.h"
#include "api/BamAlgorithms.h"
#include "NGSHelper.h"
#include "Helper.h"
#include <QTime>
#include <QString>
#include "FastqFileStream.h"
#include <QDataStream>

using namespace BamTools;
using readPair = QPair < BamAlignment, BamAlignment>;

class grouping
{
	public:
		int start_pos;
		int end_pos;
		QString barcode;


		bool operator==(const grouping &g1) const
		{
			return ((g1.start_pos == start_pos)&&(g1.end_pos == end_pos)&&(g1.barcode==barcode));
		}
};

inline uint qHash(const grouping &g1)
{
	return qHash(QString::number(g1.start_pos) + QString::number(g1.end_pos) + g1.barcode);
}

class position
{
	public:
		int start_pos;
		int end_pos;
		int chr;


		bool operator==(const position &pos1) const
		{
			return ((pos1.start_pos == start_pos)&&(pos1.end_pos == end_pos)&&(pos1.chr==chr));
		}
		bool operator<(const position &pos1) const
		{
			if (pos1.chr != chr) return (pos1.chr < chr);
			if (pos1.start_pos != start_pos) return (pos1.start_pos < start_pos);
			return (pos1.end_pos < end_pos);
		}
};

struct mip_info
{
	int counter;
	QString name;
	position left_arm;
	position right_arm;
};

inline uint qHash(const position &pos1)
{
	return qHash(QString::number(pos1.start_pos) + QString::number(pos1.end_pos) + pos1.chr);
}

class ConcreteTool
		: public ToolBase
{
	Q_OBJECT

private:
	QHash <QString,QString> createReadIndexHash(QString indexfile)
	{

		FastqFileStream indexstream(indexfile, false);
		QHash <QString,QString> headers2barcodes;
		while (!indexstream.atEnd())//foreach index read
		{
			FastqEntry read1;
			indexstream.readEntry(read1);
			QString string_header = read1.header;
			QStringList header_parts=string_header.split(" ");
			headers2barcodes[header_parts[0]]=read1.bases;
		}
		return headers2barcodes;
	}


	QMap <position,mip_info> createMipInfoMap(QString mip_file)
	{
		QMap <position,mip_info> mip_info_map;
		QFile input_file(mip_file);
		input_file.open(QIODevice::ReadOnly);
		QTextStream in(&input_file);
		QRegExp delimiters_mip_key("(\\-|\\:|\\/)");
		while (!in.atEnd())
		{
			QString line = in.readLine();
			if (line.startsWith(">")) continue;
			QStringList splitted_mip_entry=line.split("\t");
			if (splitted_mip_entry.size()<13) continue;

			position mip_position;
			mip_info new_mip_info;

			QStringList splitted_mip_key=splitted_mip_entry[0].split(delimiters_mip_key);
			mip_position.chr = splitted_mip_key[0].toInt();
			mip_position.start_pos =splitted_mip_key[1].toInt()-1;
			mip_position.end_pos=splitted_mip_key[2].toInt();

			new_mip_info.left_arm.chr = splitted_mip_entry[0].toInt();
			new_mip_info.left_arm.start_pos = splitted_mip_entry[3].toInt();
			new_mip_info.left_arm.end_pos= splitted_mip_entry[4].toInt();


			new_mip_info.right_arm.chr = splitted_mip_entry[0].toInt();
			new_mip_info.right_arm.start_pos = splitted_mip_entry[7].toInt();
			new_mip_info.right_arm.end_pos= splitted_mip_entry[8].toInt();

			//make sure that left arm is really on the left side
			if (new_mip_info.right_arm.start_pos<new_mip_info.left_arm.start_pos)
			{
				std::swap(new_mip_info.left_arm,new_mip_info.right_arm);
			}
			new_mip_info.left_arm.start_pos=new_mip_info.left_arm.start_pos-1;
			new_mip_info.name=splitted_mip_entry.back();
			new_mip_info.counter=0;

			mip_info_map[mip_position]=new_mip_info;
		}
		input_file.close();
		return mip_info_map;
	}

	void writeMipInfoMap(QMap <position,mip_info> mip_info_map, QString outfile_name)
	{
		QMapIterator<position, mip_info > i(mip_info_map);
		i.toBack();
		QFile out(outfile_name);
		out.open(QIODevice::WriteOnly);
		QTextStream outStream(&out);
		outStream <<"chr \tstart\tend\tMIP name\tcount"<<endl;
		while (i.hasPrevious())
		{
			i.previous();
			outStream << i.key().chr <<"\t" << i.key().start_pos<< "\t" << i.key().end_pos <<"\t" << i.value().name <<"\t" << i.value().counter <<endl;
		}
		out.close();
	}

	std::vector <CigarOp> correctCigarString(std::vector <CigarOp> original_cigar_ops, bool cut_front, int cut_bases )
	{
		return original_cigar_ops;
	}

	BamAlignment cutArms(BamAlignment original_alignment, position left_arm, position right_arm)
	{
		QFile out("min_mip.txt");
		out.open(QIODevice::Append);
		QTextStream outStream(&out);

		//cut on right side
		if (original_alignment.GetEndPosition()>right_arm.start_pos)
		{
			int elems_to_cut=(original_alignment.GetEndPosition())-right_arm.start_pos;
			outStream<<QString::fromStdString(original_alignment.Name)<<"\t"<<NGSHelper::Cigar2QString(original_alignment.CigarData)<<"\t"<<endl;
			outStream.flush();
			//cut bases and qualties
			original_alignment.QueryBases=original_alignment.QueryBases.substr(0,(original_alignment.QueryBases.size()-elems_to_cut));
			original_alignment.Qualities=original_alignment.Qualities.substr(0,(original_alignment.Qualities.size()-elems_to_cut));
			//check that CIGAR consists only matches there
				//correct CIGAR
		}

		//cut on left side
		if (original_alignment.Position<left_arm.end_pos)
		{
			int elems_to_cut=left_arm.end_pos-original_alignment.Position;
			outStream.flush();
			//cut bases and qualties
			original_alignment.QueryBases=original_alignment.QueryBases.substr(elems_to_cut);
			original_alignment.Qualities=original_alignment.Qualities.substr(elems_to_cut);
			//correct start
			original_alignment.Position=original_alignment.Position+elems_to_cut;
			//check that CIGAR consists only matches there
			//correct CIGAR
		}
		out.close();
		return original_alignment;
	}

public:
	ConcreteTool(int& argc, char *argv[])
		: ToolBase(argc, argv)
	{
	}

	virtual void setup()
	{
		setDescription("Removes duplicates from a bam file based on a molecular barcode file.");
		addInfile("bam", "Input BAM file.", false);
		addInfile("index", "Index FASTQ file.", false);
		addOutfile("out", "Output BAM file.", false);
		addFlag("flag", "flag duplicate reads insteadt of deleting them");
		addInfile("mip_file","input file for moleculare inversion probes (reads are trimmed to minimum MIP size to avoid readthrough).", true, "");
		addOutfile("mip_count_out","Output TSV file for counts of given mips).", true, "");
	}

	virtual void main()
	{
		//step 1: init
		QHash <QString,QString> read_headers2barcodes=createReadIndexHash(getInfile("index"));//build read_header => index hash
		BamReader reader;
		NGSHelper::openBAM(reader, getInfile("bam"));
		BamWriter writer;
		QHash <grouping, readPair > read_groups;
		writer.Open(getOutfile("out").toStdString(), reader.GetConstSamHeader(), reader.GetReferenceData());
		QString mip_count_out=getOutfile("mip_count_out");
		QString mip_file= getInfile("mip_file");
		BamAlignment al;
		QHash<QString, BamAlignment> al_map;
		int counter = 1;

		int last_start_pos=0;
		int last_ref=-1;
		int new_ref=-1;
		bool chrom_change=false;


		QMap <position,mip_info> mip_info_map= createMipInfoMap(mip_file);

		while (reader.GetNextAlignment(al))
		{
			if (((counter%10000)==0)||((chrom_change)&&(last_ref!=-1)))//reset read_groups hash after every 10000th reads to reduce memory requirements
			{
				QHash <grouping, readPair > read_groups_new;
				QHash <grouping, readPair >::iterator i;
				for (i = read_groups.begin(); i != read_groups.end(); ++i)
				{
					/*make sure that no duplicate is missed because read_groups hash reset
					(won't work if bam is not sorted by position)*/
					if ((i.key().end_pos)<last_start_pos||(chrom_change))
					{
						if (mip_file!="")
						{
							position act_position;
							act_position.chr=last_ref;
							act_position.start_pos=i.key().start_pos;
							act_position.end_pos=i.key().end_pos;
							if (mip_info_map.contains(act_position))//trim and count reads that can be matched to mips
							{
								mip_info_map[act_position].counter++;
								i.value().first=cutArms(i.value().first,mip_info_map[act_position].left_arm,mip_info_map[act_position].right_arm);
								i.value().second=cutArms(i.value().second,mip_info_map[act_position].left_arm,mip_info_map[act_position].right_arm);
								writer.SaveAlignment(i.value().first);
								writer.SaveAlignment(i.value().second);
							}
						}
						else
						{
							writer.SaveAlignment(i.value().first);
							writer.SaveAlignment(i.value().second);
						}
					}
					else
					{
						read_groups_new[i.key()]=i.value();
					}
				}
				read_groups=read_groups_new;
				chrom_change = false;
			}
			last_ref=new_ref;

			if((!al.IsPrimaryAlignment())||(!al.IsPaired())) continue;

			if((al_map.contains(QString::fromStdString(al.Name))))//if paired end and mate has been seen already
			{
					BamAlignment mate;
					mate = al_map.take(QString::fromStdString(al.Name));

					grouping act_group;
					act_group.barcode= read_headers2barcodes["@"+QString::fromStdString(al.Name)];
					act_group.start_pos= qMin(al.Position,mate.Position);
					act_group.end_pos= qMax(al.GetEndPosition(),mate.GetEndPosition());
					//write pair to group, possibly overwriting previous read pairs of same group;

					readPair act_read_pair(al,mate);
					read_groups[act_group]=act_read_pair;
			}
			else//if paired end and mate has not been seen yet
			{
				al_map.insert(QString::fromStdString(al.Name), al);
				continue;
			}

			last_start_pos=qMin(al.Position,al.GetEndPosition());

			if (al.RefID!=last_ref)
			{
				new_ref=al.RefID;
				chrom_change=true;
			}

		}

		//write remaining pairs
		QHash <grouping, readPair >::iterator i;
		for (i = read_groups.begin(); i != read_groups.end(); ++i)
		{
			if (mip_file!="")
			{
				position act_position;
				act_position.chr=last_ref;
				act_position.start_pos=i.key().start_pos;
				act_position.end_pos=i.key().end_pos;
				if (mip_info_map.contains(act_position))
				{
					mip_info_map[act_position].counter++;
				}
			}
			writer.SaveAlignment(i.value().first);
			writer.SaveAlignment(i.value().second);
		}


		//done
		reader.Close();
		writer.Close();
		if ((mip_file!="")&&(mip_count_out!="")) writeMipInfoMap(mip_info_map,mip_count_out);
	}

};
#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
