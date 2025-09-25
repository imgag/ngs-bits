
INSERT INTO `gene`(`id`, `hgnc_id`, `symbol`, `name`, `type`) VALUES 
(1,1001, 'BRCA1','Breast cancer associated gene 1', 'protein-coding gene'),
(2,1002, 'BRCA2','Breast cancer associated gene 2', 'protein-coding gene'),
(3,1003, 'OR4F5', 'olfactory receptor family 4 subfamily F member 5', 'protein-coding gene'),
(4,1004, 'DIRC1', 'disrupted in renal carcinoma 1', 'protein-coding gene');

INSERT INTO `gene_transcript` (`id`, `gene_id`, `name`, `version`, `source`, `chromosome`, `start_coding`, `end_coding`, `strand`) VALUES
(1, 3, 'ENST00000335137', '1', 'ensembl', '1', 69091, 70008, '+'),
(2, 3, 'CCDS30547', '1', 'ccds', '1', 69091, 70008, '+');

INSERT INTO `geneinfo_germline`(`symbol`, `inheritance`, `gnomad_oe_mis`, `gnomad_oe_syn`, `gnomad_oe_lof`, `comments`) VALUES
('BRCA1', 'AD', 0.00, 0.00, 0.00, ''),
('BRCA2', 'AD', NULL, NULL, NULL, '');

INSERT INTO `hpo_term` (`id`, `hpo_id`, `name`, `definition`, `synonyms`) VALUES
(1, 'HP:0000007', 'Autosomal recessive inheritance', '\"A mode of inheritance that is observed for traits related to a gene encoded on one of the autosomes (i.e., the human chromosomes 1-22) in which a trait manifests in homozygotes. In the context of medical genetics, autosomal recessive disorders manifest in homozygotes (with two copies of the mutant allele) or compound heterozygotes (whereby each copy of a gene has a distinct mutant allele).', ''),
(2, 'HP:0001427', 'Mitochondrial inheritance', '\"A mode of inheritance that is observed for traits related to a gene encoded on the mitochondrial genome. Because the mitochondrial genome is essentially always maternally inherited, a mitochondrial condition can only be transmitted by females, although the condition can affect both sexes. The proportion of mutant mitochondria can vary (heteroplasmy).', '');

INSERT INTO `hpo_genes` (`hpo_term_id`, `gene`, `details`, `evidence`) VALUES
(1, 'OR4F5', '(G2P,possible,low)', 'low'),
(2, 'OR4F5', '(HGMD,,n/a)', 'n/a');
