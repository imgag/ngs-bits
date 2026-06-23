#include "ToolBase.h"
#include "VcfFile.h"
#include "VersatileFile.h"

class ConcreteTool
		: public ToolBase
{
	Q_OBJECT

public:
	ConcreteTool(int& argc, char *argv[])
		: ToolBase(argc, argv)
		, debug_(false)
		, debug_stream_(stdout)
	{
	}

	virtual void setup()
	{
		setDescription("Sorts variant lists according to chromosomal position.");
        addInfile("in", "Input variant list in VCF format.", false, true);
		addOutfile("out", "Output variant list in VCF or VCF.GZ format.", false, true);
		//optional
		addInt("compression_level", "Output VCF compression level from 1 (fastest) to 9 (best compression). If unset, an unzipped VCF is written.", true, BGZF_NO_COMPRESSION);
		addFlag("remove_unused_contigs", "Remove comment lines of contigs, i.e. chromosomes, that are not used in the output VCF.");
		addFlag("split_chrs", "Mode with reduced memory consumption for large files. Sorts only one chromosome at a time into a tmp file and merges all tmp files at the end.");
		addFlag("debug", "Enable debug output to STDOUT.");

		changeLog(2026,  6, 23, "Added parameter '-split_chrs'. Removed parameters 'qual' and 'fai'.");
		changeLog(2022, 12,  8, "Added parameter '-remove_unused_contigs'.");
		changeLog(2020,  8, 12, "Added parameter '-compression_level' for compression level of output VCF files.");
	}

	void printTime(QString part, bool restart=true)
	{
		if (!debug_) return;
		debug_stream_ << "Execution time of '" << part << "': " << Helper::elapsedTime(debug_timer_) << Qt::endl;
		if (restart) debug_timer_.restart();
	}

	virtual void main()
	{
		//init
		QString in = getInfile("in");
		QString out = getOutfile("out");
		bool split_chrs = getFlag("split_chrs");
		bool remove_unused_contigs = getFlag("remove_unused_contigs");
		int compression_level = getInt("compression_level");
		if (compression_level<0 || compression_level>10) THROW(ArgumentException, "Invalid gzip compression level '" + QString::number(compression_level) +"' given for VCF file '" + out + "'!");

		debug_ = getFlag("debug");
		debug_timer_.start();

		//sort
		if (split_chrs) //split by chr to save memory
		{
			//determine chromosomes used
			QSet<Chromosome> chr_set;
			VersatileFile file(in);
			file.open(QFile::ReadOnly | QFile::Text);
			while(!file.atEnd())
			{
				QByteArray line = file.readLine(false).trimmed();
				if (line.isEmpty() || line[0]=='#') continue;

				int sep = line.indexOf('\t');
				if (sep==-1) continue;

				QByteArray chr = line.left(sep);
				chr_set << chr;
			}
			file.close();
			printTime("determining chromosomes");

			//sort chromosomes
			QList<Chromosome> chrs(chr_set.begin(), chr_set.end());
			std::sort(chrs.begin(), chrs.end());

			//sort input VCF by chr
			QStringList tmp_files;
			for(const Chromosome& chr: chrs)
			{
				VcfFile vl;
				vl.setChromosome(chr.str());
				vl.load(in);
				printTime("loading " + chr.str() + " (" + QString::number(vl.count()) + " variants)");
				vl.sort();
				printTime("sorting " + chr.str());
				if (remove_unused_contigs) vl.removeUnusedContigHeaders();
				QString tmp_file = Helper::tempFileName("_"+chr.str()+".vcf");
				vl.store(tmp_file, false);
				printTime("storing " + chr.str());

				tmp_files << tmp_file;

				//clear cache
				VcfFile::clearCache();
			}

			//merge tmp files //TODO Alexandr: use VersatileOutFile both for zipped and unzipped output
			if(compression_level == BGZF_NO_COMPRESSION)
			{
				//open output stream
				QSharedPointer<QFile> out_stream = Helper::openFileForWriting(out);

				//merge tmp files
				for (int i=0; i<tmp_files.count(); ++i)
				{
					QSharedPointer<QFile> tmp_stream = Helper::openFileForReading(tmp_files[i]);
					while(!tmp_stream->atEnd())
					{
						QByteArray line = tmp_stream->readLine();
						if (i!=0 && line.startsWith('#')) continue; //headers only for first file

						out_stream->write(line);
					}
				}
			}
			else
			{
				//open output stream
				QByteArray open_flags = "wb"+QByteArray::number(compression_level);
				BGZF* out_stream = bgzf_open(out.toUtf8().data(), open_flags.data());
				if (out_stream==nullptr) THROW(FileAccessException, "Could not open file '" + out + "' for writing!");

				//merge tmp files
				for (int i=0; i<tmp_files.count(); ++i)
				{
					QSharedPointer<QFile> tmp_stream = Helper::openFileForReading(tmp_files[i]);
					while(!tmp_stream->atEnd())
					{
						QByteArray line = tmp_stream->readLine();
						if (i!=0 && line.startsWith('#')) continue; //headers only for first file

						int written_bytes = bgzf_write(out_stream, line.constData(), line.size());
						if(written_bytes!=line.size()) THROW(FileAccessException, "Writing bgzipped VCF file '" + out + "' failed: not all bytes were written.");
					}
				}

				//close file
				int closed = bgzf_close(out_stream);
				if (closed!=0) THROW(FileAccessException, "Writing bgzipped VCF file '" + out + "' failed: could not close file.");
			}
			printTime("merging tmp files");

			//delete tmp files
			for (const QString& tmp_file: std::as_const(tmp_files))
			{
				QFile::remove(tmp_file);
			}
		}
		else //sort the entire VCF in memory
		{
			VcfFile vl;
			vl.load(in);
			printTime("loading");
			vl.sort();
			printTime("sorting");
			if (remove_unused_contigs)
			{	vl.removeUnusedContigHeaders();
				printTime("removing unused contigs");
			}
			vl.store(out, false, compression_level);
			printTime("storing");
		}
    }

protected:
	bool debug_;
	QTextStream debug_stream_;
	QElapsedTimer debug_timer_;
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

