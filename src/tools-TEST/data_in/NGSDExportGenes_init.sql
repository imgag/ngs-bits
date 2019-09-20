
INSERT INTO `gene`(`id`, `hgnc_id`, `symbol`, `name`, `type`) VALUES 
(1,1100, 'BRCA1','Breast cancer associated gene 1', 'protein-coding gene'),
(2,1002, 'BRCA2','Breast cancer associated gene 2', 'protein-coding gene'),
(3,1101, 'AAVS1','adeno-associated virus integration site 1', 'other'),
(4,37133, 'A1BG-AS1','A1BG antisense RNA 1', 'non-coding RNA'),
(5,8, 'A2MP1', 'alpha-2-macroglobulin pseudogene 1', 'pseudogene');

-- table `gene_transcript`
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
(85650, 1, 'CCDS2302.1', 'ccds', '2', 190656536, 190742162, '+');


INSERT INTO `geneinfo_germline` (`symbol`, `inheritance`, `gnomad_oe_mis`, `gnomad_oe_syn`, `gnomad_oe_lof`, `comments`) VALUES
('BRCA1', 'AD', 0.11, 0.22, 0.33, '28.04.2016 ahmustm1]\nHPO: Autosomal dominant inheritance');


INSERT INTO `hpo_term` (`id`, `hpo_id`, `name`, `definition`, `synonyms`) VALUES
(11495, 'HP:0000003', 'Multicystic kidney dysplasia', '\"Multicystic dysplasia of the kidney is characterized by multiple cysts of varying size in the kidney and the absence of a normal pelvocaliceal system. The condition is associated with ureteral or ureteropelvic atresia, and the affected kidney is nonfunctional.', ''),
(64350, 'HP:0002862', 'Bladder carcinoma', '\"The presence of a carcinoma of the urinary bladder.', '');

INSERT INTO `hpo_genes` (`hpo_term_id`, `gene`) VALUES
(11495, 'BRCA1'),
(64350, 'BRCA1');

