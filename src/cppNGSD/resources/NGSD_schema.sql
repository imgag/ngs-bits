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
  UNIQUE INDEX `UNIQUE_MAN_INDEX` (`name` ASC)
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
  `name_short` VARCHAR(50) NULL DEFAULT NULL,
  `name_manufacturer` VARCHAR(100) NULL DEFAULT NULL,
  `adapter1_p5` VARCHAR(45) NULL DEFAULT NULL,
  `adapter2_p7` VARCHAR(45) NULL DEFAULT NULL,
  `type` ENUM('WGS','WES','Panel','Panel Haloplex','Panel MIPs','RNA','ChIP-Seq') NOT NULL,
  `shotgun` TINYINT(1) NOT NULL,
  `umi_type` ENUM('n/a','HaloPlex HS','SureSelect HS','ThruPLEX','Safe-SeqS','MIPs') NOT NULL DEFAULT 'n/a',
  `target_file` VARCHAR(255) NULL DEFAULT NULL,
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
  `type` ENUM('GAIIx','MiSeq','HiSeq2500','NextSeq500','NovaSeq5000','NovaSeq6000','MGI-2000') NOT NULL,
  `name` VARCHAR(45) NOT NULL,
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
  `flowcell_type` ENUM('Illumina MiSeq v2','Illumina MiSeq v2 Micro','Illumina MiSeq v2 Nano','Illumina MiSeq v3','Illumina NextSeq High Output','Illumina NextSeq Mid Output','Illumina NovaSeq SP','Illumina NovaSeq S1','Illumina NovaSeq S2','Illumina NovaSeq S4','n/a') NOT NULL DEFAULT 'n/a',
  `start_date` DATE NULL DEFAULT NULL,
  `end_date` DATE NULL DEFAULT NULL,
  `device_id` INT(11) NOT NULL,
  `recipe` VARCHAR(45) NOT NULL,
  `pool_molarity` float DEFAULT NULL,
  `pool_quantification_method` enum('n/a','Tapestation','Bioanalyzer','qPCR','Tapestation & Qubit','Bioanalyzer & Qubit') NOT NULL DEFAULT 'n/a',
  `comment` TEXT NULL DEFAULT NULL,
  `quality` ENUM('n/a','good','medium','bad') NOT NULL DEFAULT 'n/a',
  `status` ENUM('n/a','run_started','run_finished','run_aborted','demultiplexing_started','analysis_started','analysis_finished','analysis_not_possible','analysis_and_backup_not_required') NOT NULL DEFAULT 'n/a',
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
  `user_id` VARCHAR(45) NOT NULL,
  `password` VARCHAR(64) NOT NULL,
  `user_role` ENUM('user','admin','special') NOT NULL,
  `name` VARCHAR(45) NOT NULL,
  `email` VARCHAR(45) NOT NULL,
  `created` DATE NOT NULL,
  `last_login` DATE NULL DEFAULT NULL,
  `active` TINYINT(1) NOT NULL,
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
  `name` VARCHAR(15) NOT NULL,
  `name_external` VARCHAR(255) NULL DEFAULT NULL,
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
  UNIQUE INDEX `id_UNIQUE` (`name` ASC),
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
  `disease_info` VARCHAR(255) NOT NULL,
  `type` ENUM('HPO term id', 'ICD10 code', 'OMIM disease/phenotype identifier', 'Orpha number', 'CGI cancer type', 'tumor fraction', 'age of onset', 'clinical phenotype (free text)') NOT NULL,
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
  `relation` ENUM('parent-child', 'tumor-normal', 'siblings', 'same sample') NOT NULL,
  `sample2_id` INT(11) NOT NULL,
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
  `internal_coordinator_id` INT(11) NOT NULL,
  `comment` TEXT NULL DEFAULT NULL,
  `analysis` ENUM('fastq','mapping','variant calling','annotation') NOT NULL DEFAULT 'annotation',
  `email_notification` varchar(200) DEFAULT NULL,
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
  `normal_id` INT(11) NULL DEFAULT NULL,
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
  INDEX `chr` (`chr` ASC),
  INDEX `start` (`start` ASC),
  INDEX `end` (`end` ASC),
  INDEX `ref` (`ref`(255) ASC),
  INDEX `obs` (`obs`(255) ASC),
  INDEX `gene` (`gene`(50) ASC),
  INDEX `1000g` (`1000g` ASC),
  INDEX `gnomad` (`gnomad` ASC),
  INDEX `comment` (`comment`(50) ASC)
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;


