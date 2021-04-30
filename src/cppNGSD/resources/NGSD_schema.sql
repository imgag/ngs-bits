SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0;
SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0;
SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='TRADITIONAL';

-- ----------------------------------------------------------------------------------------------------------
--                                                   TABLES
-- ----------------------------------------------------------------------------------------------------------

-- -----------------------------------------------------
-- Table `gene`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `gene`
(
`id` int(10) unsigned NOT NULL AUTO_INCREMENT,
`hgnc_id` int(10) unsigned NOT NULL,
`symbol` varchar(40) NOT NULL,
`name` TEXT NOT NULL,
`type` enum('protein-coding gene','pseudogene','non-coding RNA','other') NOT NULL,

PRIMARY KEY (`id`), 
UNIQUE KEY `hgnc_id` (`hgnc_id`),
UNIQUE KEY `symbol` (`symbol`),
KEY `type` (`type`)
)
ENGINE=InnoDB DEFAULT
CHARSET=utf8
COMMENT='Genes from HGNC';

-- -----------------------------------------------------
-- Table `gene_alias`
-- -----------------------------------------------------

CREATE TABLE IF NOT EXISTS `gene_alias`
(
`gene_id` int(10) unsigned NOT NULL,
`symbol` varchar(40) NOT NULL,
`type` enum('synonym','previous') NOT NULL,

KEY `fk_gene_id1` (`gene_id` ASC) ,
CONSTRAINT `fk_gene_id1`
  FOREIGN KEY (`gene_id` )
  REFERENCES `gene` (`id` )
  ON DELETE NO ACTION
  ON UPDATE NO ACTION,
KEY `symbol` (`symbol`),
KEY `type` (`type`)
) 
ENGINE=InnoDB DEFAULT 
CHARSET=utf8
COMMENT='Alternative symbols of genes';


-- -----------------------------------------------------
-- Table `gene_transcript`
-- -----------------------------------------------------

CREATE TABLE IF NOT EXISTS `gene_transcript`
(
`id` int(10) unsigned NOT NULL AUTO_INCREMENT,
`gene_id` int(10) unsigned NOT NULL,
`name` varchar(40) NOT NULL,
`source` enum('ccds', 'ensembl') NOT NULL,
`chromosome` enum('1','2','3','4','5','6','7','8','9','10','11','12','13','14','15','16','17','18','19','20','21','22','X','Y','MT') NOT NULL,
`start_coding` int(10) unsigned NULL,
`end_coding` int(10) unsigned NULL,
`strand` enum('+', '-') NOT NULL,

PRIMARY KEY (`id`),
CONSTRAINT `fk_gene_id3`
  FOREIGN KEY (`gene_id` )
  REFERENCES `gene` (`id` )
  ON DELETE NO ACTION
  ON UPDATE NO ACTION,
UNIQUE KEY `gene_name_unique` (`gene_id`, `name`),
KEY `name` (`name`) ,
KEY `source` (`source`),
KEY `chromosome` (`chromosome`),
KEY `start_coding` (`start_coding`), 
KEY `end_coding` (`end_coding`)
) 
ENGINE=InnoDB DEFAULT 
CHARSET=utf8
COMMENT='Gene transcipts';

-- -----------------------------------------------------
-- Table `gene_exon`
-- -----------------------------------------------------

CREATE TABLE IF NOT EXISTS `gene_exon`
(
`transcript_id` int(10) unsigned NOT NULL,
`start` int(10) unsigned NOT NULL,
`end` int(10) unsigned NOT NULL,

KEY `fk_transcript_id2` (`transcript_id` ASC) ,
CONSTRAINT `fk_transcript_id2`
  FOREIGN KEY (`transcript_id` )
  REFERENCES `gene_transcript` (`id` )
  ON DELETE NO ACTION
  ON UPDATE NO ACTION,
KEY `start` (`start`), 
KEY `end` (`end`)
) 
ENGINE=InnoDB DEFAULT 
CHARSET=utf8
COMMENT='Transcript exons';

-- -----------------------------------------------------
-- Table `geneinfo_germline`
-- -----------------------------------------------------

CREATE TABLE IF NOT EXISTS `geneinfo_germline`
(
`symbol` VARCHAR(40) NOT NULL,
`inheritance` ENUM('AR','AD','AR+AD','XLR','XLD','XLR+XLD','MT','MU','n/a') NOT NULL,
`gnomad_oe_syn` FLOAT NULL,
`gnomad_oe_mis` FLOAT NULL,
`gnomad_oe_lof` FLOAT NULL,
`comments` text NOT NULL,
PRIMARY KEY `symbol` (`symbol`)
)
ENGINE=InnoDB
CHARSET=utf8;

