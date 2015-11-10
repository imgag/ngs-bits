#include "ToolBase.h"
#include "Helper.h"
#include "FastqFileStream.h"
#include "math.h"
#include "WriteWorker.h"
#include "NGSHelper.h"
#include "Log.h"
#include <QFile>
#include <QSet>
#include <QHash>
#include <QDebug>
#include <QDir>
#include <QList>
#include <algorithm>
#include <QThreadPool>

///Forward/reverse output stream pair.
struct StreamPair
{
	StreamPair()
		: forward(0)
		, reverse(0)
	{}

	StreamPair(FastqOutfileStream* f, FastqOutfileStream* r)
		: forward(f)
		, reverse(r)
	{}

	FastqOutfileStream* forward;
	FastqOutfileStream* reverse;
};

///Sample datastructure
struct Sample
{
	QString name;
	QString project;
	QString barcode;
	int lane;
	StreamPair outstreams;

	QString filename(QString out_folder, QString read)
	{
		return out_folder + "/Project_" + project + "/Sample_" + name + "/" + name + "_" + barcode + "_L" + QString::number(lane).rightJustified(3, '0') + "_R" + read + "_001.fastq.gz";
	}
};

///Index statistics
struct IndexStatistics
{
	int poly_a1;
	int poly_c1;
	int poly_g1;
	int poly_t1;
	int poly_n1;
	QList<int> a_at_pos1;
	QList<int> c_at_pos1;
	QList<int> g_at_pos1;
	QList<int> t_at_pos1;
	QList<int> n_at_pos1;

	int poly_a2;
	int poly_c2;
	int poly_g2;
	int poly_t2;
	int poly_n2;
	QList<int> a_at_pos2;
	QList<int> c_at_pos2;
	QList<int> g_at_pos2;
	QList<int> t_at_pos2;
	QList<int> n_at_pos2;
};

///Lane statistics
struct LaneStatistics
{
	int read;
	int assigned_single;
	int assigned_double;
	int	unassigned;
	int ambigous_single;
	int ambigous_double;
	IndexStatistics index_stats;
};

///Global result statistics
struct ResultStatistics
{
	QMap<QString, int> samples; //sample name: project + tab + sample
	QMap<int, LaneStatistics> lanes;
};

///Barcode group (with same barcode length)
struct BarcodeGroup
{
	int i1_len;
	int i2_len;
	QHash<QByteArray, Sample*> barcode2sample;
};

