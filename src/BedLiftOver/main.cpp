#include "BedFile.h"
#include "ToolBase.h"
#include "Exceptions.h"
#include "ChainFileReader.h"
#include "Helper.h"
#include "Settings.h"

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
		setDescription("Lift-over of regions in a BED file to a different genome build.");
		addInfile("in", "Input BED file with the regions to lift.", false);
		addOutfile("out", "The file where the lifted regions will be written to.", false);

		//optional
		addOutfile("unmapped", "The file where the unmappable regions will be written to.", true, false);
		addString("chain", "Input Chain file in .chain/.chain.gz format or \"hg19_hg38\" / \"hg38_hg19\" to read from settings file.", true, "hg19_hg38");
		addInt("max_deletion", "Allowed percentage of deleted/unmapped bases in each region.", true, 5);
		addInt("max_increase", "Allowed percentage size increase of a region.", true, 10);
		addFlag("remove_special_chr", "Removes regions that are mapped to special chromosomes.");
		addFlag("merged_output", "Output lifted and unlifted regions in the output file, keeping the order of the input regions.");

		changeLog(2022,  02, 14, "First implementation");
	}

	virtual void main()
	{
		//init
		QString in = getInfile("in");
		QSharedPointer<QFile> bed = Helper::openFileForReading(in);
		QString chain = getString("chain");
		int max_inc = getInt("max_increase");
		int max_del = getInt("max_deletion");
		bool remove_special_chr = getFlag("remove_special_chr");
		bool merged_output = getFlag("merged_output");


		if (! QFile(chain).exists() && ! chain.contains('\\') && ! chain.contains('/'))
		{
			chain = Settings::string("liftover_" + chain, false);
		}

		if (max_del < 0 || 100 < max_del)
		{
			THROW(ArgumentException, "Allowed percentage of deleted/unmapped bases can't be smaller than 0 or larger than 100.");
		}

		if (max_inc < 0)
		{
			THROW(ArgumentException, "Allowed maximum size increase of the region can't be negative");
		}

		ChainFileReader reader(chain, (max_del/100.0));

		//open output
		QString lifted_path = getOutfile("out");
		QSharedPointer<QFile> lifted = Helper::openFileForWriting(lifted_path, true);

		QString unmapped_path = getOutfile("unmapped");
		if (merged_output && unmapped_path != "")
		{
			THROW(ArgumentException, "Flag 'merged_output' and 'unmapped' outfile cannot be given together. With Flag 'merged_output' all regions are reported in 'out' outfile.");
		}

		QSharedPointer<QFile> unmapped;
		if (unmapped_path != "")
		{
			unmapped = Helper::openFileForWriting(unmapped_path, true);
		}

		if (merged_output)
		{
			unmapped = lifted;
		}

		//for statistics:
		int in_count = 0;
		long long in_length = 0;
		long long unlifted_in_length = 0;

		int lifted_count = 0;
		int unlifted_count = 0;
		long long lifted_length = 0;

		//write BedLiftOver header

		QString header_line = "#BedLiftOver: Lifted file using '" + chain + "' \n";
		lifted->write(header_line.toUtf8());

		while(! bed->atEnd())
		{
			QByteArray line = bed->readLine();
			//write out headers
			if (line.startsWith("#") || line.startsWith("track ") || line.startsWith("browser "))
			{
				lifted->write(line);
				continue;
			}

			while (line.endsWith('\n') || line.endsWith('\r')) line.chop(1);
			QByteArrayList parts = line.split('\t');
			QByteArrayList annotations;
			if (parts.count() > 3)
			{
				for (int i=3; i<parts.count(); i++)
				{
					annotations.append(parts[i]);
				}
			}

			BedLine l = BedLine(parts[0], Helper::toInt(parts[1], "range start position", line), Helper::toInt(parts[2], "range end position", line), annotations);

			in_count++;
			in_length += l.length();

			try
			{
				// convert to 1-based coordinates for lifting
				BedLine lifted_line = reader.lift(l.chr(), l.start()+1, l.end());
				//convert back to 0-based for bed file.
				lifted_line.setStart(lifted_line.start()-1);

				if (lifted_line.length() > l.length()+l.length()*(max_inc/100.0))
				{
					THROW(ArgumentException, "Region increased in size more than " + QString::number(max_inc) + "%.");
				}

				if (! lifted_line.chr().isNonSpecial() && remove_special_chr)
				{
					THROW(ArgumentException, "Region was mapped to a special chromosome.");
				}

				lifted_line.annotations() = l.annotations();
				lifted->write(lifted_line.toStringWithAnnotations().toUtf8() + "\n");
				lifted_count++;
				lifted_length += lifted_line.length();
			}
			catch (ArgumentException& e)
			{
				unlifted_count++;
				unlifted_in_length += l.length();

				if (! unmapped.isNull())
				{
					unmapped->write(l.toString(false).toUtf8() +"\t#Error: " + e.message().toUtf8() + "\n");
				}
			}
		}

		// print statistics:
		QTextStream out(stdout);
		long long lifted_in_length = in_length - unlifted_in_length;
        out << "LiftOver Statistics:" << QT_ENDL;
		out << "Input regions : " << in_count << QT_ENDL;
        out << "lifted        : " << lifted_count << " (" << QString::number(100.0*lifted_count/in_count, 'f', 2) << "%)" << QT_ENDL;
        out << "unlifted      : " << unlifted_count << " (" << QString::number(100.0*unlifted_count/in_count, 'f', 2) << "%)" << QT_ENDL;
        out << QT_ENDL;
        out << "Bases input: " << in_length << QT_ENDL;
        out << "lifted     : " << lifted_in_length << " (" << QString::number(100.0*lifted_in_length/in_length, 'f', 2) << "%)" << QT_ENDL;
        out << "unlifted   : " << unlifted_in_length  << " (" << QString::number(100.0*unlifted_in_length/in_length, 'f', 2) << "%)" << QT_ENDL;
        out << QT_ENDL;
        out << "Bases after lifting: " << lifted_length << QT_ENDL;

	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

