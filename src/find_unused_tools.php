<?php

$cutoff = 3;
@$tmp = $argv[1];
if (is_numeric($tmp)) $cutoff = $tmp;

//get list of tools
$tools = array();
exec("grep SUBDIRS tools.pro | grep -v cpp | grep -v TEST | cut -f3 -d' '", $tools);

foreach($tools as $tool)
{
	//find hits
	$hits = array();
	exec("find /mnt/share/data/ /mnt/share/chips/ /mnt/share/doc/ /mnt/share/kasp/ /mnt/share/primer/ /mnt/users/ahsturm1/ -name \"Makefile\" -or -name \"*.sh\" -or -name \"*.php\" 2> /dev/null | xargs grep $tool", $hits);
	
	//filter hits
	$good_hits = array();
	foreach($hits as $hit)
	{
		//skip c++ build folder hits
		if (strpos($hit, "build-tools-Linux")!==false) continue;
		
		//skip php error messages
		if (strpos($hit, "E_USER_ERROR")!==false) continue;
		
		$good_hits[] = $hit;
	}
	
	//output
	if (count($good_hits)<=$cutoff)
	{
		print "$tool\t".count($good_hits)."\n";
		foreach($good_hits as $hit)
		{
			print "  HIT: ".$hit."\n";
		}
	}
}

?>