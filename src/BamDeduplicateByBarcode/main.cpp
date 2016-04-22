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

		position()=default;

		position(int start_pos_ini, int end_pos_ini, int chr_ini)
		{
			start_pos=start_pos_ini;
			end_pos=end_pos_ini;
			chr=chr_ini;
		}

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

struct most_frequent_read_selection
{
	readPair most_freq_read;
	QList <readPair> duplicates;
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

			QStringList splitted_mip_key=splitted_mip_entry[0].split(delimiters_mip_key);
			position mip_position(splitted_mip_key[1].toInt()-1,splitted_mip_key[2].toInt(),splitted_mip_key[0].toInt());

			mip_info new_mip_info;
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

	std::vector <CigarOp> correctCigarString(std::vector <CigarOp> original_cigar_ops, bool cut_front, int cutted_bases)
	{
		if (cut_front)
		{
			for(unsigned int i=0; i<original_cigar_ops.size(); ++i)
			{
				CigarOp co = original_cigar_ops[i];
				if ((co.Type=='M')||(co.Type=='=')||(co.Type=='X')||(co.Type=='I')||(co.Type=='S'))
				{
					int original_operation_length=co.Length;
					co.Length=(unsigned int)qMax(0,(int)co.Length-cutted_bases);
					cutted_bases=cutted_bases-original_operation_length;
					original_cigar_ops[i]=co;
					if (cutted_bases<=0)
					{
						break;
					}
				}
			}
		}
		else
		{
			for(int i=original_cigar_ops.size()-1; i>=0; --i)
			{
				CigarOp co = original_cigar_ops[i];
				if ((co.Type=='M')||(co.Type=='=')||(co.Type=='X')||(co.Type=='I')||(co.Type=='S'))
				{
					int original_operation_length=co.Length;
					co.Length=(unsigned int)qMax(0,(int)co.Length-cutted_bases);
					cutted_bases=cutted_bases-original_operation_length;
					original_cigar_ops[i]=co;
					if (cutted_bases<=0)
					{
						break;
					}
				}
			}
		}

		//remove CIGAR Operations of Length 0
		std::vector <CigarOp> cigar_ops_out=original_cigar_ops;
		int deleted_elem_count=0;
		for(unsigned int i=0; i<original_cigar_ops.size(); ++i)
		{
			CigarOp co = original_cigar_ops[i];
			if (co.Length==0)
			{
				cigar_ops_out.erase(cigar_ops_out.begin()+i-deleted_elem_count);
				++deleted_elem_count;
			}
		}
		return cigar_ops_out;//TODO: Raise error?
	}

	most_frequent_read_selection find_highest_freq_read(QList <readPair> readpairs)
	{
		//setup seq_count
		QHash <QString, QList <readPair > > readpair_seq_count;

		foreach(readPair readpair, readpairs)
		{
			readpair_seq_count[QString::fromStdString(readpair.first.QueryBases+"+"+readpair.second.QueryBases)].append(readpair);
		}

		//find highest count
		int max_read_count=0;
		QString max_seq;
		foreach(QString seq,readpair_seq_count.keys())
		{
			if ((readpair_seq_count[seq].size())>max_read_count)
			{
				max_read_count=readpair_seq_count[seq].size();
				max_seq=seq;
			}
		}
		most_frequent_read_selection result;
		result.most_freq_read=readpair_seq_count[max_seq].takeLast();

		//collect duplicates
		foreach (QList <readPair> read_pairs,readpair_seq_count.values())
		{
			result.duplicates.append(read_pairs);
		}
		return result;
	}

