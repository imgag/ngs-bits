#include "BedFile.h"
#include "ToolBase.h"
#include "NGSHelper.h"
#include "Settings.h"
#include "VcfFile.h"
#include "BedpeFile.h"
#include "Helper.h"
#include "TabixIndexedFile.h"
#include "NGSD.h"
#include <QTextStream>
#include <QFileInfo>
#include <QElapsedTimer>
#include <QDir>


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
		setDescription("Annotates a BEDPE file with NGSD count information of zipped BEDPE flat files.");
		addInfile("in", "Input BEDPE file.", false, true);
		addOutfile("out", "Output BEDPE file.", false, true);
		addInfile("ann_folder", "Input folder containing NGSD count flat files.", false, true);

		//optional
		addString("ps_name", "Processed sample name of the associated input file", true);
		addString("processing_system", "Processing system short name of the processed sample", true);
		addString("disease_group", "Disease group of the input sample", true);
		addFlag("test", "Uses NGSD test db instead of the production db");

		changeLog(2022, 2, 11, "Initial commit.");
		changeLog(2025, 1, 13, "Added annotation of counts and AF grouped by disease group");
	}

	virtual void main()
	{
		//init
		QString input_filepath = getInfile("in");
		QString output_filepath = getOutfile("out");
		QString ann_folder = getInfile("ann_folder");
		QString ps_name = getString("ps_name");
		bool test = getFlag("test");
		QByteArray processing_system = getString("processing_system").toUtf8();
		QByteArray disease_group = getString("disease_group").toUtf8().toLower();
		NGSD db(test);
		QTextStream out(stdout);

		// start timer
		QTime timer;
		timer.start();

		//load annotation files
		out << "Loading indexed SV count files...";
		QMap<StructuralVariantType, TabixIndexedFile> count_indices;

		count_indices[StructuralVariantType::DEL].load(QDir(ann_folder).filePath("sv_deletion.bedpe.gz").toUtf8());
		count_indices[StructuralVariantType::DUP].load(QDir(ann_folder).filePath("sv_duplication.bedpe.gz").toUtf8());
		count_indices[StructuralVariantType::INS].load(QDir(ann_folder).filePath("sv_insertion.bedpe.gz").toUtf8());
		count_indices[StructuralVariantType::INV].load(QDir(ann_folder).filePath("sv_inversion.bedpe.gz").toUtf8());
		count_indices[StructuralVariantType::BND].load(QDir(ann_folder).filePath("sv_translocation.bedpe.gz").toUtf8());

		out << " done. " << Helper::elapsedTime(timer) << endl;

		//load input file
		out << "Start BEDPE annotation..." << endl;
		BedpeFile bedpe_input_file;
		bedpe_input_file.load(input_filepath);

		// check if BEDPE file already contains gene annotation:
		int i_ngsd_hom = bedpe_input_file.annotationIndexByName("NGSD_HOM", false);
		int i_ngsd_het = bedpe_input_file.annotationIndexByName("NGSD_HET", false);
		int i_ngsd_af = bedpe_input_file.annotationIndexByName("NGSD_AF", false);
		int i_disease_group = bedpe_input_file.annotationIndexByName("NGSD_group", false);
		bool dg_parameter_given = true;

		// get disease group of input sample
		if (disease_group.isEmpty() || processing_system.isEmpty())
		{
			QString p_sample_id;
			if (ps_name.isEmpty()) p_sample_id = db.processedSampleId(input_filepath);
			else p_sample_id = db.processedSampleId(ps_name);

			// get disease group
			if (disease_group.isEmpty())
			{
				disease_group = db.getValue("SELECT disease_group FROM sample WHERE id = (SELECT sample_id FROM processed_sample WHERE id = :0)", false, p_sample_id).toByteArray().toLower();
				dg_parameter_given = false;
			}

			// get processing system
			if (processing_system.isEmpty())
			{
				processing_system = db.getValue("SELECT name_short FROM processing_system WHERE id = (SELECT processing_system_id FROM processed_sample WHERE id = :0)", false, p_sample_id).toByteArray();
			}
		}

		// get column indices, sample count and disease group ID from one of the annotation files
		parseBedpeGzHead(QDir(ann_folder).filePath("sv_translocation.bedpe.gz").toUtf8(), processing_system, disease_group);

		// check correct disease group mapping and valid input disease group
		QStringList disease_groups = db.getEnum("sample", "disease_group");

		if (dg_parameter_given && !disease_groups.contains(disease_group)) THROW(ArgumentException, "Given disease_group parameter: `" + disease_group + "` is not valid!");

		QMap<QString, QString> disease_group_mapping;
		for(int i = 0; i < disease_groups.size(); i++)
		{
			disease_group_mapping["GSC" + QByteArray::number(i + 1).rightJustified(2, '0')] = disease_groups[i].toLower();
		}

		if (disease_group_mapping[disease_group_id_] != disease_group)
		{
			THROW(FileParseException, "Disease Group ID mapping incorrect in annotation file: " + QDir(ann_folder).filePath("sv_translocation.bedpe.gz").toUtf8() + "!");
		}

		disease_group = disease_group_id_.toUtf8();

		// create text buffer for output file
		QByteArrayList output_buffer;

		// create annotation header
		output_buffer.append(bedpe_input_file.headers().join("\n") + "\n");

		// modify header
		QList<QByteArray> header = bedpe_input_file.annotationHeaders();
		QList<QByteArray> additional_columns;
		if (i_ngsd_hom < 0)
		{
			i_ngsd_hom = header.size();
			additional_columns.append("0");
			header.append("NGSD_HOM");
		}
		if (i_ngsd_het < 0)
		{
			i_ngsd_het = header.size();
			additional_columns.append("0");
			header.append("NGSD_HET");
		}
		if (i_ngsd_af < 0)
		{
			i_ngsd_af = header.size();
			additional_columns.append("");
			header.append("NGSD_AF");
		}
		if (i_disease_group < 0)
		{
			i_disease_group = header.size();
			additional_columns.append("");
			header.append("NGSD_group");
		}
		output_buffer << "#CHROM_A\tSTART_A\tEND_A\tCHROM_B\tSTART_B\tEND_B\t" + header.join("\t") + "\n";

		// iterate over all structural variants
		for (int i=0; i < bedpe_input_file.count(); i++)
		{

			BedpeLine sv = bedpe_input_file[i];

			QList<QByteArray> sv_annotations = sv.annotations();
			// extend annotation by new columns
			if (additional_columns.size() > 0) sv_annotations.append(additional_columns);

			// cache for bnd ids
			QSet<int> bnd_ids;

			// ignore SVs on special chromosomes
			if (sv.chr1().isNonSpecial() && sv.chr2().isNonSpecial())
			{
				BedLine sv_region;
				if (sv.type() == StructuralVariantType::BND)
				{
					sv_region = BedLine(sv.chr1(), sv.start1(), sv.end1() + 1);
				}
				else
				{
					sv_region = BedLine(sv.chr1(), std::min(sv.start1(), sv.start2()), std::max(sv.end1(), sv.end2()) + 1);
				}

				// get all svs in the SV region
				int ngsd_count_hom = 0;
				int ngsd_count_het = 0;
				int ngsd_disease_count_hom = 0;
				int ngsd_disease_count_het = 0;
				QByteArrayList matches = count_indices[sv.type()].getMatchingLines(sv_region.chr(), sv_region.start(), sv_region.end(), true);

				// check resulting lines for exact matches
				foreach (const QByteArray& match, matches)
				{
					QByteArrayList columns = match.split('\t');
					if (columns.size() < 6) THROW(FileParseException, "Too few columns for SV!");

					//skip all samples with different processing sytem
					if (processing_system != columns[idx_processing_system_].trimmed()) continue;

					//check for exact match:
					if (sv.type() == StructuralVariantType::INS)
					{
						//special handling of insertions
						if (sv_region.overlapsWith(Chromosome(columns[0]), Helper::toInt(columns[1]), Helper::toInt(columns[2])))
						{
							//exact match
							if (columns[idx_format_ + 1].split(':').at(0).trimmed() == "1/1")
							{
								ngsd_count_hom++;

								//count by disease group
								if (columns[idx_disease_group_] == disease_group)
								{
									ngsd_disease_count_hom++ ;
								}
							}
							else
							{
								ngsd_count_het++;
								//count by disease group
								if (columns[idx_disease_group_] == disease_group)
								{
									ngsd_disease_count_het++ ;
								}
							}
						}
					}
					else if (sv.type() == StructuralVariantType::BND)
					{
						//skip ids which are already counted
						int bnd_id = Helper::toInt(columns[idx_sv_id_]);
						if (bnd_ids.contains(bnd_id)) continue;

						//pos1 and pos2 of both SVs have to overlap
						if (BedLine(sv.chr1(), sv.start1(), sv.end1() + 1).overlapsWith(Chromosome(columns[0]), Helper::toInt(columns[1]), Helper::toInt(columns[2]) + 1)
							&& BedLine(sv.chr2(), sv.start2(), sv.end2() + 1).overlapsWith(Chromosome(columns[3]), Helper::toInt(columns[4]), Helper::toInt(columns[5]) + 1))
						{
							//exact match
							if (columns[idx_format_ + 1].split(':').at(0).trimmed() == "1/1")
							{
								ngsd_count_hom++;

								//count by disease group
								if (columns[idx_disease_group_] == disease_group)
								{
									ngsd_disease_count_hom++ ;
								}
							}
							else
							{
								ngsd_count_het++;

								//count by disease group
								if (columns[idx_disease_group_] == disease_group)
								{
									ngsd_disease_count_het++ ;
								}
							}
							bnd_ids.insert(bnd_id);
						}
					}
					else
					{
						//pos1 and pos2 of both SVs have to overlap
						BedLine pos1 = BedLine(sv.chr1(), sv.start1(), sv.end1());
						BedLine pos2 = BedLine(sv.chr2(), sv.start2(), sv.end2());

						if (pos1.overlapsWith(Chromosome(columns[0]), Helper::toInt(columns[1]), Helper::toInt(columns[2]))
							&& pos2.overlapsWith(Chromosome(columns[3]), Helper::toInt(columns[4]), Helper::toInt(columns[5])))
						{
							//exact match
							if (columns[idx_format_ + 1].split(':').at(0).trimmed() == "1/1")
							{
								ngsd_count_hom++;

								//count by disease group
								if (columns[idx_disease_group_] == disease_group)
								{
									ngsd_disease_count_hom++ ;
								}
							}
							else
							{
								ngsd_count_het++;

								//count by disease group
								if (columns[idx_disease_group_] == disease_group)
								{
									ngsd_disease_count_het++ ;
								}
							}
						}
					}
				}

				// write annotations
				sv_annotations[i_ngsd_hom] = QByteArray::number(ngsd_count_hom);
				sv_annotations[i_ngsd_het] = QByteArray::number(ngsd_count_het);
				if (sample_count_ >= 20)
				{
					double ngsd_af = std::min(1.0, (double) (2.0 * ngsd_count_hom + ngsd_count_het) / (double) (sample_count_ * 2.0));
					sv_annotations[i_ngsd_af] = QByteArray::number(ngsd_af, 'f', 4);
				}

				sv_annotations[i_disease_group] = QByteArray::number(ngsd_disease_count_hom) + " / " + QByteArray::number(ngsd_disease_count_het);
			}

			//write annotation back to BedpeLine
			sv.setAnnotations(sv_annotations);

			// store line in output buffer
			output_buffer << sv.toTsv() << "\n";
		}

		out << "BEDPE annotation done. " << Helper::elapsedTime(timer) << endl;

		//write buffer to file
		out << "write BEDPE output file..." ;

		// open output file
		QSharedPointer<QFile> output_file = Helper::openFileForWriting(output_filepath, false, false);
		// write buffer to file
		foreach (const QByteArray& line, output_buffer)
		{
			output_file->write(line);
		}

		out << " done. " << Helper::elapsedTime(timer) << endl;


	}

