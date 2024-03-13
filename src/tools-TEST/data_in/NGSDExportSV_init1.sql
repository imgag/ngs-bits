INSERT INTO `user`(`id`, `user_id`, `password`, `user_role`, `name`, `email`, `created`, `active`, `salt`) VALUES
(99, 'ahmustm1', '4bc26c3f212efe28ad8a58d655afb3f1dabc8eb9', 'user', 'Max Mustermann', 'no.mail@max.de', '2016-07-05', 1, 'salt123456salt123456salt123456salt123456'),
(101, 'ahkerra1', '', 'user', 'Sarah Kerrigan', 'queen_of_blades@the_swarm.org', '1998-03-31', 0, NULL);


INSERT INTO `device` (`id`, `type`, `name`) VALUES
(1, 'MiSeq', 'Neo');

INSERT INTO `sender` (`id`, `name`) VALUES
(1, 'Coriell');

INSERT INTO `project` (`id`, `name`, `type`, `internal_coordinator_id`, `analysis`) VALUES
(1, 'KontrollDNACoriell', 'test', 1, 'variants');

INSERT INTO `sequencing_run` (`id`, `name`, `fcid`, `start_date`, `device_id`, `recipe`, `status`) VALUES
(1, '#00372', 'AB2J9', '2021-02-19', 1, '158+8+158', 'analysis_finished');

INSERT INTO `sample` (`id`, `name`, `name_external`, `sample_type`, `species_id`, `gender`, `quality`, `tumor`, `ffpe`, `sender_id`, `comment`, `disease_group`, `disease_status`) VALUES
(1, 'NA12878', 'ex1', 'DNA', 1, 'female', 'good', 0 ,0, 1, 'comment_s1', 'Diseases of the blood or blood-forming organs', 'Unaffected'),
(2, 'NA12123', 'ex2', 'DNA', 1, 'female', 'good', 0 ,0, 1, 'comment_s2', 'Neoplasms', 'Affected'),
(3, 'NA12345', 'ex3', 'DNA', 1, 'male', 'bad', 1 ,1, 1, 'comment_s3', 'Diseases of the immune system', 'Affected'),
(4, 'NA12123repeat', 'ex4', 'DNA', 1, 'female', 'good', 0 ,0, 1, 'comment_s4', 'Neoplasms', 'Affected'),
(5, 'DX184894', 'ex5', 'DNA', 1, 'female', 'good', 1, 1, 1, 'comment_s5', 'Neoplasms', 'Affected'),
(6, 'DX184263', 'ex6', 'DNA', 1, 'female', 'good', 0, 0, 1, 'comment_s6', 'Neoplasms', 'Affected');

INSERT INTO `processing_system` (`id`, `name_short`, `name_manufacturer`, `adapter1_p5`, `adapter2_p7`, `type`, `shotgun`, `target_file`, `genome_id`) VALUES
(1, 'hpHBOCv5', 'HaloPlex HBOC v5', 'AGATCGGAAGAGCACACGTCTGAACTCCAGTCAC', 'AGATCGGAAGAGCGTCGTGTAGGGAAAGAGTGT', 'Panel Haloplex', 0, 'hpHBOCv5.bed', 1),
(2, 'hpHBOCv6', 'HaloPlex HBOC v6', 'AGATCGGAAGAGCACACGTCTGAACTCCAGTCAC', 'AGATCGGAAGAGCGTCGTGTAGGGAAAGAGTGT', 'Panel Haloplex', 0, 'hpHBOCv6.bed', 1),
(3, 'ssSC_vTEST', 'SureSelect Somatic vTEST', 'AGATCGGAAGAGCACACGTCTGAACTCCAGTCAC', 'AGATCGGAAGAGCGTCGTGTAGGGAAAGAGTGT', 'Panel', 1, '/mnt/share/data/enrichment/ssSC_test.bed', 1),
(4, 'IDT_xGenPrism', 'IDT xGen Human ID + IDT xGen Prism DNA', 'AGATCGGAAGAGCACACGTCTGAACTCCAGTCA', 'AGATCGGAAGAGCGTCGTGTAGGGAAAGAGTGT', 'cfDNA (patient-specific)', 1, 'idt_HumanID.bed', 1);

