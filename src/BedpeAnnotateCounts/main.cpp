#include "BedFile.h"
#include "ToolBase.h"
#include "NGSHelper.h"
#include "Settings.h"
#include "VcfFile.h"
#include "BedpeFile.h"
#include "Helper.h"
#include "TabixIndexedFile.h"
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
		addString("processing_system", "Processing system short name of the processed sample", false);

		changeLog(2022, 2, 11, "Initial commit.");
	}

	virtual void main()
	{
		//init
		QString input_filepath = getInfile("in");
		QString output_filepath = getOutfile("out");
		QString ann_folder = getInfile("ann_folder");
		QByteArray processing_system = getString("processing_system").toUtf8();
		QTextStream out(stdout);

		// start timer
		QTime timer;
		timer.start();

		//load annotation files
		out << "Loading indexed SV count files...";
		QMap<StructuralVariantType, TabixIndexedFile> count_indices;
		TabixIndexedFile del_index;
		del_index.load(QDir(ann_folder).filePath("sv_deletion.bedpe.gz").toUtf8());
		count_indices.insert(StructuralVariantType::DEL, del_index);
		TabixIndexedFile dup_index;
		dup_index.load(QDir(ann_folder).filePath("sv_duplication.bedpe.gz").toUtf8());
		count_indices.insert(StructuralVariantType::DUP, dup_index);
		TabixIndexedFile ins_index;
		ins_index.load(QDir(ann_folder).filePath("sv_insertion.bedpe.gz").toUtf8());
		count_indices.insert(StructuralVariantType::DEL, ins_index);
		TabixIndexedFile inv_index;
		inv_index.load(QDir(ann_folder).filePath("sv_inversion.bedpe.gz").toUtf8());
		count_indices.insert(StructuralVariantType::DEL, inv_index);
		TabixIndexedFile bnd_index;
		bnd_index.load(QDir(ann_folder).filePath("sv_translocation.bedpe.gz").toUtf8());
		count_indices.insert(StructuralVariantType::DEL, bnd_index);

		//TODO: extract indices for processing system and ID columns
		int idx_sv_id = -1;
		int idx_processing_system = -1;

		//TODO: extract sample count
		int sample_count = -1;

		out << " done. " << Helper::elapsedTime(timer);

		//load input file
		out << "Start BEDPE annotation..." << endl;
		BedpeFile bedpe_input_file;
		bedpe_input_file.load(input_filepath);

		// check if BEDPE file already contains gene annotation:
		int i_ngsd_count = bedpe_input_file.annotationIndexByName("NGSD_COUNT", false);
		int i_ngsd_count_overlap = bedpe_input_file.annotationIndexByName("NGSD_COUNT_OVERLAP", false);

		// create text buffer for output file
		QByteArrayList output_buffer;

		// copy comments
		output_buffer.append(bedpe_input_file.headers());

		// modify header
		QList<QByteArray> header = bedpe_input_file.annotationHeaders();
		QList<QByteArray> additional_columns;
		if (i_ngsd_count < 0)
		{
			i_ngsd_count = header.size();
			additional_columns.append("0 (0.0000)");
			header.append("NGSD_COUNT");
		}
		if (i_ngsd_count_overlap < 0)
		{
			i_ngsd_count_overlap = header.size();
			additional_columns.append("0");
			header.append("NGSD_COUNT_OVERLAP");
		}
		output_buffer << "#CHROM_A\tSTART_A\tEND_A\tCHROM_B\tSTART_B\tEND_B\t" + header.join("\t");

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
				switch (sv.type())
				{
					case StructuralVariantType::DEL:
					case StructuralVariantType::DUP:
					case StructuralVariantType::INV:
						sv_region = BedLine(sv.chr1(), sv.start1(), sv.end2());
						break;
					case StructuralVariantType::INS:
						sv_region = BedLine(sv.chr1(), std::min(sv.start1(), sv.start2()), std::max(sv.end1(), sv.end2()));
						break;
					case StructuralVariantType::BND:
						sv_region = BedLine(sv.chr1(), sv.start1(), sv.end2());
						break;
					default:
						THROW(ArgumentException, "Invalid SV type!");
						break;
				}

				// get all svs in the SV region
				int ngsd_count = 0;
				int ngsd_count_overlap = 0;
				QByteArrayList matches = count_indices[sv.type()].getMatchingLines(sv_region.chr(), sv_region.start(), sv_region.end());

				// check resulting lines for exact matches
				foreach (const QByteArray& match, matches)
				{
					QByteArrayList columns = match.split('\t');
					if (columns.size() < 6) THROW(FileParseException, "Too few columns for SV!");

					//skip all samples with different processing sytem
					if (processing_system == columns[idx_processing_system].trimmed()) continue;

					//check for exact match:
					if (sv.type() == StructuralVariantType::INS)
					{
						//special handling of insertions
						if (sv_region.overlapsWith(Chromosome(columns[0]), std::min(Helper::toInt(columns[1]), Helper::toInt(columns[4])), std::max(Helper::toInt(columns[2]), Helper::toInt(columns[5]))))
						{
							//exact match
							ngsd_count++;
							ngsd_count_overlap++;
						}
					}
					else if (sv.type() == StructuralVariantType::BND)
					{
						//skip ids which are already counted
						int bnd_id = Helper::toInt(columns[idx_sv_id]);
						if (bnd_ids.contains(bnd_id)) continue;

						//pos1 and pos2 of both SVs have to overlap
						if (BedLine(sv.chr1(), sv.start1(), sv.end1()).overlapsWith(Chromosome(columns[0]), Helper::toInt(columns[1]), Helper::toInt(columns[2]))
							&& BedLine(sv.chr2(), sv.start2(), sv.end2()).overlapsWith(Chromosome(columns[3]), Helper::toInt(columns[4]), Helper::toInt(columns[5])))
						{
							//exact match
							ngsd_count++;
							ngsd_count_overlap++;
							bnd_ids.insert(bnd_id);
						}
					}
					else
					{
						//pos1 and pos2 of both SVs have to overlap
						if (BedLine(sv.chr1(), sv.start1(), sv.end1()).overlapsWith(Chromosome(columns[0]), Helper::toInt(columns[1]), Helper::toInt(columns[2]))
							&& BedLine(sv.chr2(), sv.start2(), sv.end2()).overlapsWith(Chromosome(columns[3]), Helper::toInt(columns[4]), Helper::toInt(columns[5])))
						{
							//exact match
							ngsd_count++;
						}
						//overlap match already fulfilled by tabix selection
						ngsd_count_overlap++;
					}
				}

				// write annotations
				double af = 0.00;
				if (sample_count != 0) af = (double) ngsd_count / (double) sample_count;
				sv_annotations[i_ngsd_count] = QByteArray::number(ngsd_count)
						+ " (" + QByteArray::number(af, 'f', 4) + ")";
				sv_annotations[i_ngsd_count_overlap] = QByteArray::number(ngsd_count_overlap);
			}

			//write annotation back to BedpeLine
			sv.setAnnotations(sv_annotations);

			// store line in output buffer
			output_buffer << sv.toTsv() << "\n";
		}

		out << "BEDPE annotation done. " << Helper::elapsedTime(timer) << endl;

		//write buffer to file
		out << "write BEDPE output file..." << endl;

		// open output file
		QSharedPointer<QFile> output_file = Helper::openFileForWriting(output_filepath, false, false);
		// write buffer to file
		foreach (const QByteArray& line, output_buffer)
		{
			output_file->write(line);
		}
		output_file->flush();
		output_file->close();

		out << " done. " << Helper::elapsedTime(timer) << endl;

	}

