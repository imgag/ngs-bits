#include "ChunkProcessor.h"
#include "VcfFile.h"
#include "ToolBase.h"
#include <zlib.h>
#include <QFileInfo>
#include "Helper.h"
#include "VcfFile.h"
#include "VariantList.h"
#include <QMutex>
#include "BigWigReader.h"

ChunkProcessor::ChunkProcessor(AnalysisJob &job_, const QByteArray& name_, const QByteArray& desc_, const QByteArray& bw_filepath_)

	:QRunnable()
	, terminate_(false)
	, job(job_)
	, name(name_)
	, desc(desc_)
	, bw_filepath(bw_filepath_)
{
}

// single chunks are processed
void ChunkProcessor::run()
{
	job.error_message.clear();
	job.current_chunk_processed.clear();

	// load bw file:
	bw_reader = BigWigReader(bw_filepath);

	// read vcf file
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
				job.current_chunk_processed.append("##INFO=<ID=" + name + ",Number=1,Type=Float,Description=\"" + desc + "\">\n");
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
		QByteArray ref = parts[3];
		QByteArray alt = parts[4];
		if (alt.contains(',')) // if alt contains a list of alternatives choose the first one.
		{
			alt = alt.split(',')[0];
		}
		//get annotation data
		float anno = bw_reader.reproduceVepPhylopAnnotation(chr.str(), start, end, ref, alt);

		// add INFO column annotation
		if(parts[7] == ".") parts[7].clear(); // remove '.' if column was empty before
		if(!parts[7].isEmpty()) parts[7].append(';');
		parts[7].append(name + "=" + QByteArray::number(anno));

		job.current_chunk_processed.append(parts.join('\t') + "\n");
	}

	// annotation of job finished, clear input to keep memory usage low
	job.current_chunk.clear();
	job.status=TO_BE_WRITTEN;
}

float getAnnotation(const QByteArray&, int start, int end, const QByteArray& ref, const QByteArray& alt)
{
	// insertions:
	if (alt.length() > ref.length())
	{
		if ((ref.length() == 1) && (ref[0] != alt[0]) && (start==end)) // insertions that deletes a single base get the value of that base
		{
			return readValue(chr, end); 
			
		}
		return bw_reader.defaultValue(); // for other insertions no value can be determined
	}
	
	// deletions:
	if (ref.length() > alt.length())
	{
		if ((alt.length() == 1) && (ref[0] != alt[0])) // if a single base replaces multiple ref bases set it to zero
		{
			return 0;
		}
		
		if (ref.length() - alt.length() > 1) // if deletion deletes more than one base
		{
		   // check if only a single position has a valid value or they all have the same value: if so return that value.
		   QVector<float> intervals = readValues(chr, start, end);
		   return std::max(intervals) // TODO problem for high default values
	    }
	}
}
