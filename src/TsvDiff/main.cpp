#include "ToolBase.h"
#include "TsvFile.h"
#include "Exceptions.h"
#include "Helper.h"


template<typename T>
QString stringRepresentation(const T& /*element*/)
{
	return "[not implemented]";
}
template<>
QString stringRepresentation(const QString& element)
{
	return element;
}
template<>
QString stringRepresentation(const QStringList& element)
{
	return element.join("\t");
}

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
		addString("skip_cols", "Comma-separated list of colums to skip.", true);
		addFlag("no_error", "Do not exit with error state if differences are detected");
		addFlag("debug", "Print debug output to stderr");
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

	//matrix for dynamic programming
	class Matrix
	{
	public:
		Matrix(int n, int m)
			: n_(n)
			, m_(m)
		{
			QVector<long> tmp(m+1, -1);
			tmp[0] = 0;

			data_.reserve(n+1);
			for(int i=0; i<=n; ++i)
			{
				if (i==0)
				{
					data_.append(QVector<long>(m+1, 0));
				}
				else
				{
					data_.append(tmp);
				}
			}
		}

		int n() const { return n_; };
		int m() const { return m_; };

		long value(int n, int m) const
		{
			if (n>n_) THROW(Exception, "Matrix:value() Invalid matrix position 'n': Is " + QString::number(n) + " but max is " + QString::number(n_));
			if (m>m_) THROW(Exception, "Matrix:value() Invalid matrix position 'm': Is " + QString::number(m) + " but max is " + QString::number(m_));

			return data_[n][m];
		}

		void setValue(int n, int m, long v)
		{
			if (n>n_) THROW(Exception, "Matrix:value() Invalid matrix position 'n': Is " + QString::number(n) + " but max is " + QString::number(n_));
			if (m>m_) THROW(Exception, "Matrix:value() Invalid matrix position 'm': Is " + QString::number(m) + " but max is " + QString::number(m_));

			data_[n][m] = v;
		}

		void print(QTextStream& ostream) const
		{
			ostream << "Matrix: " << endl;
			foreach(const QVector<long>& element, data_)
			{
				for (int i=0; i<element.count(); ++i)
				{
					if (i>0) ostream << " ";
					QString value = QString::number(element[i]);
					value = value.rightJustified(4, ' ');
					ostream << value;
				}
				ostream << endl;
			}
			ostream << endl;
		}

		QList<QPair<int, int>> findMatchIndices()
		{
			QList<QPair<int, int>> output;

			int i =n_;
			for (int j=m_; j>0; --j)
			{
				if (value(i,j)==value(i,j-1)) continue;
				if (value(i,j)==value(i-1,j))
				{
					--i;
					++j;
					continue;
				}

				output << qMakePair(i-1, j-1);
				--i;
			}

			std::reverse(output.begin(), output.end());

			return output;
		}

	private:
		QVector<QVector<long>> data_;
		int n_;
		int m_;
	};

	//build matrix for dynamic programming
	template<typename T>
	long buildMatrix(const T& s1, const T& s2, int i, int j, Matrix& m)
	{
		//already calculated > return value
		long v = m.value(i,j);
		if (v!=-1) return v;

		if (s1[i-1]==s2[j-1])
		{
			v = 1 + buildMatrix(s1, s2, i-1, j-1, m);
		}
		else
		{
			v = std::max(buildMatrix(s1, s2, i-1, j, m), buildMatrix(s1, s2, i, j-1, m));
		}

		m.setValue(i, j, v);
		return v;
	}

	template<typename T>
	void compare(const T& lines1, const T& lines2, QTextStream& ostream, DiffSummary& summary, bool debug)
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
		Matrix matrix(n, m);
		buildMatrix(lines1, lines2, n, m, matrix);
		if (debug)
		{
			matrix.print(estream);
		}
		QList<QPair<int, int>> matches = matrix.findMatchIndices();
		if (debug)
		{
			estream << "Line index matches:" << endl;
			foreach(auto m, matches) estream << m.first << "/" << m.second << endl;
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
		QStringList skip_cols = getString("skip_cols").split(",");
		skip_cols.removeAll("");
		DiffSummary summary_comments;
		DiffSummary summary_content;
		QSharedPointer<QFile> out = Helper::openFileForWriting(getOutfile("out"), true);
		QTextStream ostream(out.data());
		bool no_error = getFlag("no_error");
		bool debug = getFlag("debug");
		QTextStream estream(stderr);

		//load files
		if (debug) estream << "Loading file 1.." << endl;
		TsvFile in1;
		in1.load(getInfile("in1"));
		if (debug) estream << "Loading file 2.." << endl;
		TsvFile in2;
		in2.load(getInfile("in2"));

		//compare comments
		if (!skip_comments)
		{
			removeComments(in1, skip_comments_matching);
			removeComments(in2, skip_comments_matching);

			if (debug) estream << "Comparing comment lines.." << endl;
			compare(in1.comments(), in2.comments(), ostream, summary_comments, debug);
		}

		//remove skipped columns
		if (!skip_cols.isEmpty())
		{
			if (debug) estream << "Removing skipped columns.." << endl;
			foreach(const QString& col, skip_cols)
			{
				int idx = in1.columnIndex(col, false);
				if (idx!=-1) in1.removeColumn(idx);

				idx = in2.columnIndex(col, false);
				if (idx!=-1) in2.removeColumn(idx);
			}
		}

		//compare headers
		if(in1.headers()!=in2.headers())
		{
			THROW(Exception, "Cannot compare files with differing column headers:\nin1:"+in1.headers().join("\t")+"\nin2:"+in2.headers().join("\t"));
		}

		//compare content lines
		if (debug) estream << "Comparing content lines.." << endl;
		compare(in1, in2, ostream, summary_content, debug);

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

