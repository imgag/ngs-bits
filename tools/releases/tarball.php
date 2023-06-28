<?php

error_reporting(E_ERROR | E_WARNING | E_PARSE | E_NOTICE);

include("/mnt/storage2/megSAP/pipeline/src/Common/all.php");

// parse command line arguments
$parser = new ToolBase("tarball", "Creates a tarball for a tagged version of ngs-bits.");
$parser->addString("tag", "Git tag.", false);
$parser->addString("hash", "Git hash that overrides the tag for the checkout.", true);
extract($parser->parse($argv));

//check out release version
exec2("git clone --recursive https://github.com/imgag/ngs-bits.git");
if ($hash!="")
{
	print "Using hash '$hash' instead of tag '$tag' for checkout!\n";
	exec2("cd ngs-bits && git checkout {$hash}");
}
else
{
	exec2("cd ngs-bits && git checkout {$tag}");
}
exec2("cd ngs-bits && git submodule update --recursive --init");

//remove test data
exec2("find ngs-bits/ -type d -name 'data_*' | xargs rm -rf");

//remove git infos
exec2("find ngs-bits/ -name '.git*' | xargs rm -rf");

//set fixed version
$filename = "ngs-bits/src/cppCORE/cppCORE.pro";
$file = file($filename);
for($i=0; $i<count($file); ++$i)
{
	$line = $file[$i];
	
	//skip line that determines the version
	if (starts_with($line, "SVN_VER="))
	{
		$file[$i] = "";
	}
	
	//set fixed version
	if (contains($line, '$$SVN_VER'))
	{
		$file[$i] = "DEFINES += CPPCORE_VERSION=\\\\\\\"{$tag}\\\\\\\"\n";
	}
}
file_put_contents($filename, $file);

//create tarball
$tarball = "ngs-bits-{$tag}.tgz";
exec2("tar czf {$tarball} ngs-bits/");

//calculate hash
list($stdout) = exec2("shasum -a 256 {$tarball}");
print "HASH:\n".implode("", $stdout)."\n";
?>
