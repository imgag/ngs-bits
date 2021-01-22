
INSERT INTO `gene` (`id`, `hgnc_id`, `symbol`, `name`, `type`) VALUES
(1, 1101, 'BRCA2', 'breast cancer 2, early onset', 'protein-coding gene'),
(5928, 37102, 'DDX11L1', 'DEAD/H (Asp-Glu-Ala-Asp/His) box helicase 11 like ...', 'pseudogene'),
(373742, 3418, 'EPRS', 'glutamyl-prolyl-tRNA synthetase', 'protein-coding gene'),
(391990, 9751, 'QARS1', 'glutaminyl-tRNA synthetase 1', 'protein-coding gene'),
(564644, 20, 'AARS1', 'alanyl-tRNA synthetase 1', 'protein-coding gene'),
(125845, 61, 'ABCD1', 'ATP binding cassette subfamily D member 1', 'protein-coding gene'),
(485875, 63, 'ABCD1P2', 'ATP binding cassette subfamily D member 1 pseudogene 2', 'pseudogene');

INSERT INTO `gene_alias`(`gene_id`, `symbol`, `type`) VALUES
(373742, 'QARS', 'previous'),
(391990, 'QARS', 'previous');





