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

INSERT INTO `processed_sample`(`id`, `sample_id`, `process_id`, `sequencing_run_id`, `lane`, `processing_system_id`, `project_id`, `quality`) VALUES
(3999, 1, 18, 1, '1', 96, 1, 'good'),
(4000, 1, 38, 1, '2', 145, 1, 'good'),
(4001, 1, 45, 1, '1,2,3,4', 165, 1, 'good'),
(4002, 1, 46, 1, '1,2,3,4', 165, 1, 'good'),
(4007, 1, 47, 1, '1,2,3,4', 165, 1, 'bad'),
(4008, 1, 48, 1, '1,2,3,4', 165, 1, 'good');

INSERT INTO `report_configuration` (`id`, `processed_sample_id`, `created_by`, `created_date`) VALUES
(1, 4001, 99, '2020-01-01');

INSERT INTO `sv_callset` (`id`, `processed_sample_id`, `caller`, `caller_version`, `call_date`) VALUES
(1, 4001, 'Manta', '1.6.0', '2020-01-01'),
(2, 4000, 'Manta', '1.5.0', '2019-08-13'),
(3, 3999, 'Manta', '1.2.0', '2018-12-21'),
(4, 4007, 'Manta', '1.6.0', '2020-01-05'),
(5, 4008, 'Manta', '1.6.0', '2020-01-08');

INSERT INTO `merged_processed_samples` (`processed_sample_id`, `merged_into`) VALUES
(4008, 4007);

INSERT INTO `sv_deletion` (`id`, `sv_callset_id`, `chr`, `start_min`, `start_max`, `end_min`, `end_max`, `quality_metrics`) VALUES
(1, 1, 'chr1', 1000, 1020, 12000, 13000, ''),
(2, 1, 'chr1', 1000, 1020, 20000, 20000, ''),
(3, 1, 'chr1', 5, 50, 12000, 13000, ''),
(4, 2, 'chr1', 1000, 1020, 20000, 20000, ''),
(5, 3, 'chr1', 1000, 1020, 20000, 20000, ''),
(6, 3, 'chr1', 1000, 1020, 20000, 20000, ''),
(7, 1, 'chr3', 1000, 1020, 12000, 13000, ''),
(8, 1, 'chr4', 1000, 1020, 20000, 20000, ''),
(11, 4, 'chr1', 1000, 1020, 12000, 13000, ''),
(21, 4, 'chr1', 1000, 1020, 20000, 20000, ''),
(31, 4, 'chr1', 5, 50, 12000, 13000, ''),
(41, 4, 'chr1', 1000, 1020, 20000, 20000, ''),
(51, 4, 'chr1', 1000, 1020, 20000, 20000, ''),
(61, 4, 'chr1', 1000, 1020, 20000, 20000, ''),
(71, 4, 'chr3', 1000, 1020, 12000, 13000, ''),
(81, 4, 'chr4', 1000, 1020, 20000, 20000, ''),
(15, 5, 'chr1', 1000, 1020, 12000, 13000, ''),
(25, 5, 'chr1', 1000, 1020, 20000, 20000, ''),
(35, 5, 'chr1', 5, 50, 12000, 13000, ''),
(45, 5, 'chr1', 1000, 1020, 20000, 20000, ''),
(55, 5, 'chr1', 1000, 1020, 20000, 20000, ''),
(65, 5, 'chr1', 1000, 1020, 20000, 20000, ''),
(75, 5, 'chr3', 1000, 1020, 12000, 13000, ''),
(85, 5, 'chr4', 1000, 1020, 20000, 20000, '');

