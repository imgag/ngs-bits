#include "BedFile.h"
#include "Exceptions.h"
#include "ChromosomalIndex.h"
#include "Helper.h"
#include <QFile>
#include <QStringList>
#include <QTextStream>
#include "BasicStatistics.h"
#include "algorithm"
#include "cmath"

BedLine::BedLine()
	: chr_()
	, start_(0)
	, end_(-1)
{
}

BedLine::BedLine(const Chromosome& chr, int start, int end, const QByteArrayList& annotations)
	: chr_(chr)
	, start_(start)
	, end_(end)
	, annotations_(annotations)
{
}

bool BedLine::operator<(const BedLine& rhs) const
{
	if (chr_<rhs.chr_) return true;
	else if (chr_>rhs.chr_) return false;
	else if (start_==rhs.start_) return end_<rhs.end_;
	else return start_<rhs.start_;
}

BedLine BedLine::fromString(QString str)
{
	//normalize
	str = str.replace(',', ""); //remove thousands separator
	str = str.replace(':', '\t').replace('-', '\t'); //also accept "[c]:[s]-[e]"
	str = str.replace(QRegExp("[ ]+"), "\t"); //also accept "[c] [s] [e]" (with any number of spaces)

	//split
	QStringList parts = str.split('\t');
	if (parts.count()<3) return BedLine();

	//convert
	try
	{
		return BedLine(parts[0], Helper::toInt(parts[1], "range start position", str), Helper::toInt(parts[2], "range end position", str));
	}
	catch(...)
	{
		return BedLine();
	}
}

BedFile::BedFile()
{
}

BedFile::BedFile(const Chromosome& chr, int start, int end)
{
	append(BedLine(chr, start, end));
}

void BedFile::append(const BedLine& line)
{
	//check input data
	if (!line.chr().isValid())
	{
		THROW(ArgumentException, "Invalid BED line chromosome - empty string!");
	}
	if (line.start()<1 || line.end()<1 || line.start()>line.end())
	{
		THROW(ArgumentException, "Invalid BED line range '" + QString::number(line.start()) + "' to '" + QString::number(line.end()) + "'!");
	}

	lines_.append(line);
}

long long BedFile::baseCount() const
{
    long long output = 0;
	for (int i=0; i<lines_.count(); ++i)
    {
        output += lines_[i].length();
	}
	return output;
}

QSet<Chromosome> BedFile::chromosomes() const
{
	QSet<Chromosome> output;
	for (int i=0; i<lines_.count(); ++i)
	{
		output.insert(lines_[i].chr());
	}
	return output;
}

void BedFile::load(QString filename, bool stdin_if_empty)
{
	clear();

	//parse from stream
	QSharedPointer<QFile> file = Helper::openFileForReading(filename, stdin_if_empty);
	while(!file->atEnd())
	{
		QByteArray line = file->readLine();
		while (line.endsWith('\n') || line.endsWith('\r')) line.chop(1);

		//skip empty lines
		if(line.length()==0) continue;

		//store headers
		if (line.startsWith("#") || line.startsWith("track ") || line.startsWith("browser "))
		{
			headers_.append(line);
			continue;
		}

		//error when less than 3 fields
		QByteArrayList fields = line.split('\t');
		if (fields.count()<3)
		{
			THROW(FileParseException, "BED file line with less than three fields found: '" + line.trimmed() + "'");
		}

		//check that start/end is number
		bool ok = true;
		int start = fields[1].toInt(&ok) + 1;
		if (!ok) THROW(FileParseException, "BED file line with invalid starts position found: '" + line.trimmed() + "'");
		int end = fields[2].toInt(&ok);
		if (!ok) THROW(FileParseException, "BED file line with invalid end position found: '" + line.trimmed() + "'");
		append(BedLine(fields[0], start, end, fields.mid(3)));
	}
}

void BedFile::store(QString filename, bool stdout_if_empty) const
{
	//open stream
	QSharedPointer<QFile> file = Helper::openFileForWriting(filename, stdout_if_empty);
	QTextStream stream(file.data());

	//write headers
	foreach(const QByteArray& header, headers_)
	{
		stream << header.trimmed()  << "\n";
	}

	//write contents
	foreach(const BedLine& line, lines_)
	{
		QString line_text = line.chr().str() + "\t" + QString::number(line.start()-1) + "\t" + QString::number(line.end());
		stream << line_text.toLatin1();
		foreach(const QByteArray& anno, line.annotations())
		{
			stream << '\t' << anno;
		}
		stream << "\n";
	}
}

