#include "ChromosomalFileIndex.h"
#include "Exceptions.h"
#include "Helper.h"
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QFileInfo>

ChromosomalFileIndex::ChromosomalFileIndex()
{
}

void ChromosomalFileIndex::create(const QString& db_file_name, int chr_col, int start_col, int end_col, char comment, int bin_size)
{
	//clear
	db_file_name_ = db_file_name;
	meta_.clear();
	index_.clear();
	max_length_ = 0;

	Chromosome last_chr;
	int last_start_pos = -1;
	long last_file_pos = 0;
	int line_count = 0;

	//parse from file
	long stream_pos = 0;
	QScopedPointer<QFile> file(Helper::openFileForReading(db_file_name_));
	while(!file->atEnd())
	{
		QByteArray line = file->readLine();
		stream_pos += line.length();
		while (line.endsWith('\n') || line.endsWith('\r')) line.chop(1);

		//skip empty and comment lines
		if (line.length()==0 || line[0]==comment) continue;

		//check line format
		QList<QByteArray> parts = line.split('\t');
		if (parts.size()<3)
		{
			THROW(FileParseException, "Malformed data line '" + line + "' in file '" + db_file_name_ + "'");
		}

		//check chomosome
		Chromosome chr(parts[chr_col]);
		if (chr=="")
		{
			THROW(FileParseException, "Database '" + db_file_name_ + "' contains invalid chromosome '" + chr.str() + "'!");
		}
		if (chr!=last_chr)
		{
			if (index_.contains(chr.strNormalized(false)))
			{
				THROW(FileParseException, "Database '" + db_file_name_ + "' not sorted according to chromosome column. Chromosome '" + chr.str() + "' found several times!");
			}
			last_start_pos = -1;
		}

		//check positions
		bool ok = true;
		int start = parts[start_col].toInt(&ok);
		if (!ok) THROW(FileParseException, "Could not convert database file start position to integer: '" + parts[start_col] + "'!");
		if (last_start_pos>start)
		{
			THROW(FileParseException, "Database '" + db_file_name_ + "' not sorted according to start position. Position '" + QString::number(start) + "' after position '" + QString::number(last_start_pos) + "' on chromosome '" + chr.str() + "'!");
		}
		int end = parts[end_col].toInt(&ok);
		if (!ok) THROW(FileParseException, "Could not convert database file start position to integer: '" + parts[end_col] + "'!");
		if (start>end)
		{
			THROW(FileParseException, "Database '" + db_file_name_ + "' contains invalid chromosomal range. Start position '" + QString::number(start) + "' greater than end position'" + QString::number(end) + "' on chromosome '" + chr.str() + "'!");
		}

		//chromosome changes
		if (chr!=last_chr)
		{
			//finish last chromosome
			if (last_chr.isValid())
			{
				index_[last_chr.strNormalized(false)].append(QPair<int, long>(999999999, stream_pos));
			}

			//init new chromosome
			line_count = 1;
			index_.insert(chr.strNormalized(false), QVector< QPair<int, long> >());
			index_[chr.strNormalized(false)].append(QPair<int, long>(0, last_file_pos));
		}

		//bin size reached => make entry in index
		if (line_count%bin_size==0)
		{
			index_[chr.strNormalized(false)].append(QPair<int, long>(start, last_file_pos));
		}

		last_start_pos = start;
		last_file_pos = stream_pos;
		last_chr = chr;
		++line_count;
		max_length_ = std::max(max_length_, end-start+1);
	}

	//finish last chromosome
	index_[last_chr.strNormalized(false)].append(QPair<int, long>(999999999, stream_pos));

	//set meta data
	QFileInfo file_info(db_file_name_);
	meta_.insert("db_size", QString::number(file_info.size()));
	meta_.insert("db_last_mod", file_info.lastModified().toString(Qt::ISODate));
	meta_.insert("chr_index", QString::number(chr_col));
	meta_.insert("start_index", QString::number(start_col));
	meta_.insert("end_index", QString::number(end_col));
	meta_.insert("comment_char", QChar(comment));
	meta_.insert("bin_size", QString::number(bin_size));
	meta_.insert("max_length", QString::number(max_length_));
	meta_.insert("version", version());

	//store
	QScopedPointer<QFile> out_file(Helper::openFileForWriting(db_file_name_ + ".cidx"));
	QTextStream out_stream(out_file.data());
	QMap<QString, QString>::Iterator it = meta_.begin();
	while (it != meta_.end())
	{
		out_stream << "#META\t" << it.key() << "\t" << it.value() << "\n";
		++it;
	}

	out_stream << "#chr\tchr_pos\tfile_pos\n";
	QHash<QString, QVector<QPair<int, long> > >::Iterator it2 = index_.begin();
	while (it2 != index_.end())
	{
		const QVector<QPair<int, long> >& positions = it2.value();
		for (int i=0; i<positions.count(); ++i)
		{
			out_stream << it2.key() << "\t" << positions[i].first  << "\t" << positions[i].second << "\n";
		}
		++it2;
	}
}


