#include "Exceptions.h"
#include "ToolBase.h"
#include "ChromosomalIndex.h"
#include "VariantList.h"
#include "BedFile.h"
#include "NGSD.h"
#include "Log.h"
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
		setDescription("Annotates a variant list with information from the NGSD.");
		addInfile("in", "Input variant list.", false, true);
		addOutfile("out", "Output variant list.", false, true);
		//optional
		addString("psname", "Processed sample name. If set, this name is used instead of the file name to find the sample in the DB.", true, "");
		addInfile("ref", "Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.", true, false);
		QStringList modes;
		modes << "germline" << "somatic";
		addEnum("mode", "Determines annotation mode.", true, modes, "germline");
	}

	virtual void main()
	{
		//determine refererence genome file
		QString ref_file = getInfile("ref");
		if (ref_file=="") ref_file = Settings::string("reference_genome");

		//load
		VariantList variants;
		variants.load(getInfile("in"));
		QString ps = getString("psname");
		if (ps=="") ps = getInfile("in");

		//annotate
		QString mode = getEnum("mode");
		if(mode=="germline") NGSD().annotate(variants, ps, ref_file, false);
		else if(mode=="somatic") NGSD().annotateSomatic(variants, ps, ref_file);
		else THROW(ProgrammingException, "Unknown mode '" + mode + "'!");

		//store
		variants.store(getOutfile("out"));
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
