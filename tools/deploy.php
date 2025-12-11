<?php

function ends_with($string, $suffix)
{
	return substr($string, -strlen($suffix)) == $suffix;
}

function git_tag($bin_folder)
{
	$output = [];
	$exit_code = -1;
	exec("cd {$bin_folder}//..// && git describe --tags", $output, $exit_code);
	if ($exit_code!=0) trigger_error("Coult not determine GIT version", E_USER_ERROR);
	return trim(implode("", $output));
}

//parse command line args
$exe = "";
$bin_folder = ($argc>=2) ? $argv[1] : "..\\bin\\";
if (is_file($bin_folder)) //exe given > deploy ony that exe, not all
{
	$exe = basename($bin_folder);
	$bin_folder = dirname($bin_folder);
}
$bin_folder = realpath($bin_folder)."\\";
$dest = ($argc>=3) ? $argv[2] : ".\\deploy_".git_tag($bin_folder)."\\";
if (!file_exists($dest))
{
	if (!mkdir($dest,  0777, true))
	{
		trigger_error("Could not create destination folder: {$dest}", E_USER_ERROR);
	}
}
$dest = realpath($dest)."\\";
print "bin folder: {$bin_folder}\n";
print "destination folder: {$dest}\n";

print "\n";
print "copying executables and DLLs...\n";

//copy our EXEs
foreach(glob("{$bin_folder}\\*.exe") as $file)
{
	if (strpos($file, "-TEST.exe")!==false) continue;
	if ($exe!="" && !ends_with($file, $exe)) continue;
	copy($file, $dest."\\".basename($file));
}

//copy our DLLs
foreach(glob("{$bin_folder}\\cpp*.dll") as $file)
{
	copy($file, $dest."\\".basename($file));
}

//copy Qt DLLs
$files = ["Qt6Charts.dll", "Qt6Core.dll", "Qt6Gui.dll", "Qt6Network.dll", "Qt6OpenGL.dll", "Qt6OpenGLWidgets.dll", "Qt6PrintSupport.dll", "Qt6Sql.dll", "Qt6Svg.dll", "Qt6Widgets.dll", "Qt6Xml.dll", "libgcc_s_seh-1.dll", "libstdc++-6.dll", "libwinpthread-1.dll"];
foreach($files as $file)
{
	copy("C:\\Qt\\6.8.3\\mingw_64\\bin\\".$file, $dest."\\".$file);
}

//copy Qt plugins
$plugins = ["iconengines", "imageformats", "platforms", "sqldrivers", "styles", "tls"];
foreach($plugins as $plugin)
{
	$plugin_folder = "C:\\Qt\\6.8.3\\mingw_64\\plugins\\{$plugin}\\";
	$plugin_dest_folder = $dest."\\{$plugin}\\";
	if (!file_exists($plugin_dest_folder))
	{
		if (!mkdir($plugin_dest_folder))
		{
			trigger_error("Could not create Qt plugin destination folder: {$plugin_dest_folder}", E_USER_ERROR);
		}
	}
	foreach(glob("{$plugin_folder}\\*.dll") as $file)
	{
		copy($file, "{$plugin_dest_folder}\\".basename($file));
	}
}

//move libmariadb to the main folder
if (file_exists($dest."\\sqldrivers\\libmariadb.dll"))
{
	rename($dest."\\sqldrivers\\libmariadb.dll", $dest."\\libmariadb.dll");
}

//copy htslib DLLs
$hts_lib_folder = $bin_folder."\\..\\htslib\\lib\\";
if (file_exists($hts_lib_folder))
{
	$files = ["hts-3.dll", "libbz2-1.dll", "libcrypto-3-x64.dll", "libcurl-4.dll", "liblzma-5.dll", "zlib1.dll", "libbrotlidec.dll", "libsystre-0.dll", "libnghttp2-14.dll", "libnghttp3-9.dll", "libidn2-0.dll", "libpsl-5.dll", "libtre-5.dll", "libssh2-1.dll", "libzstd.dll", "libssl-3-x64.dll", "libiconv-2.dll", "libintl-8.dll", "libunistring-5.dll", "libbrotlicommon.dll"]; 
	foreach($files as $file)
	{
		copy("{$hts_lib_folder}\\".$file, $dest."\\".$file);
	}
}
else
{
	print "Warning: htslib folder not found, cannot copy htslib DLLs\n";
}

//copy libxml2 DLLs
$hts_lib_folder = $bin_folder."\\..\\libxml2\\libs\\";
if (file_exists($hts_lib_folder))
{
	$files = ["libxml2-16.dll"];
	foreach($files as $file)
	{
		copy("{$hts_lib_folder}\\".$file, $dest."\\".$file);
	}
}
else
{
	print "Warning: libxml2 folder not found, cannot copy libxml2 DLLs\n";
}


//check if tools are executableprint
print "\n";
print "testing if tools are executable...\n";
$tools = [
	"{$dest}\\BamInfo.exe", //needs htslib
	"{$dest}\\TsvToQC.exe", //needs libxml2
	"{$dest}\\NGSDExportSamples.exe", //needs SQL
	];
foreach($tools as $tool)
{
	if (file_exists($tool)) 
	{
		$output = [];
		$exit_code = -1;
		exec("{$tool} --version", $output, $exit_code);
		foreach($output as $line)
		{
			$line = trim($line);
			if ($line=="") continue;
			print $line."\n";
		}
		print "Exit code: {$exit_code}\n";
	}
}

?>

