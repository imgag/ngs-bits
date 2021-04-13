INSERT INTO `gene`(`id`, `hgnc_id`, `symbol`, `name`, `type`) VALUES 
(1,1100, 'BRCA1','Breast cancer associated gene 1', 'protein-coding gene'),
(658347, 26573, 'ZNF597', 'zinc finger protein 597', 'protein-coding gene'),
(616286, 21298, 'AACS', 'acetoacetyl-CoA synthetase', 'protein-coding gene'),
(616287, 18226, 'AACSP1', 'acetoacetyl-CoA synthetase pseudogene 1', 'pseudogene');

INSERT INTO `gene_transcript` (`id`, `gene_id`, `name`, `source`, `chromosome`, `start_coding`, `end_coding`, `strand`) VALUES
(39236, 1, 'uc010zfz.1', 'ensembl', '2', 190656536, 190670560, '+'),
(39237, 1, 'uc010zga.1', 'ensembl', '2', 190656536, 190720597, '+'),
(39238, 1, 'uc010zgb.1', 'ensembl', '2', 190656536, 190728952, '+'),
(39239, 1, 'uc002urh.4', 'ensembl', '2', 190656536, 190742162, '+'),
(39240, 1, 'uc002urk.4', 'ensembl', '2', 190656536, 190742162, '+'),
(39241, 1, 'uc002uri.4', 'ensembl', '2', 190656536, 190742162, '+'),
(39242, 1, 'uc010zgc.2', 'ensembl', '2', 190682853, 190742162, '+'),
(39243, 1, 'uc010zgd.2', 'ensembl', '2', 190682853, 190742162, '+'),
(39244, 1, 'uc002urj.3', 'ensembl', '2', null, null, '+'),
(39245, 1, 'uc010fry.1', 'ensembl', '2', 190656536, 190738254, '+'),
(39246, 1, 'uc010frz.3', 'ensembl', '2', 190656536, 190742162, '+'),
(39247, 1, 'uc002url.3', 'ensembl', '2', 190682853, 190742162, '+'),
(39248, 1, 'uc002urm.3', 'ensembl', '2', null, null, '+'),
(39249, 1, 'uc002urn.1', 'ensembl', '2', 190718995, 190728841, '+'),
(85648, 1, 'CCDS46474.1', 'ccds', '2', 190656536, 190742162, '+'),
(85649, 1, 'CCDS46473.1', 'ccds', '2', 190656536, 190742162, '+'),
(85650, 1, 'CCDS2302.1', 'ccds', '2', 190656536, 190742162, '+'),
(1512692, 658347, 'ENST00000301744', 'ensembl', '16', 3486424, 3493153, '-'),
(1512693, 658347, 'CCDS10505.1', 'ccds', '16', 3486424, 3493153, '-'),
(1502676, 616286, 'ENST00000261686', 'ensembl', '12', 125550131, 125626759, '+'),
(1502677, 616286, 'ENST00000316519', 'ensembl', '12', 125550131, 125626775, '+'),
(1502678, 616286, 'CCDS9263.1', 'ccds', '12', 125550131, 125626775, '+'),
(1502679, 616286, 'ENST00000316543', 'ensembl', '12', 125609468, 125626775, '+'),
(1502680, 616286, 'ENST00000543665', 'ensembl', '12', 125613959, 125626775, '+'),
(1502681, 616286, 'ENST00000545511', 'ensembl', '12', 125609523, 125626775, '+'),
(1556965, 616287, 'ENST00000503486', 'ensembl', '5', NULL, NULL, '-');

INSERT INTO `geneinfo_germline` (`symbol`, `inheritance`, `gnomad_oe_mis`, `gnomad_oe_syn`, `gnomad_oe_lof`, `comments`) VALUES
('BRCA1', 'AD', 0.11, 0.22, 0.33, '28.04.2016 ahmustm1]\nHPO: Autosomal dominant inheritance'),
('ZNF597', 'n/a', 1.1, 1.1, 0.38, ''),
('AACS', 'AR', 0.94, 0.97, 0.8, '');

INSERT INTO `gene_pseudogene_relation` (`id`, `parent_gene_id`, `pseudogene_gene_id`, `gene_name`) VALUES
(899, 616286, 616287, NULL);

-------------- disease infos -----------------

INSERT INTO `hpo_term` (`id`, `hpo_id`, `name`, `definition`, `synonyms`) VALUES
(11495, 'HP:0000003', 'Multicystic kidney dysplasia', '\"Multicystic dysplasia of the kidney is characterized by multiple cysts of varying size in the kidney and the absence of a normal pelvocaliceal system. The condition is associated with ureteral or ureteropelvic atresia, and the affected kidney is nonfunctional.', ''),
(64350, 'HP:0002862', 'Bladder carcinoma', '\"The presence of a carcinoma of the urinary bladder.', '');

INSERT INTO `hpo_genes` (`hpo_term_id`, `gene`) VALUES
(11495, 'BRCA1'),
(64350, 'BRCA1');

INSERT INTO `omim_gene` (`id`, `gene`, `mim`) VALUES
(239462, 'BRCA1', '113705');

INSERT INTO `omim_phenotype` (`omim_gene_id`, `phenotype`) VALUES
(239462, 'Fanconi anemia, complementation group S, 617883 (3)'),
(239462, '{Breast-ovarian cancer, familial, 1}, 604370 (3)'),
(239462, '{Pancreatic cancer, susceptibility to, 4}, 614320 (3)');

INSERT INTO `disease_term` (`id`, `source`, `identifier`, `name`, `synonyms`) VALUES
(127612, 'OrphaNet', 'ORPHA:168829', 'Primary peritoneal carcinoma', 'EOPPC\nExtra-ovarian primary peritoneal carcinoma\nPPC\nPrimary peritoneal serous carcinoma\nSerous surface papillary carcinoma'),
(127898, 'OrphaNet', 'ORPHA:84', 'Fanconi anemia', 'Fanconi pancytopenia'),
(128326, 'OrphaNet', 'ORPHA:1331', 'Familial prostate cancer', NULL),
(131032, 'OrphaNet', 'ORPHA:213524', 'Hereditary site-specific ovarian cancer syndrome', NULL),
(131268, 'OrphaNet', 'ORPHA:145', 'Hereditary breast and ovarian cancer syndrome', NULL),
(131331, 'OrphaNet', 'ORPHA:227535', 'Hereditary breast cancer', 'Familial breast cancer\nFamilial breast carcinoma\nHereditary breast carcinoma'),
(131381, 'OrphaNet', 'ORPHA:1333', 'Familial pancreatic carcinoma', 'Familial pancreatic cancer');

INSERT INTO `disease_gene` (`disease_term_id`, `gene`) VALUES
(127612, 'BRCA1'),
(127898, 'BRCA1'),
(128326, 'BRCA1'),
(131032, 'BRCA1'),
(131268, 'BRCA1'),
(131331, 'BRCA1'),
(131381, 'BRCA1');
