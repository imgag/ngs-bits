### SampleDiff tool help
	SampleDiff (2024_08-113-g94a3b440)
	
	Calculates the differences/overlap between variant lists.
	
	Mandatory parameters:
	  -in1 <file>       Input variant list in GSvar format.
	  -in2 <file>       Input variant list in GSvar format.
	
	Optional parameters:
	  -out <file>       Output file. If unset, writes to STDOUT.
	                    Default value: ''
	  -window <int>     Window to consider around indel positions to compensate for differing alignments.
	                    Default value: '100'
	  -nei              Allow non-exact indel matches. If set, all indels in the window are considered matches.
	                    Default value: 'false'
	  -sm               Also show matches. If unset, matching variants are not printed.
	                    Default value: 'false'
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### SampleDiff changelog
	SampleDiff 2024_08-113-g94a3b440
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)