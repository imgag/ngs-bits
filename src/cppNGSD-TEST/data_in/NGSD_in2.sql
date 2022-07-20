
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

INSERT INTO `sample` (`id`, `name`, `name_external`, `sample_type`, `species_id`, `gender`, `quality`, `tumor`, `ffpe`, `sender_id`, `comment`, `disease_group`, `disease_status`) VALUES
(1, 'NA12878', 'ex1', 'DNA', 1, 'female', 'good', 0 ,0, 1, 'comment_s1', 'Diseases of the nervous system', 'Affected'),
(2, 'DX000001' , 'ext_tum_only 1', 'DNA', 1, 'male', 'good', 1, 1, 1, 'commenting stuff', 'n/a', 'n/a'),
(3, 'RX123456', 'ex1 RNA', 'RNA', 1, 'female', 'good', 0 ,0, 1, 'comment_s1', 'Diseases of the nervous system', 'Affected');

INSERT INTO `processing_system` (`id`, `name_short`, `name_manufacturer`, `adapter1_p5`, `adapter2_p7`, `type`, `shotgun`, `target_file`, `genome_id`) VALUES
(1, 'hpHSPv2', 'HaloPlex HSP v2', 'AGATCGGAAGAGCACACGTCTGAACTCCAGTCAC', 'AGATCGGAAGAGCGTCGTGTAGGGAAAGAGTGT', 'Panel Haloplex', 0, '/mnt/share/data/enrichment/hpHSP_v2_2013_12_03.bed', 1);

INSERT INTO `processed_sample`(`id`, `sample_id`, `process_id`, `sequencing_run_id`, `lane`, `processing_system_id`, `project_id`, `quality`, `comment`, `normal_id`) VALUES
(3999, 1, 3, 1, '1', 1, 1, 'medium', 'comment_ps1', null),
(4000, 2, 1, 1, '1', 1, 1, 'good', 'comment_ps2', null),
(4001, 3, 1, 1, '1', 1, 1, 'good', 'comment_ps3', null);

INSERT INTO `processed_sample_ancestry` (`processed_sample_id`, `num_snps`, `score_afr`, `score_eur`, `score_sas`, `score_eas`, `population`) VALUES
(3999, 2478, 0.0793, 0.3233, 0.2282, 0.0652, 'EUR');

INSERT INTO `diag_status`(`processed_sample_id`, `status`, `user_id`, `date`, `outcome`, `comment`) VALUES
(3999, 'done', 99, '2014-07-29 09:40:49', 'no significant findings', "free text");

-- QC infos
INSERT INTO `qc_terms`(`id`, `qcml_id`, `name`, `description`, `type`, `obsolete`) VALUES
(1, "QC:2000005", "read count", "Total number of reads (one cluster in a paired-end experiment generates two reads).", 'int', 0),
(31, "QC:2000027", "target region 20x percentage", "Percentage of the target region that is covered at...", 'float', 0),
(47, "QC:2000025", "target region read depth", "Average sequencing depth in target region.", 'float', 0),
(34, "QC:2000030", "target region 100x percentage", "Percentage of the target region that is covered at least 100-fold.", 'float', 0),
(36, "QC:2000032", "target region 500x percentage", "Percentage of the target region that is covered at least 500-fold.", 'float', 0),
(426, 'QC:2000101', 'housekeeping genes read depth', 'Average sequencing depth in exon region of housekeeping genes.', 'float', 0),
(434, 'QC:2000109', 'covered gene count', 'Number of covered genes (TPM >= 1.0)', 'int', 0);

INSERT INTO `processed_sample_qc`(`id`, `processed_sample_id`, `qc_terms_id`, `value`) VALUES
(1, 3999, 31, "95.96"),
(2, 3999, 47, "103.24"),
(3, 4000, 34, "94.7"),
(4, 4000, 36, "89.87"),
(5, 4000, 47, "210.3"),
(6, 4001, 1, "10987654"),
(7, 4001, 47, "15.85"),
(8, 4001, 426, "263.87"),
(9, 4001, 434, "136985");

INSERT INTO `sample_relations`(`sample1_id`, `relation`, `sample2_id`) VALUES
(1, 'same sample', 3);

INSERT INTO `kasp_status` (`processed_sample_id`, `random_error_prob`, `snps_evaluated`, `snps_match`) VALUES
(3999, 0.000977, 10, 10);

INSERT INTO `gaps` (`id`, `chr`, `start`, `end`, `processed_sample_id`, `status`, `history`) VALUES
(245, 'chr8', 64615713, 64615740, 3999, 'checked visually', ''),
(244, 'chr8', 64616216, 64616218, 3999, 'checked visually', ''),
(243, 'chr16', 89524046, 89524104, 3999, 'closed', '');

-- cnv_callset (we need the quality)
INSERT INTO `cnv_callset` (`id`, `processed_sample_id`, `caller`, `caller_version`, `call_date`, `quality_metrics`, `quality`) VALUES
(1, 3999, 'ClinCNV', 'v 1.16.1', '2019-10-20T09:55:01', '{"fraction of outliers":"0.052","gender of sample":"M","high-quality cnvs":"127","mean correlation to reference samples":"0.996","number of iterations":"1","quality used at final iteration":"20","was it outlier after clustering":"FALSE"}', 'good');

