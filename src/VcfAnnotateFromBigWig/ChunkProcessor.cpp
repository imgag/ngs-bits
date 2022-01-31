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
    , bw_reader(bw_filepath_)
{
}

// single chunks are processed
void ChunkProcessor::run()
{
	job.error_message.clear();
	job.current_chunk_processed.clear();

<<<<<<< HEAD
=======
	// load bw file:
	bw_reader = BigWigReader(bw_filepath);

>>>>>>> 87d48ea61b4ed64ef2f4889176cf8981c90a4a87
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
        QList<float> anno = getAnnotation(chr.str(), start, end, ref, alt);

        if (anno.length() == 0)
        {
            // if there is no annotation to add, append the line unchanged:
            job.current_chunk_processed.append(line + "\n");
            continue;
        }

		// add INFO column annotation
		if(parts[7] == ".") parts[7].clear(); // remove '.' if column was empty before
		if(!parts[7].isEmpty()) parts[7].append(';');
        parts[7].append(name + "=" + QByteArray::number(anno[0]));

		job.current_chunk_processed.append(parts.join('\t') + "\n");
	}

	// annotation of job finished, clear input to keep memory usage low
	job.current_chunk.clear();
	job.status=TO_BE_WRITTEN;
}

QList<float> ChunkProcessor::getAnnotation(const QByteArray& chr, int start, int end, const QString& ref, const QString& alt)
{
    int offset = -1; // offset is -1 as the vcf file uses genome coordinates from 1 - N but bw-files from 0 - N-1

    // insertions:
    if (alt.length() > ref.length())
    {
        if ((ref.length() == 1) && (ref[0] != alt[0]) && (start==end)) // insertions that deletes a single base get the value of that base
        {
            return interpretIntervals(bw_reader.getOverlappingIntervals(chr, end, end+1, offset));
        }
        return QList<float>();
    }

    return interpretIntervals(bw_reader.getOverlappingIntervals(chr, start, end, offset));
}

QList<float> ChunkProcessor::interpretIntervals(const QList<OverlappingInterval>& intervals)
{
    if (intervals.length() == 0)
    {
        return QList<float>();
    }
    else if (intervals.length() == 1)
    {
        return QList<float>{intervals[0].value};
    }
    else
    {
        float max = std::numeric_limits<float>::lowest();
        foreach (OverlappingInterval i, intervals)
        {
            if (i.value > max)
            {
                max = i.value;
            }
        }
        return QList<float>{max};
    }
}