	BamAlignment cutArmsSingle(BamAlignment original_alignment, position left_arm, position right_arm)
	{
		//cut on right side
		if (original_alignment.GetEndPosition()>right_arm.start_pos)
		{
			int elems_to_cut=(original_alignment.GetEndPosition())-right_arm.start_pos;
			//cut bases and qualties
			original_alignment.QueryBases=original_alignment.QueryBases.substr(0,(original_alignment.QueryBases.size()-elems_to_cut));
			original_alignment.Qualities=original_alignment.Qualities.substr(0,(original_alignment.Qualities.size()-elems_to_cut));
			//correct CIGAR
			original_alignment.CigarData=correctCigarString(original_alignment.CigarData,false,elems_to_cut);
		}

		//cut on left side
		if (original_alignment.Position<left_arm.end_pos)
		{
			int elems_to_cut=left_arm.end_pos-original_alignment.Position;
			//cut bases and qualties
			original_alignment.QueryBases=original_alignment.QueryBases.substr(elems_to_cut);
			original_alignment.Qualities=original_alignment.Qualities.substr(elems_to_cut);
			//correct start
			original_alignment.Position=original_alignment.Position+elems_to_cut;
			//correct CIGAR
			original_alignment.CigarData=correctCigarString(original_alignment.CigarData,true,elems_to_cut);
		}
		return original_alignment;
	}

	most_frequent_read_selection cutAndSelectPair(QList <readPair> original_readpairs, position left_arm, position right_arm)
	{
		QList <readPair> new_alignment_list;
		foreach(readPair original_alignments, original_readpairs)
		{
			readPair new_alignments;
			new_alignments.first=cutArmsSingle(original_alignments.first,left_arm,right_arm);
			new_alignments.second=cutArmsSingle(original_alignments.second,left_arm,right_arm);
			new_alignments.first.MatePosition=new_alignments.second.Position;
			new_alignments.second.MatePosition=new_alignments.first.Position;
			new_alignment_list.append(new_alignments);
		}
		return find_highest_freq_read(new_alignment_list);
	}

	void writeReadsToBed(QTextStream &out_stream, position act_position, QList <readPair> original_readpairs, QString barcode, bool test)
	{
		foreach(readPair original_readpair,original_readpairs)
		{
			QString read_name=QString::fromStdString(original_readpair.first.Name);
			QString seq_1=QString::fromStdString(original_readpair.first.QueryBases);
			QString seq_2=QString::fromStdString(original_readpair.second.QueryBases);
			int counter=0;
			//adjust values of unmapped to be valid bed file coordinates
			if ((act_position.chr<1)||(act_position.start_pos <0)||(act_position.start_pos <0))
			{
				act_position.chr=0;
				act_position.start_pos=0;
				act_position.end_pos=1;
			}
			if (test)
			{
				//omit readname and sequences which randomly sorted in output and thus not comparable
				out_stream << act_position.chr <<"\t" << act_position.start_pos<< "\t" << act_position.end_pos<<"\t" << "group " <<counter<<endl;
				counter++;
			}
			else
			{
				out_stream << act_position.chr <<"\t" << act_position.start_pos<< "\t" << act_position.end_pos <<"\t" << read_name << barcode << '|' << seq_1 << '|'<< seq_2<<endl;
			}
		}
	}

	void writePairToBam(BamTools::BamWriter &writer, readPair read_pair)
	{
		writer.SaveAlignment(read_pair.first);
		writer.SaveAlignment(read_pair.second);
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
		addFlag("test", "adjust output for testing purposes");
		addInfile("mip_file","input file for moleculare inversion probes (reads are filtered and cut to match only MIP inserts).", true, "");
		addOutfile("mip_count_out","Output TSV file for counts of given mips).", true, "");
		addOutfile("mip_nomatch_out","Output Bed file for reads not matching any mips).", true, "");
		addOutfile("mip_duplicate_out","Output Bed file for reads removed as duplicates).", true, "");
	}

