
INSERT INTO `user`(`id`, `user_id`, `password`, `user_role`, `name`, `email`, `created`, `active`) VALUES
(99, 'ahmustm1', '', 'user', 'Max Mustermann', '', '2016-07-05', 1);

INSERT INTO `device` (`id`, `type`, `name`) VALUES
(1, 'MiSeq', 'Neo');

INSERT INTO `sender` (`id`, `name`) VALUES
(1, 'Coriell');

INSERT INTO `project` (`id`, `name`, `type`, `internal_coordinator_id`, `analysis`) VALUES 
(1, 'KontrollDNACoriell', 'test', 1, 'variants');

INSERT INTO `sequencing_run` (`id`, `name`, `fcid`, `device_id`, `recipe`, `status`) VALUES
(1, '#00372', 'AB2J9', 1, '158+8+158', 'analysis_finished');

INSERT INTO `sample` (`id`, `name`, `sample_type`, `species_id`, `gender`, `quality`, `tumor`, `ffpe`, `sender_id`) VALUES
(1, 'NA12878', 'DNA', 1, 'n/a', 'good', 0 ,0, 1);

INSERT INTO `processing_system` (`id`, `name_short`, `name_manufacturer`, `adapter1_p5`, `adapter2_p7`, `type`, `shotgun`, `umi_type`, `target_file`, `genome_id`) VALUES
(96, 'hpDYTv3', 'HaloPlex DYT v3', 'AGATCGGAAGAGCACACGTCTGAACTCCAGTCAC', 'AGATCGGAAGAGCGTCGTGTAGGGAAAGAGTGT', 'Panel Haloplex', 0, 'n/a', '/some_path/hpDYT_v3_2014_12_12.bed', 1),
(145, 'ssHAEv6', 'SureSelectXT Human All Exon V6', 'AGATCGGAAGAGCACACGTCTGAACTCCAGTCAC', 'AGATCGGAAGAGCGTCGTGTAGGGAAAGAGTGT', 'WES', 1, 'n/a', '/some_path/ssHAEv6_2019_07_19.bed', 1),
(165, 'TruSeqPCRfree', 'TruSeq DNA PCR-Free', 'AGATCGGAAGAGCACACGTCTGAACTCCAGTCAC', 'AGATCGGAAGAGCGTCGTGTAGGGAAAGAGTGT', 'WGS', 1, 'n/a', '/some_path/WGS_hg19.bed', 1);

INSERT INTO `processed_sample`(`id`, `sample_id`, `process_id`, `sequencing_run_id`, `lane`, `processing_system_id`, `project_id`) VALUES
(3999, 1, 18, 1, '1', 96, 1),
(4000, 1, 38, 1, '2', 145, 1),
(4001, 1, 45, 1, '1,2,3,4', 165, 1);

INSERT INTO `repeat_expansion` (id, name, region, repeat_unit) VALUES
(1, 'AR', 'chrX:67545316-67545385', 'GCA'),
(2, 'ATN1', 'chr12:6936716-6936773', 'CAG'),
(3, 'ATXN1', 'chr6:16327633-16327723', 'TGC'),
(4, 'ATXN2', 'chr12:111598949-111599018', 'GCT'),
(5, 'ATXN3', 'chr14:92071010-92071040', 'CTG'),
(6, 'ATXN7', 'chr3:63912684-63912714', 'GCA'),
(7, 'ATXN8OS_CTA', 'chr13:70139353-70139383', 'CTA'),
(8, 'ATXN8OS', 'chr13:70139383-70139428', 'CTG'),
(9, 'ATXN10', 'chr22:45795354-45795424', 'ATTCT'),
(10, 'C9ORF72', 'chr9:27573528-27573546', 'GGCCCC'),
(11, 'CACNA1A', 'chr19:13207858-13207897', 'CTG'),
(12, 'FGF14', 'chr13:102161574-102161724', 'AAG'),
(13, 'FMR1', 'chrX:147912050-147912110', 'CGG'),
(14, 'FXN', 'chr9:69037286-69037304', 'GAA'),
(15, 'HTT', 'chr4:3074876-3074933', 'CAG'),
(16, 'JPH3', 'chr16:87604287-87604329', 'CTG'),
(17, 'PPP2R2B', 'chr5:146878727-146878757', 'GCT'),
(18, 'RFC1', 'chr4:39348424-39348479', 'AARRG'),
(19, 'TBP', 'chr6:170561906-170562017', 'GCA'),
(20, 'AFF2', 'chrX:148500631-148500691', 'GCC'),
(21, 'CNBP', 'chr3:129172576-129172656', 'CAGG'),
(22, 'CSTB', 'chr21:43776443-43776479', 'CGCGGGGCGGGG'),
(23, 'DIP2B', 'chr12:50505001-50505022', 'GGC'),
(24, 'DMPK', 'chr19:45770204-45770264', 'CAG'),
(25, 'PHOX2B', 'chr4:41745972-41746032', 'GCN'),
(26, 'NOTCH2NLC', 'chr1:149390802-149390841', 'GGC'),
(27, 'TCF4', 'chr18:55586155-55586227', 'CAG'),
(28, 'ABCD3', 'chr1:94418421-94418442', 'GCC'),
(29, 'ARX_1', 'chrX:25013649-25013697', 'NGC'),
(30, 'ARX_2', 'chrX:25013529-25013565', 'NGC'),
(31, 'DAB1', 'chr1:57367043-57367118', 'RAAAT'),
(32, 'GIPC1', 'chr19:14496041-14496074', 'CCG'),
(33, 'GLS', 'chr2:190880872-190880920', 'GCA'),
(34, 'LRP12', 'chr8:104588970-104588997', 'CGC'),
(35, 'NIPA1', 'chr15:22786677-22786701', 'GCG'),
(36, 'NOP56', 'chr20:2652733-2652757', 'GGCCTG'),
(37, 'PABPN1', 'chr14:23321472-23321502', 'GCN'),
(38, 'RILPL1', 'chr12:123533720-123533750', 'GGC'),
(39, 'SOX3', 'chrX:140504316-140504361', 'NGC'),
(40, 'THAP11', 'chr16:67842863-67842950', 'CAG'),
(41, 'ZFHX3', 'chr16:72787694-72787757', 'GCC'),
(42, 'EIF4A3', 'chr17:80147003-80147139', 'CCTCGCTGYGCCGCTGCCGA'),
(43, 'MARCHF6', 'chr5:10356346-10356411', 'ATTTY'),
(44, 'NUTM2B-AS1', 'chr10:79826383-79826404', 'GGC'),
(45, 'RAPGEF2', 'chr4:159342526-159342616', 'TTTYA'),
(46, 'SAMD12', 'chr8:118366812-118366918', 'TRAAA'),
(47, 'STARD7', 'chr2:96197066-96197121', 'RAAAT'),
(48, 'TNRC6A', 'chr16:24613439-24613529', 'TTTYA'),
(49, 'XYLT1', 'chr16:17470907-17470922', 'GCC'),
(50, 'YEATS2', 'chr3:183712187-183712222', 'TTTYA');
