#include "ToolBase.h"
#include "Helper.h"
#include "Exceptions.h"
#include "VariantList.h"
#include <QFile>
#include <QTextStream>
#include <QList>

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
		setDescription("Shifts indels in a variant list as far to the left as possible. Complex indels and multi-allelic deletions are not shifted!");
		addInfile("in", "Input variant list in VCF format.", false, true);
		addOutfile("out", "Output variant list in TSV format.", false, true);
		//optional
		addFlag("split", "Split multi-allelic variants.");
		//probably useful features for the future: skip comments, skip annotation descriptions
	}

	virtual void main()
	{
		//init
		bool split = getFlag("split");

		//load
		VariantList vl;
		vl.load(getInfile("in"), VariantList::VCF);

		//split multi-allelic variant
		for (int i=0; i<vl.count(); ++i)
		{
			Variant& v = vl[i];
			if (!v.obs().contains(",")) continue;

			if (!split) THROW(FileParseException, "Input file contains multi-allelic variants. Split variants using VcfSplitMultiallelic or the -split option of this tool.\nError variant: " + vl[i].toString());

			QStringList obss = vl[i].obs().split(",");
			v.setObs(obss.takeFirst());
			foreach(QString obs, obss)
			{
				Variant v2 = vl[i]; //here we cannot use 'v' because the list might have been re-allocated due to the append statement below
				v2.setObs(obs);
				vl.append(v2);
			}
		}

		//change start/end/ref/obs as needed in TSV
		for (int i=0; i<vl.count(); ++i)
		{
			Variant& v = vl[i];
			v.normalize("-");
			if (v.ref()=="-")
			{
				v.setStart(v.start()-1);
				v.setEnd(v.end()-1);
			}
		}

		//sort because we appended variants and positions have changed
		vl.sort(true);

		//store
		vl.store(getOutfile("out"), VariantList::TSV);
    }
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