QString BedFile::toText() const
{
	QString output;

	foreach(const BedLine& line, lines_)
	{
		output.append(line.chr().str() + "\t" + QString::number(line.start()-1) + "\t" + QString::number(line.end()));
		foreach(const QByteArray& anno, line.annotations())
		{
			output.append("\t" + anno);
		}
		output.append("\n");
	}

	return output;
}

void BedFile::clearAnnotations()
{
	for(int i=0; i<lines_.count(); ++i)
	{
		lines_[i].annotations().clear();
	}
}

void BedFile::clearHeaders()
{
	headers_.clear();
}

void BedFile::sort()
{
	std::sort(lines_.begin(), lines_.end());
}

void BedFile::sortWithName()
{
	LessComparatorWithName comparator;
	std::sort(lines_.begin(), lines_.end(), comparator);
}

void BedFile::removeDuplicates()
{
	if (!isSorted()) THROW(ProgrammingException, "Cannot use 'BedFile::removeDuplicates' on unsorted BED file!");

	lines_.erase(std::unique(lines_.begin(), lines_.end()), lines_.end());
}

void BedFile::merge(bool merge_back_to_back, bool merge_names, bool merged_names_unique)
{
	//in the following code, we assume that at least one line is present...
	if (lines_.count()==0) return;

	//remove annotations data
	for(int i=0; i<lines_.count(); ++i)
	{
		if (merge_names)
		{
			QByteArray name = lines_[i].annotations().isEmpty() ? "" : lines_[i].annotations()[0];
			lines_[i].annotations().clear();
			lines_[i].annotations().append(name);
		}
		else
		{
			lines_[i].annotations().clear();
		}
	}

	//sort if necessary
	if (!isSorted()) sort();

	//merge lines
	BedLine next_output_line = lines_.first();
	int next_output_index = 0;
	for (int i=1; i<lines_.count(); ++i)
	{
		const BedLine& line = lines_[i];

		if ( next_output_line.overlapsWith(line.chr(), line.start(), line.end())
			 ||
			 (merge_back_to_back && next_output_line.adjacentTo(line.chr(), line.start(), line.end()))
			)
		{
			if (line.end()>next_output_line.end())
			{
				next_output_line.setEnd(line.end());
			}
			if (merge_names)
			{
				const QByteArray& anno = line.annotations()[0];
				if (!merged_names_unique || !next_output_line.annotations().contains(anno))
				{
					next_output_line.annotations() << anno;
				}
			}
		}
		else
		{
			lines_[next_output_index] = next_output_line;
			if (merge_names)
			{
				QByteArray annos_merged = next_output_line.annotations().join(",");
				lines_[next_output_index].annotations().clear();
				lines_[next_output_index].annotations().append(annos_merged);
			}

			++next_output_index;
			next_output_line = line;
		}
	}

	//add last line
	lines_[next_output_index] = next_output_line;
	if (merge_names)
	{
		QByteArray annos_merged = next_output_line.annotations().join(",");
		lines_[next_output_index].annotations().clear();
		lines_[next_output_index].annotations().append(annos_merged);
	}

	//remove excess lines
	lines_.resize(next_output_index+1);
}

void BedFile::extend(int n)
{
	if (n<1)
	{
		THROW(ArgumentException, "Cannot extend BED file by '" + QString::number(n) + "' bases!");
	}

	for (int i=0; i<lines_.count(); ++i)
	{
		BedLine& line = lines_[i];
		line.setStart(std::max(1, line.start() - n));
		line.setEnd(line.end() + n);
	}
}

void BedFile::shrink(int n)
{
	if (n<1)
	{
		THROW(ArgumentException, "Cannot shrink BED file by '" + QString::number(n) + "' bases!");
	}

	for (int i=0; i<lines_.count(); ++i)
	{
		BedLine& line = lines_[i];
		line.setStart(line.start() + n);
		line.setEnd(line.end() - n);
	}

	removeInvalidLines();
}

void BedFile::add(const BedFile& file2)
{
	for (int i=0; i<file2.count(); ++i)
	{
		append(file2[i]);
	}
}