INSERT INTO `sv_duplication` (`id`, `sv_callset_id`, `chr`, `start_min`, `start_max`, `end_min`, `end_max`, `quality_metrics`) VALUES
(1, 1, 'chr1', 101000, 101020, 112000, 113000, ''),
(2, 1, 'chr1', 101000, 101020, 120000, 120000, ''),
(3, 1, 'chr1', 100005, 100050, 112000, 113000, ''),
(4, 2, 'chr1', 101000, 101020, 120000, 120000, ''),
(5, 3, 'chr1', 101000, 101020, 120000, 120000, ''),
(6, 3, 'chr1', 101000, 101020, 120000, 120000, ''),
(7, 1, 'chr3', 101000, 101020, 112000, 113000, ''),
(8, 1, 'chr4', 101000, 101020, 120000, 120000, ''),
(11, 4, 'chr1', 101000, 101020, 112000, 113000, ''),
(21, 4, 'chr1', 101000, 101020, 120000, 120000, ''),
(31, 4, 'chr1', 100005, 100050, 112000, 113000, ''),
(41, 4, 'chr1', 101000, 101020, 120000, 120000, ''),
(51, 4, 'chr1', 101000, 101020, 120000, 120000, ''),
(61, 4, 'chr1', 101000, 101020, 120000, 120000, ''),
(71, 4, 'chr3', 101000, 101020, 112000, 113000, ''),
(81, 4, 'chr4', 101000, 101020, 120000, 120000, ''),
(15, 5, 'chr1', 101000, 101020, 112000, 113000, ''),
(25, 5, 'chr1', 101000, 101020, 120000, 120000, ''),
(35, 5, 'chr1', 100005, 100050, 112000, 113000, ''),
(45, 5, 'chr1', 101000, 101020, 120000, 120000, ''),
(55, 5, 'chr1', 101000, 101020, 120000, 120000, ''),
(65, 5, 'chr1', 101000, 101020, 120000, 120000, ''),
(75, 5, 'chr3', 101000, 101020, 112000, 113000, ''),
(85, 5, 'chr4', 101000, 101020, 120000, 120000, '');

INSERT INTO `sv_inversion` (`id`, `sv_callset_id`, `chr`, `start_min`, `start_max`, `end_min`, `end_max`, `quality_metrics`) VALUES
(1, 1, 'chr1', 9101000, 9101020, 9112000, 9113000, ''),
(2, 1, 'chr1', 9101000, 9101020, 9120000, 9120000, ''),
(3, 1, 'chr1', 9100005, 9100050, 9112000, 9113000, ''),
(4, 2, 'chr1', 9101000, 9101020, 9120000, 9120000, ''),
(5, 3, 'chr1', 9101000, 9101020, 9120000, 9120000, ''),
(6, 3, 'chr1', 9101000, 9101020, 9120000, 9120000, ''),
(7, 1, 'chr3', 9101000, 9101020, 9112000, 9113000, ''),
(8, 1, 'chr4', 9101000, 9101020, 9120000, 9120000, '');

INSERT INTO `sv_insertion` (`id`, `sv_callset_id`, `chr`, `pos`, `ci_upper`) VALUES
(1, 1, 'chr1', 15482205, 250),
(2, 1, 'chr1', 16482455, 60),
(3, 1, 'chr1', 17482432, 77),
(4, 2, 'chr1', 15482405, 137),
(5, 3, 'chr1', 15482455, 0),
(6, 3, 'chr1', 16482443, 24),
(7, 1, 'chr3', 12482455, 0),
(8, 1, 'chr4', 11482455, 0);

INSERT INTO `sv_translocation` (`id`, `sv_callset_id`, `chr1`, `start1`, `end1`, `chr2`, `start2`, `end2`, `quality_metrics`) VALUES
(1, 1, 'chr1', 9101000, 9101020, 'chr5', 4112000, 4113000, ''),
(2, 1, 'chr1', 9101000, 9101020, 'chr5', 4120000, 4120000, ''),
(3, 1, 'chr1', 9100005, 9100050, 'chr5', 4112000, 4113000, ''),
(4, 2, 'chr1', 9101000, 9101020, 'chr5', 4120000, 4120000, ''),
(5, 3, 'chr1', 9101000, 9101020, 'chr5', 4120000, 4120000, ''),
(6, 3, 'chr1', 9101000, 9101020, 'chr5', 4120000, 4120000, ''),
(7, 1, 'chr3', 9101000, 9101020, 'chr5', 4112000, 4113000, ''),
(8, 1, 'chr4', 9101000, 9101020, 'chr5', 4120000, 4120000, ''),
(11, 1, 'chr1', 9100990, 9101010, 'chr5', 4112999, 4114000, '');

