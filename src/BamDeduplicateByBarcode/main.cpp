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
#include <algorithm>

using namespace BamTools;
using readPair = QPair < BamAlignment, BamAlignment>;

class barcode
{
	public:
		int start_pos;
		int end_pos;
		QString barcode_sequence;
		QString barcode_quality;


		bool operator==(const barcode &g1) const
		{
			return ((g1.start_pos == start_pos)&&(g1.end_pos == end_pos)&&(g1.barcode_sequence==barcode_sequence));
		}
};

inline uint qHash(const barcode &g1)
{
	return qHash(QString::number(g1.start_pos) + QString::number(g1.end_pos) + g1.barcode_sequence);
}

class Position
{
	public:
		int start_pos;
		int end_pos;
		int chr;

		Position()=default;

		Position(int start_pos_ini, int end_pos_ini, int chr_ini)
		{
			start_pos=start_pos_ini;
			end_pos=end_pos_ini;
			chr=chr_ini;
		}

		bool operator==(const Position &pos1) const
		{
			return ((pos1.start_pos == start_pos)&&(pos1.end_pos == end_pos)&&(pos1.chr==chr));
		}
		bool operator<(const Position &pos1) const
		{
			if (pos1.chr != chr) return (pos1.chr < chr);
			if (pos1.start_pos != start_pos) return (pos1.start_pos < start_pos);
			return (pos1.end_pos < end_pos);
		}
};

class BarcodeCountInfo
{
	private:
	int _group_count;
	float _avg_quality;


	public:
	int get_group_count()
	{
		return _group_count;
	}

	float get_avg_quality()
	{
		return _avg_quality;
	}

	void add_read_group(float quality)
	{
		if (_group_count==0)
		{
			_avg_quality=quality;
		}
		else
		{
			_avg_quality=(_avg_quality*_group_count+quality)/(_group_count+1);
		}
		++_group_count;
	}

	BarcodeCountInfo(float quality_ini)
	{
		_group_count=1;
		_avg_quality=quality_ini;
	}

	BarcodeCountInfo()
	{
		_group_count=0;
		_avg_quality=0.0;
	}

};

struct mip_info
{
	int counter_unique; //number of reads after dedup
	int counter_all; //number of reads before dedup
	int counter_singles; //number of barcode families with only one read
	QHash <QString,int> barcode_counters; //read counts of each barcode
	QString name; //name of amplicon
	Position left_arm;
	Position right_arm;
};

struct hs_info
{
	int counter_unique; //number of reads after dedup
	int counter_all; //number of reads before dedup
	int counter_singles; //number of barcode families with only one read
	QHash <QString,int> barcode_counters; //read counts of each barcode
	QString name; //name of amplicon
};

struct most_frequent_read_selection
{
	readPair most_freq_read;
	QList <readPair> duplicates;
};

typedef QPair <QHash <barcode, QList<readPair> >, QList <barcode> > reduced_singles;

inline uint qHash(const Position &pos1)
{
	return qHash(QString::number(pos1.start_pos) + QString::number(pos1.end_pos) + pos1.chr);
}