void ChromosomalFileIndex::load(const QString& db_file_name)
{
	//clear
	db_file_name_ = db_file_name;
	meta_.clear();
	index_.clear();
	max_length_ = 0;

	//parse from stream
	QScopedPointer<QFile> file(Helper::openFileForReading(db_file_name_ + ".cidx"));
	while(!file->atEnd())
	{
		QByteArray line = file->readLine();
		while (line.endsWith('\n') || line.endsWith('\r')) line.chop(1);

		//skip empty lines
		if(line.length()==0) continue;

		//check line format
		QList<QByteArray> parts = line.split('\t');
		if (parts.size()!=3)
		{
			THROW(FileParseException, "Malformed meta data line '" + line + "' in index file '" + db_file_name_ + ".cidx'");
		}

		//comment
		if (line[0]=='#')
		{
			//store meta data
			if (parts[0]=="#META")
			{
				meta_.insert(parts[1], parts[2]);

				if (parts[1]=="max_length")
				{
					max_length_ = parts[2].toInt();
				}
			}
		}
		else
		{
			//create chromosome when missing
			Chromosome chr(parts[0]);
			if (!index_.contains(chr.strNormalized(false)))
			{
				index_.insert(chr.strNormalized(false), QVector< QPair<int, long> >());
			}

			//insert position
			bool ok = true;
			int cpos = parts[1].toInt(&ok);
			if (!ok)
			{
				THROW(FileParseException, "Could not convert start position to integer in line '" + line + "'.");
			}
			long fpos = parts[2].toLong(&ok);
			if (!ok)
			{
				THROW(FileParseException, "Could not convert end position to long integer in line '" + line + "'.");
			}
			index_[chr.strNormalized(false)].append(QPair<int, long>(cpos, fpos));
		}
	}
}

QPair<long, long> ChromosomalFileIndex::filePosition(const Chromosome& chr, int start, int end)
{
	//error checks
	if (index_.count()==0)
	{
		THROW(ArgumentException, "Cannot search in empty chromosomal index!");
	}
	if (start>end)
	{
		THROW(ArgumentException,"Invalid position range in chromosomal index search - start '" + QString::number(start) + "' is greater than end '" + QString::number(end) + "'!");
	}

	//tweak input
	start = std::max(0, start-max_length_);

	//chromosome not found
	if (!index_.contains(chr.strNormalized(false)))
	{
		return QPair<long, long>(0,0);
	}
	//qDebug() << "POS: " << chr.strNormalized(false) << " " << start << " " << end;
	
	//find start position
	const QVector<QPair<int, long> >& pos_data = index_[chr.strNormalized(false)];
	QVector<QPair<int,long> >::const_iterator it = std::lower_bound(pos_data.begin(), pos_data.end(), QPair<int, long>(start, -1));
	//qDebug() << "FIRST1: " << it->first << " " << it->second << end;
	if (it!=pos_data.begin()) --it;
	//qDebug() << "FIRST2: " << it->first << " " << it->second << end;
	long start_pos = it->second;

	//find end position
	while(it->first<=end)
	{
		++it;
	}

	//qDebug() << "LAST: " << it->first << " " << it->second << end;
	return QPair<long, long>(start_pos, it->second);
}

QStringList ChromosomalFileIndex::lines(const Chromosome& chr, int start, int end)
{
	//get required meta data
	char comment = meta_["comment_char"][0].toLatin1();
	int col_chr = meta_["chr_index"].toInt();
	int col_start = meta_["start_index"].toInt();
	int col_end = meta_["end_index"].toInt();

	//init output
	QStringList output;

	//find matching lines
	QPair<long, long> file_range = filePosition(chr, start, end);
	//qDebug() << "RANGE: " << file_range.first << " " << file_range.second;
	QScopedPointer<QFile> file(Helper::openFileForReading(db_file_name_));
	file->seek(file_range.first);
	long stream_pos = file_range.first;
	while(!file->atEnd())
	{
		QByteArray line = file->readLine();
		stream_pos += line.length();
		while (line.endsWith('\n') || line.endsWith('\r')) line.chop(1);
		//qDebug() << "LINE: " << line;
		//qDebug() << "POS : " << stream_pos;

		if (line.count()==0 || line[0]==comment) continue;

		QList<QByteArray> parts = line.split('\t');
		if (chr==Chromosome(parts[col_chr]) && overlap(parts[col_start].toInt(), parts[col_end].toInt(), start, end))
		{
			output.append(line);
		}

		//update position (we need to track the current position because stream.pos is too slow)
		if (stream_pos>=file_range.second) break;
	}

	return output;
}

bool ChromosomalFileIndex::isUpToDate(QString db_file_name)
{
	//check that index exists
	if (!QFile::exists(db_file_name + ".cidx")) return false;

	//parse index file
	bool version_found = false;
	QScopedPointer<QFile> file(Helper::openFileForReading(db_file_name + ".cidx"));
	while(!file->atEnd())
	{
		QByteArray line = file->readLine().trimmed();

		//stop when we hit the first content line
		if (!line.startsWith('#')) break;

		//parse meta data
		if (line.startsWith("#META\t"))
		{
			QList<QByteArray> parts = line.split('\t');
			if (parts.size()!=3) return false;

			QString key = parts[1];
			QString value = parts[2];

			//size
			if (key=="db_size" && value!=QString::number(QFileInfo(db_file_name).size()))
			{
				return false;
			}

			//modification date
			if (key=="db_last_mod" && value!=QFileInfo(db_file_name).lastModified().toString(Qt::ISODate))
			{
				return false;
			}

			//maximum length
			if (key=="version")
			{
				version_found = true;
				if (value!=version()) return false;
			}
		}
	}

	if (!version_found) // we need to check it because it was not contained in the first cidx version
	{
		return false;
	}

	return true;
}