INSERT INTO `report_configuration_sv` (`id`, `report_configuration_id`, `sv_deletion_id`, `sv_duplication_id`, `sv_insertion_id`, `sv_inversion_id`, `sv_translocation_id`, `type`, `causal`, `class`, `inheritance`, `de_novo`, `mosaic`, `compound_heterozygous`, `exclude_artefact`, `exclude_frequency`, `exclude_phenotype`, `exclude_mechanism`, `exclude_other`, `comments`, `comments2`) VALUES
(1, 1, NULL, NULL, NULL, NULL, 1, 'diagnostic variant', false, '5', 'n/a', false, false, false, false, false, false, false, false, false, false),
(2, 1, NULL, NULL, NULL, 4, NULL, 'diagnostic variant', false, '4', 'n/a', false, false, false, false, false, false, false, false, false, false),
(3, 1, NULL, NULL, 7, NULL, NULL, 'diagnostic variant', false, '5', 'n/a', false, false, false, false, false, false, false, false, false, false),
(4, 1, NULL, 41, NULL, NULL, NULL, 'diagnostic variant', false, '4', 'n/a', false, false, false, false, false, false, false, false, false, false),
(5, 1, 11, NULL, NULL, NULL, NULL, 'diagnostic variant', false, '5', 'n/a', false, false, false, false, false, false, false, false, false, false),
(6, 1, NULL, NULL, NULL, NULL, 3, 'diagnostic variant', false, '3', 'n/a', false, false, false, false, false, false, false, false, false, false),
(7, 1, NULL, NULL, NULL, 2, NULL, 'diagnostic variant', false, '4', 'n/a', false, false, false, false, false, false, false, false, false, false),
(8, 1, NULL, NULL, 1, NULL, NULL, 'diagnostic variant', false, '3', 'n/a', false, false, false, false, false, false, false, false, false, false),
(11, 1, NULL, 25, NULL, NULL, NULL, 'diagnostic variant', false, '4', 'n/a', false, false, false, false, false, false, false, false, false, false),
(21, 1, 2, NULL, NULL, NULL, NULL, 'diagnostic variant', false, '5', 'n/a', false, false, false, false, false, false, false, false, false, false),
(31, 1, NULL, NULL, NULL, NULL, 4, 'diagnostic variant', false, '3', 'n/a', false, false, false, false, false, false, false, false, false, false),
(41, 1, NULL, NULL, NULL, 1, NULL, 'diagnostic variant', false, '3', 'n/a', false, false, false, false, false, false, false, false, false, false),
(51, 1, NULL, NULL, 2, NULL, NULL, 'diagnostic variant', false, '5', 'n/a', false, false, false, false, false, false, false, false, false, false),
(61, 1, NULL, 7, NULL, NULL, NULL, 'diagnostic variant', false, '5', 'n/a', false, false, false, false, false, false, false, false, false, false),
(71, 1, 81, NULL, NULL, NULL, NULL, 'diagnostic variant', false, '5', 'n/a', false, false, false, false, false, false, false, false, false, false),
(81, 1, NULL, NULL, NULL, NULL, 7, 'diagnostic variant', false, '4', 'n/a', false, false, false, false, false, false, false, false, false, false),
(15, 1, NULL, NULL, NULL, 8, NULL, 'diagnostic variant', false, '4', 'n/a', false, false, false, false, false, false, false, false, false, false),
(25, 1, NULL, NULL, 3, NULL, NULL, 'diagnostic variant', false, '3', 'n/a', false, false, false, false, false, false, false, false, false, false),
(35, 1, NULL, 75, NULL, NULL, NULL, 'diagnostic variant', false, '4', 'n/a', false, false, false, false, false, false, false, false, false, false),
(45, 1, 15, NULL, NULL, NULL, NULL, 'diagnostic variant', false, '2', 'n/a', false, false, false, false, false, false, false, false, false, false),
(55, 1, NULL, NULL, NULL, NULL, 11, 'diagnostic variant', false, '5', 'n/a', false, false, false, false, false, false, false, false, false, false);