
INSERT INTO `user`(`id`, `user_id`, `password`, `user_role`, `name`, `email`, `created`, `active`, `salt`) VALUES
(99, 'ahmustm1', '4bc26c3f212efe28ad8a58d655afb3f1dabc8eb9', 'user', 'Max Mustermann', 'no.mail@max.de', '2016-07-05', 1, 'salt123456salt123456salt123456salt123456');


INSERT INTO `device` (`id`, `type`, `name`) VALUES
(1, 'MiSeq', 'Neo');

INSERT INTO `sender` (`id`, `name`) VALUES
(1, 'Coriell');

INSERT INTO `project` (`id`, `name`, `type`, `internal_coordinator_id`, `analysis`) VALUES
(1, 'KontrollDNACoriell', 'test', 1, 'variants');

INSERT INTO `sequencing_run` (`id`, `name`, `fcid`, `device_id`, `recipe`, `status`) VALUES
(1, '#00372', 'AB2J9', 1, '158+8+158', 'analysis_finished');

INSERT INTO `sample` (`id`, `name`, `name_external`, `sample_type`, `tissue`, `species_id`, `gender`, `quality`, `tumor`, `ffpe`, `sender_id`, `comment`, `disease_group`, `disease_status`) VALUES
(1, 'RX001', 'ex1', 'RNA', 'Skin', 1, 'female', 'good', 0 ,0, 1, 'comment_s1', 'n/a', 'Affected'),
(2, 'RX002', 'ex2', 'RNA', 'Skin', 1, 'female', 'good', 0 ,0, 1, 'comment_s2', 'n/a', 'Affected'),
(3, 'RX003', 'ex3', 'RNA', 'Skin', 1, 'male', 'good', 0 ,0, 1, 'comment_s3', 'n/a', 'Affected'),
(4, 'RX004', 'ex4', 'RNA', 'Skin', 1, 'male', 'good', 0 ,0, 1, 'comment_s4', 'n/a', 'Affected'),
(5, 'RX005', 'ex5', 'RNA', 'Blood', 1, 'female', 'good', 0 ,0, 1, 'comment_s5', 'n/a', 'Affected'),
(6, 'RX006', 'ex6', 'RNA', 'Blood', 1, 'female', 'good', 0 ,0, 1, 'comment_s6', 'n/a', 'Affected'),
(7, 'RX007', 'ex7', 'RNA', 'Blood', 1, 'male', 'good', 0 ,0, 1, 'comment_s7', 'n/a', 'Affected'),
(8, 'RX008', 'ex8', 'RNA', 'Blood', 1, 'male', 'good', 0 ,0, 1, 'comment_s8', 'n/a', 'Affected');

INSERT INTO `processing_system` (`id`, `name_short`, `name_manufacturer`, `adapter1_p5`, `adapter2_p7`, `type`, `shotgun`, `target_file`, `genome_id`) VALUES
(1, 'nebRNAU2_mrna', 'NEBNext Ultra II Directional RNA mRNA', 'AGATCGGAAGAGCACACGTCTGAACTCCAGTCAC', 'AGATCGGAAGAGCGTCGTGTAGGGAAAGAGTGT', 'RNA', 1, 'nebRNAU2_mrna_2021_12_13.bed', 1);

INSERT INTO `processed_sample`(`id`, `sample_id`, `process_id`, `sequencing_run_id`, `lane`, `processing_system_id`, `project_id`, `quality`, `comment`, `normal_id`) VALUES
(5001, 1, 1, 1, '1', 1, 1, 'medium', 'comment_ps1', null),
(5002, 2, 1, 1, '1', 1, 1, 'good', 'comment_ps2', null),
(5003, 3, 1, 1, '1', 1, 1, 'medium', 'comment_ps3', null),
(5004, 4, 1, 1, '1', 1, 1, 'good', 'comment_ps4', null),
(5005, 5, 1, 1, '1', 1, 1, 'medium', 'comment_ps5', null),
(5006, 6, 1, 1, '1', 1, 1, 'good', 'comment_ps6', null),
(5007, 7, 1, 1, '1', 1, 1, 'medium', 'comment_ps7', null),
(5008, 8, 1, 1, '1', 1, 1, 'good', 'comment_ps8', null);