class ConcreteTool
		: public ToolBase
{
	Q_OBJECT

private:
	QHash <QString,QString> create_read_index_hash(QString indexfile)
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

	int get_edit_distance( const QString& s, const QString& t )
	{
	#define D( i, j ) d[(i) * n + (j)]
		int i;
		int j;
		int m = s.length() + 1;
		int n = t.length() + 1;
		int *d = new int[m * n];
		int result;

		for ( i = 0; i < m; i++ )
		D( i, 0 ) = i;
		for ( j = 0; j < n; j++ )
		D( 0, j ) = j;
		for ( i = 1; i < m; i++ ) {
		for ( j = 1; j < n; j++ ) {
			if ( s[i - 1] == t[j - 1] ) {
			D( i, j ) = D( i - 1, j - 1 );
			} else {
			int x = D( i - 1, j );
			int y = D( i - 1, j - 1 );
			int z = D( i, j - 1 );
			D( i, j ) = 1 + qMin( qMin(x, y), z );
			}
		}
		}
		result = D( m - 1, n - 1 );
		delete[] d;
		return result;
	#undef D
	}


	QMap <Position,mip_info> create_mip_info_map(QString mip_file)
	{
		QMap <Position,mip_info> mip_info_map;
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
			Position mip_position(splitted_mip_key[1].toInt()-1,splitted_mip_key[2].toInt(),splitted_mip_key[0].toInt());

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
			new_mip_info.counter_unique=0;
			new_mip_info.counter_all=0;
			new_mip_info.counter_singles=0;

			mip_info_map[mip_position]=new_mip_info;
		}
		input_file.close();
		return mip_info_map;
	}

	QMap <Position,hs_info> create_hs_info_map(QString hs_file)
	{
		QMap <Position,hs_info> hs_info_map;
		QFile input_file(hs_file);
		input_file.open(QIODevice::ReadOnly);
		QTextStream in(&input_file);
		while (!in.atEnd())
		{
			QString line = in.readLine();
			if (!(line.startsWith("chr"))) continue;
			QStringList splitted_hs_entry=line.split("\t");
			if (splitted_hs_entry.size()<4) continue;

			Position hs_position(splitted_hs_entry[1].toInt(),splitted_hs_entry[2].toInt(),splitted_hs_entry[0].mid(3).toInt());

			hs_info new_hs_info;
			new_hs_info.name=splitted_hs_entry[3];
			new_hs_info.counter_unique=0;
			new_hs_info.counter_all=0;
			new_hs_info.counter_singles=0;

			hs_info_map[hs_position]=new_hs_info;
		}
		input_file.close();
		return hs_info_map;
	}

	void write_dup_count_histo(QHash <int, BarcodeCountInfo> dup_count_histo, QTextStream &outStream)
	{
		QList <int> dup_counts =dup_count_histo.keys();
		std::sort(dup_counts.begin(), dup_counts.end());

		outStream <<endl<< "===amplicon counts distribution===" <<endl;
		outStream << "duplicate counts" <<"\t" << "# of amplicons" <<endl;
		foreach(int dup_count, dup_counts)
		{
			outStream << dup_count <<"\t" << dup_count_histo[dup_count].get_group_count() << endl;
		}
	}

	void writeMipInfoMap(QMap <Position,mip_info> mip_info_map, QString outfile_name, QHash <int, BarcodeCountInfo> dup_count_histo, int lost_singles)
	{
		QMapIterator<Position, mip_info > i(mip_info_map);
		i.toBack();
		QFile out(outfile_name);
		out.open(QIODevice::WriteOnly|QIODevice::Text);
		QTextStream outStream(&out);
		outStream <<"chr \tstart\tend\tMIP name\tcount"<<endl;
		while (i.hasPrevious())
		{
			i.previous();
			outStream << i.key().chr <<"\t" << i.key().start_pos<< "\t" << i.key().end_pos <<"\t" << i.value().name <<"\t" << i.value().counter_unique <<"\t" << i.value().counter_all <<"\t" << i.value().counter_singles <<endl;
		}
		write_dup_count_histo(dup_count_histo, outStream);
		outStream <<endl;
		outStream << lost_singles << " read(s) were deleted in target region because of ambiguity of single matching";
		out.close();
	}

	void writeHSInfoMap(QMap <Position,hs_info> hs_info_map, QString outfile_name, QHash <int, BarcodeCountInfo> dup_count_histo, int lost_singles)
	{
		QMapIterator<Position, hs_info > i(hs_info_map);
		i.toBack();
		QFile out(outfile_name);
		out.open(QIODevice::WriteOnly|QIODevice::Text);
		QTextStream outStream(&out);
		outStream <<"chr \tstart\tend\tAmplicon name\tcount"<<endl;
		while (i.hasPrevious())
		{
			i.previous();
			outStream << i.key().chr <<"\t" << i.key().start_pos<< "\t" << i.key().end_pos <<"\t" << i.value().name <<"\t" << i.value().counter_unique <<"\t"<< i.value().counter_all <<"\t"<< i.value().counter_singles<<endl;
		}
		write_dup_count_histo(dup_count_histo, outStream);
		outStream <<endl;
		outStream << lost_singles << " read(s) were deleted in target region because of ambiguity of single matching";
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
		return cigar_ops_out;
	}

	reduced_singles reduceSingleReads(int allowed_edit_distance, const QHash <barcode, QList<readPair> > &read_groups)
	{
		QList <barcode> lost_singles;

		if (allowed_edit_distance<=0) return qMakePair(read_groups,lost_singles);
		QHash <barcode, QList<readPair> > read_groups_new=read_groups;
		QHash <barcode, QList<readPair> >::const_iterator i;

		for (i = read_groups.begin(); i != read_groups.end(); ++i)
		//iterate over old structure because elements on new structure might get deleted
		{
			if (read_groups_new[i.key()].count()==1) //check singlenessed in new structure because singles might already got matched by others
			{
				barcode single_key =i.key();
				QList<readPair> single_value=i.value();
				barcode match;
				bool is_match=false;
				bool is_unambigious_match=true;
				QHash <barcode, QList<readPair> >::iterator j;
				for (j = read_groups_new.begin(); j != read_groups_new.end(); ++j)
				{
					int edit_distance=get_edit_distance(j.key().barcode_sequence,single_key.barcode_sequence);
					//if same position and similar enough (but not identical) barcode
					if ((j.key().end_pos==single_key.end_pos)&&(j.key().start_pos==single_key.start_pos)&&(0<edit_distance)&&(edit_distance<=allowed_edit_distance))
					{
						if(is_match)//if second match
						{
							is_unambigious_match=false;
							break;
						}
						else
						{
							match=j.key();
							is_match=true;
						}
					}
				}

				if (is_match)
				{
					if (is_unambigious_match)
					{

						//check that match is not itself an ambigious single barcode
						bool is_match2=false;
						bool is_unambigious_match2=true;
						if (read_groups_new[match].count()==1)
						{
							QHash <barcode, QList<readPair> >::iterator j;
							for (j = read_groups_new.begin(); j != read_groups_new.end(); ++j)
							{
								int edit_distance=get_edit_distance(j.key().barcode_sequence,match.barcode_sequence);
								//if same position and similar enough (but not identical) barcode
								if ((j.key().end_pos==match.end_pos)&&(j.key().start_pos==match.start_pos)&&(0<edit_distance)&&(edit_distance<=allowed_edit_distance))
								{
									if(is_match2)//if second match
									{
										is_unambigious_match2=false;
										break;
									}
									else is_match2=true;
								}
							}
						}

						if (is_unambigious_match2)
						{
							//add the single read to match
							read_groups_new[match].append(i.value());
							//remove old entry of single
							read_groups_new.remove(single_key);
						}
					}
					else
					{
						lost_singles.append(single_key);
						read_groups_new.remove(single_key);
					}
				}
			}
		}

		return qMakePair(read_groups_new,lost_singles);
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

	BamAlignment cut_arms_single(BamAlignment original_alignment, Position left_arm, Position right_arm)
	{
		//cut on right side
		if (original_alignment.GetEndPosition()>right_arm.start_pos)
		{
			int elems_to_cut=(original_alignment.GetEndPosition())-right_arm.start_pos;
			//cut bases and qualties
			original_alignment.QueryBases=original_alignment.QueryBases.substr(1,(original_alignment.QueryBases.size()-elems_to_cut));
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

	most_frequent_read_selection cutAndSelectPair(QList <readPair> original_readpairs, Position left_arm, Position right_arm)
	{
		QList <readPair> new_alignment_list;
		foreach(readPair original_alignments, original_readpairs)
		{
			readPair new_alignments;
			new_alignments.first=cut_arms_single(original_alignments.first,left_arm,right_arm);
			new_alignments.second=cut_arms_single(original_alignments.second,left_arm,right_arm);
			new_alignments.first.MatePosition=new_alignments.second.Position;
			new_alignments.second.MatePosition=new_alignments.first.Position;
			new_alignment_list.append(new_alignments);
		}
		return find_highest_freq_read(new_alignment_list);
	}

	void writeReadsToBed(QTextStream &out_stream, Position act_position, QList <readPair> original_readpairs, QString barcode, bool test)
	{
		if (!(out_stream.device())) return;
		foreach(readPair original_readpair,original_readpairs)
		{
			QString read_name=QString::fromStdString(original_readpair.first.Name);
			QString seq_1=QString::fromStdString(original_readpair.first.QueryBases);
			QString seq_2=QString::fromStdString(original_readpair.second.QueryBases);
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
				out_stream << act_position.chr <<"\t" << act_position.start_pos<< "\t" << act_position.end_pos<<"\t" <<endl;
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

	void store_read_counts_mip(QMap <Position,mip_info>  &mip_info_map, QHash <int,BarcodeCountInfo> &dup_count_histo, Position act_position, int dup_count, barcode act_barcode)
	{
		mip_info_map[act_position].counter_unique++;
		mip_info_map[act_position].barcode_counters[act_barcode.barcode_sequence]=dup_count;
		if (dup_count==1) mip_info_map[act_position].counter_singles++;
		mip_info_map[act_position].counter_all+=dup_count;
		if (dup_count_histo.contains(dup_count))
		{
			dup_count_histo[dup_count].add_read_group(1.0);
		}
		else
		{
			dup_count_histo[dup_count]=BarcodeCountInfo(500.0);
		}
	}

	void store_read_counts_hs(QMap <Position,hs_info>  &hs_info_map, QHash <int,BarcodeCountInfo> &dup_count_histo, Position act_position, int dup_count, barcode act_barcode)
	{
		hs_info_map[act_position].counter_unique++;
		hs_info_map[act_position].barcode_counters[act_barcode.barcode_sequence]=dup_count;
		if (dup_count==1) hs_info_map[act_position].counter_singles++;
		hs_info_map[act_position].counter_all+=dup_count;
		if (dup_count_histo.contains(dup_count))
		{
			dup_count_histo[dup_count].add_read_group(1.0);
		}
		else
		{
			dup_count_histo[dup_count]=BarcodeCountInfo(0.0);
		}
	}

	int new_lost_singles_counts(QList <barcode> lost_singles,const QMap <Position,mip_info> &mip_info_map,const QMap <Position,hs_info> &hs_info_map, int last_ref)
	{
		int lost_single_counts=0;
		foreach(barcode lost_single, lost_singles)
		{
			if (mip_info_map.contains(Position(lost_single.start_pos,lost_single.end_pos,last_ref))) ++lost_single_counts;
		}
		foreach(barcode lost_single, lost_singles)
		{
			if (hs_info_map.contains(Position(lost_single.start_pos,lost_single.end_pos,last_ref))) ++lost_single_counts;
		}
		return lost_single_counts;
	}

	void add_read_pair( const QHash <QString,QString> &read_headers2barcodes, QHash <barcode, QList<readPair> > &read_groups, readPair read_pair)
	{
		barcode act_group;
		act_group.barcode_sequence= read_headers2barcodes["@"+QString::fromStdString(read_pair.first.Name)];
		act_group.start_pos= qMin(read_pair.first.Position,read_pair.second.Position);
		act_group.end_pos= qMax(read_pair.first.GetEndPosition(),read_pair.second.GetEndPosition());
		read_groups[act_group].append(read_pair);
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
		addInt("min_group", "minimal numbers of reads to keep a barcode group.", true, 1);
		addInt("dist", "edit distance for single read matching .", true, 0);
		addInfile("mip_file","input file for MIPS (reads are filtered and cut to match only MIP inserts).", true, "");
		addInfile("hs_file","input file for Haloplex HS amplicons (reads are filtered to match only amplicons).", true, "");
		addOutfile("stats","Output TSV file for statistics).", true, "");
		addOutfile("nomatch_out","Output Bed file for reads not matching any amplicon).", true, "");
		addOutfile("duplicate_out","Output Bed file for reads removed as duplicates).", true, "");
	}

	virtual void main()
	{
		//init: parse input parameters
		QHash <QString,QString> read_headers2barcodes=create_read_index_hash(getInfile("index"));//build read_header => index hash
		BamReader reader;
		NGSHelper::openBAM(reader, getInfile("bam"));
		BamWriter writer;
		writer.Open(getOutfile("out").toStdString(), reader.GetConstSamHeader(), reader.GetReferenceData());
		QString stats_out_name=getOutfile("stats");
		QString nomatch_out_name=getOutfile("nomatch_out");
		QString duplicate_out_name=getOutfile("duplicate_out");
		QString mip_file= getInfile("mip_file");
		QString hs_file= getInfile("hs_file");
		int edit_distance=getInt("dist");
		int minimal_group_size=getInt("min_group");
		bool test =getFlag("test");

		//init: setup remaining variables
		QHash <barcode, QList<readPair> > read_groups;
		BamAlignment al;
		QHash<QString, BamAlignment> al_map;
		int counter = 1;
		QHash <int,BarcodeCountInfo> dup_count_histo;
		int lost_single_counts=0;
		int last_start_pos=0;
		int last_ref=-1;
		int new_ref=-1;
		bool chrom_change=false;

		QMap <Position,mip_info> mip_info_map;
		if(mip_file!="") mip_info_map= create_mip_info_map(mip_file);

		QMap <Position,hs_info> hs_info_map;
		if(hs_file!="") hs_info_map= create_hs_info_map(hs_file);

		QTextStream nomatch_out_stream;
		QFile nomatch_out(nomatch_out_name);
		if (nomatch_out_name!="")
		{
			nomatch_out.open(QIODevice::WriteOnly);
			nomatch_out_stream.setDevice(&nomatch_out);
		}

		QTextStream duplicate_out_stream;
		QFile duplicate_out(duplicate_out_name);
		if (duplicate_out_name!="")
		{
			duplicate_out.open(QIODevice::WriteOnly);
			duplicate_out_stream.setDevice(&duplicate_out);
		}

		//start deduplicating
		while (reader.GetNextAlignment(al))
		{
			++counter;
			//write after every 10000th read to reduce memory requirements
			if (((counter%10000)==0)||(chrom_change))
			{
				reduced_singles reduced_singles_res=reduceSingleReads(edit_distance,read_groups);
				read_groups=reduced_singles_res.first;
				QList<barcode> lost_singles=reduced_singles_res.second;

				//count single reads lost due to ambiguity within amplicons
				lost_single_counts+=new_lost_singles_counts(lost_singles,mip_info_map,hs_info_map,last_ref);

				QHash <barcode, QList<readPair> > read_groups_new;
				QHash <barcode, QList<readPair> >::iterator i;
				for (i = read_groups.begin(); i != read_groups.end(); ++i)
				{
					/*assure that we already moved pass the readpair so no duplicates are missed by in-between reset of read group
					 * (won't work if input is not sorted by position)*/
					if ((i.key().end_pos)<last_start_pos||(chrom_change))
					{
						int read_count=i.value().count();
						Position act_position(i.key().start_pos,i.key().end_pos,last_ref);
						if (mip_file!="")
						{
							if (mip_info_map.contains(act_position)&&(read_count>=minimal_group_size))
							{
								store_read_counts_mip(mip_info_map, dup_count_histo, act_position, read_count,i.key());
								most_frequent_read_selection read_selection = cutAndSelectPair(i.value(),mip_info_map[act_position].left_arm,mip_info_map[act_position].right_arm);
								writePairToBam(writer, read_selection.most_freq_read);
								writeReadsToBed(duplicate_out_stream,act_position,read_selection.duplicates,i.key().barcode_sequence,test);
							}
							else//write reads not matching a mip to a bed file
							{
								writeReadsToBed(nomatch_out_stream,act_position,i.value(),i.key().barcode_sequence,test);
							}

						}
						else if (hs_file!="")
						{
							if (hs_info_map.contains(act_position)&&(read_count>=minimal_group_size))//select and count reads that can be matched to haloplex hs barcodes
							{
								store_read_counts_hs(hs_info_map, dup_count_histo, act_position, read_count,i.key());
								most_frequent_read_selection read_selection = find_highest_freq_read(i.value());
								writePairToBam(writer, read_selection.most_freq_read);
								writeReadsToBed(duplicate_out_stream,act_position,read_selection.duplicates,i.key().barcode_sequence,test);
							}
							else//write reads not matching a mip to a bed file
							{
								writeReadsToBed(nomatch_out_stream,act_position,i.value(),i.key().barcode_sequence,test);
							}
						}
						else
						{
							most_frequent_read_selection read_selection = find_highest_freq_read(i.value());
							writePairToBam(writer, read_selection.most_freq_read);
							writeReadsToBed(duplicate_out_stream,act_position,read_selection.duplicates,i.key().barcode_sequence,test);
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
					readPair act_read_pair(al,al_map.take(QString::fromStdString(al.Name)));
					add_read_pair(read_headers2barcodes, read_groups, act_read_pair);
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
				if (last_ref!=-1) chrom_change=true;
			}

		}

		//write remaining pairs
		reduced_singles reduced_singles_res=reduceSingleReads(edit_distance,read_groups);
		read_groups=reduced_singles_res.first;
		QList<barcode> lost_singles=reduced_singles_res.second;
		//count single reads lost due to ambiguity within amplicons

		lost_single_counts+=new_lost_singles_counts(lost_singles,mip_info_map,hs_info_map,last_ref);

		QHash <barcode, QList<readPair> >::iterator i;
		for (i = read_groups.begin(); i != read_groups.end(); ++i)
		{
			int read_count=i.value().count();
			Position act_position(i.key().start_pos,i.key().end_pos,last_ref);
			if (mip_file!="")
			{
				if (mip_info_map.contains(act_position)&&(read_count>=minimal_group_size))//trim and count reads that can be matched to mips
				{
					store_read_counts_mip(mip_info_map, dup_count_histo, act_position, i.value().count(),i.key());
					most_frequent_read_selection read_selection = cutAndSelectPair(i.value(),mip_info_map[act_position].left_arm,mip_info_map[act_position].right_arm);
					writePairToBam(writer, read_selection.most_freq_read);
					writeReadsToBed(duplicate_out_stream,act_position,read_selection.duplicates,i.key().barcode_sequence,test);
				}
				else//write reads not matching a mip to a bed file
				{
					writeReadsToBed(nomatch_out_stream,act_position,i.value(),i.key().barcode_sequence,test);
				}
			}
			else if (hs_file!="")
			{
				if (hs_info_map.contains(act_position)&&(read_count>=minimal_group_size))//trim, select and count reads that can be matched to mips
				{
					store_read_counts_hs(hs_info_map, dup_count_histo, act_position, i.value().count(),i.key());
					most_frequent_read_selection read_selection = find_highest_freq_read(i.value());
					writePairToBam(writer, read_selection.most_freq_read);
					writeReadsToBed(duplicate_out_stream,act_position,read_selection.duplicates,i.key().barcode_sequence,test);
				}
				else//write reads not matching a mip to a bed file
				{
					writeReadsToBed(nomatch_out_stream,act_position,i.value(),i.key().barcode_sequence,test);
				}
			}
			else
			{
				most_frequent_read_selection read_selection = find_highest_freq_read(i.value());
				writePairToBam(writer, read_selection.most_freq_read);
				writeReadsToBed(duplicate_out_stream,act_position,read_selection.duplicates,i.key().barcode_sequence,test);
			}
		}

		if ((mip_file!="")&&(stats_out_name!="")) writeMipInfoMap(mip_info_map,stats_out_name,dup_count_histo,lost_single_counts);
		if ((hs_file!="")&&(stats_out_name!="")) writeHSInfoMap(hs_info_map,stats_out_name,dup_count_histo,lost_single_counts);

		//close files
		if (duplicate_out_name!="")	duplicate_out.close();
		if (nomatch_out_name!="") nomatch_out.close();
		reader.Close();
		writer.Close();
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
