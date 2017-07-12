
-- table `gene`
INSERT INTO `gene` (`id`, `hgnc_id`, `symbol`, `name`, `type`) VALUES
(1, 1101, 'BRCA2', 'breast cancer 2, early onset', 'protein-coding gene'),
(2, 1102, 'DUMMY', 'bal', 'protein-coding gene'),
(22712, 9121, 'PMS1', 'PMS1 homolog 1, mismatch repair system component', 'protein-coding gene');

-- table `gene_transcript`
INSERT INTO `gene_transcript` (`id`, `gene_id`, `name`, `source`, `chromosome`, `start_coding`, `end_coding`, `strand`) VALUES
(1, 1, 'uc001uua.1', 'ensembl', '13', 32899266, 32907523, '+'),
(2, 1, 'uc001uub.1', 'ensembl', '13', 32890598, 32972907, '+'),
(3, 1, 'uc031qky.1', 'ensembl', '13', 32929167, 32936796, '+'),
(4, 1, 'uc031qkz.1', 'ensembl', '13', null, null, '+'),
(5, 1, 'CCDS9344.1', 'ccds', '13', 32890598, 32972907, '+'),
(6, 2, 'uc031qkz.1', 'ensembl', '13', 32945090, 32945093, '+'),
(39236, 22712, 'uc010zfz.1', 'ensembl', '2', 190656536, 190670560, '+'),
(39237, 22712, 'uc010zga.1', 'ensembl', '2', 190656536, 190720597, '+'),
(39238, 22712, 'uc010zgb.1', 'ensembl', '2', 190656536, 190728952, '+'),
(39239, 22712, 'uc002urh.4', 'ensembl', '2', 190656536, 190742162, '+'),
(39240, 22712, 'uc002urk.4', 'ensembl', '2', 190656536, 190742162, '+'),
(39241, 22712, 'uc002uri.4', 'ensembl', '2', 190656536, 190742162, '+'),
(39242, 22712, 'uc010zgc.2', 'ensembl', '2', 190682853, 190742162, '+'),
(39243, 22712, 'uc010zgd.2', 'ensembl', '2', 190682853, 190742162, '+'),
(39245, 22712, 'uc010fry.1', 'ensembl', '2', 190656536, 190738254, '+'),
(39246, 22712, 'uc010frz.3', 'ensembl', '2', 190656536, 190742162, '+'),
(39247, 22712, 'uc002url.3', 'ensembl', '2', 190682853, 190742162, '+'),
(39248, 22712, 'uc002urm.3', 'ensembl', '2', null, null, '+'),
(39249, 22712, 'uc002urn.1', 'ensembl', '2', 190718995, 190728841, '+'),
(85648, 22712, 'CCDS46474.1', 'ccds', '2', 190656536, 190742162, '+'),
(85649, 22712, 'CCDS46473.1', 'ccds', '2', 190656536, 190742162, '+'),
(85650, 22712, 'CCDS2302.1', 'ccds', '2', 190656536, 190742162, '+');