void BedFile::subtract(const BedFile& file2)
{
	//check target region is merged/sorted and create index
	if (!file2.isMergedAndSorted())
	{
		THROW(ArgumentException, "Merged and sorted BED file required for calculating the difference of BED files!");
	}
	ChromosomalIndex<BedFile> file2_idx(file2);

	//remove annotations
	clearAnnotations();

	//subtract
	int removed_lines = 0;
	for (int i=0; i<lines_.count(); ++i)
	{
		QVector<int> matches = file2_idx.matchingIndices(lines_[i].chr(), lines_[i].start(), lines_[i].end());
		foreach(int index, matches)
		{
			const BedLine& line2 = file2[index];

			//check overlap (needed because we append lines)
			if (!lines_[i].overlapsWith(line2.chr(), line2.start(), line2.end())) continue;

			// subtract all (make region invalid)
			if (line2.start()<=lines_[i].start() && line2.end()>=lines_[i].end())
			{
				lines_[i].setStart(0);
				lines_[i].setEnd(0);
				++removed_lines;
			}
			// subract from middle (2 regions are created)
			else if (line2.start()>lines_[i].start() && line2.end()<lines_[i].end())
			{
				//create new region (right part)
				append(BedLine(lines_[i].chr(), line2.end()+1, lines_[i].end()));
				//trim old region (left part)
				lines_[i].setEnd(line2.start()-1);
			}
			// subtract from left
			else if (line2.start()>lines_[i].start())
			{
				lines_[i].setEnd(line2.start()-1);
			}
			// subtract from right
			else
			{
				lines_[i].setStart(line2.end()+1);
			}
		}
	}

	//remove invalid lines, if necessary
	if (removed_lines!=0) removeInvalidLines();
}

void BedFile::intersect(const BedFile& file2)
{
	//check target region is merged/sorted and create index
	if (!file2.isMergedAndSorted())
	{
		THROW(ArgumentException, "Merged and sorted BED file required for for calculating the intersect of BED files!");
	}
	ChromosomalIndex<BedFile> file2_idx(file2);

	//remove annotations and headers
	clearAnnotations();

	//intersect
	int lines_original = lines_.count();
	for (int i=0; i<lines_original; ++i)
	{
		const BedLine& line = lines_[i];
		QVector<int> matches = file2_idx.matchingIndices(line.chr(), line.start(), line.end());
		//not match => not intersect => remove
		if (matches.count()==0)
		{
			lines_[i].setStart(0);
			lines_[i].setEnd(0);
			continue;
		}

		//intersect with first region (update line)
		int start_original = line.start();
		int end_original = line.end();
		lines_[i].setStart(std::max(start_original, file2[matches[0]].start()));
		lines_[i].setEnd(std::min(end_original, file2[matches[0]].end()));

		//intersect with more regions (insert new lines => we must not use the 'line' variable inside the loop because the vector can be reallocated!)
		Chromosome chr_original = line.chr();
		for (int j=1; j<matches.count(); ++j)
		{
			lines_.append(BedLine(chr_original, std::max(start_original, file2[matches[j]].start()), std::min(end_original, file2[matches[j]].end()) ));
		}
	}

	removeInvalidLines();
}

void BedFile::overlapping(const BedFile& file2)
{
	//check target region is merged/sorted and create index
	if (!file2.isMergedAndSorted())
	{
		THROW(ArgumentException, "Merged and sorted BED file required for calculating the overlap of BED files!");
	}
	ChromosomalIndex<BedFile> file2_idx(file2);

	//overlapping
	for (int i=0; i<lines_.count(); ++i)
	{
		if (file2_idx.matchingIndex(lines_[i].chr(), lines_[i].start(), lines_[i].end())==-1)
		{
			lines_[i].setStart(0);
			lines_[i].setEnd(0);
		}
	}

	removeInvalidLines();
}

