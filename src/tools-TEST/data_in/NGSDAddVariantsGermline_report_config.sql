--report config
INSERT INTO `report_configuration` (`id`, `processed_sample_id`, `created_by`, `created_date`) VALUES
(1, 4001, 1, '2000-01-01 00:00:00');

INSERT INTO `variant` (`id`, `chr`, `start`, `end`, `ref`, `obs`, `gnomad`, `coding`, `comment`) VALUES
(1, 'chr1', 115256554, 115256554, 'A', 'G', NULL, 'NRAS:ENST00000369535:synonymous_variant:LOW:exon3/7:c.157T>C:p.Leu53=:PF00071', NULL);

INSERT INTO `report_configuration_variant` (`id`, `report_configuration_id`, `variant_id`, `type`, `causal`, `inheritance`, `de_novo`, `mosaic`, `compound_heterozygous`, `exclude_artefact`, `exclude_frequency`, `exclude_phenotype`, `exclude_mechanism`, `exclude_other`, `comments`, `comments2`) VALUES
(1, 1, 1, 'diagnostic variant', 1, 'n/a', 0, 0, 0, 0, 0, 0, 0, 0, '', '');


INSERT INTO `cnv_callset` (`id`, `processed_sample_id`, `caller`, `caller_version`, `call_date`, `quality_metrics`, `quality`) VALUES
(1, 4001, 'ClinCNV', 'v1.16.4', '2019-08-26 14:21:42', '', 'good');

INSERT INTO `cnv` (`id`, `cnv_callset_id`, `chr`, `start`, `end`, `cn`, `quality_metrics`) VALUES
(1, 1, 'chr1', 17248493, 17275441, 1, '');

INSERT INTO `report_configuration_cnv` (`id`, `report_configuration_id`, `cnv_id`, `type`, `causal`, `class`, `inheritance`, `de_novo`, `mosaic`, `compound_heterozygous`, `exclude_artefact`, `exclude_frequency`, `exclude_phenotype`, `exclude_mechanism`, `exclude_other`, `comments`, `comments2`) VALUES
(1, 1, 1, 'diagnostic variant', 0, 'n/a', 'AD', 0, 0, 0, 0, 0, 1, 0, 0, '', '');


INSERT INTO `sv_callset` (`id`, `processed_sample_id`, `caller`, `caller_version`, `call_date`) VALUES
(1, 4001, 'Manta', '1.6.0', '2020-01-01');

INSERT INTO `sv_deletion` (`id`, `sv_callset_id`, `chr`, `start_min`, `start_max`, `end_min`, `end_max`, `quality_metrics`) VALUES
(1, 1, 'chr1', 1000, 1020, 12000, 13000, '');

INSERT INTO `report_configuration_sv` (`id`, `report_configuration_id`, `sv_deletion_id`, `sv_duplication_id`, `sv_insertion_id`, 
            `sv_inversion_id`, `sv_translocation_id`, `type`, `causal`, `class`, `inheritance`, `de_novo`, `mosaic`, `compound_heterozygous`, 
            `exclude_artefact`, `exclude_frequency`, `exclude_phenotype`, `exclude_mechanism`, `exclude_other`, `comments`, `comments2`) VALUES
(1, 1, 1, NULL, NULL, NULL, NULL, 'diagnostic variant', 1, '4', 'n/a', false, false, false, false, false, false, false, false, 'comment1', 'comment2');
