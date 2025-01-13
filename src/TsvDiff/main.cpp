#include "ToolBase.h"
#include "TsvFile.h"
#include "Exceptions.h"
#include "Helper.h"

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

struct IndexPair
{
	QString col;
	int i1;
	int i2;
};

struct CompSettings
{
	QList<IndexPair> comp_indices;
	QHash<QString, double> diff_abs;
};

template<typename T>
bool is_equal(const T& a, const T& b, const CompSettings& /*comp_settings*/)
{
	return a==b;
}
template<>
bool is_equal(const QStringList& a, const QStringList& b, const CompSettings& comp_settings)
{
	foreach(const IndexPair& indices, comp_settings.comp_indices)
	{
		const QString& str1 = a[indices.i1];
		const QString& str2 = b[indices.i2];
		if (str1!=str2)
		{
			bool num_equal = false;
			if (Helper::isNumeric(str1) && Helper::isNumeric(str2))
			{
				double diff = abs(str1.toDouble()-str2.toDouble());
				if (comp_settings.diff_abs.contains(indices.col))
				{
					double max_diff  = comp_settings.diff_abs[indices.col];
					if (diff<=max_diff) num_equal = true;
				}
			}
			if (!num_equal) return false;
		}
	}
	return true;
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
		addString("skip_cols", "Comma-separated list of colums to skip during line comparison.", true);
		addString("comp", "Comma-separated list of columns to use for comparison (all other columns are ignored).", true);
		addString("diff_abs", "Comma-separated list of column=difference tuples for defining maximum allowed numeric difference of columns.", true);
		addFlag("no_error", "Do not exit with error state if differences are detected.");
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

	//TODO Marc: use non-recursive function to allow more than 40000 lines
	//build matrix for dynamic programming
	template<typename T>
	long buildMatrix(const T& s1, const T& s2, int i, int j, Matrix& m, const CompSettings& comp_settings)
	{
		//already calculated > return value
		long v = m.value(i,j);
		if (v!=-1) return v;

		if (is_equal(s1[i-1], s2[j-1], comp_settings))
		{
			v = 1 + buildMatrix(s1, s2, i-1, j-1, m, comp_settings);
		}
		else
		{
			v = std::max(buildMatrix(s1, s2, i-1, j, m, comp_settings), buildMatrix(s1, s2, i, j-1, m, comp_settings));
		}

		m.setValue(i, j, v);
		return v;
	}

	template<typename T>
	void compare(const T& lines1, const T& lines2, QTextStream& ostream, DiffSummary& summary, const CompSettings& comp_settings, bool debug)
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
		buildMatrix(lines1, lines2, n, m, matrix, comp_settings);
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
		bool debug = getFlag("debug");
		DiffSummary summary_comments;
		DiffSummary summary_content;
		QTextStream estream(stderr);

		//load files
		if (debug) estream << "Loading file 1.." << endl;
		TsvFile in1;
		in1.load(getInfile("in1"));
		if (debug) estream << "Loading file 2.." << endl;
		TsvFile in2;
		in2.load(getInfile("in2"));

		//deterine columns used for comparison
		CompSettings comp_settings;
		QStringList comp_cols = getString("comp").split(",");
		comp_cols.removeAll("");
		if (comp_cols.isEmpty()) //no comparison columns given > use all
		{
			foreach(const QString& col, in1.headers())
			{
				if (skip_cols.contains(col)) continue;
				comp_cols << col;
			}
			foreach(const QString& col, in2.headers())
			{
				if (skip_cols.contains(col)) continue;
				if (!comp_cols.contains(col)) comp_cols << col;
			}
		}
		foreach(QString col, comp_cols)
		{
			int i1 = in1.columnIndex(col, false);
			if (i1==-1) THROW(Exception, "Could not find column '" + col + "' in 'in1'!");
			int i2 = in2.columnIndex(col, false);
			if (i2==-1) THROW(Exception, "Could not find column '" + col + "' in 'in2'!");
			comp_settings.comp_indices << IndexPair{col, i1, i2};
		}

		//parse numeric difference
		QStringList diff_abs = getString("diff_abs").split(",");
		diff_abs.removeAll("");
		foreach(QString entry, diff_abs)
		{
			QStringList parts = entry.split('=');
			if (parts.count()!=2 || !Helper::isNumeric(parts[1])) THROW(Exception, "Absolute column difference entry '" + entry + "' not valid!");
			comp_settings.diff_abs[parts[0]] = parts[1].toDouble();
		}

		//compare comments
		if (!skip_comments)
		{
			removeComments(in1, skip_comments_matching);
			removeComments(in2, skip_comments_matching);

			if (debug) estream << "Comparing comment lines.." << endl;
			compare(in1.comments(), in2.comments(), ostream, summary_comments, comp_settings, debug);
		}

		//compare content lines
		if (debug) estream << "Comparing content lines.." << endl;
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

