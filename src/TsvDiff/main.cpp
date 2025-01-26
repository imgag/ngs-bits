#include "ToolBase.h"
#include "TsvFile.h"
#include "Exceptions.h"
#include "Helper.h"
#include "BasicStatistics.h"
#include <QBitArray>

//TODO Marc: optimize time and space further by only using two QBitArrays instead of n+1

template<typename T>
QString stringRepresentation(const T& element)
{
	return element;
}
template<>
QString stringRepresentation(const QStringList& element)
{
	return element.join("\t");
}

struct CompSettings
{
	QHash<int, double> diff_abs;
};

template<typename T>
bool is_equal(const T& a, const T& b, const CompSettings& /*comp_settings*/)
{
	return a==b;
}
template<>
bool is_equal(const QStringList& a, const QStringList& b, const CompSettings& comp_settings)
{
	for(int i=0; i<a.count(); ++i)
	{
		if (a[i]==b[i]) continue;

		if (comp_settings.diff_abs.contains(i))
		{
			bool ok1 = false;
			bool ok2 = false;
			double diff = abs( a[i].toDouble(&ok1)-b[i].toDouble(&ok2));
			if (!ok1 || !ok2 || diff>comp_settings.diff_abs[i]) return false;
		}
		else return false;
	}
	return true;
}

enum class Direction
{
	DIAGONAL,
	LEFT,
	TOP,
	LEFT_OR_TOP
};

class DirectionVector
{
public:
	DirectionVector()
	{
	}
	DirectionVector(int size)
		: left_(size)
		, top_(size)
	{
	}

	void set(int i, Direction d)
	{
		if (d==Direction::TOP)
		{
			top_.setBit(i, true);
		}
		if (d==Direction::LEFT)
		{
			left_.setBit(i, true);
		}
		if (d==Direction::LEFT_OR_TOP)
		{
			left_.setBit(i, true);
			top_.setBit(i, true);
		}
	}

	Direction get(int i) const
	{
		if (top_.at(i)) return Direction::TOP;
		if (left_.at(i)) return Direction::LEFT;
		if (top_.at(i) && left_.at(i)) return Direction::LEFT_OR_TOP;
		return Direction::DIAGONAL;
	}

