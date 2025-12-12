#include "ToolBase.h"
#include "TSVFileStream.h"
#include "Helper.h"
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
		setDescription("Converts TSV file to different table formats.");
		setExtendedDescription(QStringList() << "Comment lines are not written to the output.");

		addEnum("format", "Output format.", false, QStringList() << "txt" << "md" << "html");

		addInfile("in", "Input TSV file. If unset, reads from STDIN.", true);
		addOutfile("out", "Output file. If unset, writes to STDOUT.", true);

		changeLog(2025, 12, 11, "Initial inplementation.");
	}


	//Returns text of element escaped based on format
	QByteArray text(QByteArrayList parts, int i, bool pad = false)
	{
		QByteArray v = (i<parts.count()) ? parts[i] : "";
		if (format_=="html")
		{
			v = QString(v).toHtmlEscaped().toUtf8();
		}
		else if (format_=="md")
		{
			v.replace('|', "\\|");
		}

		if (pad)
		{
			v = v.leftJustified(col_size_[i]);
		}

		return v;
	}

	//Writes text line with indentation
	void write(QByteArray text, bool newline = false)
	{
		if (indent_>0) out_->write(QByteArray().fill(' ', indent_));
		out_->write(text);
		if (newline) out_->write("\n");
	}

	virtual void main()
	{
		//init
		TSVFileStream in(getInfile("in"));
		QByteArrayList headers = in.header();
		out_ = Helper::openFileForWriting(getOutfile("out"), true);
		format_ = getEnum("format").toUtf8();
		indent_ = 0;

		//determine column sizes
		if (format_=="txt" || format_=="md")
		{
			for(int i=0; i<in.columns(); ++i)
			{
				col_size_[i] = text(headers, i).size();
			}
			while(!in.atEnd())
			{
				QByteArrayList parts = in.readLine();
				for (int i=0; i<in.columns(); ++i)
				{
					col_size_[i] = std::max(col_size_[i], text(parts, i).size());
				}
			}
			in.reset();
		}

		//before table
		if (format_=="html")
		{
			write("<html>", true);
			indent_ += 2;
			write("<head>", true);
			indent_ += 2;
			write("<style>", true);
			indent_ += 2;
			write("table { border-collapse: collapse; width: auto; border: 1px solid #444; }", true);
			write("table td { border: 1px solid #444; padding: 2px; }", true);
			write("table th { border: 1px solid #444; text-align: left; padding: 2px; background: #ccc; font-weight: 600; }", true);
			write("table tr:nth-child(even) td { background: #f3f3f3; }", true);
			write("table tr:hover td { background: #d0d7df; }", true);
			indent_ -= 2;
			write("</style>", true);
			indent_ -= 2;
			write("</head>", true);
			write("<body>", true);
			indent_ += 2;
			write("<table>", true);
			indent_ += 2;
		}

		//header line
		if (!headers.isEmpty())
		{
			if (format_=="html")
			{
				write("<tr>", true);
				indent_ += 2;
				for(int i=0; i<in.columns(); ++i)
				{
					write("<th>"+text(headers, i)+"</th>", true);
				}
				indent_ -= 2;
				write("</tr>", true);
			}
			else if (format_=="txt")
			{
				for(int i=0; i<in.columns(); ++i)
				{
					if (i!=0) write(" ");
					write(text(headers, i, true));
				}
				write("\n");
				for(int i=0; i<in.columns(); ++i)
				{
					if (i!=0) write(" ");
					write(QByteArray().fill('-', col_size_[i]));
				}
				write("\n");
			}
			else if (format_=="md")
			{
				for(int i=0; i<in.columns(); ++i)
				{
					write("|");
					write(text(headers, i, true));
				}
				write("|\n");
				for(int i=0; i<in.columns(); ++i)
				{
					write("|");
					write(QByteArray().fill('-', col_size_[i]));
				}
				write("|\n");
			}
		}

		//content lines
		while(!in.atEnd())
		{
			QByteArrayList parts = in.readLine();

			if (format_=="html")
			{
				write("<tr>", true);
				indent_ += 2;
				for(int i=0; i<in.columns(); ++i)
				{
					write("<td>"+text(parts, i)+"</td>", true);
				}
				indent_ -= 2;
				write("</tr>", true);
			}
			else if (format_=="txt")
			{
				for(int i=0; i<in.columns(); ++i)
				{
					if (i!=0) write(" ");
					write(text(parts, i, true));
				}
				write("\n");
			}
			else if (format_=="md")
			{
				for(int i=0; i<in.columns(); ++i)
				{
					write("|");
					write(text(parts, i, true));
				}
				write("|\n");
			}
		}

		//after
		if (format_=="html")
		{
			indent_ -= 2;
			write("</table>", true);
			indent_ -= 2;
			write("</body>", true);
			indent_ -= 2;
			write("</html>", true);
		}
	}

private:
	QByteArray format_;
	int indent_;
	QSharedPointer<QFile> out_;
	QHash<int, long long> col_size_;
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