void BedFile::chunk(int chunk_size)
{
	QVector<BedLine> new_lines;
	new_lines.reserve(lines_.count());

	for (int i=0; i<lines_.count(); ++i)
	{
		const BedLine& line = lines_[i];
		if (line.length()>chunk_size)
		{
			//determine how many chunks are optimal
			double length = line.length();
			int n = floor(length/chunk_size);
			if (fabs(chunk_size-(length/n)) > fabs(chunk_size-(length/(n+1))))
			{
				n += 1;
			}
			//calculate actual chunk sizes
			QVector<int> chunk_sizes(n, chunk_size);
			int rest = line.length()-n*chunk_size;
			int current_chunk = 0;
			while (rest!=0)
			{
				int sign = BasicStatistics::sign(rest);
				chunk_sizes[current_chunk] += sign;
				rest -= sign;
				++current_chunk;
				if (current_chunk==n) current_chunk = 0;
			}
			//create new lines
			int start = line.start();
			BedLine new_line = line;
			for (int i=0; i<n; ++i)
			{
				int end = start+chunk_sizes[i]-1;
				new_line.setStart(start);
				new_line.setEnd(end);
				new_lines.append(new_line);
				start = end+1;
			}
		}
		else
		{
			new_lines.append(line);
		}
	}

	lines_.swap(new_lines);
}

void BedFile::removeInvalidLines()
{
	//shift valid lines to the front
	int o=0;
	for (int i=0; i<lines_.count(); ++i)
	{
		const BedLine& line = lines_[i];
		if (line.start()>0 && line.start()<=line.end())
		{
			if (i!=o) lines_[o] = line;
			++o;
		}
	}

	//remove excess lines
	lines_.resize(o);
}

bool BedFile::isSorted() const
{
	for (int i=1; i<lines_.count(); ++i)
	{
		if (lines_[i]<lines_[i-1]) return false;
	}

	return true;
}

bool BedFile::isMerged() const
{
	if (isSorted())
	{
		for (int i=1; i<lines_.count(); ++i)
		{
			const BedLine& line = lines_[i];
			if (lines_[i-1].overlapsWith(line.chr(), line.start(), line.end()))
			{
				return false;
			}
		}
	}
	else
	{
		BedFile tmp = *this;
		tmp.sort();
		for (int i=1; i<tmp.count(); ++i)
		{
			const BedLine& line = tmp[i];
			if (tmp[i-1].overlapsWith(line.chr(), line.start(), line.end()))
			{
				return false;
			}
		}

	}

	return true;
}

bool BedFile::isMergedAndSorted() const
{
	for (int i=1; i<lines_.count(); ++i)
	{
		const BedLine& line = lines_[i];
		if (line<lines_[i-1])
		{
			return false;
		}
		if (lines_[i-1].overlapsWith(line.chr(), line.start(), line.end()))
		{
			return false;
		}
	}

	return true;
}

bool BedFile::overlapsWith(const Chromosome& chr, int start, int end) const
{
	for (int i=0; i<lines_.count(); ++i)
	{
		if (lines_[i].overlapsWith(chr, start, end))
		{
			return true;
		}
	}

	return false;
}

BedFile BedFile::fromText(const QByteArray& string)
{
	BedFile output;

	QByteArrayList lines = string.split('\n');
	foreach (QByteArray line, lines)
	{
		line = line.trimmed();
		if(line.isEmpty()) continue;

		//store headers
		if (line.startsWith("#") || line.startsWith("track ") || line.startsWith("browser "))
		{
			output.appendHeader(line);
			continue;
		}

		//error when less than 3 fields
		QByteArrayList fields = line.split('\t');
		if (fields.count()<3)
		{
			THROW(FileParseException, "BED file line with less than three fields found: '" + line.trimmed() + "'");
		}

		//check that start/end is number
		bool ok = true;
		int start = fields[1].toInt(&ok) + 1;
		if (!ok) THROW(FileParseException, "BED file line with invalid starts position found: '" + line.trimmed() + "'");
		int end = fields[2].toInt(&ok);
		if (!ok) THROW(FileParseException, "BED file line with invalid end position found: '" + line.trimmed() + "'");
		output.append(BedLine(fields[0], start, end, fields.mid(3)));
	}

	return output;
}

bool BedFile::LessComparatorWithName::operator()(const BedLine& a, const BedLine& b) const
{
	if (a<b) return true;

	if (b<a) return false;

	QByteArray a_name = a.annotations().isEmpty() ? QByteArray() : a.annotations()[0];
	QByteArray b_name = b.annotations().isEmpty() ? QByteArray() : b.annotations()[0];
	return a_name<b_name;
}
