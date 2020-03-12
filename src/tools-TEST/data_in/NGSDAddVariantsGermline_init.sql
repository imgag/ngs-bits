
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