-- -----------------------------------------------------
-- Table `mid`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `mid`
(
  `id` INT(11) NOT NULL AUTO_INCREMENT,
  `name` VARCHAR(255) NOT NULL,
  `sequence` VARCHAR(45) NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE INDEX `name_UNIQUE` (`name` ASC)
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;


-- -----------------------------------------------------
-- Table `genome`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `genome`
(
  `id` INT(11) NOT NULL AUTO_INCREMENT,
  `build` VARCHAR(45) NOT NULL,
  `comment` TEXT NULL DEFAULT NULL,
  PRIMARY KEY (`id`),
  UNIQUE INDEX `build_UNIQUE` (`build` ASC)
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;


-- -----------------------------------------------------
-- Table `processing_system`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `processing_system`
(
  `id` INT(11) NOT NULL AUTO_INCREMENT,
  `name_short` VARCHAR(50) NOT NULL,
  `name_manufacturer` VARCHAR(100) NOT NULL,
  `adapter1_p5` VARCHAR(45) NULL DEFAULT NULL,
  `adapter2_p7` VARCHAR(45) NULL DEFAULT NULL,
  `type` ENUM('WGS','WGS (shallow)','WES','Panel','Panel Haloplex','Panel MIPs','RNA','ChIP-Seq', 'cfDNA (patient-specific)', 'cfDNA') NOT NULL,
  `shotgun` TINYINT(1) NOT NULL,
  `umi_type` ENUM('n/a','HaloPlex HS','SureSelect HS','ThruPLEX','Safe-SeqS','MIPs','QIAseq','IDT-UDI-UMI','IDT-xGen-Prism') NOT NULL DEFAULT 'n/a',
  `target_file` VARCHAR(255) NULL DEFAULT NULL COMMENT 'filename of sub-panel BED file relative to the megSAP enrichment folder.',
  `genome_id` INT(11) NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE INDEX `name_short` (`name_short` ASC),
  UNIQUE INDEX `name_manufacturer` (`name_manufacturer` ASC),
  INDEX `fk_processing_system_genome1` (`genome_id` ASC),
  CONSTRAINT `fk_processing_system_genome1`
    FOREIGN KEY (`genome_id`)
    REFERENCES `genome` (`id`)
    ON UPDATE NO ACTION
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;


-- -----------------------------------------------------
-- Table `device`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `device`
(
  `id` INT(11) NOT NULL AUTO_INCREMENT,
  `type` ENUM('GAIIx','MiSeq','HiSeq2500','NextSeq500','NovaSeq5000','NovaSeq6000','MGI-2000','SequelII','PromethION') NOT NULL,
  `name` VARCHAR(45) NOT NULL,
  `comment` TEXT NULL DEFAULT NULL,
  PRIMARY KEY (`id`),
  UNIQUE INDEX `name_UNIQUE` (`name` ASC)
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;


-- -----------------------------------------------------
-- Table `sequencing_run`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `sequencing_run`
(
  `id` INT(11) NOT NULL AUTO_INCREMENT,
  `name` VARCHAR(45) NOT NULL,
  `fcid` VARCHAR(45) NULL DEFAULT NULL,
  `flowcell_type` ENUM('Illumina MiSeq v2','Illumina MiSeq v2 Micro','Illumina MiSeq v2 Nano','Illumina MiSeq v3','Illumina NextSeq High Output','Illumina NextSeq Mid Output','Illumina NovaSeq SP','Illumina NovaSeq S1','Illumina NovaSeq S2','Illumina NovaSeq S4','PromethION FLO-PRO002','SMRTCell 8M','n/a') NOT NULL DEFAULT 'n/a',
  `start_date` DATE NULL DEFAULT NULL,
  `end_date` DATE NULL DEFAULT NULL,
  `device_id` INT(11) NOT NULL,
  `recipe` VARCHAR(45) NOT NULL COMMENT 'Read length for reads and index reads separated by \'+\'',
  `pool_molarity` float DEFAULT NULL,
  `pool_quantification_method` enum('n/a','Tapestation','Bioanalyzer','qPCR','Tapestation & Qubit','Bioanalyzer & Qubit','Bioanalyzer & Tecan Infinite','Fragment Analyzer & Qubit','Fragment Analyzer & Tecan Infinite','Illumina 450bp & Qubit ssDNA','PCR Size & ssDNA') NOT NULL DEFAULT 'n/a',
  `comment` TEXT NULL DEFAULT NULL,
  `quality` ENUM('n/a','good','medium','bad') NOT NULL DEFAULT 'n/a',
  `status` ENUM('n/a','run_started','run_finished','run_aborted','demultiplexing_started','analysis_started','analysis_finished','analysis_not_possible','analysis_and_backup_not_required') NOT NULL DEFAULT 'n/a',
  `backup_done` TINYINT(1) NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`),
  UNIQUE INDEX `name_UNIQUE` (`name` ASC),
  UNIQUE INDEX `fcid_UNIQUE` (`fcid` ASC),
  INDEX `fk_run_device1` (`device_id` ASC),
  CONSTRAINT `fk_run_device1`
    FOREIGN KEY (`device_id`)
    REFERENCES `device` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;

-- -----------------------------------------------------
-- Table `runqc_read`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `runqc_read`
(
  `id` INT(11) NOT NULL AUTO_INCREMENT,
  `read_num` INT NOT NULL,
  `cycles` INT NOT NULL,
  `is_index` BOOLEAN NOT NULL,
  `q30_perc` FLOAT NOT NULL,
  `error_rate` FLOAT DEFAULT NULL,
  `sequencing_run_id` INT(11) NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE (`sequencing_run_id`, `read_num`),
  INDEX `fk_sequencing_run_id` (`sequencing_run_id` ASC),
  CONSTRAINT `fk_sequencing_run_id`
    FOREIGN KEY (`sequencing_run_id`)
    REFERENCES `sequencing_run` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;


-- -----------------------------------------------------
-- Table `runqc_lane`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `runqc_lane`
(
  `id` INT(11) NOT NULL AUTO_INCREMENT,
  `lane_num` INT NOT NULL,
  `cluster_density` FLOAT NOT NULL,
  `cluster_density_pf` FLOAT NOT NULL,
  `yield` FLOAT NOT NULL,
  `error_rate` FLOAT DEFAULT NULL,
  `q30_perc` FLOAT NOT NULL,
  `occupied_perc` FLOAT DEFAULT NULL,
  `runqc_read_id` INT(11) NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE (`runqc_read_id`, `lane_num`),
  INDEX `fk_runqc_read_id` (`runqc_read_id` ASC),
  CONSTRAINT `fk_runqc_read_id`
    FOREIGN KEY (`runqc_read_id`)
    REFERENCES `runqc_read` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;


-- -----------------------------------------------------
-- Table `species`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `species`
(
  `id` INT(11) NOT NULL AUTO_INCREMENT,
  `name` VARCHAR(45) NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE INDEX `name_UNIQUE` (`name` ASC)
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;


-- -----------------------------------------------------
-- Table `sender`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `sender`
(
  `id` INT(11) NOT NULL AUTO_INCREMENT,
  `name` VARCHAR(45) NOT NULL,
  `phone` VARCHAR(45) NULL DEFAULT NULL,
  `email` VARCHAR(45) NULL DEFAULT NULL,
  `affiliation` VARCHAR(100) NULL DEFAULT NULL,
  PRIMARY KEY (`id`),
  UNIQUE INDEX `name_UNIQUE` (`name` ASC)
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;


-- -----------------------------------------------------
-- Table `user`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `user`
(
  `id` INT(11) NOT NULL AUTO_INCREMENT,
  `user_id` VARCHAR(45) NOT NULL COMMENT 'Use the lower-case Windows domain name!',
  `password` VARCHAR(64) NOT NULL,
  `user_role` ENUM('user','admin','special') NOT NULL,
  `name` VARCHAR(45) NOT NULL,
  `email` VARCHAR(45) NOT NULL,
  `created` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `last_login` DATETIME NULL DEFAULT NULL,
  `active` TINYINT(1) DEFAULT 1 NOT NULL,
  `salt` VARCHAR(40) NULL DEFAULT NULL,
  PRIMARY KEY (`id`),
  UNIQUE INDEX `name_UNIQUE` (`user_id` ASC),
  UNIQUE INDEX `email_UNIQUE` (`email` ASC)
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;


-- -----------------------------------------------------
-- Table `sample`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `sample`
(
  `id` INT(11) NOT NULL AUTO_INCREMENT,
  `name` VARCHAR(20) NOT NULL,
  `name_external` VARCHAR(255) NULL DEFAULT NULL COMMENT 'External names.<br>If several, separate by comma!<br>Always enter full names, no short forms!',
  `received` DATE NULL DEFAULT NULL,
  `receiver_id` INT(11) NULL DEFAULT NULL,
  `sample_type` ENUM('DNA','DNA (amplicon)','DNA (native)','RNA') NOT NULL,
  `species_id` INT(11) NOT NULL,
  `concentration` FLOAT NULL DEFAULT NULL,
  `volume` FLOAT NULL DEFAULT NULL,
  `od_260_280` FLOAT NULL DEFAULT NULL,
  `gender` ENUM('male','female','n/a') NOT NULL,
  `comment` TEXT NULL DEFAULT NULL,
  `quality` ENUM('n/a','good','medium','bad') NOT NULL DEFAULT 'n/a',
  `od_260_230` FLOAT NULL DEFAULT NULL,
  `integrity_number` FLOAT NULL DEFAULT NULL,
  `tumor` TINYINT(1) NOT NULL,
  `ffpe` TINYINT(1) NOT NULL,
  `sender_id` INT(11) NOT NULL,
  `disease_group` ENUM('n/a','Neoplasms','Diseases of the blood or blood-forming organs','Diseases of the immune system','Endocrine, nutritional or metabolic diseases','Mental, behavioural or neurodevelopmental disorders','Sleep-wake disorders','Diseases of the nervous system','Diseases of the visual system','Diseases of the ear or mastoid process','Diseases of the circulatory system','Diseases of the respiratory system','Diseases of the digestive system','Diseases of the skin','Diseases of the musculoskeletal system or connective tissue','Diseases of the genitourinary system','Developmental anomalies','Other diseases') NOT NULL DEFAULT 'n/a',
  `disease_status` ENUM('n/a','Affected','Unaffected') NOT NULL DEFAULT 'n/a',
  PRIMARY KEY (`id`),
  UNIQUE INDEX `name_UNIQUE` (`name` ASC),
  INDEX `fk_samples_species1` (`species_id` ASC),
  INDEX `sender_id` (`sender_id` ASC),
  INDEX `receiver_id` (`receiver_id` ASC),
  INDEX `name_external` (`name_external` ASC),
  INDEX `tumor` (`tumor` ASC),
  INDEX `quality` (`quality` ASC),
  INDEX `disease_group` (`disease_group`),
  INDEX `disease_status` (`disease_status`),
  CONSTRAINT `fk_samples_species1`
    FOREIGN KEY (`species_id`)
    REFERENCES `species` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `sample_ibfk_1`
    FOREIGN KEY (`sender_id`)
    REFERENCES `sender` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `sample_ibfk_2`
    FOREIGN KEY (`receiver_id`)
    REFERENCES `user` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;

-- -----------------------------------------------------
-- Table `sample_disease_info`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `sample_disease_info`
(
  `id` INT(11) NOT NULL AUTO_INCREMENT,
  `sample_id` INT(11) NOT NULL,
  `disease_info` TEXT NOT NULL,
  `type` ENUM('HPO term id', 'ICD10 code', 'OMIM disease/phenotype identifier', 'Orpha number', 'CGI cancer type', 'tumor fraction', 'age of onset', 'clinical phenotype (free text)', 'RNA reference tissue') NOT NULL,
  `user_id` int(11) DEFAULT NULL,
  `date` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  CONSTRAINT `fk_sample_id`
    FOREIGN KEY (`sample_id`)
    REFERENCES `sample` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `fk_user`
    FOREIGN KEY (`user_id`)
    REFERENCES `user` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;

-- -----------------------------------------------------
-- Table `sample_relations`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `sample_relations`
(
  `id` INT(11) NOT NULL AUTO_INCREMENT,
  `sample1_id` INT(11) NOT NULL,
  `relation` ENUM('parent-child', 'tumor-normal', 'siblings', 'same sample', 'tumor-cfDNA', 'same patient', 'cousins', 'twins', 'twins (monozygotic)') NOT NULL,
  `sample2_id` INT(11) NOT NULL,
  `user_id` int(11) DEFAULT NULL,
  `date` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE INDEX `relation_unique` (`sample1_id` ASC, `relation` ASC, `sample2_id` ASC),
  CONSTRAINT `fk_sample1_id`
    FOREIGN KEY (`sample1_id`)
    REFERENCES `sample` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `fk_sample2_id`
    FOREIGN KEY (`sample2_id`)
    REFERENCES `sample` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `fk_user2`
    FOREIGN KEY (`user_id`)
    REFERENCES `user` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;

-- -----------------------------------------------------
-- Table `project`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `project`
(
  `id` INT(11) NOT NULL AUTO_INCREMENT,
  `name` VARCHAR(45) NOT NULL,
  `aliases` TEXT DEFAULT NULL,
  `type` ENUM('diagnostic','research','test','external') NOT NULL,
  `internal_coordinator_id` INT(11) NOT NULL COMMENT 'Person who is responsible for this project.<br>The person will be notified when new samples are available.',
  `comment` TEXT NULL DEFAULT NULL,
  `analysis` ENUM('fastq','mapping','variants') NOT NULL DEFAULT 'variants' COMMENT 'Bioinformatics analysis to be done for non-tumor germline samples in this project.<br>"fastq" skips the complete analysis.<br>"mapping" creates the BAM file but calls no variants.<br>"variants" performs the full analysis.',
  `preserve_fastqs` tinyint(1) NOT NULL DEFAULT '0' COMMENT 'Prevents FASTQ files from being deleted after mapping in this project.<br>Has no effect if megSAP is not configured to delete FASTQs automatically.<br>For diagnostics, do not check. For other project types ask the bioinformatician in charge.',
  `email_notification` varchar(200) DEFAULT NULL COMMENT 'List of email addresses (separated by semicolon) that are notified in addition to the project coordinator when new samples are available.',
  PRIMARY KEY (`id`),
  UNIQUE INDEX `name_UNIQUE` (`name` ASC),
  INDEX `internal_coordinator_id` (`internal_coordinator_id` ASC),
  CONSTRAINT `project_ibfk_1`
    FOREIGN KEY (`internal_coordinator_id`)
    REFERENCES `user` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;


-- -----------------------------------------------------
-- Table `processed_sample`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `processed_sample`
(
  `id` INT(11) NOT NULL AUTO_INCREMENT,
  `sample_id` INT(11) NOT NULL,
  `process_id` INT(2) NOT NULL,
  `sequencing_run_id` INT(11) NULL DEFAULT NULL,
  `lane` varchar(15) NOT NULL COMMENT 'Comma-separated lane list (1-8)',
  `mid1_i7` INT(11) NULL DEFAULT NULL,
  `mid2_i5` INT(11) NULL DEFAULT NULL,
  `operator_id` INT(11) NULL DEFAULT NULL,
  `processing_system_id` INT(11) NOT NULL,
  `comment` TEXT NULL DEFAULT NULL,
  `project_id` INT(11) NOT NULL,
  `processing_input` FLOAT NULL DEFAULT NULL,
  `molarity` FLOAT NULL DEFAULT NULL,
  `normal_id` INT(11) NULL DEFAULT NULL COMMENT 'For tumor samples, a normal sample can be given here which is used as reference sample during the data analysis.',
  `quality` ENUM('n/a','good','medium','bad') NOT NULL DEFAULT 'n/a',
  PRIMARY KEY (`id`),
  UNIQUE INDEX `sample_psid_unique` (`sample_id` ASC, `process_id` ASC),
  INDEX `fk_processed_sample_samples1` (`sample_id` ASC),
  INDEX `fk_processed_sample_mid1` (`mid1_i7` ASC),
  INDEX `fk_processed_sample_processing_system1` (`processing_system_id` ASC),
  INDEX `fk_processed_sample_run1` (`sequencing_run_id` ASC),
  INDEX `fk_processed_sample_mid2` (`mid2_i5` ASC),
  INDEX `project_id` (`project_id` ASC),
  INDEX `operator_id` (`operator_id` ASC),
  INDEX `normal_id_INDEX` (`normal_id` ASC),
  INDEX `quality` (`quality` ASC),
  CONSTRAINT `fk_processed_sample_mid1`
    FOREIGN KEY (`mid1_i7`)
    REFERENCES `mid` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `fk_processed_sample_mid2`
    FOREIGN KEY (`mid2_i5`)
    REFERENCES `mid` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `fk_processed_sample_processing_system1`
    FOREIGN KEY (`processing_system_id`)
    REFERENCES `processing_system` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `fk_processed_sample_run1`
    FOREIGN KEY (`sequencing_run_id`)
    REFERENCES `sequencing_run` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `fk_processed_sample_sample2`
    FOREIGN KEY (`normal_id`)
    REFERENCES `processed_sample` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `fk_processed_sample_samples1`
    FOREIGN KEY (`sample_id`)
    REFERENCES `sample` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `processed_sample_ibfk_1`
    FOREIGN KEY (`project_id`)
    REFERENCES `project` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `processed_sample_ibfk_2`
    FOREIGN KEY (`operator_id`)
    REFERENCES `user` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;


-- -----------------------------------------------------
-- Table `variant`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `variant`
(
  `id` INT(11) NOT NULL AUTO_INCREMENT,
  `chr` ENUM('chr1','chr2','chr3','chr4','chr5','chr6','chr7','chr8','chr9','chr10','chr11','chr12','chr13','chr14','chr15','chr16','chr17','chr18','chr19','chr20','chr21','chr22','chrY','chrX','chrMT') NOT NULL,
  `start` INT(11) NOT NULL,
  `end` INT(11) NOT NULL,
  `ref` TEXT NOT NULL,
  `obs` TEXT NOT NULL,
  `1000g` FLOAT NULL DEFAULT NULL,
  `gnomad` FLOAT NULL DEFAULT NULL,
  `gene` TEXT NULL DEFAULT NULL,
  `variant_type` TEXT NULL DEFAULT NULL,
  `coding` TEXT NULL DEFAULT NULL,  
  `comment` TEXT NULL DEFAULT NULL,
  PRIMARY KEY (`id`),
  UNIQUE INDEX `variant_UNIQUE` (`chr` ASC, `start` ASC, `end` ASC, `ref`(255) ASC, `obs`(255) ASC),
  INDEX `gene` (`gene`(50) ASC),
  INDEX `1000g` (`1000g` ASC),
  INDEX `gnomad` (`gnomad` ASC),
  INDEX `comment` (`comment`(50) ASC)
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;


-- -----------------------------------------------------
-- Table `variant_publication`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `variant_publication`
(
  `id` INT(11) NOT NULL AUTO_INCREMENT,
  `sample_id` INT(11) NOT NULL,
  `variant_id` INT(11) NOT NULL,
  `db` ENUM('LOVD','ClinVar') NOT NULL,
  `class` ENUM('1','2','3','4','5') NOT NULL,
  `details` TEXT NOT NULL,
  `user_id` INT(11) NOT NULL,
  `date` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
PRIMARY KEY (`id`),
CONSTRAINT `fk_variant_publication_has_user`
  FOREIGN KEY (`user_id`)
  REFERENCES `user` (`id`)
  ON DELETE NO ACTION
  ON UPDATE NO ACTION,
CONSTRAINT `fk_variant_publication_has_sample`
    FOREIGN KEY (`sample_id`)
    REFERENCES `sample` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
CONSTRAINT `fk_variant_publication_has_variant`
  FOREIGN KEY (`variant_id`)
  REFERENCES `variant` (`id`)
  ON DELETE NO ACTION
  ON UPDATE NO ACTION
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;

-- -----------------------------------------------------
-- Table `variant_classification`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `variant_classification`
(
  `id` INT(11) NOT NULL AUTO_INCREMENT,
  `variant_id` INT(11) NOT NULL,
  `class` ENUM('n/a','1','2','3','4','5','M') NOT NULL,
  `comment` TEXT NULL DEFAULT NULL,
PRIMARY KEY (`id`),
UNIQUE KEY `fk_variant_classification_has_variant` (`variant_id`),
CONSTRAINT `fk_variant_classification_has_variant`
  FOREIGN KEY (`variant_id`)
  REFERENCES `variant` (`id`)
  ON DELETE NO ACTION
  ON UPDATE NO ACTION,
INDEX `class` (`class` ASC)
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;

-- -----------------------------------------------------
-- Table `somatic_variant_classification`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `somatic_variant_classification`
(
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `variant_id` int(11) NOT NULL,
  `class` ENUM('n/a','activating','likely_activating','inactivating','likely_inactivating','unclear','test_dependent') NOT NULL,
  `comment` TEXT,
  PRIMARY KEY (`id`),
  UNIQUE KEY `somatic_variant_classification_has_variant` (`variant_id`),
  CONSTRAINT `somatic_variant_classification_has_variant` 
    FOREIGN KEY (`variant_id`)
    REFERENCES `variant` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION
)
ENGINE = InnoDB 
DEFAULT CHARACTER SET = utf8;

-- -----------------------------------------------------
-- Table `somatic_vicc_interpretation`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `somatic_vicc_interpretation`
(
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `variant_id` int(11) NOT NULL,
  `null_mutation_in_tsg` BOOLEAN NULL DEFAULT NULL,
  `known_oncogenic_aa` BOOLEAN NULL DEFAULT NULL,
  `oncogenic_funtional_studies` BOOLEAN NULL DEFAULT NULL,
  `strong_cancerhotspot` BOOLEAN NULL DEFAULT NULL,
  `located_in_canerhotspot` BOOLEAN NULL DEFAULT NULL,
  `absent_from_controls` BOOLEAN NULL DEFAULT NULL,
  `protein_length_change` BOOLEAN NULL DEFAULT NULL,
  `other_aa_known_oncogenic` BOOLEAN NULL DEFAULT NULL,
  `weak_cancerhotspot` BOOLEAN NULL DEFAULT NULL,
  `computational_evidence` BOOLEAN NULL DEFAULT NULL,
  `mutation_in_gene_with_etiology` BOOLEAN NULL DEFAULT NULL,
  `very_weak_cancerhotspot` BOOLEAN NULL DEFAULT NULL,
  `very_high_maf` BOOLEAN NULL DEFAULT NULL,
  `benign_functional_studies` BOOLEAN NULL DEFAULT NULL,
  `high_maf` BOOLEAN NULL DEFAULT NULL,
  `benign_computational_evidence` BOOLEAN NULL DEFAULT NULL,
  `synonymous_mutation` BOOLEAN NULL DEFAULT NULL,
  `comment` TEXT NULL DEFAULT NULL,
  `created_by` int(11) DEFAULT NULL,
  `created_date` DATETIME NOT NULL,
  `last_edit_by` int(11) DEFAULT NULL,
  `last_edit_date` TIMESTAMP NULL DEFAULT NULL,
PRIMARY KEY (`id`),
UNIQUE KEY `somatic_vicc_interpretation_has_variant` (`variant_id`),
CONSTRAINT `somatic_vicc_interpretation_has_variant`
  FOREIGN KEY (`variant_id`)
  REFERENCES `variant` (`id`)
  ON DELETE NO ACTION
  ON UPDATE NO ACTION,
CONSTRAINT `somatic_vicc_interpretation_created_by_user` 
  FOREIGN KEY (`created_by`)
  REFERENCES `user` (`id`) 
  ON DELETE NO ACTION 
  ON UPDATE NO ACTION,
CONSTRAINT `somatic_vicc_interpretation_last_edit_by_user`
  FOREIGN KEY (`last_edit_by`)
  REFERENCES `user` (`id`)
  ON DELETE NO ACTION
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;

-- -----------------------------------------------------
-- Table `somatic_gene_role`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `somatic_gene_role`
(
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `symbol` VARCHAR(40) NOT NULL,
  `gene_role` ENUM('activating','loss_of_function', 'ambiguous') NOT NULL,
  `high_evidence` BOOLEAN DEFAULT FALSE,
  `comment` TEXT NULL DEFAULT NULL,
PRIMARY KEY (`id`),
UNIQUE KEY (`symbol`)
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;

-- -----------------------------------------------------
-- Table `detected_variant`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `detected_variant`
(
  `processed_sample_id` INT(11) NOT NULL,
  `variant_id` INT(11) NOT NULL,
  `genotype` ENUM('hom','het') NOT NULL,
  PRIMARY KEY (`processed_sample_id`, `variant_id`),
  INDEX `fk_detected_variant_variant1` (`variant_id` ASC),
  CONSTRAINT `fk_processed_sample_has_variant_processed_sample1`
    FOREIGN KEY (`processed_sample_id`)
    REFERENCES `processed_sample` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `fk_processed_sample_has_variant_variant1`
    FOREIGN KEY (`variant_id`)
    REFERENCES `variant` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;

-- -----------------------------------------------------
-- Table `qc_terms`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `qc_terms`
(
  `id` INT(11) NOT NULL AUTO_INCREMENT,
  `qcml_id` VARCHAR(10) NULL DEFAULT NULL,
  `name` VARCHAR(45) NOT NULL,
  `description` TEXT NOT NULL,
  `type` ENUM('float', 'int', 'string') NOT NULL,
  `obsolete` TINYINT(1) NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE INDEX `name_UNIQUE` (`name` ASC),
  UNIQUE INDEX `qcml_id_UNIQUE` (`qcml_id` ASC)
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;


-- -----------------------------------------------------
-- Table `processed_sample_qc`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `processed_sample_qc`
(
  `id` INT(11) NOT NULL AUTO_INCREMENT,
  `processed_sample_id` INT(11) NOT NULL,
  `qc_terms_id` INT(11) NOT NULL,
  `value` VARCHAR(30) NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE INDEX `c_processing_id_qc_terms_id` (`processed_sample_id` ASC, `qc_terms_id` ASC),
  INDEX `fk_qcvalues_processing1` (`processed_sample_id` ASC),
  INDEX `fk_processed_sample_annotaiton_qcml1` (`qc_terms_id` ASC),
  CONSTRAINT `fk_processed_sample_annotaiton_qcml1`
    FOREIGN KEY (`qc_terms_id`)
    REFERENCES `qc_terms` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `fk_qcvalues_processing1`
    FOREIGN KEY (`processed_sample_id`)
    REFERENCES `processed_sample` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;


-- -----------------------------------------------------
-- Table `sample_group`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `sample_group`
(
  `id` INT(11) NOT NULL AUTO_INCREMENT,
  `name` VARCHAR(45) NOT NULL,
  `comment` TEXT NULL DEFAULT NULL,
  PRIMARY KEY (`id`),
  UNIQUE INDEX `name_UNIQUE` (`name` ASC)
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;


-- -----------------------------------------------------
-- Table `nm_sample_sample_group`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `nm_sample_sample_group`
(
  `id` INT(11) NOT NULL AUTO_INCREMENT,
  `sample_group_id` INT(11) NOT NULL,
  `sample_id` INT(11) NOT NULL,
  PRIMARY KEY (`id`),
  INDEX `fk_sample_group_has_sample_sample1` (`sample_id` ASC),
  INDEX `fk_sample_group_has_sample_sample_group1` (`sample_group_id` ASC),
  CONSTRAINT `fk_sample_group_has_sample_sample1`
    FOREIGN KEY (`sample_id`)
    REFERENCES `sample` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `fk_sample_group_has_sample_sample_group1`
    FOREIGN KEY (`sample_group_id`)
    REFERENCES `sample_group` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;


-- -----------------------------------------------------
-- Table `detected_somatic_variant`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `detected_somatic_variant`
(
  `id` INT(11) NOT NULL AUTO_INCREMENT,
  `processed_sample_id_tumor` INT(11) NOT NULL,
  `processed_sample_id_normal` INT(11) NULL DEFAULT NULL,
  `variant_id` INT(11) NOT NULL,
  `variant_frequency` FLOAT NOT NULL,
  `depth` INT(11) NOT NULL,
  `quality_snp` FLOAT NULL DEFAULT NULL,
  PRIMARY KEY (`id`),
  UNIQUE INDEX `detected_somatic_variant_UNIQUE` (`processed_sample_id_tumor` ASC, `processed_sample_id_normal` ASC, `variant_id` ASC),
  INDEX `variant_id_INDEX` (`variant_id` ASC),
  INDEX `processed_sample_id_tumor_INDEX` (`processed_sample_id_tumor` ASC),
  INDEX `processed_sample_id_normal_INDEX` (`processed_sample_id_normal` ASC),
  INDEX `fk_dsv_has_ps1` (`processed_sample_id_tumor` ASC),
  INDEX `fk_dsv_has_ps2` (`processed_sample_id_normal` ASC),
  CONSTRAINT `fk_dsv_has_ps1`
    FOREIGN KEY (`processed_sample_id_tumor`)
    REFERENCES `processed_sample` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `fk_dsv_has_ps2`
    FOREIGN KEY (`processed_sample_id_normal`)
    REFERENCES `processed_sample` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `fk_dsv_has_v`
    FOREIGN KEY (`variant_id`)
    REFERENCES `variant` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;


-- -----------------------------------------------------
-- Table `diag_status`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `diag_status`
(
  `processed_sample_id` INT(11) NOT NULL,
  `status` ENUM('pending','in progress','done','done - follow up pending','cancelled','not usable because of data quality') NOT NULL,
  `user_id` INT(11) NOT NULL,
  `date` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `outcome` ENUM('n/a','no significant findings','uncertain','significant findings','significant findings - second method', 'significant findings - non-genetic', 'candidate gene') NOT NULL DEFAULT 'n/a',
  `comment` TEXT NULL DEFAULT NULL,
  PRIMARY KEY (`processed_sample_id`),
  INDEX `user_id` (`user_id` ASC),
  CONSTRAINT `diag_status_ibfk_1`
    FOREIGN KEY (`processed_sample_id`)
    REFERENCES `processed_sample` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `diag_status_ibfk_2`
    FOREIGN KEY (`user_id`)
    REFERENCES `user` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;


-- -----------------------------------------------------
-- Table `kasp_status`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `kasp_status`
(
  `processed_sample_id` INT(11) NOT NULL,
  `random_error_prob` FLOAT UNSIGNED NOT NULL,
  `snps_evaluated` INT(10) UNSIGNED NOT NULL,
  `snps_match` INT(10) UNSIGNED NOT NULL,
  PRIMARY KEY (`processed_sample_id`),
  CONSTRAINT `processed_sample0`
    FOREIGN KEY (`processed_sample_id`)
    REFERENCES `processed_sample` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;

-- -----------------------------------------------------
-- Table `hpo_term`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `hpo_term`
(
  `id` INT(10) UNSIGNED NOT NULL AUTO_INCREMENT,
  `hpo_id` VARCHAR(10) NOT NULL,
  `name` TEXT NOT NULL,
  `definition` TEXT NOT NULL,
  `synonyms` TEXT NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE INDEX `hpo_id` (`hpo_id` ASC),
  INDEX `name` (`name`(100) ASC),
  INDEX `synonyms` (`synonyms`(100) ASC)
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;


-- -----------------------------------------------------
-- Table `hpo_genes`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `hpo_genes`
(
  `hpo_term_id` INT(10) UNSIGNED NOT NULL,
  `gene` VARCHAR(40) CHARACTER SET 'utf8' NOT NULL,
  PRIMARY KEY (`hpo_term_id`, `gene`),
  CONSTRAINT `hpo_genes_ibfk_1`
    FOREIGN KEY (`hpo_term_id`)
    REFERENCES `hpo_term` (`id`)
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;


-- -----------------------------------------------------
-- Table `hpo_parent`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `hpo_parent`
(
  `parent` INT(10) UNSIGNED NOT NULL,
  `child` INT(10) UNSIGNED NOT NULL,
  PRIMARY KEY (`parent`, `child`),
  INDEX `child` (`child` ASC),
  CONSTRAINT `hpo_parent_ibfk_2`
    FOREIGN KEY (`child`)
    REFERENCES `hpo_term` (`id`),
  CONSTRAINT `hpo_parent_ibfk_1`
    FOREIGN KEY (`parent`)
    REFERENCES `hpo_term` (`id`)
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;


-- -----------------------------------------------------
-- Table `analysis_job`
-- -----------------------------------------------------

CREATE TABLE IF NOT EXISTS `analysis_job`
(
  `id` INT(11) NOT NULL AUTO_INCREMENT,
  `type` enum('single sample','multi sample','trio','somatic') NOT NULL,
  `high_priority` TINYINT(1) NOT NULL,
  `args` text NOT NULL,
  `sge_id` varchar(10) DEFAULT NULL,
  `sge_queue` varchar(50) DEFAULT NULL,
  PRIMARY KEY (`id`)
)
ENGINE=InnoDB
DEFAULT CHARSET=utf8;


-- -----------------------------------------------------
-- Table `analysis_job_sample`
-- -----------------------------------------------------

CREATE TABLE IF NOT EXISTS `analysis_job_sample`
(
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `analysis_job_id` int(11) NOT NULL,
  `processed_sample_id` int(11) NOT NULL,
  `info` text NOT NULL,
  PRIMARY KEY (`id`),
  CONSTRAINT `fk_analysis_job_id`
    FOREIGN KEY (`analysis_job_id` )
    REFERENCES `analysis_job` (`id` )
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `fk_processed_sample_id`
    FOREIGN KEY (`processed_sample_id` )
    REFERENCES `processed_sample` (`id` )
    ON DELETE NO ACTION
    ON UPDATE NO ACTION
)
ENGINE=InnoDB
DEFAULT CHARSET=utf8;


-- -----------------------------------------------------
-- Table `analysis_job_history`
-- -----------------------------------------------------

CREATE TABLE IF NOT EXISTS `analysis_job_history`
(
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `analysis_job_id` int(11) NOT NULL,
  `time` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `user_id` int(11) DEFAULT NULL,
  `status` enum('queued','started','finished','cancel','canceled','error') NOT NULL,
  `output` text DEFAULT NULL,
  PRIMARY KEY (`id`),
  CONSTRAINT `fk_analysis_job_id2`
    FOREIGN KEY (`analysis_job_id` )
    REFERENCES `analysis_job` (`id` )
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `fk_user_id2`
    FOREIGN KEY (`user_id` )
    REFERENCES `user` (`id` )
    ON DELETE NO ACTION
    ON UPDATE NO ACTION
)
ENGINE=InnoDB
DEFAULT CHARSET=utf8;


-- -----------------------------------------------------
-- Table `omim_gene`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `omim_gene`
(
  `id` INT(11) UNSIGNED NOT NULL AUTO_INCREMENT,
  `gene` VARCHAR(40) CHARACTER SET 'utf8' NOT NULL,
  `mim` VARCHAR(10) CHARACTER SET 'utf8' NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `mim_unique` (`mim`)
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;


-- -----------------------------------------------------
-- Table `omim_phenotype`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `omim_phenotype`
(
  `omim_gene_id` INT(11) UNSIGNED NOT NULL,
  `phenotype` VARCHAR(255) CHARACTER SET 'utf8' NOT NULL,
  PRIMARY KEY (`omim_gene_id`, `phenotype`),
  CONSTRAINT `omim_gene_id_FK`
    FOREIGN KEY (`omim_gene_id` )
    REFERENCES `omim_gene` (`id` )
    ON DELETE NO ACTION
    ON UPDATE NO ACTION
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;

-- -----------------------------------------------------
-- Table `omim_preferred_phenotype`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `omim_preferred_phenotype`
(
  `gene` VARCHAR(40) CHARACTER SET 'utf8' NOT NULL,
  `disease_group` ENUM('Neoplasms','Diseases of the blood or blood-forming organs','Diseases of the immune system','Endocrine, nutritional or metabolic diseases','Mental, behavioural or neurodevelopmental disorders','Sleep-wake disorders','Diseases of the nervous system','Diseases of the visual system','Diseases of the ear or mastoid process','Diseases of the circulatory system','Diseases of the respiratory system','Diseases of the digestive system','Diseases of the skin','Diseases of the musculoskeletal system or connective tissue','Diseases of the genitourinary system','Developmental anomalies','Other diseases') NOT NULL,
  `phenotype_accession` VARCHAR(6) CHARACTER SET 'utf8' NOT NULL,
  PRIMARY KEY (`gene`,`disease_group`)
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;

-- -----------------------------------------------------
-- Table `merged_processed_samples`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `merged_processed_samples`
(
  `processed_sample_id` INT(11) NOT NULL,
  `merged_into` INT(11) NOT NULL,
  PRIMARY KEY (`processed_sample_id`),
  CONSTRAINT `merged_processed_samples_ps_id`
    FOREIGN KEY (`processed_sample_id`)
    REFERENCES `processed_sample` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `merged_processed_samples_merged_into`
    FOREIGN KEY (`merged_into`)
    REFERENCES `processed_sample` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;

-- -----------------------------------------------------
-- Table `somatic_report_configuration`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `somatic_report_configuration` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `ps_tumor_id` int(11) NOT NULL,
  `ps_normal_id` int(11) NOT NULL,
  `created_by` int(11) NOT NULL,
  `created_date` DATETIME NOT NULL,
  `last_edit_by` int(11) DEFAULT NULL,
  `last_edit_date` timestamp NULL DEFAULT NULL,
  `mtb_xml_upload_date` timestamp NULL DEFAULT NULL,
  `target_file` VARCHAR(255) NULL DEFAULT NULL COMMENT 'filename of sub-panel BED file without path',
  `tum_content_max_af` BOOLEAN NOT NULL DEFAULT FALSE COMMENT 'include tumor content calculated by median value maximum allele frequency',
  `tum_content_max_clonality` BOOLEAN NOT NULL DEFAULT FALSE COMMENT 'include tumor content calculated by maximum CNV clonality',
  `tum_content_hist` BOOLEAN NOT NULL DEFAULT FALSE COMMENT 'include histological tumor content estimate ',
  `msi_status` BOOLEAN NOT NULL DEFAULT FALSE COMMENT 'include microsatellite instability status',
  `cnv_burden` BOOLEAN NOT NULL DEFAULT FALSE,
  `hrd_score` TINYINT(1) NOT NULL DEFAULT 0 COMMENT 'homologous recombination deficiency score',
  `tmb_ref_text` VARCHAR(200) NULL DEFAULT NULL COMMENT 'reference data as free text for tumor mutation burden',
  `quality` ENUM('no abnormalities','tumor cell content too low', 'quality of tumor DNA too low', 'DNA quantity too low', 'heterogeneous sample') NULL DEFAULT NULL COMMENT 'user comment on the quality of the DNA',
  `fusions_detected` BOOLEAN NOT NULL DEFAULT FALSE COMMENT 'fusions or other SVs were detected. Cannot be determined automatically, because manta files contain too many false positives',
  `cin_chr` TEXT NULL DEFAULT NULL COMMENT 'comma separated list of instable chromosomes',
  `limitations` TEXT NULL DEFAULT NULL COMMENT 'manually created text if the analysis has special limitations',
  `filter` VARCHAR(255) NULL DEFAULT NULL COMMENT 'name of the variant filter', 
  PRIMARY KEY (`id`),
  UNIQUE INDEX `combo_som_rep_conf_ids` (`ps_tumor_id` ASC, `ps_normal_id` ASC),
  CONSTRAINT `somatic_report_config_created_by_user` 
    FOREIGN KEY (`created_by`)
    REFERENCES `user` (`id`) 
    ON DELETE NO ACTION 
    ON UPDATE NO ACTION,
  CONSTRAINT `somatic_report_config_last_edit_by_user`
    FOREIGN KEY (`last_edit_by`)
    REFERENCES `user` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `somatic_report_config_ps_normal_id`
    FOREIGN KEY (`ps_normal_id`)
    REFERENCES `processed_sample` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `somatic_report_config_ps_tumor_id`
    FOREIGN KEY (`ps_tumor_id`) 
    REFERENCES `processed_sample` (`id`) 
    ON DELETE NO ACTION 
    ON UPDATE NO ACTION
)
ENGINE = InnoDB
DEFAULT CHARSET = utf8;

-- -----------------------------------------------------
-- Table `somatic_report_configuration_variant`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `somatic_report_configuration_variant` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `somatic_report_configuration_id` int(11) NOT NULL,
  `variant_id` int(11) NOT NULL,
  `exclude_artefact` BOOLEAN NOT NULL,
  `exclude_low_tumor_content` BOOLEAN NOT NULL,
  `exclude_low_copy_number` BOOLEAN NOT NULL,
  `exclude_high_baf_deviation` BOOLEAN NOT NULL,
  `exclude_other_reason` BOOLEAN NOT NULL,
  `include_variant_alteration` text DEFAULT NULL,
  `include_variant_description` text DEFAULT NULL,
  `comment` text NOT NULL,
  PRIMARY KEY (`id`),
  CONSTRAINT `som_rep_conf_var_has_som_rep_conf_id` 
    FOREIGN KEY (`somatic_report_configuration_id`) 
    REFERENCES `somatic_report_configuration` (`id`) 
    ON DELETE NO ACTION 
    ON UPDATE NO ACTION,
  CONSTRAINT `somatic_report_configuration_variant_has_variant_id`
    FOREIGN KEY (`variant_id`)
    REFERENCES `variant` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  UNIQUE INDEX `som_conf_var_combo_uniq_index` (`somatic_report_configuration_id` ASC, `variant_id` ASC)
)
ENGINE=InnoDB
DEFAULT CHARSET=utf8;

-- -----------------------------------------------------
-- Table `somatic_report_configuration_germl_snv`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `somatic_report_configuration_germl_var` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `somatic_report_configuration_id` int(11) NOT NULL,
  `variant_id` int(11) NOT NULL,
  `tum_freq` FLOAT UNSIGNED NULL DEFAULT NULL COMMENT 'frequency of this variant in the tumor sample.',
  `tum_depth` int(11) UNSIGNED NULL DEFAULT NULL COMMENT 'depth of this variant in the tumor sample.',
  PRIMARY KEY (`id`),
  CONSTRAINT `som_rep_conf_germl_var_has_rep_conf_id`
    FOREIGN KEY (`somatic_report_configuration_id`)
	REFERENCES `somatic_report_configuration` (`id`)
	ON DELETE NO ACTION
	ON UPDATE NO ACTION,
  CONSTRAINT `som_rep_germl_var_has_var_id`
    FOREIGN KEY (`variant_id`)
	REFERENCES `variant` (`id`)
	ON DELETE NO ACTION
	ON UPDATE NO ACTION,
  UNIQUE INDEX `som_conf_germl_var_combo_uni_idx` (`somatic_report_configuration_id` ASC, `variant_id` ASC)
) COMMENT='variants detected in control tissue that are marked as tumor related by the user'
ENGINE=InnoDB
DEFAULT CHARSET=utf8;


-- -----------------------------------------------------
-- Table `somatic_cnv_callset`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `somatic_cnv_callset`
(
  `id` INT(11) UNSIGNED NOT NULL AUTO_INCREMENT,
  `ps_tumor_id` INT(11) NOT NULL,
  `ps_normal_id` INT(11) NOT NULL,
  `caller` ENUM('ClinCNV') NOT NULL,
  `caller_version` varchar(25) NOT NULL,
  `call_date` DATETIME NOT NULL,
  `quality_metrics` TEXT DEFAULT NULL COMMENT 'quality metrics as JSON key-value array',
  `quality` ENUM('n/a','good','medium','bad') NOT NULL DEFAULT 'n/a',
  PRIMARY KEY (`id`),
  INDEX `caller` (`caller` ASC),
  INDEX `call_date` (`call_date` ASC),
  INDEX `quality` (`quality` ASC),
  UNIQUE INDEX `combo_ids` (`ps_tumor_id` ASC, `ps_normal_id` ASC),
  CONSTRAINT `som_cnv_callset_ps_normal_id`
    FOREIGN KEY (`ps_normal_id`)
    REFERENCES `processed_sample` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `som_cnv_callset_ps_tumor_id`
    FOREIGN KEY (`ps_tumor_id`) 
    REFERENCES `processed_sample` (`id`) 
    ON DELETE NO ACTION 
    ON UPDATE NO ACTION
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8
COMMENT='somatic CNV call set';

-- -----------------------------------------------------
-- Table `somatic_cnv`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `somatic_cnv`
(
  `id` INT(11) UNSIGNED NOT NULL AUTO_INCREMENT,
  `somatic_cnv_callset_id` INT(11) UNSIGNED NOT NULL,
  `chr` ENUM('chr1','chr2','chr3','chr4','chr5','chr6','chr7','chr8','chr9','chr10','chr11','chr12','chr13','chr14','chr15','chr16','chr17','chr18','chr19','chr20','chr21','chr22','chrY','chrX','chrMT') NOT NULL,
  `start` INT(11) UNSIGNED NOT NULL,
  `end` INT(11) UNSIGNED NOT NULL,
  `cn` FLOAT UNSIGNED NOT NULL COMMENT 'copy-number change in whole sample, including normal parts',
  `tumor_cn` INT(11) UNSIGNED NOT NULL COMMENT 'copy-number change normalized to tumor tissue only',
  `tumor_clonality` FLOAT NOT NULL COMMENT 'tumor clonality, i.e. fraction of tumor cells',
  `quality_metrics` TEXT DEFAULT NULL COMMENT 'quality metrics as JSON key-value array',
  PRIMARY KEY (`id`),
  CONSTRAINT `som_cnv_references_cnv_callset`
    FOREIGN KEY (`somatic_cnv_callset_id`)
    REFERENCES `somatic_cnv_callset` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `unique_callset_cnv_pair`
    UNIQUE(`somatic_cnv_callset_id`,`chr`,`start`,`end`),
  INDEX `chr` (`chr` ASC),
  INDEX `start` (`start` ASC),
  INDEX `end` (`end` ASC),
  INDEX `tumor_cn` (`tumor_cn` ASC)
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8
COMMENT='somatic CNV';

-- -----------------------------------------------------
-- Table `somatic_report_configuration_cnv`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `somatic_report_configuration_cnv` 
(
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `somatic_report_configuration_id` int(11) NOT NULL,
  `somatic_cnv_id` int(11) UNSIGNED NOT NULL,
  `exclude_artefact` BOOLEAN NOT NULL,
  `exclude_low_tumor_content` BOOLEAN NOT NULL,
  `exclude_low_copy_number` BOOLEAN NOT NULL,
  `exclude_high_baf_deviation` BOOLEAN NOT NULL,
  `exclude_other_reason` BOOLEAN NOT NULL,
  `comment` text NOT NULL,
  PRIMARY KEY (`id`),
  CONSTRAINT `som_rep_conf_cnv_has_som_rep_conf_id` 
    FOREIGN KEY (`somatic_report_configuration_id`) 
    REFERENCES `somatic_report_configuration` (`id`) 
    ON DELETE NO ACTION 
    ON UPDATE NO ACTION,
  CONSTRAINT `som_report_conf_cnv_has_som_cnv_id`
    FOREIGN KEY (`somatic_cnv_id`)
    REFERENCES `somatic_cnv` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
    UNIQUE INDEX `som_conf_cnv_combo_uniq_index` (`somatic_report_configuration_id` ASC, `somatic_cnv_id` ASC)
)
ENGINE=InnoDB
DEFAULT CHARSET=utf8;


-- -----------------------------------------------------
-- Table `report_configuration`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `report_configuration`
(
  `id` INT(11) NOT NULL AUTO_INCREMENT,
  `processed_sample_id` INT(11) NOT NULL,
  `created_by` int(11) NOT NULL,
  `created_date` DATETIME NOT NULL,
  `last_edit_by` int(11) DEFAULT NULL,
  `last_edit_date` TIMESTAMP NULL DEFAULT NULL,
  `finalized_by` int(11) DEFAULT NULL,
  `finalized_date` TIMESTAMP NULL DEFAULT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `processed_sample_id_unique` (`processed_sample_id`),
  CONSTRAINT `fk_processed_sample_id2`
    FOREIGN KEY (`processed_sample_id` )
    REFERENCES `processed_sample` (`id` )
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `report_configuration_created_by`
    FOREIGN KEY (`created_by`)
    REFERENCES `user` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `report_configuration_last_edit_by`
    FOREIGN KEY (`last_edit_by`)
    REFERENCES `user` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `report_configuration_finalized_by`
    FOREIGN KEY (`finalized_by`)
    REFERENCES `user` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;

-- -----------------------------------------------------
-- Table `report_configuration_variant`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `report_configuration_variant`
(
  `id` INT(11) NOT NULL AUTO_INCREMENT,
  `report_configuration_id` INT(11) NOT NULL,
  `variant_id` INT(11) NOT NULL,
  `type` ENUM('diagnostic variant', 'candidate variant', 'incidental finding') NOT NULL,
  `causal` BOOLEAN NOT NULL,
  `inheritance` ENUM('n/a', 'AR','AD','XLR','XLD','MT') NOT NULL,
  `de_novo` BOOLEAN NOT NULL,
  `mosaic` BOOLEAN NOT NULL,
  `compound_heterozygous` BOOLEAN NOT NULL,
  `exclude_artefact` BOOLEAN NOT NULL,
  `exclude_frequency` BOOLEAN NOT NULL,
  `exclude_phenotype` BOOLEAN NOT NULL,
  `exclude_mechanism` BOOLEAN NOT NULL,
  `exclude_other` BOOLEAN NOT NULL,
  `comments` text NOT NULL,
  `comments2` text NOT NULL,
  PRIMARY KEY (`id`),
  CONSTRAINT `fk_report_configuration`
    FOREIGN KEY (`report_configuration_id` )
    REFERENCES `report_configuration` (`id` )
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `fk_report_configuration_variant_has_variant`
    FOREIGN KEY (`variant_id`)
    REFERENCES `variant` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  UNIQUE INDEX `config_variant_combo_uniq` (`report_configuration_id` ASC, `variant_id` ASC)
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;

-- -----------------------------------------------------
-- Table `disease_term`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `disease_term`
(
  `id` INT(11) UNSIGNED NOT NULL AUTO_INCREMENT,
  `source` ENUM('OrphaNet') NOT NULL,
  `identifier` VARCHAR(40) CHARACTER SET 'utf8' NOT NULL,
  `name` TEXT CHARACTER SET 'utf8' NOT NULL,
  `synonyms` TEXT CHARACTER SET 'utf8' DEFAULT NULL,
  PRIMARY KEY (`id`),
  INDEX `disease_source` (`source` ASC),
  UNIQUE KEY `disease_id` (`identifier`),
  INDEX `disease_name` (`name`(50) ASC),
  INDEX `disease_synonyms` (`synonyms`(50) ASC)
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;

-- -----------------------------------------------------
-- Table `disease_gene`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `disease_gene`
(
  `disease_term_id` INT(11) UNSIGNED NOT NULL,
  `gene` VARCHAR(40) CHARACTER SET 'utf8' NOT NULL,
  PRIMARY KEY (`disease_term_id`, `gene`),
  CONSTRAINT `disease_genes_ibfk_1`
    FOREIGN KEY (`disease_term_id`)
    REFERENCES `disease_term` (`id`)
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;

-- -----------------------------------------------------
-- Table `cnv_callset`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `cnv_callset`
(
  `id` INT(11) UNSIGNED NOT NULL AUTO_INCREMENT,
  `processed_sample_id` INT(11) NOT NULL,
  `caller` ENUM('CnvHunter', 'ClinCNV') NOT NULL,
  `caller_version` varchar(25) NOT NULL,
  `call_date` DATETIME DEFAULT NULL,
  `quality_metrics` TEXT DEFAULT NULL COMMENT 'quality metrics as JSON key-value array',
  `quality` ENUM('n/a','good','medium','bad') NOT NULL DEFAULT 'n/a',
  PRIMARY KEY (`id`),
  INDEX `caller` (`quality` ASC),
  INDEX `call_date` (`call_date` ASC),
  INDEX `quality` (`quality` ASC),
  UNIQUE KEY `cnv_callset_references_processed_sample` (`processed_sample_id`),
  CONSTRAINT `cnv_callset_references_processed_sample`
    FOREIGN KEY (`processed_sample_id`)
    REFERENCES `processed_sample` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8
COMMENT='germline CNV call set';

-- -----------------------------------------------------
-- Table `cnv`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `cnv`
(
  `id` INT(11) UNSIGNED NOT NULL AUTO_INCREMENT,
  `cnv_callset_id` INT(11) UNSIGNED NOT NULL,
  `chr` ENUM('chr1','chr2','chr3','chr4','chr5','chr6','chr7','chr8','chr9','chr10','chr11','chr12','chr13','chr14','chr15','chr16','chr17','chr18','chr19','chr20','chr21','chr22','chrY','chrX','chrMT') NOT NULL,
  `start` INT(11) UNSIGNED NOT NULL,
  `end` INT(11) UNSIGNED NOT NULL,
  `cn` INT(11) UNSIGNED NOT NULL COMMENT 'copy-number',
  `quality_metrics` TEXT DEFAULT NULL COMMENT 'quality metrics as JSON key-value array',
  PRIMARY KEY (`id`),
  CONSTRAINT `cnv_references_cnv_callset`
    FOREIGN KEY (`cnv_callset_id`)
    REFERENCES `cnv_callset` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  INDEX `chr` (`chr` ASC),
  INDEX `start` (`start` ASC),
  INDEX `end` (`end` ASC),
  INDEX `cn` (`cn` ASC)
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8
COMMENT='germline CNV';

-- -----------------------------------------------------
-- Table `report_configuration_cnv`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `report_configuration_cnv`
(
  `id` INT(11) NOT NULL AUTO_INCREMENT,
  `report_configuration_id` INT(11) NOT NULL,
  `cnv_id` INT(11) UNSIGNED NOT NULL,
  `type` ENUM('diagnostic variant', 'candidate variant', 'incidental finding') NOT NULL,
  `causal` BOOLEAN NOT NULL,
  `class` ENUM('n/a','1','2','3','4','5','M') NOT NULL,
  `inheritance` ENUM('n/a', 'AR','AD','XLR','XLD','MT') NOT NULL,
  `de_novo` BOOLEAN NOT NULL,
  `mosaic` BOOLEAN NOT NULL,
  `compound_heterozygous` BOOLEAN NOT NULL,
  `exclude_artefact` BOOLEAN NOT NULL,
  `exclude_frequency` BOOLEAN NOT NULL,
  `exclude_phenotype` BOOLEAN NOT NULL,
  `exclude_mechanism` BOOLEAN NOT NULL,
  `exclude_other` BOOLEAN NOT NULL,
  `comments` text NOT NULL,
  `comments2` text NOT NULL,
  PRIMARY KEY (`id`),
  CONSTRAINT `fk_report_configuration2`
    FOREIGN KEY (`report_configuration_id` )
    REFERENCES `report_configuration` (`id` )
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `fk_report_configuration_cnv_has_cnv`
    FOREIGN KEY (`cnv_id`)
    REFERENCES `cnv` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  UNIQUE INDEX `config_variant_combo_uniq` (`report_configuration_id` ASC, `cnv_id` ASC)
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;

-- -----------------------------------------------------
-- Table `sv_callset`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `sv_callset`
(
  `id` INT(11) UNSIGNED NOT NULL AUTO_INCREMENT,
  `processed_sample_id` INT(11) NOT NULL,
  `caller` ENUM('Manta') NOT NULL,
  `caller_version` varchar(25) NOT NULL,
  `call_date` DATETIME DEFAULT NULL,
  PRIMARY KEY (`id`),
  INDEX `call_date` (`call_date` ASC),
  UNIQUE KEY `sv_callset_references_processed_sample` (`processed_sample_id`),
  CONSTRAINT `sv_callset_references_processed_sample`
    FOREIGN KEY (`processed_sample_id`)
    REFERENCES `processed_sample` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8
COMMENT='SV call set';

-- -----------------------------------------------------
-- Table `sv_deletion`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `sv_deletion`
(
  `id` INT(11) UNSIGNED NOT NULL AUTO_INCREMENT,
  `sv_callset_id` INT(11) UNSIGNED NOT NULL,
  `chr` ENUM('chr1','chr2','chr3','chr4','chr5','chr6','chr7','chr8','chr9','chr10','chr11','chr12','chr13','chr14','chr15','chr16','chr17','chr18','chr19','chr20','chr21','chr22','chrY','chrX','chrMT') NOT NULL,
  `start_min` INT(11) UNSIGNED NOT NULL,
  `start_max` INT(11) UNSIGNED NOT NULL,
  `end_min` INT(11) UNSIGNED NOT NULL,
  `end_max` INT(11) UNSIGNED NOT NULL,
  `quality_metrics` TEXT DEFAULT NULL COMMENT 'quality metrics as JSON key-value array',
  PRIMARY KEY (`id`),
  CONSTRAINT `sv_del_references_sv_callset`
    FOREIGN KEY (`sv_callset_id`)
    REFERENCES `sv_callset` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  INDEX `exact_match` (`chr`, `start_min`, `start_max`, `end_min`, `end_max`),
  INDEX `overlap_match` (`chr`, `start_min`, `end_max`)
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8
COMMENT='SV deletion';

-- -----------------------------------------------------
-- Table `sv_duplication`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `sv_duplication`
(
  `id` INT(11) UNSIGNED NOT NULL AUTO_INCREMENT,
  `sv_callset_id` INT(11) UNSIGNED NOT NULL,
  `chr` ENUM('chr1','chr2','chr3','chr4','chr5','chr6','chr7','chr8','chr9','chr10','chr11','chr12','chr13','chr14','chr15','chr16','chr17','chr18','chr19','chr20','chr21','chr22','chrY','chrX','chrMT') NOT NULL,
  `start_min` INT(11) UNSIGNED NOT NULL,
  `start_max` INT(11) UNSIGNED NOT NULL,
  `end_min` INT(11) UNSIGNED NOT NULL,
  `end_max` INT(11) UNSIGNED NOT NULL,
  `quality_metrics` TEXT DEFAULT NULL COMMENT 'quality metrics as JSON key-value array',
  PRIMARY KEY (`id`),
  CONSTRAINT `sv_dup_references_sv_callset`
    FOREIGN KEY (`sv_callset_id`)
    REFERENCES `sv_callset` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  INDEX `exact_match` (`chr`, `start_min`, `start_max`, `end_min`, `end_max`),
  INDEX `overlap_match` (`chr`, `start_min`, `end_max`)
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8
COMMENT='SV duplication';

-- -----------------------------------------------------
-- Table `sv_insertion`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `sv_insertion`
(
  `id` INT(11) UNSIGNED NOT NULL AUTO_INCREMENT,
  `sv_callset_id` INT(11) UNSIGNED NOT NULL,
  `chr` ENUM('chr1','chr2','chr3','chr4','chr5','chr6','chr7','chr8','chr9','chr10','chr11','chr12','chr13','chr14','chr15','chr16','chr17','chr18','chr19','chr20','chr21','chr22','chrY','chrX','chrMT') NOT NULL,
  `pos`INT(11) UNSIGNED NOT NULL,
  `ci_lower` INT(5) UNSIGNED NOT NULL DEFAULT 0,
  `ci_upper` INT(5) UNSIGNED NOT NULL,
  `inserted_sequence` TEXT DEFAULT NULL,
  `known_left` TEXT DEFAULT NULL,
  `known_right` TEXT DEFAULT NULL,
  `quality_metrics` TEXT DEFAULT NULL COMMENT 'quality metrics as JSON key-value array',
  PRIMARY KEY (`id`),
  CONSTRAINT `sv_ins_references_sv_callset`
    FOREIGN KEY (`sv_callset_id`)
    REFERENCES `sv_callset` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  INDEX `match` (`chr`, `pos`, `ci_upper`)
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8
COMMENT='SV insertion';

-- -----------------------------------------------------
-- Table `sv_inversion`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `sv_inversion`
(
  `id` INT(11) UNSIGNED NOT NULL AUTO_INCREMENT,
  `sv_callset_id` INT(11) UNSIGNED NOT NULL,
  `chr` ENUM('chr1','chr2','chr3','chr4','chr5','chr6','chr7','chr8','chr9','chr10','chr11','chr12','chr13','chr14','chr15','chr16','chr17','chr18','chr19','chr20','chr21','chr22','chrY','chrX','chrMT') NOT NULL,
  `start_min` INT(11) UNSIGNED NOT NULL,
  `start_max` INT(11) UNSIGNED NOT NULL,
  `end_min` INT(11) UNSIGNED NOT NULL,
  `end_max` INT(11) UNSIGNED NOT NULL,
  `quality_metrics` TEXT DEFAULT NULL COMMENT 'quality metrics as JSON key-value array',
  PRIMARY KEY (`id`),
  CONSTRAINT `sv_inv_references_sv_callset`
    FOREIGN KEY (`sv_callset_id`)
    REFERENCES `sv_callset` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  INDEX `exact_match` (`chr`, `start_min`, `start_max`, `end_min`, `end_max`),
  INDEX `overlap_match` (`chr`, `start_min`, `end_max`)
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8
COMMENT='SV inversion';

-- -----------------------------------------------------
-- Table `sv_translocation`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `sv_translocation`
(
  `id` INT(11) UNSIGNED NOT NULL AUTO_INCREMENT,
  `sv_callset_id` INT(11) UNSIGNED NOT NULL,
  `chr1` ENUM('chr1','chr2','chr3','chr4','chr5','chr6','chr7','chr8','chr9','chr10','chr11','chr12','chr13','chr14','chr15','chr16','chr17','chr18','chr19','chr20','chr21','chr22','chrY','chrX','chrMT') NOT NULL,
  `start1` INT(11) UNSIGNED NOT NULL,
  `end1` INT(11) UNSIGNED NOT NULL,
  `chr2` ENUM('chr1','chr2','chr3','chr4','chr5','chr6','chr7','chr8','chr9','chr10','chr11','chr12','chr13','chr14','chr15','chr16','chr17','chr18','chr19','chr20','chr21','chr22','chrY','chrX','chrMT') NOT NULL,
  `start2` INT(11) UNSIGNED NOT NULL,
  `end2` INT(11) UNSIGNED NOT NULL,
  `quality_metrics` TEXT DEFAULT NULL COMMENT 'quality metrics as JSON key-value array',
  PRIMARY KEY (`id`),
  CONSTRAINT `sv_bnd_references_sv_callset`
    FOREIGN KEY (`sv_callset_id`)
    REFERENCES `sv_callset` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  INDEX `match` (`chr1`, `start1`, `end1`, `chr2`, `start2`, `end2`)
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8
COMMENT='SV translocation';

-- -----------------------------------------------------
-- Table `report_configuration_sv`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `report_configuration_sv`
(
  `id` INT(11) NOT NULL AUTO_INCREMENT,
  `report_configuration_id` INT(11) NOT NULL,
  `sv_deletion_id` INT(11) UNSIGNED DEFAULT NULL,
  `sv_duplication_id` INT(11) UNSIGNED DEFAULT NULL,
  `sv_insertion_id` INT(11) UNSIGNED DEFAULT NULL,
  `sv_inversion_id` INT(11) UNSIGNED DEFAULT NULL,
  `sv_translocation_id` INT(11) UNSIGNED DEFAULT NULL,
  `type` ENUM('diagnostic variant', 'candidate variant', 'incidental finding') NOT NULL,
  `causal` BOOLEAN NOT NULL,
  `class` ENUM('n/a','1','2','3','4','5','M') NOT NULL,
  `inheritance` ENUM('n/a', 'AR','AD','XLR','XLD','MT') NOT NULL,
  `de_novo` BOOLEAN NOT NULL,
  `mosaic` BOOLEAN NOT NULL,
  `compound_heterozygous` BOOLEAN NOT NULL,
  `exclude_artefact` BOOLEAN NOT NULL,
  `exclude_frequency` BOOLEAN NOT NULL,
  `exclude_phenotype` BOOLEAN NOT NULL,
  `exclude_mechanism` BOOLEAN NOT NULL,
  `exclude_other` BOOLEAN NOT NULL,
  `comments` text NOT NULL,
  `comments2` text NOT NULL,
  PRIMARY KEY (`id`),
  CONSTRAINT `fk_report_configuration3`
    FOREIGN KEY (`report_configuration_id` )
    REFERENCES `report_configuration` (`id` )
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `fk_report_configuration_sv_has_sv_deletion`
    FOREIGN KEY (`sv_deletion_id`)
    REFERENCES `sv_deletion` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `fk_report_configuration_sv_has_sv_duplication`
    FOREIGN KEY (`sv_duplication_id`)
    REFERENCES `sv_duplication` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `fk_report_configuration_sv_has_sv_insertion`
    FOREIGN KEY (`sv_insertion_id`)
    REFERENCES `sv_insertion` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `fk_report_configuration_sv_has_sv_inversion`
    FOREIGN KEY (`sv_inversion_id`)
    REFERENCES `sv_inversion` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `fk_report_configuration_sv_has_sv_translocation`
    FOREIGN KEY (`sv_translocation_id`)
    REFERENCES `sv_translocation` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  UNIQUE INDEX `config_variant_combo_uniq` (`report_configuration_id` ASC, `sv_deletion_id` ASC, `sv_duplication_id` ASC, `sv_insertion_id` ASC, `sv_inversion_id` ASC, `sv_translocation_id` ASC)
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;

-- -----------------------------------------------------
-- Table `evaluation_sheet_data`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `evaluation_sheet_data`
(
  `id` INT(11) NOT NULL AUTO_INCREMENT,
  `processed_sample_id` INT(11) NOT NULL, 
  `dna_rna_id` TEXT CHARACTER SET 'utf8' DEFAULT NULL,
  `reviewer1` INT NOT NULL,
  `review_date1` DATE NOT NULL,
  `reviewer2` INT NOT NULL,
  `review_date2` DATE NOT NULL,
  `analysis_scope` TEXT CHARACTER SET 'utf8' DEFAULT NULL,
  `acmg_requested` BOOLEAN DEFAULT FALSE NOT NULL,
  `acmg_analyzed` BOOLEAN DEFAULT FALSE NOT NULL,
  `acmg_noticeable` BOOLEAN DEFAULT FALSE NOT NULL,
  `filtered_by_freq_based_dominant` BOOLEAN DEFAULT FALSE NOT NULL,
  `filtered_by_freq_based_recessive` BOOLEAN DEFAULT FALSE NOT NULL,
  `filtered_by_cnv` BOOLEAN DEFAULT FALSE NOT NULL,
  `filtered_by_mito` BOOLEAN DEFAULT FALSE NOT NULL,
  `filtered_by_x_chr` BOOLEAN DEFAULT FALSE NOT NULL,
  `filtered_by_phenotype` BOOLEAN DEFAULT FALSE NOT NULL,
  `filtered_by_multisample` BOOLEAN DEFAULT FALSE NOT NULL,
  `filtered_by_trio_stringent` BOOLEAN DEFAULT FALSE NOT NULL,
  `filtered_by_trio_relaxed` BOOLEAN DEFAULT FALSE NOT NULL,
  PRIMARY KEY (`id`),
  INDEX `processed_sample_id` (`processed_sample_id` ASC),
  UNIQUE KEY `evaluation_sheet_data_references_processed_sample` (`processed_sample_id`),
  CONSTRAINT `evaluation_sheet_data_references_processed_sample`
    FOREIGN KEY (`processed_sample_id`)
    REFERENCES `processed_sample` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `evaluation_sheet_data_references_user1`
    FOREIGN KEY (`reviewer1`)
    REFERENCES `user` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `evaluation_sheet_data_references_user2`
    FOREIGN KEY (`reviewer2`)
    REFERENCES `user` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;

-- -----------------------------------------------------
-- Table `preferred_transcripts`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `preferred_transcripts`
(
  `id` INT(11) NOT NULL AUTO_INCREMENT,
  `name` varchar(40) NOT NULL,
  `added_by` INT(11) NOT NULL,
  `added_date` timestamp NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE INDEX `combo_som_rep_conf_ids` (`name` ASC),
  CONSTRAINT `preferred_transcriptsg_created_by_user`
    FOREIGN KEY (`added_by`)
    REFERENCES `user` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;

SET SQL_MODE=@OLD_SQL_MODE;
SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS;
SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS;

-- -----------------------------------------------------
-- Table `variant_validation`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `variant_validation`
(
  `id` INT(11) NOT NULL AUTO_INCREMENT,
  `user_id` INT(11) NOT NULL,
  `date` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `sample_id` INT(11) NOT NULL,
  `variant_type` ENUM('SNV_INDEL', 'CNV', 'SV') NOT NULL,
  `variant_id` INT(11) DEFAULT NULL,
  `cnv_id` INT(11) UNSIGNED DEFAULT NULL,
  `sv_deletion_id` INT(11) UNSIGNED DEFAULT NULL,
  `sv_duplication_id` INT(11) UNSIGNED DEFAULT NULL,
  `sv_insertion_id` INT(11) UNSIGNED DEFAULT NULL,
  `sv_inversion_id` INT(11) UNSIGNED DEFAULT NULL,
  `sv_translocation_id` INT(11) UNSIGNED DEFAULT NULL,
  `genotype` ENUM('hom','het') DEFAULT NULL,
  `validation_method` ENUM('Sanger sequencing', 'breakpoint PCR', 'qPCR', 'MLPA', 'Array', 'n/a') NOT NULL DEFAULT 'n/a',
  `status` ENUM('n/a','to validate','to segregate','for reporting','true positive','false positive','wrong genotype') NOT NULL DEFAULT 'n/a',
  `comment` TEXT NULL DEFAULT NULL,
PRIMARY KEY (`id`),
INDEX `fk_user_id` (`user_id` ASC),
CONSTRAINT `vv_user`
  FOREIGN KEY (`user_id`)
  REFERENCES `user` (`id`)
  ON DELETE NO ACTION
  ON UPDATE NO ACTION,
CONSTRAINT `fk_variant_validation_has_sample`
    FOREIGN KEY (`sample_id`)
    REFERENCES `sample` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
CONSTRAINT `fk_variant_validation_has_variant`
  FOREIGN KEY (`variant_id`)
  REFERENCES `variant` (`id`)
  ON DELETE NO ACTION
  ON UPDATE NO ACTION,
CONSTRAINT `fk_variant_validation_has_cnv`
  FOREIGN KEY (`cnv_id`)
  REFERENCES `cnv` (`id`)
  ON DELETE NO ACTION
  ON UPDATE NO ACTION,
CONSTRAINT `fk_variant_validation_has_sv_deletion`
  FOREIGN KEY (`sv_deletion_id`)
  REFERENCES `sv_deletion` (`id`)
  ON DELETE NO ACTION
  ON UPDATE NO ACTION,
CONSTRAINT `fk_variant_validation_has_sv_duplication`
  FOREIGN KEY (`sv_duplication_id`)
  REFERENCES `sv_duplication` (`id`)
  ON DELETE NO ACTION
  ON UPDATE NO ACTION,
CONSTRAINT `fk_variant_validation_has_sv_insertion`
  FOREIGN KEY (`sv_insertion_id`)
  REFERENCES `sv_insertion` (`id`)
  ON DELETE NO ACTION
  ON UPDATE NO ACTION,
CONSTRAINT `fk_variant_validation_has_sv_inversion`
  FOREIGN KEY (`sv_inversion_id`)
  REFERENCES `sv_inversion` (`id`)
  ON DELETE NO ACTION
  ON UPDATE NO ACTION,
CONSTRAINT `fk_variant_validation_has_sv_translocation`
  FOREIGN KEY (`sv_translocation_id`)
  REFERENCES `sv_translocation` (`id`)
  ON DELETE NO ACTION
  ON UPDATE NO ACTION,
UNIQUE INDEX `variant_validation_unique_var` (`sample_id`, `variant_id`, `cnv_id`, `sv_deletion_id`, `sv_duplication_id`, `sv_insertion_id`, `sv_inversion_id`, `sv_translocation_id`),
INDEX `status` (`status` ASC),
INDEX `variant_type` (`variant_type` ASC)
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;

-- -----------------------------------------------------
-- Table `study`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `study`
(
  `id` INT(11) NOT NULL AUTO_INCREMENT,
  `name` VARCHAR(50) NOT NULL,
  `description` TEXT NOT NULL,
PRIMARY KEY (`id`),
UNIQUE INDEX `name_UNIQUE` (`name` ASC)
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;

-- -----------------------------------------------------
-- Table `study_sample`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `study_sample`
(
  `id` INT(11) NOT NULL AUTO_INCREMENT,
  `study_id` INT(11) NOT NULL,
  `processed_sample_id` INT(11) NOT NULL,
  `study_sample_idendifier` VARCHAR(50) DEFAULT NULL,
PRIMARY KEY (`id`),
CONSTRAINT `fk_study_sample_has_study`
  FOREIGN KEY (`study_id`)
  REFERENCES `study` (`id`)
  ON DELETE NO ACTION
  ON UPDATE NO ACTION,
CONSTRAINT `fk_study_sample_has_ps`
  FOREIGN KEY (`processed_sample_id`)
  REFERENCES `processed_sample` (`id`)
  ON DELETE NO ACTION
  ON UPDATE NO ACTION,
  UNIQUE INDEX `unique_sample_ps` (`processed_sample_id`, `study_sample_idendifier`),
INDEX `i_study_sample_idendifier` (`study_sample_idendifier` ASC)
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;

-- -----------------------------------------------------
-- Table `secondary_analysis`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `secondary_analysis`
(
  `id` INT(11) NOT NULL AUTO_INCREMENT,
  `type` enum('multi sample','trio','somatic') NOT NULL,
  `gsvar_file` VARCHAR(1000) NOT NULL,
PRIMARY KEY (`id`),
UNIQUE INDEX `unique_gsvar` (`gsvar_file`)
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;

-- -----------------------------------------------------
-- Table `gaps`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `gaps`
(
  `id` INT(11) NOT NULL AUTO_INCREMENT,
  `chr` ENUM('chr1','chr2','chr3','chr4','chr5','chr6','chr7','chr8','chr9','chr10','chr11','chr12','chr13','chr14','chr15','chr16','chr17','chr18','chr19','chr20','chr21','chr22','chrY','chrX','chrMT') NOT NULL,
  `start` INT(11) UNSIGNED NOT NULL,
  `end` INT(11) UNSIGNED NOT NULL,
  `processed_sample_id` INT(11) NOT NULL,
  `status` enum('checked visually','to close','in progress','closed','canceled') NOT NULL,
  `history` TEXT,
PRIMARY KEY (`id`),
INDEX `gap_index` (`chr` ASC, `start` ASC, `end` ASC),
INDEX `processed_sample_id` (`processed_sample_id` ASC),
INDEX `status` (`status` ASC),
CONSTRAINT `fk_gap_has_processed_sample1`
  FOREIGN KEY (`processed_sample_id`)
  REFERENCES `processed_sample` (`id`)
  ON DELETE NO ACTION
  ON UPDATE NO ACTION
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;


-- -----------------------------------------------------
-- Table `gene_pseudogene_relation`
-- -----------------------------------------------------

CREATE TABLE IF NOT EXISTS `gene_pseudogene_relation`
(
`id` int(10) unsigned NOT NULL AUTO_INCREMENT,
`parent_gene_id` int(10) unsigned NOT NULL,
`pseudogene_gene_id` int(10) unsigned DEFAULT NULL,
`gene_name` varchar(40) DEFAULT NULL,

PRIMARY KEY (`id`),
CONSTRAINT `fk_parent_gene_id`
  FOREIGN KEY (`parent_gene_id` )
  REFERENCES `gene` (`id` )
  ON DELETE NO ACTION
  ON UPDATE NO ACTION,
CONSTRAINT `fk_pseudogene_gene_id`
  FOREIGN KEY (`pseudogene_gene_id` )
  REFERENCES `gene` (`id` )
  ON DELETE NO ACTION
  ON UPDATE NO ACTION,
UNIQUE KEY `pseudo_gene_relation` (`parent_gene_id`, `pseudogene_gene_id`, `gene_name`),
INDEX `parent_gene_id` (`parent_gene_id` ASC),
INDEX `pseudogene_gene_id` (`pseudogene_gene_id` ASC)
) 
ENGINE=InnoDB DEFAULT 
CHARSET=utf8
COMMENT='Gene-Pseudogene relation';

-- -----------------------------------------------------
-- Table `processed_sample_ancestry`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `processed_sample_ancestry`
(
  `processed_sample_id` INT(11) NOT NULL,
  `num_snps` INT(11) NOT NULL,
  `score_afr` FLOAT NOT NULL,
  `score_eur` FLOAT NOT NULL,
  `score_sas` FLOAT NOT NULL,
  `score_eas` FLOAT NOT NULL,
  `population` enum('AFR','EUR','SAS','EAS','ADMIXED/UNKNOWN') NOT NULL,
PRIMARY KEY (`processed_sample_id`),
CONSTRAINT `fk_processed_sample_ancestry_has_processed_sample`
  FOREIGN KEY (`processed_sample_id`)
  REFERENCES `processed_sample` (`id`)
  ON DELETE NO ACTION
  ON UPDATE NO ACTION
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;

-- -----------------------------------------------------
-- Table `subpanels`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `subpanels`
(
  `id` INT(11) NOT NULL AUTO_INCREMENT,
  `name` varchar(200) NOT NULL,
  `created_by` int(11) DEFAULT NULL,
  `created_date` DATE NOT NULL,
  `mode` ENUM('exon', 'gene') NOT NULL,
  `extend` INT(11) NOT NULL,
  `genes` MEDIUMTEXT NOT NULL,
  `roi` MEDIUMTEXT NOT NULL,
  `archived` TINYINT(1) NOT NULL,
PRIMARY KEY (`id`),
UNIQUE KEY `name` (`name`),
INDEX(`created_by`),
INDEX(`created_date`),
INDEX(`archived`),
CONSTRAINT `subpanels_created_by_user`
  FOREIGN KEY (`created_by`)
  REFERENCES `user` (`id`)
  ON DELETE NO ACTION
  ON UPDATE NO ACTION
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;

-- -----------------------------------------------------
-- Table `cfdna_panels`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `cfdna_panels`
(
  `id` INT(11) NOT NULL AUTO_INCREMENT,
  `tumor_id` INT(11) NOT NULL,
  `cfdna_id` INT(11) DEFAULT NULL,
  `created_by` INT(11) DEFAULT NULL,
  `created_date` DATE NOT NULL,
  `processing_system_id` INT(11) NOT NULL,
  `bed` MEDIUMTEXT NOT NULL,
  `vcf` MEDIUMTEXT NOT NULL,
PRIMARY KEY (`id`),
INDEX(`created_by`),
INDEX(`created_date`),
INDEX(`tumor_id`),
UNIQUE `unique_cfdna_panel`(`tumor_id`, `processing_system_id`),
CONSTRAINT `cfdna_panels_tumor_id`
  FOREIGN KEY (`tumor_id`)
  REFERENCES `processed_sample` (`id`)
  ON DELETE NO ACTION
  ON UPDATE NO ACTION,
CONSTRAINT `cfdna_panels_cfdna_id`
  FOREIGN KEY (`cfdna_id`)
  REFERENCES `processed_sample` (`id`)
  ON DELETE NO ACTION
  ON UPDATE NO ACTION,
CONSTRAINT `cfdna_panels_created_by_user`
  FOREIGN KEY (`created_by`)
  REFERENCES `user` (`id`)
  ON DELETE NO ACTION
  ON UPDATE NO ACTION,
CONSTRAINT `cfdna_panels_processing_system_id`
  FOREIGN KEY (`processing_system_id`)
  REFERENCES `processing_system` (`id`)
  ON DELETE NO ACTION
  ON UPDATE NO ACTION
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;

-- -----------------------------------------------------
-- Table `cfdna_panel_genes`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `cfdna_panel_genes`
(
  `id` INT(11) NOT NULL AUTO_INCREMENT,
  `gene_name` VARCHAR(40) CHARACTER SET 'utf8' NOT NULL,
  `chr` ENUM('chr1','chr2','chr3','chr4','chr5','chr6','chr7','chr8','chr9','chr10','chr11','chr12','chr13','chr14','chr15','chr16','chr17','chr18','chr19','chr20','chr21','chr22','chrY','chrX','chrMT') NOT NULL,
  `start` INT(11) UNSIGNED NOT NULL,
  `end` INT(11) UNSIGNED NOT NULL,
  `date` DATE NOT NULL,
  `bed` MEDIUMTEXT NOT NULL,
PRIMARY KEY (`id`),
UNIQUE INDEX(`gene_name`)
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;
