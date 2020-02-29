
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
(145, 'ssHAEv6', 'SureSelectXT Human All Exon V6', 'AGATCGGAAGAGCACACGTCTGAACTCCAGTCAC', 'AGATCGGAAGAGCGTCGTGTAGGGAAAGAGTGT', 'WES', 1, 'n/a', '/some_path/ssHAEv6_2019_07_19.bed', 1);

INSERT INTO `processed_sample`(`id`, `sample_id`, `process_id`, `sequencing_run_id`, `lane`, `processing_system_id`, `project_id`) VALUES
(3999, 1, 18, 1, '1', 96, 1),
(4000, 1, 38, 1, '2', 145, 1);

INSERT INTO `report_configuration` (`id`, `processed_sample_id`, `created_by`, `created_date`) VALUES
(1, 4000, 1, '2000-01-01 00:00:00');

INSERT INTO `cnv_callset` (`id`, `processed_sample_id`, `caller`, `caller_version`) VALUES 
(1, 4000, 'ClinCNV', '1.16.2');

INSERT INTO `cnv` (`id`, `cnv_callset_id`, `chr`, `start`, `end`, `cn`) VALUES
(1, 1, 'chr1', 1000000, 1200000, 6),
(2, 1, 'chr1', 2000000, 2200000, 6),
(3, 1, 'chr1', 2000000, 3000000, 3),
(4, 1, 'chr1', 5000000, 6000000, 1),
(5, 1, 'chr1', 5000000, 6000000, 1);

INSERT INTO `report_configuration_cnv` (`id`, `report_configuration_id`, `cnv_id`, `type`, `causal`, `class`, `inheritance`, `de_novo`, `mosaic`, `compound_heterozygous`, `exclude_artefact`, `exclude_frequency`, `exclude_phenotype`, `exclude_mechanism`, `exclude_other`, `comments`, `comments2`) VALUES
(1, 1, 1, 'diagnostic variant', false, '5', 'n/a', false, false, false, false, false, false, false, false, false, false),
(2, 1, 2, 'diagnostic variant', false, '5', 'n/a', false, false, false, false, false, false, false, false, false, false),
(3, 1, 3, 'diagnostic variant', false, '4', 'n/a', false, false, false, false, false, false, false, false, false, false),
(4, 1, 4, 'diagnostic variant', false, '5', 'n/a', false, false, false, false, false, false, false, false, false, false),
(5, 1, 5, 'diagnostic variant', false, '4', 'n/a', false, false, false, false, false, false, false, false, false, false);


