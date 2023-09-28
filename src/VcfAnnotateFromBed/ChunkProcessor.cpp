#include "ChunkProcessor.h"
#include "VcfFile.h"
#include "ToolBase.h"
#include <zlib.h>
#include <QFileInfo>
#include "Helper.h"
#include "VcfFile.h"
#include "VariantList.h"
#include <QMutex>

ChunkProcessor::ChunkProcessor(AnalysisJob &job_, QByteArray name_, const BedFile& bed_file_, const ChromosomalIndex<BedFile>& bed_index_, QByteArray bed_file_path_, QByteArray sep_)

	:QRunnable()
	, terminate_(false)
	, job(job_)
	, name(name_)
	, bed_file(bed_file_)
	, bed_index(bed_index_)
	, bed_file_path(bed_file_path_)
	, sep(sep_)
{
}



// single chunks are processed
void ChunkProcessor::run()
{
	job.error_message.clear();
	job.current_chunk_processed.clear();
	// read file
	foreach(QByteArray line, job.current_chunk)
	{
		while (line.endsWith('\n') || line.endsWith('\r')) line.chop(1);

		//skip empty lines
		if (line.trimmed().isEmpty()) continue;

		//write out headers unchanged
		if (line.startsWith('#'))
		{
			//append header line for new annotation
			if (line.startsWith("#CHROM"))
			{
				job.current_chunk_processed.append("##INFO=<ID=" + name + ",Number=.,Type=String,Description=\"Annotation from " + QFileInfo(bed_file_path).fileName().toLatin1() + " delimited by '" + sep + "'\">\n");
			}
			job.current_chunk_processed.append(line + "\n");
			continue;
		}

		//split line and extract variant infos
		QList<QByteArray> parts = line.split('\t');
		if (parts.count()<VcfFile::MIN_COLS)
		{
			job.error_message.append("FileParseException: VCF line with too few columns: " + line);
			job.status = ERROR;
			return;
		}
		Chromosome chr = parts[0];
		bool ok = false;
		int start = parts[1].toInt(&ok);
		if (!ok)
		{
			job.error_message.append("FileParseException: Could not convert VCF variant position '" + parts[1] + "' to integer!");
			job.status = ERROR;
			return;
		}
		int end = start + parts[3].length() - 1; //length of ref

		//get annotation data
		QByteArrayList annos;
		QVector<int> indices = bed_index.matchingIndices(chr, start, end);
		foreach(int index, indices)
		{
			annos << bed_file[index].annotations()[0];
		}

		//write output line
		if (annos.isEmpty())
		{
			job.current_chunk_processed.append(line + "\n");
		}
		else
		{
			// add INFO column annotation
			if(parts[7] == ".") parts[7].clear(); // remove '.' if column was empty before
			if(!parts[7].isEmpty()) parts[7].append(';');
			parts[7].append(name + "=" + VcfFile::encodeInfoValue(annos.join(sep)).toUtf8());

			job.current_chunk_processed.append(parts.join('\t') + "\n");

		}
	}

	// annotation of job finished, clear input to keep memory usage low
	job.current_chunk.clear();
	job.status=TO_BE_WRITTEN;
}