-- -----------------------------------------------------
-- Table `variant_validation`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `variant_validation`
(
  `id` INT(11) NOT NULL AUTO_INCREMENT,
  `user_id` INT(11) NOT NULL,
  `date` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `sample_id` INT(11) NOT NULL,
  `variant_id` INT(11) NOT NULL,
  `genotype` ENUM('hom','het') NOT NULL,
  `status` ENUM('n/a','to validate','to segregate','for reporting','true positive','false positive','wrong genotype') NOT NULL DEFAULT 'n/a',
  `type` ENUM('research','diagnostics') NOT NULL DEFAULT 'research',
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
UNIQUE INDEX `variant_validation_unique` (`sample_id`, `variant_id`),
INDEX `status` (`status` ASC)
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
-- Table `detected_variant_counts`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `detected_variant_counts`
(
  `variant_id` INT(11) NOT NULL,
  `count_het` INT(11) NOT NULL,
  `count_hom` INT(11) NOT NULL,
  PRIMARY KEY (`variant_id`),
  CONSTRAINT `fk_detected_variant_counts`
    FOREIGN KEY (`variant_id`)
    REFERENCES `variant` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8
COMMENT='Precalculated variant counts used for fast annotation';

-- -----------------------------------------------------
-- Table `detected_variant_counts_by_group`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `detected_variant_counts_by_group`
(
  `variant_id` INT(11) NOT NULL,
  `disease_group` ENUM('Neoplasms','Diseases of the blood or blood-forming organs','Diseases of the immune system','Endocrine, nutritional or metabolic diseases','Mental, behavioural or neurodevelopmental disorders','Sleep-wake disorders','Diseases of the nervous system','Diseases of the visual system','Diseases of the ear or mastoid process','Diseases of the circulatory system','Diseases of the respiratory system','Diseases of the digestive system','Diseases of the skin','Diseases of the musculoskeletal system or connective tissue','Diseases of the genitourinary system','Developmental anomalies','Other diseases') NOT NULL,
  `count_hom` INT(11) NOT NULL,
  `count_het` INT(11) NOT NULL,
  PRIMARY KEY (`variant_id`,`disease_group`),
  CONSTRAINT `fk_detected_variant_counts_by_group`
    FOREIGN KEY (`variant_id`)
    REFERENCES `variant` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8
COMMENT='Precalculated variant counts by disease group used for fast annotation. There is not entry for variant/group combination without a variant to save space.';

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

CREATE TABLE `analysis_job`
(
  `id` int(11) NOT NULL AUTO_INCREMENT,
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

CREATE TABLE `analysis_job_sample`
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

CREATE TABLE `analysis_job_history`
(
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `analysis_job_id` int(11) NOT NULL,
  `time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
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
-- Table `report_configuration`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `report_configuration`
(
  `id` INT(11) NOT NULL AUTO_INCREMENT,
  `processed_sample_id` INT(11) NOT NULL,
  `created_by` int(11) NOT NULL,
  `created_date` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `processed_sample_id_unique` (`processed_sample_id`),
  CONSTRAINT `fk_processed_sample_id2`
    FOREIGN KEY (`processed_sample_id` )
    REFERENCES `processed_sample` (`id` )
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `fk_user2`
    FOREIGN KEY (`created_by`)
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
  `inheritance` ENUM('n/a', 'AR','AD','AR+AD','XLR','XLD','XLR+XLD','MT') NOT NULL,
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
  `call_date` TIMESTAMP NOT NULL,
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
  INDEX `cn` (`end` ASC)
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8
COMMENT='germline CNV';



-- ----------------------------------------------------------------------------------------------------------
--                                                 INITIAL DATA
-- ----------------------------------------------------------------------------------------------------------


-- -----------------------------------------------------
-- Table `user`
-- -----------------------------------------------------
INSERT INTO user VALUES (NULL, 'admin', 'dd94709528bb1c83d08f3088d4043f4742891f4f', 'admin', 'Admin','no_valid@email.de', CURDATE(), NULL, 1);
INSERT INTO user VALUES (NULL, 'genlab_import', '', 'special', 'GenLab import','no_valid@email2.de', CURDATE(), NULL, 1);
INSERT INTO user VALUES (NULL, 'unknown', '', 'special', 'Unknown user','no_valid@email3.de', CURDATE(), NULL, 1);


-- -----------------------------------------------------
-- Table `species`
-- -----------------------------------------------------
INSERT INTO species VALUES (NULL, 'human');


-- -----------------------------------------------------
-- Table `genome`
-- -----------------------------------------------------
INSERT INTO genome VALUES (NULL, 'hg19', 'Human genome hg19');
INSERT INTO genome VALUES (NULL, 'hg38', 'Human genome hg38');

SET SQL_MODE=@OLD_SQL_MODE;
SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS;
SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS;
