#include "ToolBase.h"
#include "NGSD.h"
#include "Exceptions.h"

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
		setDescription("Sets up the NDSD database.");
		//optional
		addFlag("force", "Forces re-initialization if the database already contains tables.");
		addFlag("test_db", "Performs initialization of the test database instead of on the procution database.");
	}

	virtual void main()
	{
		//init
		QTextStream out(stdout);
		NGSD db(getFlag("test_db"));
		SqlQuery query = db.getQuery();

		//check if tables are present
		query.exec("SHOW TABLES");
		if (query.size()>0)
		{
			if (!getFlag("force"))
			{
				out << "Database is not empty! Use '-force' flag to overwrite database contents!" << endl;
				exit(1);
			}
			else
			{
				query.exec("SET FOREIGN_KEY_CHECKS = 0;");
				qDebug() << query.size();
				while(query.next())
				{
					qDebug() << query.value(0);
				}

				{
					$tables = array_values($result);
					$db_connect->executeStmt("DROP TABLE ".$tables[0].";");
				}
				query.exec("SET FOREIGN_KEY_CHECKS = 1;");
			}
		}



		echo "Database setup succesfully.\n";
		echo "You are now able to login as user 'admin' and password 'admin' via the web fronted.\n";
		echo "**Change password on first login!**\n";

/*
require_once(dirname($_SERVER['SCRIPT_FILENAME'])."/../Common/all.php");

error_reporting(E_ERROR | E_WARNING | E_PARSE | E_NOTICE);

$parser = new ToolBase("db_import_hgnc", "\$Rev$", "Imports HGNC data to NGSD.");
$parser->addInfile("in",  "HGNC flat file (download and unzip ftp://ftp.ebi.ac.uk/pub/databases/genenames/hgnc_complete_set.txt.gz)", false);
$parser->addEnum("db",  "Database to connect to.", true, array("NGSD", "NGSD_TEST"), "NGSD");
$parser->addFlag("force",  "Force re-import when data is already present.");
extract($parser->parse($argv));

//init
$valid_loci = array("pseudogene", "non-coding RNA", "other", "protein-coding gene");
$db = DB::getInstance($db);

//check tables exist
function check_table_exists($table)
{
	global $db;
	$res = $db->executeQuery("SHOW TABLES LIKE '$table'");
	if (count($res)==0) trigger_error("Table '$table' does not exist!", E_USER_ERROR);
}
check_table_exists('gene');
check_table_exists('gene_alias');

//clear tables when requested
function is_table_empty($table)
{
	global $db;
	$res = $db->executeQuery("SELECT COUNT(*) as c FROM $table");
	return $res[0]['c']==0;
}
if (!is_table_empty('gene') || !is_table_empty('gene_alias'))
{
	print "Data already present!\n";
	if ($force)
	{
		print "Clearing tables because '-force' was used!\n";
		$db->executeStmt("DELETE FROM gene_alias");
		$db->executeStmt("DELETE FROM gene");
	}
	else
	{
		trigger_error("Use '-force' to overwrite old data!", E_USER_ERROR);
	}
}

//import
$valid_chrs = array('1','2','3','4','5','6','7','8','9','10','11','12','13','14','15','16','17','18','19','20','21','22','X','Y','M');
$gene_hash = $db->prepare("INSERT INTO gene (hgnc_id, symbol, name, chromosome, type) VALUES (:0, :1, :2, :3, :4);");
$alias_hash = $db->prepare("INSERT INTO gene_alias (gene_id, symbol, type) VALUES (:0, :1, :2);");
$h = fopen($in, "r");
while(!feof($h))
{
	$line = trim(fgets($h));
	if ($line=="" || starts_with($line, "HGNC ID")) continue;

	list($id, $symbol, $name, $status, , $locus_group, $prev, , $syn, , $chr) = explode("\t", $line);

	//check status
	if ($status=="Entry Withdrawn" || $status=="Symbol Withdrawn") continue;
	if ($status!="Approved")
	{
		trigger_error("Unknown status '$status' in id '$id'", E_USER_ERROR);
	}

	//check locus
	if ($locus_group=="phenotype") continue;
	if (!in_array($locus_group, $valid_loci))
	{
		trigger_error("Unknown locus group '$locus_group' in id '$id'", E_USER_ERROR);
	}

	//curate data
	$id = substr($id, 5);
	$chr = strtr($chr, array("q"=>" ", "p"=>" ", "cen"=>" ", "mitochondria"=>"M"))." ";
	$chr = trim(substr($chr, 0, strpos ($chr, " ")));
	if (!in_array($chr, $valid_chrs))
	{
		$chr = 'none';
	}

	//insert gene
	//print "'$id' '$symbol' '$name' '$chr' '$locus_group'\n";
	$db->bind($gene_hash, "0", $id);
	$db->bind($gene_hash, "1", $symbol);
	$db->bind($gene_hash, "2", $name);
	$db->bind($gene_hash, "3", $chr);
	$db->bind($gene_hash, "4", $locus_group);
	$gene_id = $db->insertGetID($gene_hash);

	//insert aliases
	$syn = explode(',', $syn);
	foreach($syn as $s)
	{
		$s = trim($s);
		if ($s=="") continue;
		$db->bind($alias_hash, "0", $gene_id);
		$db->bind($alias_hash, "1", $s);
		$db->bind($alias_hash, "2", 'synonym');
		$db->execute($alias_hash, true);
	}
	$prev = explode(',', $prev);
	foreach($prev as $p)
	{
		$p = trim($p);
		if ($p=="") continue;
		$db->bind($alias_hash, "0", $gene_id);
		$db->bind($alias_hash, "1", $p);
		$db->bind($alias_hash, "2", 'previous');
		$db->execute($alias_hash, true);
	}
}
*/
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
