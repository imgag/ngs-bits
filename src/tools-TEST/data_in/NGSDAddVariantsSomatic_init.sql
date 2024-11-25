/*
Also used by NGSDAddVariantsRNA
*/ 
INSERT INTO `user`(`id`, `user_id`, `password`, `user_role`, `name`, `email`, `created`, `active`) VALUES
(99, 'ahmustm1', '', 'user', 'Max Mustermann', '', '2016-07-05', 1);

INSERT INTO `device` (`id`, `type`, `name`) VALUES
(1, 'MiSeq', 'Neo');

INSERT INTO `sender` (`id`, `name`) VALUES
(1, 'oncologist');

INSERT INTO `project` (`id`, `name`, `type`, `internal_coordinator_id`, `analysis`) VALUES 
(1, 'KontrollDNACoriell', 'test', 1, 'variants');

INSERT INTO `sequencing_run` (`id`, `name`, `fcid`, `device_id`, `recipe`, `status`) VALUES
(1, '#00372', 'AB2J9', 1, '158+8+158', 'analysis_finished');


INSERT INTO `sample` (`id`, `name`, `name_external`, `sample_type`, `species_id`, `gender`, `quality`, `tumor`, `ffpe`, `sender_id`, `comment`, `disease_group`, `disease_status`) VALUES
(5, 'DX184894', 'ex5', 'DNA', 1, 'female', 'good', 1, 1, 1, 'comment_s5', 'Neoplasms', 'Affected'),
(6, 'DX184263', 'ex6', 'DNA', 1, 'female', 'good', 0, 0, 1, 'comment_s6', 'Neoplasms', 'Affected'),
(7, 'RNA184894', 'ex5', 'RNA', 1, 'female', 'good', 1, 1, 1, 'comment_s5', 'Neoplasms', 'Affected');


INSERT INTO `processing_system` (`id`, `name_short`, `name_manufacturer`, `adapter1_p5`, `adapter2_p7`, `type`, `shotgun`, `target_file`, `genome_id`) VALUES
(1, 'hpHBOCv5', 'HaloPlex HBOC v5', 'AGATCGGAAGAGCACACGTCTGAACTCCAGTCAC', 'AGATCGGAAGAGCGTCGTGTAGGGAAAGAGTGT', 'Panel Haloplex', 0, 'hpHBOCv5.bed', 1),
(2, 'nebRNAU2_qiaRRNA_umi', 'NEBNext Ultra II Directional RNA + QIAseq FastSelect rRNA removal UMI', 'AGATCGGAAGAGCACACGTCTGAACTCCAGTCAC', 'AGATCGGAAGAGCGTCGTGTAGGGAAAGAGTG', 'RNA', 1, 'nebRNAU2_qiaRRNA_2021_12_13.bed', 1);

INSERT INTO `processed_sample`(`id`, `sample_id`, `process_id`, `sequencing_run_id`, `lane`, `processing_system_id`, `project_id`, `quality`, `comment`, `normal_id`) VALUES
(7, 6, 1, '1', '1,2,3,4', 1, 1, 'good', 'comment_ps7', null),
(8, 5, 1, '1', '1,2,3,4', 1, 1, 'good', 'comment_ps8', 7),
(9, 7, 1, '1', '1,2,3,4', 1, 1, 'good', 'comment_ps9', null);