private:

	int sample_count_ = 0;
	int idx_processing_system_ = -1;
	int idx_sv_id_ = -1;
	int idx_format_ = -1;
	int idx_disease_group_ = -1;
	QString disease_group_id_;

	void parseBedpeGzHead(QString file_path, QByteArray processing_system, QByteArray disease_group)
	{
		QTextStream out(stdout);
		//open input file
		FILE* instream = fopen(file_path.toUtf8().data(), "rb");
		if (instream==nullptr) THROW(FileAccessException, "Could not open file '" + file_path + "' for reading!");
		gzFile file = gzdopen(fileno(instream), "rb"); //always open in binary mode because windows and mac open in text mode
		if (file==nullptr) THROW(FileAccessException, "Could not open file '" + file_path + "' for reading!");

		const int buffer_size = 1048576; //1MB buffer
		char* buffer = new char[buffer_size];

		//regex for disease group ID extraction
		QRegularExpression regex(R"(ID=(GSC\d+))");

		//parse BEDPE GZ file
		while(!gzeof(file))
		{
			// get next line
			char* char_array = gzgets(file, buffer, buffer_size);
			//handle errors like truncated GZ file
			if (char_array==nullptr)
			{
				int error_no = Z_OK;
				QByteArray error_message = gzerror(file, &error_no);
				if (error_no!=Z_OK && error_no!=Z_STREAM_END)
				{
					THROW(FileParseException, "Error while reading file '" + file_path + "': " + error_message);
				}
			}
			QByteArray line = QByteArray(char_array);

			//break if headerlines are read
			if(!line.startsWith("#")) break;

			//parse sample count
			if(line.startsWith("##sample_count=(" + processing_system + ","))
			{
				sample_count_ = Helper::toInt(line.split(',').at(1).split(')').at(0), "sample_count header line");
			}

			//parse header line and extract header indices
			if(line.startsWith("#CHROM_A"))
			{
				QByteArrayList header = line.trimmed().split('\t');
				for (int i = 0; i < header.size(); ++i)
				{
					if (header.at(i) == "PROCESSING_SYSTEM")
					{
						idx_processing_system_ = i;
					}
					else if (header.at(i) == "ID")
					{
						idx_sv_id_ = i;
					}
					else if (header.at(i) == "FORMAT")
					{
						idx_format_ = i;
					}
					else if (header.at(i) == "DISEASE_GROUP")
					{
						idx_disease_group_ = i;
					}
				}
			}

			//get disease group ID
			if(line.contains(disease_group))
			{
				disease_group_id_ = regex.match(line).captured(1);
			}
		}

		if (disease_group_id_.isEmpty()) THROW(FileParseException, "Annotation file doesn't contain info about disease group ID for given disease group: '" + disease_group + "'");

		//close file
		gzclose(file);


		//check if all required informations are parsed:
		if (sample_count_ == 0) out << "WARNING: Annotation file doesn't contain sample count for this processing system! NGSD count annotation will be empty.\n";
		else if (sample_count_ < 20) out << "WARNING: Annotation file contains less than 20 samples for this processing system! NGSD allele frequency cannot be calculated.\n";
		if (idx_processing_system_ == -1) THROW(FileParseException, "Annotation file doesn't contain processing system column!");
		if (idx_sv_id_ == -1) THROW(FileParseException, "Annotation file doesn't contain SV id column!");
		if (idx_format_ == -1) THROW(FileParseException, "Annotation file doesn't contain format column!");
	}

};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
