<?php

include("/mnt/users/bioinf/megSAP/src/Common/all.php");

$ver_old = $argv[1];
$command = implode(" ", array_slice($argv, 2));

//check that old version exists
$folder_old = "/mnt/share/opt/ngs-bits-{$ver_old}";
if (!file_exists($folder_old))
{
	trigger_error("Old ngs-bits folder {$folder_old} does not exist!", E_USER_ERROR);
}

//execute old command
list($stdout1, $stderr1) = exec2("/usr/bin/time -v {$folder_old}/{$command}");
foreach($stderr1 as $line)
{
	$line = trim($line);
	if (starts_with($line, "Elapsed (wall clock) time"))
	{
		list(, $time) = explode("):", $line);
		$parts = explode(":", $time);
		$time_old = 60*$parts[0] + $parts[1];
	}
	if (starts_with($line, "Maximum resident set size"))
	{
		list(, $mem_old) = explode("):", $line);
	}	
}

//execute new command
list($stdout2, $stderr2) = exec2("/usr/bin/time -v ../../bin/{$command}");
foreach($stderr2 as $line)
{
	$line = trim($line);
	if (starts_with($line, "Elapsed (wall clock) time"))
	{
		list(, $time) = explode("):", $line);
		$parts = explode(":", $time);
		$time_new = 60*$parts[0] + $parts[1];
	}
	if (starts_with($line, "Maximum resident set size"))
	{
		list(, $mem_new) = explode("):", $line);
	}	
}

//print output
print "Time  : $time_old > $time_new [s]\n";
print "Memory: $mem_old > $mem_new [kB]\n";

//compare output
for($i=0; $i<count($stdout1); ++$i)
{
	$line_old = trim($stdout1[$i]);
	$line_new = trim($stdout2[$i]);
	if ($line_old!=$line_new)
	{
		print "Differing line $i:\n";
		print "-$line_old\n";
		print "+$line_new\n";		
	}
}


?>
