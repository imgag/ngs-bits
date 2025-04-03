<?php

function contains($haystack, $needle)
{
	if (gettype($haystack)!="string") $haystack = strval($haystack);
	if (gettype($needle)!="string") $needle = strval($needle);
	
	return strpos($haystack,$needle)!== false;
}

function ends_with($string, $suffix)
{
	return substr($string, -strlen($suffix)) == $suffix;
}

$dir = dirname(__FILE__);

//determine test data folders
$folders = [];
exec("find {$dir}/../../src/ -name 'data_in' -or -name 'data_out'", $folders);

//determine test lines in which test data files are used
$test_lines = [];
foreach($folders as $folder)
{
	exec("ls {$folder}/../*.h | xargs egrep 'data_in|data_out'", $test_lines);
}

foreach($folders as $folder)
{
	$folder = realpath($folder);
	$base = basename($folder)."/";
	
	//get test data file names
	$files = [];
	exec("ls $folder", $files);
	
	//check if test data files are actually used
	foreach($files as $file)
	{
		//skip index files
		if (ends_with($file, ".bai") && file_exists($folder."/".substr($file,0,-4))) continue;
		if (ends_with($file, ".tbi") && file_exists($folder."/".substr($file,0,-4))) continue;
		if (ends_with($file, ".fai") && file_exists($folder."/".substr($file,0,-4))) continue;
		if (ends_with($file, ".crai") && file_exists($folder."/".substr($file,0,-5))) continue;
		if (ends_with($file, ".cidx") && file_exists($folder."/".substr($file,0,-5))) continue;
		
		//special handing
		if ($file=="NGSDExportStudyGHGA_sample_data") continue; //filename used via text file
		if ($file=="VcfAnnotateFromVcf_an1_ClinVar.vcf.gz") continue; //filename used via text file
		
		//perform check
		$found = false;
		foreach($test_lines as $line)
		{
			if (contains($line, $base.$file))
			{
				$found = true;
				break;
			}
		}
		if (!$found)
		{
			print "unused: {$folder}/{$file}\n";
		}
	}
}



?>