	int count() const
	{
		return left_.count();
	}

protected:
	QBitArray left_;
	QBitArray top_;
};

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
		setDescription("Compares TSV files.");
		addInfile("in1", "First input TSV file.", false);
		addInfile("in2", "Second input TSV file.", false);

		//optional
		addOutfile("out", "Output file with differences. If unset, writes to stdout.", true);
		addFlag("skip_comments", "Do not compare comment lines starting with '##'.");
		addString("skip_comments_matching", "Comma-separated list of sub-strings for skipping comment lines (case-sensitive matching).", true);
		addString("skip_cols", "Comma-separated list of colums to skip during line comparison.", true);
		addString("comp", "Comma-separated list of columns to use for comparison (all other columns are ignored).", true);
		addString("diff_abs", "Comma-separated list of column=difference tuples for defining maximum allowed numeric difference of columns.", true);
		addFlag("no_error", "Do not exit with error state if differences are detected.");
		addInt("debug", "Debug level (0=none, 1=basic, 2=extended", true, 0);
	}

	struct DiffSummary
	{
		int added = 0;
		int removed = 0;

		bool hasDifferences() const
		{
			return added+removed>0;
		}
	};

	void removeComments(TsvFile& file, QStringList strings)
	{
		if (strings.isEmpty()) return;

		QStringList tmp = file.comments();
		foreach(const QString& str, strings)
		{
			auto it = std::remove_if(tmp.begin(), tmp.end(), [str](const QString& line){ return line.contains(str);});
			tmp.erase(it, tmp.end());
		}
		file.setComments(tmp);

	}

	//matrix with directions (storing only the possible direction for trace-back instead of the score saves a lot of space - 2 bits instead of 4 bytes)
	class Matrix
	{
	public:
		Matrix(int n, int m)
			: n_(n)
			, m_(m)
		{
			data_.reserve(n+1);
			for(int i=0; i<=n; ++i)
			{
				data_.append(DirectionVector(m+1));
			}
		}

		int n() const { return n_; };
		int m() const { return m_; };

		Direction value(int n, int m) const
		{
			if (n>n_) THROW(Exception, "Matrix:value() Invalid matrix position 'n': Is " + QString::number(n) + " but max is " + QString::number(n_));
			if (m>m_) THROW(Exception, "Matrix:value() Invalid matrix position 'm': Is " + QString::number(m) + " but max is " + QString::number(m_));

			return data_[n].get(m);
		}

		void setValue(int n, int m, Direction d)
		{
			if (n>n_) THROW(Exception, "Matrix:value() Invalid matrix position 'n': Is " + QString::number(n) + " but max is " + QString::number(n_));
			if (m>m_) THROW(Exception, "Matrix:value() Invalid matrix position 'm': Is " + QString::number(m) + " but max is " + QString::number(m_));

			data_[n].set(m, d);
		}

		void print(QTextStream& ostream) const
		{
			ostream << "Matrix: " << endl;
			foreach(const DirectionVector& element, data_)
			{
				for (int i=0; i<element.count(); ++i)
				{
					if (i>0) ostream << " ";
					Direction d = element.get(i);
					if (d==Direction::DIAGONAL) ostream << 'd';
					if (d==Direction::TOP) ostream << 't';
					if (d==Direction::LEFT) ostream << 'l';
					if (d==Direction::LEFT_OR_TOP) ostream << 'x';
				}
				ostream << endl;
			}
			ostream << endl;
		}

		QList<QPair<int, int>> findMatchIndices()
		{
			QList<QPair<int, int>> output;

			int i = n_;
			for (int j=m_; j>0 && i>0; --j)
			{
				Direction d = value(i,j);
				if (d==Direction::LEFT || d==Direction::LEFT_OR_TOP) continue;
				if (d==Direction::TOP)
				{
					--i;
					++j;
					continue;
				}

				output.prepend(qMakePair(i-1, j-1));
				--i;
			}

			return output;
		}

	private:
		QVector<DirectionVector> data_;
		int n_;
		int m_;
	};

	//fill direction matrix (optimization: we have only two lines of the score matrix in memory)
	template<typename T>
	void fillMatrix(const T& s1, const T& s2, Matrix& matrix, const CompSettings& comp_settings)
	{
		QVector<int> before = QVector<int>(matrix.m()+1, 0);
		QVector<int> current = QVector<int>(matrix.m()+1, 0);
		for (int i = 1; i <= matrix.n(); ++i)
		{
			if (i%10000==0) QTextStream(stderr) << QDateTime::currentDateTime().toString(Qt::ISODateWithMs) << " " << i << endl;
			for (int j = 1; j <= matrix.m(); ++j)
			{

				if (is_equal(s1[i-1], s2[j-1], comp_settings))
				{
					current[j] = before[j-1] + 1;
					matrix.setValue(i, j, Direction::DIAGONAL);
				}
				else
				{
					int left = current[j-1];
					int top = before[j];
					if (left==top)
					{
						current[j] = left;
						matrix.setValue(i, j, Direction::LEFT_OR_TOP);
					}
					else if (left>top)
					{
						current[j] = left;
						matrix.setValue(i, j, Direction::LEFT);
					}
					else
					{
						current[j] = top;
						matrix.setValue(i, j, Direction::TOP);
					}
				}
			}
			before.swap(current);
		}
	}

	template<typename T>
	void compare(const T& lines1, const T& lines2, QTextStream& ostream, DiffSummary& summary, const CompSettings& comp_settings, int debug)
	{
		QTextStream estream(stderr);
		int n = lines1.count();
		int m = lines2.count();
		if (n==0 && m==0) return;

		//special case that one one has length zero
		if (n>0 && m==0)
		{
			for (int i=0; i<n; ++i)
			{
				ostream << "-" << stringRepresentation(lines1[i]) << endl;
			}
			return;
		}
		if (n==0 && m>0)
		{
			for (int i=0; i<m; ++i)
			{
				ostream << "+" << stringRepresentation(lines2[i]) << endl;
			}
			return;
		}

		//determine LCS
		if (debug) estream << QDateTime::currentDateTime().toString(Qt::ISODateWithMs) << " building matrix..." << endl;
		Matrix matrix(n, m);
		if (debug) estream << QDateTime::currentDateTime().toString(Qt::ISODateWithMs) << " filling matrix..." << endl;
		fillMatrix(lines1, lines2, matrix, comp_settings);
		if (debug==2)
		{
			matrix.print(estream);
		}
		if (debug) estream << QDateTime::currentDateTime().toString(Qt::ISODateWithMs) << " determining matching pairs..." << endl;
		QList<QPair<int, int>> matches = matrix.findMatchIndices();
		if (debug==2)
		{
			estream << "Line index matches:" << endl;
			foreach(auto m, matches) estream << m.first << "/" << m.second << endl;
		}
		if (debug) estream << QDateTime::currentDateTime().toString(Qt::ISODateWithMs) << " printing output..." << endl;
		
		//special handling when there are no matches
		if (matches.isEmpty())
		{
			for (int i=0; i<n; ++i)
			{
				ostream << "-" << stringRepresentation(lines1[i]) << endl;
				summary.removed += 1;
			}
			for (int i=0; i<m; ++i)
			{
				ostream << "+" << stringRepresentation(lines2[i]) << endl;
				summary.added += 1;
			}
			return;
		}
		
		//before first match
		for (int i=0; i<matches[0].first; ++i)
		{
			ostream << "-" << stringRepresentation(lines1[i]) << endl;
			summary.removed += 1;
		}
		for (int i=0; i<matches[0].second; ++i)
		{
			ostream << "+" << stringRepresentation(lines2[i]) << endl;
			summary.added += 1;
		}

		//between matches
		for (int m=1; m<matches.count(); ++m)
		{
			for (int i=matches[m-1].first+1; i<matches[m].first; ++i)
			{
				ostream << "-" << stringRepresentation(lines1[i]) << endl;
				summary.removed += 1;
			}
			for (int i=matches[m-1].second+1; i<matches[m].second; ++i)
			{
				ostream << "+" << stringRepresentation(lines2[i]) << endl;
				summary.added += 1;
			}
		}

		//after matches
		for (int i=matches.last().first+1; i<n; ++i)
		{
			ostream << "-" << stringRepresentation(lines1[i]) << endl;
			summary.removed += 1;
		}
		for (int i=matches.last().second+1; i<m; ++i)
		{
			ostream << "+" << stringRepresentation(lines2[i]) << endl;
			summary.added += 1;
		}
	}

	virtual void main()
	{
		//init
		bool skip_comments = getFlag("skip_comments");
		QStringList skip_comments_matching = getString("skip_comments_matching").split(",");
		skip_comments_matching.removeAll("");
		QSet<QString> skip_cols = getString("skip_cols").split(",").toSet();
		skip_cols.remove("");
		QSharedPointer<QFile> out = Helper::openFileForWriting(getOutfile("out"), true);
		QTextStream ostream(out.data());
		bool no_error = getFlag("no_error");
		int debug = BasicStatistics::bound(getInt("debug"), 0, 2);
		DiffSummary summary_comments;
		DiffSummary summary_content;
		QTextStream estream(stderr);
		QTime timer;
		timer.start();

		//load files
		if (debug) estream << QDateTime::currentDateTime().toString(Qt::ISODateWithMs) << " Loading file 1..." << endl;
		TsvFile in1;
		in1.load(getInfile("in1"), true);
		if (debug) estream << QDateTime::currentDateTime().toString(Qt::ISODateWithMs) << " Loading file 2..." << endl;
		TsvFile in2;
		in2.load(getInfile("in2"), true);

		//determine columns used for comperison
		QSet<QString> comp_cols = getString("comp").split(",").toSet();
		comp_cols.remove("");
		if (comp_cols.isEmpty()) // "comp" not set => use all columns
		{
			comp_cols = in1.headers().toSet() + in2.headers().toSet();
		}
		foreach(QString col, skip_cols)
		{
			comp_cols.remove(col);
		}
		if (!comp_cols.isEmpty())
		{
			if (debug) estream << QDateTime::currentDateTime().toString(Qt::ISODateWithMs) << " Removing unused columns..." << endl;
			for(int i=in1.columnCount()-1; i>=0; --i)
			{
				if (!comp_cols.contains(in1.headers()[i]))
				{
					in1.removeColumn(i);
				}
			}
			for(int i=in2.columnCount()-1; i>=0; --i)
			{
				if (!comp_cols.contains(in2.headers()[i]))
				{
					in2.removeColumn(i);
				}
			}
		}

		//compare headers
		if  (in1.headers()!=in2.headers()) THROW(Exception, "Cannot compare files with differing columns!\nin1: "+in1.headers().join(", ")+"\nin2: "+in2.headers().join(", ")+"");

		//parse numeric difference
		CompSettings comp_settings;
		QStringList diff_abs = getString("diff_abs").split(",");
		diff_abs.removeAll("");
		foreach(QString entry, diff_abs)
		{
			QStringList parts = entry.split('=');
			if (parts.count()!=2 || !Helper::isNumeric(parts[1])) THROW(Exception, "Absolute column difference entry '" + entry + "' not valid!");
			comp_settings.diff_abs[in1.columnIndex(parts[0])] = parts[1].toDouble();
		}

		//compare comments
		if (!skip_comments)
		{
			removeComments(in1, skip_comments_matching);
			removeComments(in2, skip_comments_matching);

			if (debug) estream << QDateTime::currentDateTime().toString(Qt::ISODateWithMs) << " Comparing comment lines..." << endl;
			compare(in1.comments(), in2.comments(), ostream, summary_comments, comp_settings, debug);
		}

		//compare content lines
		if (debug) estream << QDateTime::currentDateTime().toString(Qt::ISODateWithMs) << " Comparing content lines..." << endl;
		compare(in1, in2, ostream, summary_content, comp_settings, debug);

		//output
		bool has_differences = summary_comments.hasDifferences() || summary_content.hasDifferences();
		if (has_differences)
		{
			ostream << "Difference summary:" << endl;
			if (summary_comments.added) ostream << "comment lines added: "  << summary_comments.added << endl;
			if (summary_comments.removed) ostream << "comment lines removed: "  << summary_comments.removed << endl;
			if (summary_content.added) ostream << "content lines added: "  << summary_content.added << endl;
			if (summary_content.removed) ostream << "content lines removed: "  << summary_content.removed << endl;
		}

		if (debug) estream << QDateTime::currentDateTime().toString(Qt::ISODateWithMs) << " Overall runtime: " << Helper::elapsedTime(timer) << endl;

		//set exit code
		if (has_differences && !no_error)
		{
			setExitErrorState(true);
		}
    }
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