//TODO: test with same/different sample/barcode on different lanes
class ConcreteTool
        : public ToolBase
{
    Q_OBJECT

private:

	ResultStatistics statistics_;
	QList<FastqOutfileStream*> outstreams_;
	QMap <int, QList<BarcodeGroup> > barcodes_;//lane specific barcode groups
	bool all_single;//are all barcodes single indexed?

	///Helper function to write output with worker pool
	void writeWithThread(FastqOutfileStream* outstream, FastqEntry& outentry)
	{
        WriteWorker* worker = new WriteWorker(outstream ,outentry);
		QThreadPool::globalInstance()->start(worker);
    }

	///Helper function that creates an output stream and adds it to the list of streams
	FastqOutfileStream* createStream(QString filename)
	{
		FastqOutfileStream* stream = new FastqOutfileStream(filename, false);
		outstreams_.append(stream);
		return stream;
	}

	///Returns barcode read lengths
	void getBarcodeReadLengths(QString input_file1, int& r1, int& r2)
	{
		r1=0;
		r2=0;

		FastqFileStream input_stream1(input_file1, false);
		if (input_stream1.atEnd()) return;

		FastqEntry input_read1;
		input_stream1.readEntry(input_read1);

		QList<QByteArray> barcode_seqs=input_read1.header.split(':').last().split('+');//end of a header is ":[input_read_seq1]+[input_read_seq2]"
		r1 = barcode_seqs[0].length();
		if(barcode_seqs.length()==1) return;
		r2 = barcode_seqs[1].length();
	}

	QSet<QByteArray> makeBarcodesWithMismatch(QByteArray barcode, int max_mismatches)
	{
		const QList<char> base_list = QList<char>() << 'A' << 'C'<< 'T'<< 'G' <<'N';//allowed bases

		QSet<QByteArray> barcodes_with_mismatches;//set of barcodes with at most [mismatch] mismatches
		barcodes_with_mismatches.insert(barcode);//add original barcode for first cycle

		for(int mis=0; mis<max_mismatches; ++mis)//[mismatch] cylces, introducing one mismatch in each cycle
		{
			QSet<QByteArray> barcodes = barcodes_with_mismatches;//results from previous cycles are original barcodes for this cycle
			foreach(QByteArray bar, barcodes)//add mismatch to each barcode
			{
				for (int base = 0; base != bar.size(); ++base)//for each position of working copy
				{
					QByteArray barcode_with_mismatch = bar;//make working copy of original barcode
					foreach(char mis, base_list)//use each allowed base once as mismatch
					{
						barcode_with_mismatch[base] = mis;//change base at act position
						barcodes_with_mismatches.insert(barcode_with_mismatch);//insert modified barcode into set
					}
				}
			}
		}
		return barcodes_with_mismatches;
	}

	void addIndexStatistics(const QByteArray &barcode_seq1, const QByteArray &barcode_seq2, int lane)
	{
		bool is_poly1=true;//assume index is poly-base until contradiction is found
		//read each position of index read 1
		for(int iii=0; iii<barcode_seq1.size(); ++iii)
		{
			if (barcode_seq1[0]!=barcode_seq1[iii]) is_poly1=false;
			if (barcode_seq1[iii]=='A') statistics_.lanes[lane].index_stats.a_at_pos1[iii]++;
			if (barcode_seq1[iii]=='C') statistics_.lanes[lane].index_stats.c_at_pos1[iii]++;
			if (barcode_seq1[iii]=='G') statistics_.lanes[lane].index_stats.g_at_pos1[iii]++;
			if (barcode_seq1[iii]=='T') statistics_.lanes[lane].index_stats.t_at_pos1[iii]++;
			if (barcode_seq1[iii]=='N') statistics_.lanes[lane].index_stats.n_at_pos1[iii]++;
		}
		if ((is_poly1)&&(barcode_seq1.size()>0))
		{
			switch(barcode_seq1[0])
			{
				case 'A':
					statistics_.lanes[lane].index_stats.poly_a1++;
					break;

				case 'C':
					statistics_.lanes[lane].index_stats.poly_c1++;
					break;

				case 'G':
					statistics_.lanes[lane].index_stats.poly_g1++;
					break;

				case 'T':
					statistics_.lanes[lane].index_stats.poly_t1++;
					break;

				case 'N':
					statistics_.lanes[lane].index_stats.poly_n1++;
					break;

				default://very unlikely
					THROW(FileParseException, "Found homopolymer of unsupported base '" + barcode_seq1[0]);
			}
		}

		bool is_poly2=true;//assume index is poly-base until contradiction is found
		//read each position of index read 2
		for(int iii=0; iii<barcode_seq2.size(); ++iii)
		{
			if (barcode_seq2[0]!=barcode_seq2[iii]) is_poly2=false;
			if (barcode_seq2[iii]=='A') statistics_.lanes[lane].index_stats.a_at_pos2[iii]++;
			if (barcode_seq2[iii]=='C') statistics_.lanes[lane].index_stats.c_at_pos2[iii]++;
			if (barcode_seq2[iii]=='G') statistics_.lanes[lane].index_stats.g_at_pos2[iii]++;
			if (barcode_seq2[iii]=='T') statistics_.lanes[lane].index_stats.t_at_pos2[iii]++;
			if (barcode_seq2[iii]=='N') statistics_.lanes[lane].index_stats.n_at_pos2[iii]++;
		}
		if ((is_poly2)&&(barcode_seq2.size()>0))
		{
			switch(barcode_seq2[0])
			{
				case 'A':
					statistics_.lanes[lane].index_stats.poly_a2++;
					break;

				case 'C':
					statistics_.lanes[lane].index_stats.poly_c2++;
					break;

				case 'G':
					statistics_.lanes[lane].index_stats.poly_g2++;
					break;

				case 'T':
					statistics_.lanes[lane].index_stats.poly_t2++;
					break;

				case 'N':
					statistics_.lanes[lane].index_stats.poly_n2++;
					break;

				default://very unlikely
					THROW(FileParseException, "Found homopolymer of unsupported base '" + barcode_seq1[0]);
			}
		}
	}

	void parseBarcodes(QString sheet, int r1_length, int r2_length, int mms, int mmd, QString out_folder, bool rev2)
	{
		Sample* ambigous_sample = new Sample;
		all_single=true;
		QFile inFile(sheet);
        if (!inFile.open(QIODevice::ReadOnly | QIODevice::Text))
        {
			THROW(ArgumentException, "File " + sheet+ " is not readable!");
        }

		//delete/create output folder
		QDir out_dir(out_folder);
		if (out_dir.exists() && !QDir(out_folder).removeRecursively())
		{
			THROW(ArgumentException, "Output folder '" + out_folder + "' could not be deleted!");
        }
		if (!out_dir.mkpath("."))
		{
			THROW(ArgumentException, "Output folder '" + out_folder + "' could not be created!");
		}

        QTextStream in(&inFile);
		QMap <int, QSet<QByteArray> > used_barcodes; //lane specific
		QSet<QByteArray> empty_set;
		QList<BarcodeGroup> empty_list;
		while (!in.atEnd())
		{
            QString line = in.readLine(); //read one line at a time
			if (line.startsWith("lane\t") || line.startsWith("#lane")) continue;

			Sample* concrete_sample = new Sample;//init pointer to a sample
			QStringList line_parts = line.split("\t");
			if (line_parts.count()<5) THROW(FileParseException, "Sample sheet line has less than 5 parts: " + line);
			concrete_sample->lane = Helper::toInt(line_parts[0], "lane", line);
			if (!barcodes_.contains(concrete_sample->lane))//if first entry for this lane
			{
				used_barcodes[concrete_sample->lane]=empty_set;//init lane specific used_barcodes
				barcodes_[concrete_sample->lane]=empty_list;//initialize lane specific barcode groups
			}
			if (line_parts[1]=="") THROW(FileParseException, "Empty project in sample sheet line: " + line);
			concrete_sample->project = line_parts[1];
			if (line_parts[2]=="") THROW(FileParseException, "Empty sample in sample sheet line: " + line);
			concrete_sample->name = line_parts[2];
			if (line_parts[3]=="") THROW(FileParseException, "Empty i7 barcode in sample sheet line: " + line);
			QByteArray barcode1 = line_parts[3].toLatin1();
			QByteArray barcode2 = line_parts[4].toLatin1();
			if (rev2) barcode2 = NGSHelper::changeSeq(barcode2, true, true);
			concrete_sample->barcode =  barcode1 + barcode2;

			//create project folder
			out_dir.mkdir("Project_" + concrete_sample->project);

			//initialize sample statistics (lane statistics is initialized during demultiplexing)
			QString project_sample = concrete_sample->project + "\t" + concrete_sample->name;
			if (!statistics_.samples.contains(project_sample))
			{
				statistics_.samples.insert(project_sample, 0);
			}

            //create sample directory
			QDir project_dir(out_folder + "/Project_" + concrete_sample->project);
			project_dir.mkdir("Sample_" + concrete_sample->name);

			//open filestreams
			FastqOutfileStream* outstream_pointer1 = createStream(concrete_sample->filename(out_folder, "1"));
			FastqOutfileStream* outstream_pointer2 = createStream(concrete_sample->filename(out_folder, "2"));
			concrete_sample->outstreams = StreamPair(outstream_pointer1,outstream_pointer2);

			if (r1_length<barcode1.size())
            {
				Log::warn("Barcode "+barcode1+" for sample "+concrete_sample->name+" is longer than barcode read! Truncated barcode is used!");
				barcode1=barcode1.left(r1_length);
            }

			if (r2_length<barcode2.size())
            {
				Log::warn("Barcode "+barcode2+" for sample "+concrete_sample->name+" is longer than barcode read! Truncated barcode is used!");
				barcode2=barcode2.left(r2_length);
            }

			if (used_barcodes[concrete_sample->lane].contains(barcode1 + "_" + barcode2))
            {
				THROW(ArgumentException, "Barcode combination " + barcode1 + "/" + barcode2+ " used more than once!");
            }
			used_barcodes[concrete_sample->lane].insert(barcode1 + "_" + barcode2);

            //introduce mismatches to barcodes
			QSet<QByteArray> barcodes_with_mismatches1;
			QSet<QByteArray> barcodes_with_mismatches2;

            if (barcode2.size()>0)//if this is a double-indexed read
            {
				all_single=false;
				barcodes_with_mismatches1 = makeBarcodesWithMismatch(barcode1, mmd);
				barcodes_with_mismatches2 = makeBarcodesWithMismatch(barcode2, mmd);
            }
            else
            {
				barcodes_with_mismatches1 = makeBarcodesWithMismatch(barcode1, mms);
                barcodes_with_mismatches2.insert("");//barcode 2 is empty
            }

			//find barcode group
			BarcodeGroup* group = 0;
			for (int i=0; i<barcodes_[concrete_sample->lane].size(); ++i)
			{
				if (barcodes_[concrete_sample->lane][i].i1_len==barcode1.size() && barcodes_[concrete_sample->lane][i].i2_len==barcode2.size())
				{
					group = &barcodes_[concrete_sample->lane][i];
				}
			}
			//create new lane specific barcode group if it does not exist yet
			if (group==0)
			{
				BarcodeGroup tmp;
				tmp.i1_len = barcode1.size();
				tmp.i2_len = barcode2.size();
				barcodes_[concrete_sample->lane].append(tmp);
				group = &(barcodes_[concrete_sample->lane].last());
			}

			//insert new barcodes
			foreach(QByteArray bar1, barcodes_with_mismatches1)
			{
				foreach(QByteArray bar2, barcodes_with_mismatches2)
				{
					QByteArray barcode = bar1+"\t"+bar2;
					if (group->barcode2sample.contains(barcode))
                    {
						group->barcode2sample[barcode] = ambigous_sample; //flag ambigous barcode with same length
                    }
                    else
                    {
						group->barcode2sample[barcode] = concrete_sample;
                    }
                }
			}
        }

		//flag ambigious barcodes of different lengths

		foreach(const int lane_index, barcodes_.keys())//for each lane
		{
			for(int iii = 0; iii<barcodes_[lane_index].length(); ++iii) //compare every barcode group
			{
				BarcodeGroup groupi=barcodes_[lane_index][iii];
				for(int jjj=iii+1 ; jjj<barcodes_[lane_index].length(); ++jjj)//compare with every remaining barcode group
				{
					BarcodeGroup groupj=barcodes_[lane_index][jjj];

					//if one barcode group is double indexed and the other is single indexed, they can't be ambigous
					if ((groupi.i2_len!=0)!=(groupj.i2_len!=0)) continue;

					//if barcode i length is shorter than barcode j
					if ((groupi.i1_len<=groupj.i1_len)&&(groupi.i2_len<=groupj.i2_len))
					{
						//iterate through barcodes of groupj's hash
						foreach(const QByteArray barcode, groupj.barcode2sample.keys())
						{
							const QList <QByteArray> parts = barcode.split('\t');
							//truncate groupj's barcode
							QByteArray truncated_barcode = parts[0].left(groupj.i1_len)+"\t"+parts[1].left(groupj.i2_len);
							//look it up in groupi's hash
							if (groupi.barcode2sample.contains(truncated_barcode))
							{
								//set them to ambigous
								barcodes_[lane_index][iii].barcode2sample[truncated_barcode]=ambigous_sample;
								barcodes_[lane_index][jjj].barcode2sample[barcode]=ambigous_sample;
							}
						}
					}

					//if barcode j length is shorter than barcode i
					else if ((groupj.i1_len<=groupi.i1_len)&&(groupj.i2_len<=groupi.i2_len))
					{
						//iterate through barcodes of groupi's hash
						foreach(const QByteArray barcode, groupi.barcode2sample.keys())
						{
							QList <QByteArray> parts = barcode.split('\t');
							//truncate groupi's barcode
							QByteArray truncated_barcode = parts[0].left(groupj.i1_len)+"\t"+parts[1].left(groupj.i2_len);
							//look it up in groupj's hash
							if (groupj.barcode2sample.contains(truncated_barcode))
							{
								//set them to ambigous
								barcodes_[lane_index][jjj].barcode2sample[truncated_barcode]=ambigous_sample;
								barcodes_[lane_index][iii].barcode2sample[barcode]=ambigous_sample;
							}
						}
					}

					//if one barcode has a longer i5 and the other one a longer i7
					else
					{
						//iterate through barcodes of groupi's hash
						foreach(const QByteArray barcodei, groupi.barcode2sample.keys())
						{
							QList <QByteArray> partsi = barcodei.split('\t');
							//iterate through barcodes of groupj's hash
							foreach(const QByteArray barcodej, groupi.barcode2sample.keys())
							{
								int min_i1_size=qMin(groupi.i1_len,groupj.i1_len);
								int min_i2_size=qMin(groupi.i2_len,groupj.i2_len);

								QList <QByteArray> partsj = barcodej.split('\t');

								//if truncated barcodes match
								if((partsi[0].left(min_i1_size)==partsj[0].left(min_i1_size))&&(partsi[1].left(min_i2_size)==partsj[1].left(min_i2_size)))
								{
									//set them to ambigous
									barcodes_[lane_index][jjj].barcode2sample[barcodej]=ambigous_sample;
									barcodes_[lane_index][iii].barcode2sample[barcodei]=ambigous_sample;
								}
							}
						}
					}
				}
			}
		}
		inFile.close();
    }

	void demultiplex(QStringList in1, QStringList in2, QString out_folder)
	{
        //init filestreams for ambigous reads
		FastqOutfileStream* ambigous_fastq1 = createStream(out_folder+"/ambigous_R1.fastq.gz");
		FastqOutfileStream* ambigous_fastq2 = createStream(out_folder+"/ambigous_R2.fastq.gz");

		//streams for unassigned reads
		QHash< int, StreamPair > unassigned_outstreams;

		//demultiplex
		for (int i=0; i<in1.count(); ++i)
		{
			FastqFileStream input_stream1(in1[i], false);
			FastqFileStream input_stream2(in2[i], false);
			while (!input_stream1.atEnd() && !input_stream2.atEnd())//foreach input read
			{
				FastqEntry input_read1;
                input_stream1.readEntry(input_read1);
				FastqEntry input_read2;
                input_stream2.readEntry(input_read2);

				//extract lane
				QList<QByteArray> header_parts = input_read1.header.split(':');
				if (header_parts.count()<10) THROW(ArgumentException, "FASTQ header has less tham 10 ':'-separated parts: " + input_read1.header);
				int lane = Helper::toInt(header_parts[3], "lane", input_read1.header);

				//extract barcode(s)
				QList<QByteArray> barcode_seqs = header_parts[9].split('+');
				QByteArray barcode_seq1=barcode_seqs[0];
				QByteArray barcode_seq2="";
				if(barcode_seqs.length()>1) barcode_seq2=barcode_seqs[1];//could be that only one index read was used

				//init unassigned streams and lane statistics (if not present yet)
				if (!unassigned_outstreams.contains(lane))
				{
					//streams
					FastqOutfileStream* outstream_pointer1 = createStream(out_folder+"/unassigned_L"+QString::number(lane).rightJustified(3, '0')+"_R1.fastq.gz");
					FastqOutfileStream* outstream_pointer2 = createStream(out_folder+"/unassigned_L"+QString::number(lane).rightJustified(3, '0')+"_R2.fastq.gz");
					unassigned_outstreams[lane] = StreamPair(outstream_pointer1, outstream_pointer2);

					//statistics
					statistics_.lanes[lane] = LaneStatistics();

					//index at_position statistics
					for(int z=0; z<barcode_seq1.size(); ++z)
					{
						statistics_.lanes[lane].index_stats.a_at_pos1.append(0);
						statistics_.lanes[lane].index_stats.c_at_pos1.append(0);
						statistics_.lanes[lane].index_stats.g_at_pos1.append(0);
						statistics_.lanes[lane].index_stats.t_at_pos1.append(0);
						statistics_.lanes[lane].index_stats.n_at_pos1.append(0);
					}

					for(int z=0; z<barcode_seq2.size(); ++z)
					{
						statistics_.lanes[lane].index_stats.a_at_pos2.append(0);
						statistics_.lanes[lane].index_stats.c_at_pos2.append(0);
						statistics_.lanes[lane].index_stats.g_at_pos2.append(0);
						statistics_.lanes[lane].index_stats.t_at_pos2.append(0);
						statistics_.lanes[lane].index_stats.n_at_pos2.append(0);
					}
				}
				addIndexStatistics(barcode_seq1,barcode_seq2,lane);

				bool found_single = false;
				bool found_double = false;
				Sample found_sample;
				foreach(const BarcodeGroup& group, barcodes_[lane])
				{
					bool is_double = group.i2_len!=0;
					if (!is_double && found_double) continue; //a read matching a double index must not be assigned to a single index

					QByteArray barcode = barcode_seq1.left(group.i1_len)+"\t"+barcode_seq2.left(group.i2_len);
					if(group.barcode2sample.contains(barcode)) //check if (usable part) of index read seq 1 exists in barcodes
                    {
						if (is_double)
						{
							found_sample = *group.barcode2sample.value(barcode);
							found_double = true;
							break;//if a double index was matched, no further searching is needed
						}
						else
						{
							if (!found_single)//if the index reads haven't matched another barcode combination already
							{
								found_sample = *group.barcode2sample.value(barcode);
								found_single = true;
								if (all_single) break;//if a single index was matched, and there are no double indexes, no further searching is needed
							}
						}
                    }
                }

				//write FASTQs and update statistics
				statistics_.lanes[lane].read += 1;
				if (found_double)
                {
					if (found_sample.name=="")
                    {
						statistics_.lanes[lane].ambigous_double += 1;
						writeWithThread(ambigous_fastq1, input_read1);
						writeWithThread(ambigous_fastq1, input_read2);
                    }
                    else
                    {
						writeWithThread(found_sample.outstreams.forward, input_read1);
						writeWithThread(found_sample.outstreams.reverse, input_read2);
						statistics_.lanes[lane].assigned_double += 1;
						statistics_.samples[found_sample.project+"\t"+found_sample.name] += 1;
                    }
                }
				else if (found_single)
                {
					if (found_sample.name=="")
					{
						statistics_.lanes[lane].ambigous_single += 1;
						writeWithThread(ambigous_fastq1, input_read1);
						writeWithThread(ambigous_fastq2, input_read2);
                    }
                    else
                    {
						writeWithThread(found_sample.outstreams.forward, input_read1);
						writeWithThread(found_sample.outstreams.reverse, input_read2);
						statistics_.lanes[lane].assigned_single += 1;
						statistics_.samples[found_sample.project+"\t"+found_sample.name] += 1;
                    }
                }
                else
                {
					writeWithThread(unassigned_outstreams[lane].forward, input_read1);
					writeWithThread(unassigned_outstreams[lane].reverse, input_read2);
					statistics_.lanes[lane].unassigned += 1;
                }
			}
		}
    }

	QString writeSummaryPerPositionStat(int a, int c, int g, int t, int n)
	{
		QString output_row="";
		int sum=a+c+g+t+n;
		output_row=output_row+QString::number(100.0*a/sum, 'f', 2) + "%";
		output_row=output_row+QString::number(100.0*c/sum, 'f', 2) + "%";
		output_row=output_row+QString::number(100.0*g/sum, 'f', 2) + "%";
		output_row=output_row+QString::number(100.0*t/sum, 'f', 2) + "%";
		output_row=output_row+QString::number(100.0*n/sum, 'f', 2) + "%";
		return output_row;
	}

	void writeSummary(QString filename)
    {
		QStringList output;

		//determine overall counts
		int all_reads = 0;
		int all_assigned = 0;
		int all_unassigned = 0;
		int all_ambigous = 0;
		foreach(LaneStatistics lane_stat, statistics_.lanes.values())
		{
			all_reads += lane_stat.read;
			all_assigned += lane_stat.assigned_single + lane_stat.assigned_double;
			all_unassigned += lane_stat.unassigned;
			all_ambigous += lane_stat.ambigous_single + lane_stat.ambigous_double;
		}

		//write general statistics
		output << "==Overall statistics==";
		output << "Number of reads: " + QString::number(all_reads);
		output << "Assigned reads: " + QString::number(all_assigned) + " (" + QString::number(100.0*all_assigned/all_reads, 'f', 2) + "%)";
		output << "Unassigned reads: " + QString::number(all_unassigned) + " (" + QString::number(100.0*all_unassigned/all_reads, 'f', 2) + "%)";
		output << "Ambigous reads: " + QString::number(all_ambigous) + " (" + QString::number(100.0*all_ambigous/all_reads, 'f', 2) + "%)";
		output << "";

		//write lane statistics
		foreach(int lane_number, statistics_.lanes.keys())
		{
			LaneStatistics lane_stat=statistics_.lanes[lane_number];
			output << "==Statistics lane "+QString::number(lane_number)+"==";
			output << "total reads:\t"+QString::number(lane_stat.read);
			output << "assigned double index reads:\t"+QString::number(lane_stat.assigned_double)+ " (" + QString::number(100.0*lane_stat.assigned_double/lane_stat.read, 'f', 2) + "%)";
			output << "assigned single index reads:\t"+QString::number(lane_stat.assigned_single)+ " (" + QString::number(100.0*lane_stat.assigned_single/lane_stat.read, 'f', 2) + "%)";
			output << "unassigned reads:\t"+QString::number(lane_stat.unassigned)+ " (" + QString::number(100.0*lane_stat.unassigned/lane_stat.read, 'f', 2) + "%)";
			output << "ambigous single index reads:\t"+QString::number(lane_stat.ambigous_single)+ " (" + QString::number(100.0*lane_stat.ambigous_single/lane_stat.read, 'f', 2) + "%)";
			output << "ambigous double index reads:\t"+QString::number(lane_stat.ambigous_double)+ " (" + QString::number(100.0*lane_stat.ambigous_double/lane_stat.read, 'f', 2) + "%)";
			output << "";
			output << "Homopolymer A (Index1): " + QString::number(lane_stat.index_stats.poly_a1)+ " (" + QString::number(100.0*lane_stat.index_stats.poly_a1/lane_stat.read, 'f', 2) + "%)";
			output << "Homopolymer C (Index1): " + QString::number(lane_stat.index_stats.poly_c1)+ " (" + QString::number(100.0*lane_stat.index_stats.poly_c1/lane_stat.read, 'f', 2) + "%)";
			output << "Homopolymer G (Index1): " + QString::number(lane_stat.index_stats.poly_g1)+ " (" + QString::number(100.0*lane_stat.index_stats.poly_g1/lane_stat.read, 'f', 2) + "%)";
			output << "Homopolymer T (Index1): " + QString::number(lane_stat.index_stats.poly_t1)+ " (" + QString::number(100.0*lane_stat.index_stats.poly_t1/lane_stat.read, 'f', 2) + "%)";
			output << "Homopolymer N (Index1): " + QString::number(lane_stat.index_stats.poly_n1)+ " (" + QString::number(100.0*lane_stat.index_stats.poly_n1/lane_stat.read, 'f', 2) + "%)";
			output << "Homopolymer A (Index2): " + QString::number(lane_stat.index_stats.poly_a2)+ " (" + QString::number(100.0*lane_stat.index_stats.poly_a2/lane_stat.read, 'f', 2) + "%)";
			output << "Homopolymer C (Index2): " + QString::number(lane_stat.index_stats.poly_c2)+ " (" + QString::number(100.0*lane_stat.index_stats.poly_c2/lane_stat.read, 'f', 2) + "%)";
			output << "Homopolymer G (Index2): " + QString::number(lane_stat.index_stats.poly_g2)+ " (" + QString::number(100.0*lane_stat.index_stats.poly_g2/lane_stat.read, 'f', 2) + "%)";
			output << "Homopolymer T (Index2): " + QString::number(lane_stat.index_stats.poly_t2)+ " (" + QString::number(100.0*lane_stat.index_stats.poly_t2/lane_stat.read, 'f', 2) + "%)";
			output << "Homopolymer N (Index2): " + QString::number(lane_stat.index_stats.poly_n2)+ " (" + QString::number(100.0*lane_stat.index_stats.poly_n2/lane_stat.read, 'f', 2) + "%)";
			output << "";
			output << "Percent Base \tA\tC\tG\tT\tN";
			for(int iii=0;iii<lane_stat.index_stats.a_at_pos1.size();++iii)
			{
				output << "Index 1, Base "+QString::number(iii+1)+":\t"+writeSummaryPerPositionStat(lane_stat.index_stats.a_at_pos1[iii],lane_stat.index_stats.c_at_pos1[iii],lane_stat.index_stats.g_at_pos1[iii],lane_stat.index_stats.t_at_pos1[iii],lane_stat.index_stats.n_at_pos1[iii]);
			}
			for(int iii=0;iii<lane_stat.index_stats.a_at_pos2.size();++iii)
			{
				output << "Index 2, Base "+QString::number(iii+1)+":\t"+writeSummaryPerPositionStat(lane_stat.index_stats.a_at_pos2[iii],lane_stat.index_stats.c_at_pos2[iii],lane_stat.index_stats.g_at_pos2[iii],lane_stat.index_stats.t_at_pos2[iii],lane_stat.index_stats.n_at_pos2[iii]);
			}
			output << "";
		}
		//write sample statistics
		output << "==Sample statistics==";
		output << "#project\tsample\tread_count\tpercentage_of_reads\tpercentage_of_assigned_reads";
		QMap<QString, int>::const_iterator it = statistics_.samples.begin();
		while (it!=statistics_.samples.end())
		{
			int count = it.value();
			float percent_all = 100.0 * count / all_reads;
			float percent_assigned = 100.0 * count / all_assigned;
			QStringList parts = it.key().split("\t");
			output << parts[0] + "\t" + parts[1] + "\t" + QString::number(count) + "\t" + QString::number(percent_all,'f',2) + "\t" + QString::number(percent_assigned,'f',2);

			++it;
		}

		Helper::storeTextFile(Helper::openFileForWriting(filename, true), output);
    }

public:
    ConcreteTool(int& argc, char *argv[])
        : ToolBase(argc, argv)
    {
    }

    virtual void setup()
    {
		setDescription("Demultiplexes FASTQ files from different samples according to barcode sequences given in a samplesheet.");
		addInfile("sheet", "Samplesheet TSV file (lane, project, sample, i7 barcode, i5 barcode).", false);
		addInfileList("in1", "Input FASTQ forward file(s).", false);
		addInfileList("in2", "Input FASTQ reverse file(s).", false);
		addString("out","Output directory.", true, "Unaligned");
		//optional
		addInt("mms", "Maximum tolerated mismatches for single barcode read.", true, 1);
		addInt("mmd", "Maximum tolerated mismatches for double barcode (per barcode read).", true, 2);
		addFlag("rev2", "Use reverse complement of second barcode sequence.");
		addString("summary", "Summary file name (created in output folder). If unset, summary is written to STDOUT.", true, "");
    }

	virtual void main()
    {
		//init
		QStringList in1 = getInfileList("in1");
		QStringList in2 = getInfileList("in2");
		if (in1.size()!=in2.size()) THROW(ArgumentException, "FASTQ file count for forward/reverse reads differs!");
		QString out_folder = getString("out");
		int mms = getInt("mms");
		int mmd = getInt("mmd");
		bool rev2 = getFlag("rev2");
		QString summary = getString("summary");
		QThreadPool::globalInstance()->setMaxThreadCount(1);

		//get read lengths
		int r1_length, r2_length;
		getBarcodeReadLengths(in1[0], r1_length, r2_length);

		//create barcodes with mismatches
		parseBarcodes(getInfile("sheet"), r1_length, r2_length, mms, mmd, out_folder, rev2);

		//demultiplex
		demultiplex(in1, in2, out_folder);

		//wait till all threads are finished (otherwise closing the streams below leads to a crash)
		if (!QThreadPool::globalInstance()->waitForDone()) THROW(ProgrammingException, "Could not close all threads!");

		//close all output streams
		foreach(FastqOutfileStream* stream, outstreams_)
		{
			stream->close();
		}

		//write statistics (STDOUT or in output folder)
		if (summary!="")
		{
			summary = out_folder + QDir::separator() + summary;
		}
		writeSummary(summary);
    }
};

#include "main.moc"

int main(int argc, char *argv[])
{
    ConcreteTool tool(argc, argv);
    return tool.execute();
}