-- gene infos
INSERT INTO `gene` (`id`, `hgnc_id`, `symbol`, `name`, `type`) VALUES
(622167, 2652, 'CYP7B1', 'cytochrome P450 family 7 subfamily B member 1', 'protein-coding gene'),
(650913, 10985, 'SLC25A15', 'solute carrier family 25 member 15', 'protein-coding gene'),
(652410, 11237, 'SPG7', 'SPG7 matrix AAA peptidase subunit, paraplegin', 'protein-coding gene'),
(636152, 7105, 'MITF', 'melanocyte inducing transcription factor', 'protein-coding gene');

INSERT INTO `gene_transcript` (`id`, `gene_id`, `name`, `source`, `chromosome`, `start_coding`, `end_coding`, `strand`) VALUES
(1568912, 622167, 'ENST00000310193', 'ensembl', '8', 64596642, 64798587, '-'),
(1503635, 650913, 'ENST00000338625', 'ensembl', '13', 40793227, 40809667, '+'),
(1515928, 652410, 'ENST00000268704', 'ensembl', '16', 89508418, 89557093, '+'),
(1515930, 652410, 'ENST00000341316', 'ensembl', '16', 89508418, 89536910, '+'),
(1545575, 636152, 'ENST00000314557', 'ensembl', '3', 69936723, 69965248, '+'),
(1545577, 636152, 'ENST00000314589', 'ensembl', '3', 69866291, 69965248, '+'),
(1545579, 636152, 'ENST00000328528', 'ensembl', '3', 69763842, 69965248, '+');

INSERT INTO `gene_exon` (`transcript_id`, `start`, `end`) VALUES
(1503635, 40789497, 40789663),
(1503635, 40793158, 40793281),
(1503635, 40799057, 40799315),
(1503635, 40805118, 40805255),
(1503635, 40807294, 40807463),
(1503635, 40808438, 40808596),
(1503635, 40809543, 40810111),
(1515928, 89508403, 89508600),
(1515928, 89510490, 89510592),
(1515928, 89512948, 89513037),
(1515928, 89524006, 89524247),
(1515928, 89526329, 89526468),
(1515928, 89529477, 89529579),
(1515928, 89530683, 89530808),
(1515928, 89531904, 89532066),
(1515928, 89532463, 89532636),
(1515928, 89544648, 89544772),
(1515928, 89546658, 89546760),
(1515928, 89548003, 89548113),
(1515928, 89550494, 89550609),
(1515928, 89552979, 89553135),
(1515928, 89553794, 89553960),
(1515928, 89554486, 89554563),
(1515928, 89556887, 89557766),
(1515930, 89508411, 89508600),
(1515930, 89510490, 89510592),
(1515930, 89512948, 89513037),
(1515930, 89524006, 89524247),
(1515930, 89526329, 89526468),
(1515930, 89529477, 89529579),
(1515930, 89530683, 89530808),
(1515930, 89531904, 89532066),
(1515930, 89532463, 89532636),
(1515930, 89536765, 89537721),
(1568912, 64596135, 64596929),
(1568912, 64604682, 64604857),
(1568912, 64615026, 64615232),
(1568912, 64615691, 64616281),
(1568912, 64624403, 64624539),
(1568912, 64798466, 64798761),
(1545575, 69936587, 69936755),
(1545575, 69937822, 69938049),
(1545575, 69939098, 69939181),
(1545575, 69941236, 69941331),
(1545575, 69949051, 69949168),
(1545575, 69951830, 69951886),
(1545575, 69956455, 69956530),
(1545575, 69959273, 69959420),
(1545575, 69964847, 69965668),
(1545577, 69866279, 69866346),
(1545577, 69879134, 69879383),
(1545577, 69937822, 69938049),
(1545577, 69939098, 69939181),
(1545577, 69941236, 69941331),
(1545577, 69949051, 69949168),
(1545577, 69951830, 69951886),
(1545577, 69956455, 69956530),
(1545577, 69959273, 69959420),
(1545577, 69964847, 69965647),
(1545579, 69763811, 69763942),
(1545579, 69879134, 69879383),
(1545579, 69937822, 69938049),
(1545579, 69939098, 69939181),
(1545579, 69941236, 69941331),
(1545579, 69949051, 69949168),
(1545579, 69951830, 69951886),
(1545579, 69956455, 69956530),
(1545579, 69959273, 69959420),
(1545579, 69964847, 69968337);

INSERT INTO `omim_gene` (`id`, `gene`, `mim`) VALUES
(244380, 'SPG7', '602783'),
(245171, 'CYP7B1', '603711'),
(245296, 'SLC25A15', '603861'),
(50562, 'MITF', '156845');


INSERT INTO `omim_phenotype` (`omim_gene_id`, `phenotype`) VALUES
(244380, 'Spastic paraplegia 7, autosomal recessive, 607259 (3)'),
(245171, 'Bile acid synthesis defect, congenital, 3, 613812 (3)'),
(245171, 'Spastic paraplegia 5A, autosomal recessive, 270800 (3)'),
(245296, 'Hyperornithinemia-hyperammonemia-homocitrullinemia syndrome, 238970 (3)'),
(50562, 'COMMAD syndrome, 617306 (3)'),
(50562, '{Melanoma, cutaneous malignant, susceptibility to, 8}, 614456 (3)');


INSERT INTO `omim_preferred_phenotype`(`gene`, `disease_group`, `phenotype_accession`) VALUES
('CYP7B1', 'Diseases of the nervous system', '270800');