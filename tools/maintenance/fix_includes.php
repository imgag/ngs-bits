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
function files_recursive($folder, $ext)
{
    $output = [];
	
    $iti = new RecursiveDirectoryIterator($folder);
    foreach(new RecursiveIteratorIterator($iti) as $file)
	{		
		if (ends_with($file, $ext))
		{
			$output[] = (string)$file;
		}
    }
		
    return $output;
}

//checks if a class was used in the given file
function include_used($input, $file_content_lc)
{
	$check = [];
	if (is_array($input))
	{
		foreach($input as $item)
		{
			$check[] = strtolower($item);
		}
	}
	else
	{
		$check[] = strtolower($input);
	}
	
	foreach($check as $name_lc)
	{
		if (contains($file_content_lc, $name_lc)) return true;
	}
	return false;
}

### determine classes and structs defined in header ###
$h2def = [];
$base2path = [];
foreach(files_recursive("../../src/", ".h") as $filename)
{
	$basename = basename($filename, ".h");
	$base2path[$basename][] = $filename;
	
	foreach(file($filename) as $line)
	{
		$line = strtr($line, ["CPPCORESHARED_EXPORT"=>"", "CPPNGSSHARED_EXPORT"=>"", "CPPNGSDSHARED_EXPORT"=>"", "CPPGUISHARED_EXPORT"=>"", "CPPRESTSHARED_EXPORT"=>"", ":"=>": ", ";"=>""]);
		$line = simplify($line);
		
		$parts = explode(" ", $line);
		if (count($parts)<2) continue;
		
		$def = "";
		if (starts_with($line, "enum "))
		{
			$def = $parts[1];
		}
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
		if (starts_with($line, "using ") && count($parts)>=3 && $parts[2]=="=")
		{
			$def = $parts[1];
		}
		if (starts_with($line, "typedef ") && count($parts)>=3)
		{
			$def = end($parts);
		}		
		
		if ($def!="")
		{
			if (!isset($h2def[$basename])) $h2def[$basename] = [];
			
			if (!in_array($def, $h2def[$basename]))
			{
				$h2def[$basename][] = $def;
			}
		}
	}
}
$h2def['XmlHelper'][] = 'XmlHelper'; //only status methods > no class/struct

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

### determine Qt headers ###

$qt_headers = [];
foreach(files_recursive("C:/Qt/Qt5.12.12/5.12.12/mingw73_64/include/", ".h") as $header)
{
	if (contains($header, "\\private\\")) continue;
	$name = strtolower(basename($header, ".h"));
	$qt_headers[$name] = $header;
}

//check for headers with same name in Qt and ngs-bits
foreach($h2def as $header => $definitions)
{
	$header = strtolower($header).".h";
	if (isset($qt_headers[$header]))
	{
		print "warning: Qt and ngs-bits header with same name: {$header}\n";
	}
}

$qt_headers_extra = [
	"QDate"=>true,
	"QRegularExpressionMatchIterator"=>true,
	"QPaintEvent"=>true,
	"QContextMenuEvent"=>true,
	"QVariantList"=>true,
	"QMultiMap"=>true,
	"QXmlStreamWriter"=>true,
	"QXmlStreamReader"=>true,
	"QMetaMethod"=>true,
	"QTableWidgetItem"=>true,
	"QTreeWidgetItem"=>true,
	"QCloseEvent"=>true,
	"QTime"=>true,
	"QDragEnterEvent"=>true,
	"QDropEvent"=>true,	
	"QListIterator"=>true,	
	"QDoubleSpinBox"=>true,	
	"QListWidgetItem"=>true,	
	"QProcessEnvironment"=>true,
	"QDomElement"=>true,
	"QDomDocument"=>true,
	"QRandomGenerator"=>true
];

### clean up ngs-bits headers ###
$stats = [];
$filenames = array_merge(files_recursive("../../src/", ".h"), files_recursive("../../src/", ".cpp"));
foreach($filenames as $filename)
{	
	$file = file($filename);
	$file_content_lc = "";
	foreach($file as $line)
	{
		$line = trim($line);
		if (starts_with($line, "#include")) continue;
		$file_content_lc .= strtolower($line)."\n";
	}
	
	//determine unused includes that can be deleted
	$i_del = [];
	for ($i=0; $i<count($file); ++$i)
	{
		$line = trim($file[$i]);
		if (!starts_with($line, "#include")) continue;
				
		$line = simplify($line);
		$header = trim(strtr($line, ["#include"=>"", "<"=>"", ">"=>"", "\""=>"", ".h"=>""]));
		$basename = basename($header);
		
		//with comment, e.g. //Comment to prevent removal by fix_includes.php
		if (contains($header, "//")) continue;
		
		//Qt stuff
		if (starts_with($header, "ui_")) continue;
		if ($header=="main.moc") continue;
		if (ends_with($header, "/qglobal")) continue;
		
		//ngs-bits stuff
		if ($header=="cppCORE_global" || $header=="cppGUI_global" || $header=="cppNGS_global" || $header=="cppNGSD_global" || $header=="cppXML_global" || $header=="cppREST_global" || $header=="cppVISUAL_global") continue;
		if ($header=="TestFramework" || $header=="TestFrameworkNGS") continue;
		if (ends_with($header, ".cpp")) continue;
		
		//std C++ stufff
		if ($header=="cmath" || $header=="numeric" || $header=="limits" || $header=="utility" || $header=="algorithm" || $header=="algorithm" || $header=="random" || $header=="iostream" || $header=="math" || $header=="vector" || $header=="time" || $header=="tuple") continue;
		
		//3rd party stuff
		if (starts_with($header, "htslib/")) continue;
		if (starts_with($header, "libxml/")) continue;
		if (contains($filename, "QrCodeGenerator")) continue;
		
		if (isset($qt_headers[strtolower($header)]) || isset($qt_headers[strtolower($basename)]) || isset($qt_headers_extra[$header])) //Qt
		{
			$names = $basename;
			if ($basename=="QDebug")
			{
				$names = ["qdebug", "qwarning"];
			}
			if (!include_used($names, $file_content_lc)) 
			{
				$i_del[$i] = $line;
			}
		}
		else if (isset($h2def[$basename])) //ngs-bits
		{
			if (!include_used($h2def[$basename], $file_content_lc)) 
			{
				$i_del[$i] = $line;
			}
		}
		else
		{
			print "warning: unhandled include of '$header' in '$filename'\n";
		}
	}
	
	//delete lines
	$c_del = count($i_del);
	if ($c_del>0)
	{
		$h = fopen($filename, "w");
		if ($h===false) trigger_error("Could not open file for readning: $filename", E_USER_ERROR);
		for ($i=0; $i<count($file); ++$i)
		{
			if (isset($i_del[$i])) continue;
			fputs($h, $file[$i]);
		}
		fclose($h);
	}
	
	//update stats
	if (!isset($stats[$c_del]))
	{
		$stats[$c_del] = 0;
	}
	$stats[$c_del] += 1;
}

//statistics output
print "\nDeleted includes:\n";
ksort($stats);
foreach($stats as $c_deleted => $c_files)
{
  print "  {$c_deleted} in {$c_files} files\n";
}

?>
