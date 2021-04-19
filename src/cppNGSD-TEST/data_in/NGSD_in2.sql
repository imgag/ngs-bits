
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
(2, 'DX000001' , 'ext_tum_only 1', 'DNA', 1, 'male', 'good', 1, 1, 1, 'commenting stuff', 'n/a', 'n/a');

INSERT INTO `processing_system` (`id`, `name_short`, `name_manufacturer`, `adapter1_p5`, `adapter2_p7`, `type`, `shotgun`, `target_file`, `genome_id`) VALUES
(1, 'hpHSPv2', 'HaloPlex HSP v2', 'AGATCGGAAGAGCACACGTCTGAACTCCAGTCAC', 'AGATCGGAAGAGCGTCGTGTAGGGAAAGAGTGT', 'Panel Haloplex', 0, '/mnt/share/data/enrichment/hpHSP_v2_2013_12_03.bed', 1);

INSERT INTO `processed_sample`(`id`, `sample_id`, `process_id`, `sequencing_run_id`, `lane`, `processing_system_id`, `project_id`, `quality`, `comment`, `normal_id`) VALUES
(3999, 1, 3, 1, '1', 1, 1, 'medium', 'comment_ps1', null),
(4000, 2, 1, 1, '1', 1, 1, 'good', 'comment_ps2', null);

INSERT INTO `diag_status`(`processed_sample_id`, `status`, `user_id`, `date`, `outcome`, `comment`) VALUES
(3999, 'done', 99, '2014-07-29 09:40:49', 'no significant findings', "free text");

-- QC infos
INSERT INTO `qc_terms`(`id`, `qcml_id`, `name`, `description`, `type`, `obsolete`) VALUES
(31, "QC:2000027", "target region 20x percentage", "Percentage of the target region that is covered at...", 'float', 0),
(47, "QC:2000025", "target region read depth", "Average sequencing depth in target region.", 'float', 0),
(34, "QC:2000030", "target region 100x percentage", "Percentage of the target region that is covered at least 100-fold.", 'float', 0),
(36, "QC:2000032", "target region 500x percentage", "Percentage of the target region that is covered at least 500-fold.", 'float', 0);

INSERT INTO `processed_sample_qc`(`id`, `processed_sample_id`, `qc_terms_id`, `value`) VALUES
(1, 3999, 31, "95.96"),
(2, 3999, 47, "103.24"),
(3, 4000, 34, "94.7"),
(4, 4000, 36, "89.87"),
(5, 4000, 47, "210.3");

INSERT INTO `kasp_status` (`processed_sample_id`, `random_error_prob`, `snps_evaluated`, `snps_match`) VALUES
(3999, 0.000977, 10, 10);

INSERT INTO `gaps` (`id`, `chr`, `start`, `end`, `processed_sample_id`, `status`, `history`) VALUES
(245, 'chr8', 65528270, 65528297, 3999, 'checked visually', ''),
(244, 'chr8', 65528773, 65528775, 3999, 'checked visually', ''),
(243, 'chr16', 89590454, 89590512, 3999, 'closed', '');

-- cnv_callset (we need the quality)
INSERT INTO `cnv_callset` (`id`, `processed_sample_id`, `caller`, `caller_version`, `call_date`, `quality_metrics`, `quality`) VALUES
(1, 3999, 'ClinCNV', 'v 1.16.1', '2019-10-20T09:55:01', '{"fraction of outliers":"0.052","gender of sample":"M","high-quality cnvs":"127","number of iterations":"1","quality used at final iteration":"20","was it outlier after clustering":"FALSE"}', 'good');

-- gene infos
INSERT INTO `gene` (`id`, `hgnc_id`, `symbol`, `name`, `type`) VALUES
(622167, 2652, 'CYP7B1', 'cytochrome P450 family 7 subfamily B member 1', 'protein-coding gene'),
(650913, 10985, 'SLC25A15', 'solute carrier family 25 member 15', 'protein-coding gene'),
(652410, 11237, 'SPG7', 'SPG7 matrix AAA peptidase subunit, paraplegin', 'protein-coding gene'),
(636152, 7105, 'MITF', 'melanocyte inducing transcription factor', 'protein-coding gene');

