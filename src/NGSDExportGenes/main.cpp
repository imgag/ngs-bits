#include "ToolBase.h"
#include "NGSD.h"
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
		setDescription("Lists genes from NGSD.");
		addOutfile("out", "Output TSV file. If unset, writes to STDOUT.", true);
		//optional
		addFlag("add_disease_info", "Annotate with disease information from HPO, OrphaNet and OMIM (slow).");
		addFlag("test", "Uses the test database instead of on the production database.");

		changeLog(2021,  4,  13, "Added more information (imprinting, pseudogenes, OrphaNet, OMIM).");
		changeLog(2019,  9,  20, "Added several columns with gene details.");
		changeLog(2018,  5,  3 , "First version");
	}

	virtual void main()
	{
		//init
		NGSD db(getFlag("test"));
		bool add_disease_info = getFlag("add_disease_info");
		
		//write header
		QSharedPointer<QFile> output = Helper::openFileForWriting(getOutfile("out"), true);
		output->write("#symbol");
		output->write("\tHGNC id");
		output->write("\ttype");
		output->write("\tname");
		output->write("\ttranscripts coding (ensembl)");
		output->write("\ttranscripts non-coding (ensembl)");
		output->write("\tgenomAD oe (syn)");
		output->write("\tgenomAD oe (mis)");
		output->write("\tgenomAD oe (lof)");
		output->write("\tinheritance");
		output->write("\timprinting");
		output->write("\tpseudogenes");
		if (add_disease_info)
		{
			output->write("\tHPO terms");
			output->write("\tOMIM phenotypes");
			output->write("\tOrphaNet diseases");
		}
		output->write("\n");

		//write content
		SqlQuery query = db.getQuery();
		query.exec("SELECT g.symbol, g.hgnc_id, g.type, g.name, (SELECT COUNT(*) FROM gene_transcript gt WHERE gene_id=g.id AND gt.source='ensembl' AND gt.start_coding IS NOT NULL) as trans, "
				   "(SELECT COUNT(*) FROM gene_transcript gt WHERE gene_id=g.id AND gt.source='ensembl' AND gt.start_coding IS NULL) as trans_nc FROM gene g ORDER BY g.symbol ASC");
		while (query.next())
		{
			QByteArray gene_symbol = query.value("symbol").toByteArray();
			output->write(gene_symbol + "\t");
			output->write("HGNC:" + query.value("hgnc_id").toByteArray() + "\t");
			output->write(query.value("type").toByteArray() + "\t");
			output->write(query.value("name").toByteArray() + "\t");
			output->write(query.value("trans").toByteArray() + "\t");
			output->write(query.value("trans_nc").toByteArray() + "\t");
		
			//gene info
			GeneInfo gene_info = db.geneInfo(gene_symbol);
			output->write(gene_info.oe_syn.replace("n/a", "").toLatin1() + "\t");
			output->write(gene_info.oe_mis.replace("n/a", "").toLatin1() + "\t");
			output->write(gene_info.oe_lof.replace("n/a", "").toLatin1() + "\t");
			output->write(gene_info.inheritance.replace("n/a", "").toLatin1() + "\t");
			QString inprinting_info = "";
			if(!gene_info.imprinting_source_allele.isEmpty() || !gene_info.imprinting_status.isEmpty())
			{
				inprinting_info = gene_info.imprinting_source_allele + " (" + gene_info.imprinting_status + ")";
			}
			output->write(inprinting_info.toLatin1() + "\t");
			output->write(gene_info.pseudogenes.join(", ").toLatin1());

			//disease info
			if (add_disease_info)
			{
				output->write("\t");

				//HPO terms
				QByteArrayList hpos;
				PhenotypeList phenos = db.phenotypes(gene_symbol);
				foreach(const Phenotype& pheno, phenos)
				{
					hpos << pheno.toString();
				}
				output->write(hpos.join("; ") + "\t");

				//OMIM
				QByteArrayList omim_phenos;
				foreach(const OmimInfo& omim, db.omimInfo(gene_symbol))
				{
					foreach(const Phenotype& p, omim.phenotypes)
					{
						omim_phenos << p.name();
					}
				}
				output->write(omim_phenos.join("; ") + "\t");

				//Orphanet
				QByteArrayList orpha_diseases;
				SqlQuery query2 = db.getQuery();
				query2.exec("SELECT dt.identifier, dt.name FROM disease_term dt, disease_gene dg WHERE dg.disease_term_id=dt.id AND dt.source='OrphaNet' AND dg.gene='" + gene_symbol + "'");
				while (query2.next())
				{
					QByteArray identifier = query2.value("identifier").toByteArray();
					QByteArray name = query2.value("name").toByteArray();
					orpha_diseases << identifier + " - " + name;
				}
				output->write(orpha_diseases.join("; "));
			}
			
			output->write("\n");
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
