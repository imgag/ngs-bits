INSERT INTO `gene` (`id`, `hgnc_id`, `symbol`, `name`, `type`, `ensembl_id`, `ncbi_id`) VALUES 
(628605, 14825, 'OR4F5', 'olfactory receptor family 4 subfamily F member 5', 'protein-coding gene', 'ENSG00000186092', 79501),
(630046, 28208, 'PERM1', 'PPARGC1 and ESRR induced regulator, muscle 1', 'protein-coding gene', 'ENSG00000187642', 84808);

INSERT INTO `gene_transcript` (`id`, `gene_id`, `name`, `version`, `source`, `chromosome`, `start_coding`, `end_coding`, `strand`, `biotype`, `is_gencode_basic`, `is_ensembl_canonical`, `is_mane_select`, `is_mane_plus_clinical`) VALUES 
(1607635, 628605, 'CCDS30547', 2, 'ccds', '1', 65565, 70008, '+', 'protein coding', 0, 0, 0, 0),
(1607698, 630046, 'CCDS76083', 1, 'ccds', '1', 976172, 981029, '-', 'protein coding', 0, 0, 0, 0),
(1607701, 630046, 'CCDS90836', 1, 'ccds', '1', 976172, 981166, '-', 'protein coding', 0, 0, 0, 0);

INSERT INTO `gene_exon` (`transcript_id`, `start`, `end`) VALUES 
(1607635, 65565, 65573),
(1607635, 69037, 70008),
(1607698, 976172, 976269),
(1607698, 976499, 976624),
(1607698, 978881, 981029),
(1607701, 976172, 976269),
(1607701, 976499, 976624),
(1607701, 978881, 980657),
(1607701, 981137, 981166);