INSERT INTO `processed_sample`(`id`, `sample_id`, `process_id`, `sequencing_run_id`, `lane`, `processing_system_id`, `project_id`, `quality`, `comment`, `normal_id`) VALUES
(3999, 1, 3, 1, '1', 1, 1, 'medium', 'comment_ps1', null),
(4000, 1, 4, 1, '1', 1, 1, 'medium', 'comment_ps2', null),
(4001, 2, 4, 1, '1', 1, 1, 'medium', 'comment_ps3', null),
(4002, 3, 1, 1, '1', 1, 1, 'good', 'comment_ps4', 3999),
(4003, 4, 1, 1, '1', 1, 1, 'good', 'comment_ps4', null),
(5, 2, 23, 1, '1', 1, 1, 'medium', 'comment_ps5', null),
(6, 3, 44, 1, '1', 1, 1, 'medium', 'comment_ps6', null),
(7, 6, 1, '1', '1,2,3,4', 3, 1, 'good', 'comment_ps7', null),
(8, 5, 1, '1', '1,2,3,4', 3, 1, 'good', 'comment_ps8', 7);

-- structural variants
INSERT INTO `sv_callset` (`id`, `processed_sample_id`, `caller`, `caller_version`, `call_date`) VALUES
(1, 3999, 'Manta', '1.6.0', '2020-01-01'),
(2, 4000, 'Manta', '1.6.0', '2020-01-02'),
(3, 4001, 'Manta', '1.6.0', '2020-01-03'),
(4, 4002, 'Manta', '1.6.0', '2020-01-04');

INSERT INTO `sv_deletion` (`id`, `sv_callset_id`, `chr`, `start_min`, `start_max`, `end_min`, `end_max`, `quality_metrics`) VALUES
(1, 1, 'chr1', 1000, 1020, 12000, 13000, ''),
(2, 1, 'chr1', 1000, 1020, 20000, 20000, ''),
(3, 1, 'chr1', 5, 50, 12000, 13000, ''),
(4, 2, 'chr1', 1000, 1020, 12000, 13000, ''),
(5, 2, 'chr1', 1000, 1020, 20000, 20000, ''),
(6, 3, 'chr1', 5, 50, 12000, 13000, '');

INSERT INTO `sv_duplication` (`id`, `sv_callset_id`, `chr`, `start_min`, `start_max`, `end_min`, `end_max`, `quality_metrics`) VALUES
(1, 1, 'chr1', 101000, 101020, 112000, 113000, ''),
(2, 1, 'chr1', 101000, 101020, 120000, 120000, ''),
(3, 1, 'chr1', 100005, 100050, 112000, 113000, '');

INSERT INTO `sv_inversion` (`id`, `sv_callset_id`, `chr`, `start_min`, `start_max`, `end_min`, `end_max`, `quality_metrics`) VALUES
(1, 1, 'chr1', 9101000, 9101020, 9112000, 9113000, ''),
(2, 1, 'chr1', 9101000, 9101020, 9120000, 9120000, ''),
(3, 1, 'chr1', 9100005, 9100050, 9112000, 9113000, ''),
(4, 3, 'chr1', 9101000, 9101020, 9112000, 9113000, ''),
(5, 3, 'chr1', 9101000, 9101020, 9120000, 9120000, ''),
(6, 3, 'chr1', 9100005, 9100050, 9112000, 9113000, '');

INSERT INTO `sv_insertion` (`id`, `sv_callset_id`, `chr`, `pos`, `ci_upper`) VALUES
(1, 1, 'chr1', 15482205, 250),
(2, 1, 'chr1', 16482455, 60),
(3, 1, 'chr1', 17482432, 77),
(4, 4, 'chr1', 15482205, 250),
(5, 4, 'chr1', 16482455, 60),
(6, 4, 'chr1', 17482432, 77);

INSERT INTO `sv_translocation` (`id`, `sv_callset_id`, `chr1`, `start1`, `end1`, `chr2`, `start2`, `end2`, `quality_metrics`) VALUES
(1, 1, 'chr1', 9101000, 9101020, 'chr5', 4112000, 4113000, ''),
(2, 1, 'chr1', 9101000, 9101020, 'chr5', 4120000, 4120000, ''),
(3, 1, 'chr1', 9100005, 9100050, 'chr5', 4112000, 4113000, '');

