<?php

function simplify($str)
{
	return preg_replace('!\s+!', ' ', trim($str));
}

function starts_with($string, $prefix)
{
	return substr($string, 0, strlen($prefix)) == $prefix;
}

function ends_with($string, $suffix)
{
	return substr($string, -strlen($suffix)) == $suffix;
}

function contains($haystack, $needle)
{
	if (gettype($haystack)!="string") $haystack = strval($haystack);
	if (gettype($needle)!="string") $needle = strval($needle);
	
	return strpos($haystack, $needle)!== false;
}

//determine header files in src/ folder
function headers($folder)
{
    $output = [];
	
    $iti = new RecursiveDirectoryIterator($folder);
    foreach(new RecursiveIteratorIterator($iti) as $file)
	{		
		if (ends_with($file, ".h"))
		{
			$output[] = (string)$file;
		}
    }
		
    return $output;
}
//define headers that are not changed
$keep = []; //TODO "TestFramework", "SimpleCrypt", "QrCodeFactory", "QrCodeGenerator", "QrCode", "QrSegment"];

//determine classes and structs defined in header
$h2def = [];
$base2path = [];
foreach(headers("../../src/") as $filename)
{
	$basename = basename($filename, ".h");
	$base2path[$basename][] = $filename;
	
	//TODO if (in_array($basename, $keep)) continue; //kept anyway > no parsing
	
	foreach(file($filename) as $line)
	{
		$line = strtr($line, ["CPPCORESHARED_EXPORT"=>"", "CPPNGSSHARED_EXPORT"=>"", "CPPNGSDSHARED_EXPORT"=>"", "CPPGUISHARED_EXPORT"=>"", "CPPRESTSHARED_EXPORT"=>"", ":"=>": "]);
		$line = simplify($line);
		
		$parts = explode(" ", $line);
		if (count($parts)<2) continue;
		
		$def = "";
		if (starts_with($line, "enum class "))
		{
			$def = $parts[2];
		}
		if (starts_with($line, "class "))
		{ 
			if (ends_with($parts[1], ";")) continue; //forward declarations
			
			$def = $parts[1];
		}
		if (starts_with($line, "struct "))
		{
			$def = $parts[1];
		}
		
		if ($def!="" && !in_array($def, $h2def[$basename]))
		{
			$h2def[$basename][] = $parts[2];
		}
	}
}

//check for headers without definitions
foreach($h2def as $header => $definitions)
{
	if (count($definitions)==0)
	{
		print "warning: header {$header} has no definitions\n";
	}
}

//check for duplicate header names
foreach($base2path as $basename => $paths)
{
	if (count($paths)>1)
	{
		print "warning: several header files with name {$basename}: ".implode(", ", $paths)."\n";
	}
}

print_r($h2def);
?>
