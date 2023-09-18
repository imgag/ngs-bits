
INSERT INTO `user`(`id`, `user_id`, `password`, `user_role`, `name`, `email`, `created`, `active`, `salt`) VALUES
(99, 'ahmustm1', '4bc26c3f212efe28ad8a58d655afb3f1dabc8eb9', 'user', 'Max Mustermann', 'no.mail@max.de', '2016-07-05', 1, 'salt123456salt123456salt123456salt123456');


INSERT INTO `device` (`id`, `type`, `name`) VALUES
(1, 'NovaSeqXPlus', 'Luna');

INSERT INTO `sender` (`id`, `name`) VALUES
(1, 'Coriell');

INSERT INTO `project` (`id`, `name`, `type`, `internal_coordinator_id`, `analysis`) VALUES
(1, 'NovaSeqX_Genomes', 'test', 1, 'variants'),
(2, 'NovaSeqX_Exomes', 'test', 1, 'variants'),
(3, 'NovaSeqX_RNA', 'test', 1, 'variants');

INSERT INTO `sequencing_run` (`id`, `name`, `fcid`, `device_id`, `recipe`, `status`) VALUES
(1, '#09876', 'FLO3C3LL', 1, '151+19+10+151', 'run_finished');

INSERT INTO `sample` (`id`, `name`, `name_external`, `sample_type`, `species_id`, `gender`, `quality`, `tumor`, `ffpe`, `sender_id`, `comment`, `disease_group`, `disease_status`) VALUES
(1, 'NA12878', 'ex1', 'DNA', 1, 'female', 'good', 0 ,0, 1, 'comment_s1', 'Diseases of the nervous system', 'Affected'),
(2, 'DX000001' , 'ext_tum 1', 'DNA', 1, 'male', 'good', 1, 1, 1, 'commenting stuff', 'n/a', 'n/a'),
(3, 'DX000002' , 'ext_normal 1', 'DNA', 1, 'male', 'good', 1, 1, 1, 'commenting stuff', 'n/a', 'n/a'),
(4, 'RX123456', 'ex1 RNA', 'RNA', 1, 'female', 'good', 0 ,0, 1, 'comment_s1', 'Diseases of the nervous system', 'Affected');

INSERT INTO `processing_system` (`id`, `name_short`, `name_manufacturer`, `adapter1_p5`, `adapter2_p7`, `type`, `shotgun`, `umi_type`, `target_file`, `genome_id`) VALUES
(1, 'TruSeqPCRfree', 'TruSeq DNA PCR-Free', 'AGATCGGAAGAGCACACGTCTGAACTCCAGTCAC', 'AGATCGGAAGAGCGTCGTGTAGGGAAAGAGTGT', 'WGS', 1, 'n/a', 'WGS_hg38.bed', 1),
(2, 'twistCustomExomeV2', 'Twist Custom Exome IMGAG V2 (TE-94167158)', 'AGATCGGAAGAGCACACGTCTGAACTCCAGTCA', 'AGATCGGAAGAGCGTCGTGTAGGGAAAGAGTGT', 'WES', 1, 'n/a', 'twistCustomExomeV2_2021_12_14.bed', 1),
(3, 'nebRNAU2_qiaRRNA_umi', 'NEBNext Ultra II Directional RNA + QIAseq FastSelect rRNA removal UMI', 'AGATCGGAAGAGCACACGTCTGAACTCCAGTCAC', 'AGATCGGAAGAGCGTCGTGTAGGGAAAGAGTG', 'RNA', 1, 'IDT-UDI-UMI', 'nebRNAU2_qiaRRNA_2021_12_13.bed', 1);

INSERT INTO `mid` (`id`, `name`, `sequence`) VALUES
( 1, 'TruSeq_i7_01', "CGATCGAT"),
( 2, 'TruSeq_i5_01', "ATCGATCG"),
( 3, 'Twist_i7_01', "CGATCGATCG"),
( 4, 'Twist_i5_01', "ATCGATCGAT"),
( 5, 'Twist_i7_02', "AAAAATTTTT"),
( 6, 'Twist_i5_02', "CCCCCGGGGG"),
( 7, 'Twist_i7_03', "TTTTTAAAAA"),
( 8, 'Twist_i5_03', "GGGGGCCCCC"),
( 9, 'Twist_i7_04', "TTTTTCCCCC"),
(10, 'Twist_i5_04', "GGGGGAAAAA"),
(11, 'Twist_i7_05', "TATTATTATT"),
(12, 'Twist_i5_05', "GCGGCGGCGG"),
(13, 'NEB_i7_01', "ATATATAT"),
(14, 'NEB_i5_01', "CGCGCGCG");

INSERT INTO `processed_sample`(`id`, `sample_id`, `process_id`, `sequencing_run_id`, `lane`, `mid1_i7`, `mid2_i5`, `processing_system_id`, `project_id`, `quality`, `comment`, `normal_id`) VALUES
(123, 1, 45, 1, '1,2', 1, 2, 1, 1, 'n/a', 'WGS sample', null),
(124, 1, 58, 1, '3', 3, 4, 2, 2, 'n/a', 'WES sample', null),
(125, 1, 59, 1, '3', 5, 6, 2, 2, 'n/a', 'WES sample', null),
(126, 3, 1, 1, '4', 7, 8, 2, 2, 'n/a', 'WES Normal sample', null),
(127, 2, 1, 1, '4', 9, 10, 2, 2, 'n/a', 'WES Tumor sample', 126),
(128, 4, 1, 1, '5', 13, 14, 3, 3, 'n/a', 'RNA sample', null);