INSERT INTO `gene_transcript` (`id`, `gene_id`, `name`, `source`, `chromosome`, `start_coding`, `end_coding`, `strand`) VALUES
(1568912, 622167, 'ENST00000310193', 'ensembl', '8', 65509199, 65711144, '-'),
(1503635, 650913, 'ENST00000338625', 'ensembl', '13', 41367363, 41383803, '+'),
(1515928, 652410, 'ENST00000268704', 'ensembl', '16', 89574826, 89623501, '+'),
(1515930, 652410, 'ENST00000341316', 'ensembl', '16', 89574826, 89603318, '+'),
(1545575, 636152, 'ENST00000314557', 'ensembl', '3', 69985874, 70014399, '+'),
(1545577, 636152, 'ENST00000314589', 'ensembl', '3', 69915442, 70014399, '+'),
(1545579, 636152, 'ENST00000328528', 'ensembl', '3', 69812993, 70014399, '+');

INSERT INTO `gene_exon` (`transcript_id`, `start`, `end`) VALUES
(1503635, 41363633, 41363799),
(1503635, 41367294, 41367417),
(1503635, 41373193, 41373451),
(1503635, 41379254, 41379391),
(1503635, 41381430, 41381599),
(1503635, 41382574, 41382732),
(1503635, 41383679, 41384247),
(1515928, 89574811, 89575008),
(1515928, 89576898, 89577000),
(1515928, 89579356, 89579445),
(1515928, 89590414, 89590655),
(1515928, 89592737, 89592876),
(1515928, 89595885, 89595987),
(1515928, 89597091, 89597216),
(1515928, 89598312, 89598474),
(1515928, 89598871, 89599044),
(1515928, 89611056, 89611180),
(1515928, 89613066, 89613168),
(1515928, 89614411, 89614521),
(1515928, 89616902, 89617017),
(1515928, 89619387, 89619543),
(1515928, 89620202, 89620368),
(1515928, 89620894, 89620971),
(1515928, 89623295, 89624174),
(1515930, 89574819, 89575008),
(1515930, 89576898, 89577000),
(1515930, 89579356, 89579445),
(1515930, 89590414, 89590655),
(1515930, 89592737, 89592876),
(1515930, 89595885, 89595987),
(1515930, 89597091, 89597216),
(1515930, 89598312, 89598474),
(1515930, 89598871, 89599044),
(1515930, 89603173, 89604129),
(1568912, 65508692, 65509486),
(1568912, 65517239, 65517414),
(1568912, 65527583, 65527789),
(1568912, 65528248, 65528838),
(1568912, 65536960, 65537096),
(1568912, 65711023, 65711318),
(1545575, 69985738, 69985906),
(1545575, 69986973, 69987200),
(1545575, 69988249, 69988332),
(1545575, 69990387, 69990482),
(1545575, 69998202, 69998319),
(1545575, 70000981, 70001037),
(1545575, 70005606, 70005681),
(1545575, 70008424, 70008571),
(1545575, 70013998, 70014819),
(1545577, 69915430, 69915497),
(1545577, 69928285, 69928534),
(1545577, 69986973, 69987200),
(1545577, 69988249, 69988332),
(1545577, 69990387, 69990482),
(1545577, 69998202, 69998319),
(1545577, 70000981, 70001037),
(1545577, 70005606, 70005681),
(1545577, 70008424, 70008571),
(1545577, 70013998, 70014798),
(1545579, 69812962, 69813093),
(1545579, 69928285, 69928534),
(1545579, 69986973, 69987200),
(1545579, 69988249, 69988332),
(1545579, 69990387, 69990482),
(1545579, 69998202, 69998319),
(1545579, 70000981, 70001037),
(1545579, 70005606, 70005681),
(1545579, 70008424, 70008571),
(1545579, 70013998, 70017488);

INSERT INTO `omim_gene` (`id`, `gene`, `mim`) VALUES
(244380, 'SPG7', '602783'),
(245171, 'CYP7B1', '603711'),
(245296, 'SLC25A15', '603861');


INSERT INTO `omim_phenotype` (`omim_gene_id`, `phenotype`) VALUES
(244380, 'Spastic paraplegia 7, autosomal recessive, 607259 (3)'),
(245171, 'Bile acid synthesis defect, congenital, 3, 613812 (3)'),
(245171, 'Spastic paraplegia 5A, autosomal recessive, 270800 (3)'),
(245296, 'Hyperornithinemia-hyperammonemia-homocitrullinemia syndrome, 238970 (3)');


INSERT INTO `omim_preferred_phenotype`(`gene`, `disease_group`, `phenotype_accession`) VALUES
('CYP7B1', 'Diseases of the nervous system', '270800');