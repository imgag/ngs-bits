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
		setDescription("Converts a VCF file to a tab-separated text file.");
		setExtendedDescription(QStringList() << "Multi-allelic variants are not supported. Use VcfBreakMulti to split multi-allelic variants into several lines."
											 << "Multi-sample VCFs are not supported. Use VcfExtractSamples to split them to one VCF per sample.");
		addInfile("in", "Input variant list in VCF format.", false, true);
		addOutfile("out", "Output variant list in TSV format.", false, true);
	}

	virtual void main()
	{
		//load
		VariantList vl;
		vl.load(getInfile("in"), VCF);

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

		//store
		vl.store(getOutfile("out"), TSV);
    }
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