private:

	void parseBedpeGzHead(QString file_path, QByteArray processing_system, int& sample_count, int& idx_processing_system, int& idx_sv_id)
	{
//		//init vars
//		sample_count = -1;
//		idx_processing_system = -1;
//		idx_sv_id = -1;

//		//open input file
//		FILE* instream = fopen(file_path.toStdString(), "rb");
//		gzFile file = gzdopen(fileno(instream), "rb"); //always open in binary mode because windows and mac open in text mode
//		if (file==NULL)
//		{
//			THROW(FileAccessException, "Could not open file '" + file_path + "' for reading!");
//		}

//		const int buffer_size = 1048576; //1MB buffer
//		char* buffer = new char[buffer_size];

//		//parse BEDPE GZ file
//		while(!gzeof(file))
//		{
//			// get next line
//			char* char_array = gzgets(file, buffer, buffer_size);
//			//handle errors like truncated GZ file
//			if (char_array==nullptr)
//			{
//				int error_no = Z_OK;
//				QByteArray error_message = gzerror(file, &error_no);
//				if (error_no!=Z_OK && error_no!=Z_STREAM_END)
//				{
//					THROW(FileParseException, "Error while reading file '" + file_path + "': " + error_message);
//				}
//			}
//			QByteArray line = QByteArray(char_array);

//			//break if headerlines are read
//			if(!line.startsWith("#")) break;

//			//parse sample count
//			if(line.startsWith("##sample_count="))
//			{

//			}

//			//parse header line
//			if(line.startsWith("#"))
//			{

//			}



//		}

//		//close file
//		gzclose(file);
//		delete[] buffer;


	}

};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
