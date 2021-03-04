
INSERT INTO `geneinfo_germline`(`symbol`, `inheritance`, `gnomad_oe_mis`, `gnomad_oe_syn`, `gnomad_oe_lof`, `comments`) VALUES
('BRCA1', 'AD', 0.00, 0.00, 0.00, 'valid => keep'),
('RNF53', 'AD', NULL, NULL, NULL, 'old name of BRCA1 => delete'),
('BLABLA', 'AD', NULL, NULL, NULL, 'unknown => delete'),
('FANCD1', 'AD', NULL, NULL, NULL, 'old name of BRCA2 => update');


INSERT INTO `somatic_gene_role` (`symbol`, `gene_role`, `high_evidence`, `comment`) VALUES
('BRCA1', 'loss_of_function', true, 'valid => keep'),
('RNF53', 'loss_of_function', true, 'old name of BRCA1 => delete'),
('BLABLA', 'ambiguous', false, 'unknown => delete'),
('FANCD1', 'loss_of_function', true, 'old name of BRCA2 => update');


