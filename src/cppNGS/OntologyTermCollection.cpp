#include "OntologyTermCollection.h"
#include "Helper.h"


OntologyTerm::OntologyTerm()
	:id_()
	,name_("")
	,def_("")
	,type_("")
	,synonyms_()
	,parent_ids_()
	,is_obsolete_(false)
{
}

OntologyTerm::OntologyTerm(QByteArray& id, QByteArray& name, QByteArray& def, QList<QByteArray>& is_as, bool is_obsolete)
	:id_(id)
	,name_(name)
	,def_(def)
	,type_("")
	,synonyms_()
	,parent_ids_(is_as)
	,is_obsolete_(is_obsolete)
{
}

bool OntologyTerm::isChildOf(const QByteArray& parent_id) const
{
	foreach(const QByteArray& parent_term, parent_ids_)
	{
		if(parent_term.contains(parent_id))
		{
			return true;
		}
	}
	return false;
}

OntologyTermCollection::OntologyTermCollection()
{
}

OntologyTermCollection::OntologyTermCollection(QString filename, bool skip_obsolete_terms)
{
	QSharedPointer<QFile> fp = Helper::openFileForReading(filename);

	while(!fp->atEnd())
	{
		OntologyTerm temp;
		QByteArray line = fp->readLine().trimmed();

		//parse version
		if (line.startsWith("data-version:"))
		{
			version_ = line.mid(13).trimmed();
		}

		//A new set of terms always begins with [Term] followed by a set of tag:value pairs
		if(line == "[Term]")
		{
			line = fp->readLine().trimmed();

			do
			{
				if(line.startsWith("id:"))
				{
					temp.setId(line.mid(3).trimmed());
				}
				if(line.startsWith("name:"))
				{
					temp.setName(line.mid(5).trimmed());
				}
				if(line.startsWith("def:")) //Example: def: "Raw read length of a single read before trimming. Comma-separated list of lengths if several." [PXS:QC]
				{
					QByteArray def = line.mid(4).trimmed();
					int start = def.indexOf('"')+1;
					int end = def.lastIndexOf('"');
					def = def.mid(start, end-start);
					temp.setDefinition(def);
				}
				if(line.startsWith("xref: value-type:xsd\\:")) //Example: xref: value-type:xsd\:int "The allowed value-type for this CV term."
				{
					line.append(":");
					line.replace('"', ':');
					QByteArray type = line.split(':')[3].trimmed();
					temp.setType(type);
				}
				if(line.startsWith("is_a:")) //Example: is_a: QC:2000002 ! NGS aquisition parameter
				{
					QByteArray parent = line.mid(5).trimmed();
					int end = parent.lastIndexOf('!');
					parent = parent.mid(0,end).trimmed();
					temp.addParentID(parent);
				}

				if(line.startsWith("synonym:") && line.contains(" EXACT ")) //Example: synonym: "Breast fibroadenomas" EXACT [HPO:skoehler]
				{
					QByteArray synonym = line.mid(8).trimmed();
					int start = synonym.indexOf('"')+1;
					int end = synonym.lastIndexOf('"');
					synonym = synonym.mid(start, end-start);
					temp.addSynonym(synonym);
				}

				if(line.startsWith("is_obsolete:"))
				{
					temp.setIsObsolete(line.contains("true"));
				}

				if(line.startsWith("replaced_by:"))
				{
					temp.setReplacedById(line.mid(12).trimmed());
				}

				line = fp->readLine().trimmed();
			}
			while(!line.isEmpty());

			if(temp.isObsolete() && skip_obsolete_terms) continue;

			add(temp);
		}
	}
	fp->close();
}

void OntologyTermCollection::add(const OntologyTerm& term)
{
	if (containsByID(term.id()))
	{
		THROW(ArgumentException, "OntologyTermCollection::add: Term with id '" + term.id() + "' already persent!");
	}

	ontology_terms_.append(term);
}

const OntologyTerm& OntologyTermCollection::getByID(const QByteArray& id) const
{
	foreach(const OntologyTerm& term, ontology_terms_)
	{
		if(term.id() == id)
		{
			return term;
		}
	}
	THROW(ArgumentException, "OntologyTermCollection::getByID: No term with id '" + id + "' found.");
}

bool OntologyTermCollection::containsByID(const QByteArray& id) const
{
	foreach(const OntologyTerm& term, ontology_terms_)
	{
		if(term.id() == id)
		{
			return true;
		}
	}
	return false;
}

bool OntologyTermCollection::containsByName(const QByteArray& name) const
{
	foreach(const OntologyTerm& term, ontology_terms_)
	{
		if(term.name() == name)
		{
			return true;
		}
	}
	return false;
}

const OntologyTerm& OntologyTermCollection::get(int index) const
{
	if (index<0 || index>=size())
	{
		THROW(ArgumentException, "OntologyTermCollection::get: No term with index '" + QString::number(index) + "' found.");
	}

	return ontology_terms_[index];
}

QList<QByteArray> OntologyTermCollection::childIDs(const QByteArray& term_id, bool recursive) const
{
	QList<QByteArray> ids;
	foreach(const OntologyTerm& term, ontology_terms_)
	{
		if(term.isChildOf(term_id))
		{
			ids.append(term.id());
			if(recursive)
			{
				foreach(QByteArray recursive_term, childIDs(term.id(), true))
				{
					//remove duplicates
					if (! ids.contains(recursive_term))
					{
						ids.append(recursive_term);
					}
				}

			}
		}

	}
	return ids;
}