	virtual void main()
	{
		//step 1: init
		QHash <QString,QString> read_headers2barcodes=createReadIndexHash(getInfile("index"));//build read_header => index hash
		BamReader reader;
		NGSHelper::openBAM(reader, getInfile("bam"));
		BamWriter writer;
		QHash <grouping, QList<readPair> > read_groups;
		writer.Open(getOutfile("out").toStdString(), reader.GetConstSamHeader(), reader.GetReferenceData());
		QString mip_count_out=getOutfile("mip_count_out");
		QString mip_nomatch_out=getOutfile("mip_nomatch_out");
		QString mip_duplicate_out=getOutfile("mip_duplicate_out");
		QString mip_file= getInfile("mip_file");
		bool test =getFlag("test");
		BamAlignment al;
		QHash<QString, BamAlignment> al_map;
		int counter = 1;

		int last_start_pos=0;
		int last_ref=-1;
		int new_ref=-1;
		bool chrom_change=false;

		QMap <position,mip_info> mip_info_map= createMipInfoMap(mip_file);

		QTextStream nomatch_out_stream;
		QFile nomatch_out(mip_nomatch_out);
		if (mip_nomatch_out!="")
		{
			nomatch_out.open(QIODevice::WriteOnly);
			nomatch_out_stream.setDevice(&nomatch_out);
		}

		QTextStream duplicate_out_stream;
		QFile duplicate_out(mip_duplicate_out);
		if (mip_duplicate_out!="")
		{
			duplicate_out.open(QIODevice::WriteOnly);
			duplicate_out_stream.setDevice(&duplicate_out);
		}

		while (reader.GetNextAlignment(al))
		{
			if (((counter%10000)==0)||((chrom_change)&&(last_ref!=-1)))//reset read_groups hash after every 10000th reads to reduce memory requirements
			{
				QHash <grouping, QList<readPair> > read_groups_new;
				QHash <grouping, QList<readPair> >::iterator i;
				for (i = read_groups.begin(); i != read_groups.end(); ++i)
				{
					/*assure that we already moved pass the readpair so no duplicates are missed by in-between reset of read group
					 * (won't work if input is not sorted by position)*/
					if ((i.key().end_pos)<last_start_pos||(chrom_change))
					{
						position act_position(i.key().start_pos,i.key().end_pos,last_ref);
						if (mip_file!="")
						{
							if (mip_info_map.contains(act_position))//trim, select and count unique reads that can be matched to mips
							{
								mip_info_map[act_position].counter++;
								most_frequent_read_selection read_selection = cutAndSelectPair(i.value(),mip_info_map[act_position].left_arm,mip_info_map[act_position].right_arm);
								writePairToBam(writer, read_selection.most_freq_read);
								writeReadsToBed(duplicate_out_stream,act_position,read_selection.duplicates,i.key().barcode,test);
							}
							else if (mip_nomatch_out!="")//write reads not matching a mip to a bed file
							{
								writeReadsToBed(nomatch_out_stream,act_position,i.value(),i.key().barcode,test);
							}
						}
						else
						{
							most_frequent_read_selection read_selection = find_highest_freq_read(i.value());
							writePairToBam(writer, read_selection.most_freq_read);
							writeReadsToBed(duplicate_out_stream,act_position,read_selection.duplicates,i.key().barcode,test);
						}
					}
					else
					{
						read_groups_new[i.key()].append(i.value());
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
					read_groups[act_group].append(act_read_pair);
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
		QHash <grouping, QList<readPair> >::iterator i;
		for (i = read_groups.begin(); i != read_groups.end(); ++i)
		{
			position act_position(i.key().start_pos,i.key().end_pos,last_ref);
			if (mip_file!="")
			{
				if (mip_info_map.contains(act_position))//trim and count reads that can be matched to mips
				{
					mip_info_map[act_position].counter++;
					most_frequent_read_selection read_selection = cutAndSelectPair(i.value(),mip_info_map[act_position].left_arm,mip_info_map[act_position].right_arm);
					writePairToBam(writer, read_selection.most_freq_read);
					writeReadsToBed(duplicate_out_stream,act_position,read_selection.duplicates,i.key().barcode,test);
				}
				else if (mip_nomatch_out!="")//write reads not matching a mip to a bed file
				{
					writeReadsToBed(nomatch_out_stream,act_position,i.value(),i.key().barcode,test);
				}
			}
			else
			{
				most_frequent_read_selection read_selection = find_highest_freq_read(i.value());
				writePairToBam(writer, read_selection.most_freq_read);
				writeReadsToBed(duplicate_out_stream,act_position,read_selection.duplicates,i.key().barcode,test);
			}
		}


		//done
		if (mip_duplicate_out!="")
		{
			duplicate_out.close();
		}
		//done
		if (mip_nomatch_out!="")
		{
			nomatch_out.close();
		}
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
