SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0;
SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0;
SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='TRADITIONAL';

-- ----------------------------------------------------------------------------------------------------------
--                                                   TABLES
-- ----------------------------------------------------------------------------------------------------------

-- -----------------------------------------------------
-- Table `gene`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `gene` (
`id` int(11) unsigned NOT NULL AUTO_INCREMENT,
`hgnc_id` int(11) unsigned NOT NULL,
`symbol` varchar(40) NOT NULL,
`name` TEXT NOT NULL,
`chromosome` enum('1','2','3','4','5','6','7','8','9','10','11','12','13','14','15','16','17','18','19','20','21','22','X','Y','M','none') NOT NULL,
`type` enum('protein-coding gene','pseudogene','non-coding RNA','other') NOT NULL,

PRIMARY KEY (`id`), 
UNIQUE KEY `hgnc_id` (`hgnc_id`),
UNIQUE KEY `symbol` (`symbol`),
KEY `chromosome` (`chromosome`), 
KEY `type` (`type`)
)
ENGINE=InnoDB DEFAULT
CHARSET=utf8
COMMENT='Genes from HGNC';

-- -----------------------------------------------------
-- Table `gene_alias`
-- -----------------------------------------------------

CREATE TABLE IF NOT EXISTS `gene_alias` (
`gene_id` int(11) unsigned NOT NULL,
`symbol` varchar(40) NOT NULL,
`type` enum('synonym','previous') NOT NULL,

KEY `fk_gene_id1` (`gene_id` ASC) ,
CONSTRAINT `fk_gene_id1`
  FOREIGN KEY (`gene_id` )
  REFERENCES `gene` (`id` )
  ON DELETE NO ACTION
  ON UPDATE NO ACTION
) 
ENGINE=InnoDB DEFAULT 
CHARSET=utf8
COMMENT='Alternative symbols of genes';


-- -----------------------------------------------------
-- Table `gene_transcript`
-- -----------------------------------------------------

CREATE TABLE IF NOT EXISTS `gene_transcript` (
`id` int(11) unsigned NOT NULL AUTO_INCREMENT,
`gene_id` int(11) unsigned NOT NULL,
`name` varchar(40) NOT NULL,
`source` enum('ccds', 'ucsc','refseq') NOT NULL,
`start_coding` int(11) unsigned NOT NULL,
`end_coding` int(11) unsigned NOT NULL,
`strand` enum('+', '-') NOT NULL,

PRIMARY KEY (`id`), 
KEY `fk_gene_id2` (`gene_id` ASC) ,
CONSTRAINT `fk_gene_id3`
  FOREIGN KEY (`gene_id` )
  REFERENCES `gene` (`id` )
  ON DELETE NO ACTION
  ON UPDATE NO ACTION,
UNIQUE KEY `name` (`name`),
KEY `source` (`source`),
KEY `start_coding` (`start_coding`), 
KEY `end_coding` (`end_coding`)
) 
ENGINE=InnoDB DEFAULT 
CHARSET=utf8
COMMENT='Gene transcipts';

-- -----------------------------------------------------
-- Table `gene_exon`
-- -----------------------------------------------------

CREATE TABLE IF NOT EXISTS `gene_exon` (
`transcript_id` int(11) unsigned NOT NULL,
`start` int(11) unsigned NOT NULL,
`end` int(11) unsigned NOT NULL,

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
-- Table `mid`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `mid` (
  `id` INT(11) NOT NULL AUTO_INCREMENT ,
  `name` VARCHAR(255) NOT NULL ,
  `sequence` VARCHAR(45) NOT NULL ,
  PRIMARY KEY (`id`) ,
  UNIQUE INDEX `UNIQUE_MAN_INDEX` (`name` ASC) )
ENGINE = InnoDB
AUTO_INCREMENT = 1
DEFAULT CHARACTER SET = latin1;


-- -----------------------------------------------------
-- Table `genome`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `genome` (
  `id` INT(11) NOT NULL AUTO_INCREMENT ,
  `build` VARCHAR(45) NOT NULL ,
  PRIMARY KEY (`id`) ,
  UNIQUE INDEX `build_UNIQUE` (`build` ASC) )
ENGINE = InnoDB
AUTO_INCREMENT = 1
DEFAULT CHARACTER SET = latin1;


-- -----------------------------------------------------
-- Table `processing_system`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `processing_system` (
  `id` INT(11) NOT NULL AUTO_INCREMENT ,
  `name` VARCHAR(45) NOT NULL ,
  `adapter1_p5` VARCHAR(45) NULL DEFAULT NULL ,
  `adapter2_p7` VARCHAR(45) NULL DEFAULT NULL ,
  `shotgun` TINYINT(1) NOT NULL ,
  `target_file` VARCHAR(255) NULL DEFAULT NULL ,
  `name_short` VARCHAR(50) NULL DEFAULT NULL ,
  `genome_id` INT(11) NOT NULL ,
  PRIMARY KEY (`id`) ,
  UNIQUE INDEX `name_UNIQUE` (`name` ASC) ,
  UNIQUE INDEX `name_short` (`name_short` ASC) ,
  INDEX `fk_processing_system_genome1` (`genome_id` ASC) ,
  CONSTRAINT `fk_processing_system_genome1`
    FOREIGN KEY (`genome_id` )
    REFERENCES `genome` (`id` )
    ON UPDATE NO ACTION)
ENGINE = InnoDB
AUTO_INCREMENT = 1
DEFAULT CHARACTER SET = latin1
COMMENT = '		';


-- -----------------------------------------------------
-- Table `device`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `device` (
  `id` INT(11) NOT NULL AUTO_INCREMENT ,
  `type` ENUM('GAIIx','MiSeq') NOT NULL ,
  `name` VARCHAR(45) NOT NULL ,
  PRIMARY KEY (`id`) ,
  UNIQUE INDEX `name_UNIQUE` (`name` ASC) )
ENGINE = InnoDB
AUTO_INCREMENT = 1
DEFAULT CHARACTER SET = latin1;


-- -----------------------------------------------------
-- Table `sequencing_run`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `sequencing_run` (
  `id` INT(11) NOT NULL AUTO_INCREMENT ,
  `name` VARCHAR(45) NOT NULL ,
  `fcid` VARCHAR(45) NOT NULL ,
  `start_date` DATE NULL DEFAULT NULL ,
  `end_date` DATE NULL DEFAULT NULL ,
  `device_id` INT(11) NOT NULL ,
  `recipe` VARCHAR(45) NOT NULL ,
  `comment` TEXT NULL DEFAULT NULL ,
  `quality` ENUM('n/a','good','medium','bad') NOT NULL DEFAULT 'n/a' ,
  PRIMARY KEY (`id`) ,
  UNIQUE INDEX `name_UNIQUE` (`name` ASC) ,
  UNIQUE INDEX `fcid_UNIQUE` (`fcid` ASC) ,
  INDEX `fk_run_device1` (`device_id` ASC) ,
  CONSTRAINT `fk_run_device1`
    FOREIGN KEY (`device_id` )
    REFERENCES `device` (`id` )
    ON DELETE NO ACTION
    ON UPDATE NO ACTION)
ENGINE = InnoDB
AUTO_INCREMENT = 1
DEFAULT CHARACTER SET = latin1;


-- -----------------------------------------------------
-- Table `species`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `species` (
  `id` INT(11) NOT NULL AUTO_INCREMENT ,
  `name` VARCHAR(45) NOT NULL ,
  PRIMARY KEY (`id`) ,
  UNIQUE INDEX `name_UNIQUE` (`name` ASC) )
ENGINE = InnoDB
AUTO_INCREMENT = 1
DEFAULT CHARACTER SET = latin1;


-- -----------------------------------------------------
-- Table `sender`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `sender` (
  `id` INT(11) NOT NULL AUTO_INCREMENT ,
  `name` VARCHAR(45) NOT NULL ,
  `phone` VARCHAR(45) NULL ,
  `email` VARCHAR(45) NULL ,
  `affiliation` VARCHAR(100) NULL ,
  PRIMARY KEY (`id`) )
ENGINE = InnoDB;


-- -----------------------------------------------------
-- Table `user`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `user` (
  `id` INT(11) NOT NULL AUTO_INCREMENT ,
  `user_id` VARCHAR(45) NOT NULL ,
  `password` VARCHAR(64) NOT NULL ,
  `user_role` ENUM('user','admin') NOT NULL ,
  `name` VARCHAR(45) NOT NULL ,
  `email` VARCHAR(45) NOT NULL ,
  `created` DATE NOT NULL ,
  `last_login` DATE NULL DEFAULT NULL ,
  `active` TINYINT(1) NOT NULL ,
  PRIMARY KEY (`id`) ,
  UNIQUE INDEX `name_UNIQUE` (`user_id` ASC) ,
  UNIQUE INDEX `email_UNIQUE` (`email` ASC) )
ENGINE = InnoDB
AUTO_INCREMENT = 1
DEFAULT CHARACTER SET = latin1;


-- -----------------------------------------------------
-- Table `sample`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `sample` (
  `id` INT(11) NOT NULL AUTO_INCREMENT ,
  `name` VARCHAR(15) NOT NULL ,
  `name_external` VARCHAR(255) NULL DEFAULT NULL ,
  `received` DATE NULL DEFAULT NULL ,
  `received_by` INT(11) NOT NULL ,
  `sample_type` ENUM('Amplicon','DNA','RNA','native') NOT NULL ,
  `species_id` INT(11) NOT NULL ,
  `concentration` FLOAT NULL DEFAULT NULL ,
  `volume` FLOAT NULL DEFAULT NULL ,
  `od_260_280` FLOAT NULL DEFAULT NULL ,
  `gender` ENUM('male','female','n/a') NOT NULL ,
  `comment` TEXT NULL DEFAULT NULL ,
  `quality` ENUM('n/a','good','medium','bad') NOT NULL DEFAULT 'n/a' ,
  `od_260_230` FLOAT NULL DEFAULT NULL ,
  `sender_id` INT(11) NOT NULL ,
  PRIMARY KEY (`id`) ,
  UNIQUE INDEX `id_UNIQUE` (`name` ASC) ,
  INDEX `fk_samples_species1` (`species_id` ASC) ,
  INDEX `fk_samples_sender` (`sender_id` ASC) ,
  INDEX `fk_samples_receivedby` (`received_by` ASC) ,
  CONSTRAINT `fk_samples_species1`
    FOREIGN KEY (`species_id` )
    REFERENCES `species` (`id` )
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `fk_samples_sender`
    FOREIGN KEY (`sender_id` )
    REFERENCES `sender` (`id` )
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `fk_samples_receivedby`
    FOREIGN KEY (`received_by` )
    REFERENCES `user` (`id` )
    ON DELETE NO ACTION
    ON UPDATE NO ACTION)
ENGINE = InnoDB
AUTO_INCREMENT = 1
DEFAULT CHARACTER SET = latin1;


-- -----------------------------------------------------
-- Table `project`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `project` (
  `id` INT(11) NOT NULL AUTO_INCREMENT ,
  `name` VARCHAR(45) NOT NULL ,
  `type` ENUM('diagnostic','research','extern','test') NOT NULL ,
  `internal_coordinator_id` INT(11) NOT NULL ,
  `comment` TEXT NULL DEFAULT NULL ,
  PRIMARY KEY (`id`) ,
  UNIQUE INDEX `name_UNIQUE` (`name` ASC) ,
  INDEX `fk_internal_coordinator` (`internal_coordinator_id` ASC) ,
  CONSTRAINT `fk_internal_coordinator`
    FOREIGN KEY (`internal_coordinator_id` )
    REFERENCES `user` (`id` )
    ON DELETE NO ACTION
    ON UPDATE NO ACTION)
ENGINE = InnoDB
AUTO_INCREMENT = 1
DEFAULT CHARACTER SET = latin1;


-- -----------------------------------------------------
-- Table `processed_sample`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `processed_sample` (
  `id` INT(11) NOT NULL AUTO_INCREMENT ,
  `sample_id` INT(11) NOT NULL ,
  `process_id` INT(2) NOT NULL ,
  `sequencing_run_id` INT(11) NOT NULL ,
  `lane` INT(2) NOT NULL ,
  `mid1_i7` INT(11) NULL DEFAULT NULL ,
  `mid2_i5` INT(11) NULL DEFAULT NULL ,
  `analysis` ENUM('fastq','annot.','diff. annot.') NOT NULL ,
  `operator_id` INT(11) NOT NULL ,
  `processing_system_id` INT(11) NOT NULL ,
  `comment` TEXT NULL DEFAULT NULL ,
  `project_id` INT(11) NOT NULL ,
  `molarity` FLOAT NULL ,
  `status` ENUM('received','processed','sequenced','analysed','finished') NOT NULL ,
  `normal_id` int(11) DEFAULT NULL,
  PRIMARY KEY (`id`) ,
  UNIQUE INDEX `c_sample_processid` (`sample_id` ASC, `process_id` ASC) ,
  INDEX `fk_processed_sample_samples1` (`sample_id` ASC) ,
  INDEX `fk_processed_sample_mid1` (`mid1_i7` ASC) ,
  INDEX `fk_processed_sample_processing_system1` (`processing_system_id` ASC) ,
  INDEX `fk_processed_sample_run1` (`sequencing_run_id` ASC) ,
  INDEX `fk_processed_sample_mid2` (`mid2_i5` ASC) ,
  INDEX `fk_processed_sample_project` (`project_id` ASC) ,
  INDEX `fk_processed_sample_operator` (`operator_id` ASC) ,
  INDEX `fk_processed_sample_normal_id` (`normal_id` ASC) ,
  CONSTRAINT `fk_processed_sample_mid1`
    FOREIGN KEY (`mid1_i7` )
    REFERENCES `mid` (`id` )
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `fk_processed_sample_mid2`
    FOREIGN KEY (`mid2_i5` )
    REFERENCES `mid` (`id` )
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `fk_processed_sample_processing_system1`
    FOREIGN KEY (`processing_system_id` )
    REFERENCES `processing_system` (`id` )
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `fk_processed_sample_run1`
    FOREIGN KEY (`sequencing_run_id` )
    REFERENCES `sequencing_run` (`id` )
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `fk_processed_sample_samples1`
    FOREIGN KEY (`sample_id` )
    REFERENCES `sample` (`id` )
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `fk_processed_sample_project`
    FOREIGN KEY (`project_id` )
    REFERENCES `project` (`id` )
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `fk_processed_sample_operator`
    FOREIGN KEY (`operator_id` )
    REFERENCES `user` (`id` )
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `fk_processed_sample_normal_id`
    FOREIGN KEY (`normal_id` )
    REFERENCES `processed_sample` (`id` )
    ON DELETE NO ACTION
    ON UPDATE NO ACTION)
ENGINE = InnoDB
AUTO_INCREMENT = 1
DEFAULT CHARACTER SET = latin1;


-- -----------------------------------------------------
-- Table `variant`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `variant` (
  `id` INT(11) NOT NULL AUTO_INCREMENT ,
  `chr` ENUM('chr1','chr2','chr3','chr4','chr5','chr6','chr7','chr8','chr9','chr10','chr11','chr12','chr13','chr14','chr15','chr16','chr17','chr18','chr19','chr20','chr21','chr22','chrY','chrX','chrM','other') NOT NULL ,
  `start` INT(11) NOT NULL ,
  `end` INT(11) NOT NULL ,
  `ref` TEXT NOT NULL ,
  `obs` TEXT NOT NULL ,
  `dbSNP` TEXT NULL DEFAULT NULL ,
  `1000g` FLOAT NULL DEFAULT NULL ,
  `exac` FLOAT NULL DEFAULT NULL ,
  `esp6500` FLOAT NULL DEFAULT NULL ,
  `gene` TEXT NULL DEFAULT NULL ,
  `variant_type` TEXT NULL DEFAULT NULL ,
  `coding` TEXT NULL DEFAULT NULL ,
  `genome_id` INT(11) NOT NULL ,
  `vus` ENUM('n/a','1','2','3','4','5','M') NULL DEFAULT 'n/a' ,
  PRIMARY KEY (`id`) ,
  UNIQUE INDEX `variant_UNIQUE` (`chr` ASC, `start` ASC, `end` ASC, ref(255) ASC, obs(255) ASC) ,
  INDEX `fk_variant_genome1` (`genome_id` ASC) ,
  INDEX `index_variant` (`chr` ASC, `start` ASC, `end` ASC, ref(255) ASC, obs(255) ASC) ,
  INDEX `gene` (gene(50) ASC) ,
  CONSTRAINT `fk_variant_genome1`
    FOREIGN KEY (`genome_id` )
    REFERENCES `genome` (`id` )
    ON UPDATE NO ACTION)
ENGINE = InnoDB
AUTO_INCREMENT = 1
DEFAULT CHARACTER SET = latin1;


-- -----------------------------------------------------
-- Table `detected_variant`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `detected_variant` (
  `id` INT(11) NOT NULL AUTO_INCREMENT ,
  `processed_sample_id` INT(11) NOT NULL ,
  `variant_id` INT(11) NOT NULL ,
  `genotype` ENUM('hom','het') NOT NULL ,
  `quality` TEXT NULL ,
  `validated` ENUM('n/a','to validate','cleared','not cleared','for reporting','true positive','false positive','wrong genotype') NOT NULL DEFAULT 'n/a' ,
  `validation_comment` TEXT NULL ,
  `comment` TEXT NULL ,
  `report` TINYINT(1) NOT NULL DEFAULT 0,
  PRIMARY KEY (`id`) ,
  UNIQUE INDEX `detected_variant_UNIQUE` (`processed_sample_id` ASC, `variant_id` ASC) ,
  INDEX `fk_detected_variant_variant1` (`variant_id` ASC) ,
  INDEX `fk_detected_variant_processed_sample1` (`processed_sample_id` ASC) ,
  CONSTRAINT `fk_processed_sample_has_variant_processed_sample1`
    FOREIGN KEY (`processed_sample_id` )
    REFERENCES `processed_sample` (`id` )
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `fk_processed_sample_has_variant_variant1`
    FOREIGN KEY (`variant_id` )
    REFERENCES `variant` (`id` )
    ON DELETE NO ACTION
    ON UPDATE NO ACTION)
ENGINE = InnoDB
AUTO_INCREMENT = 1
DEFAULT CHARACTER SET = latin1;


-- -----------------------------------------------------
-- Table `ngso`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `ngso` (
  `id` INT(11) NOT NULL AUTO_INCREMENT ,
  `ngso_id` VARCHAR(10) NOT NULL ,
  `name` VARCHAR(45) NOT NULL ,
  `description` TEXT NOT NULL ,
  PRIMARY KEY (`id`) ,
  UNIQUE INDEX `name_UNIQUE` (`name` ASC) )
ENGINE = InnoDB
AUTO_INCREMENT = 1
DEFAULT CHARACTER SET = latin1;


-- -----------------------------------------------------
-- Table `nm_processed_sample_ngso`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `nm_processed_sample_ngso` (
  `id` INT(11) NOT NULL AUTO_INCREMENT ,
  `processed_sample_id` INT(11) NOT NULL ,
  `ngso_id` INT(11) NOT NULL ,
  `value` VARCHAR(20) NOT NULL ,
  PRIMARY KEY (`id`) ,
  UNIQUE INDEX `c_processing_id_ngso_id` (`processed_sample_id` ASC, `ngso_id` ASC) ,
  INDEX `fk_qcvalues_processing1` (`processed_sample_id` ASC) ,
  INDEX `fk_processed_sample_annotaiton_NGSO1` (`ngso_id` ASC) ,
  CONSTRAINT `fk_processed_sample_annotaiton_NGSO1`
    FOREIGN KEY (`ngso_id` )
    REFERENCES `ngso` (`id` )
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `fk_qcvalues_processing1`
    FOREIGN KEY (`processed_sample_id` )
    REFERENCES `processed_sample` (`id` )
    ON DELETE NO ACTION
    ON UPDATE NO ACTION)
ENGINE = InnoDB
AUTO_INCREMENT = 1
DEFAULT CHARACTER SET = latin1;


-- -----------------------------------------------------
-- Table `sample_group`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `sample_group` (
  `id` INT(11) NOT NULL AUTO_INCREMENT ,
  `name` VARCHAR(45) NOT NULL ,
  `comment` TEXT NULL DEFAULT NULL ,
  PRIMARY KEY (`id`) ,
  UNIQUE INDEX `name_UNIQUE` (`name` ASC) )
ENGINE = InnoDB
AUTO_INCREMENT = 1
DEFAULT CHARACTER SET = latin1;


-- -----------------------------------------------------
-- Table `nm_sample_sample_group`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `nm_sample_sample_group` (
  `id` INT(11) NOT NULL AUTO_INCREMENT ,
  `sample_group_id` INT(11) NOT NULL ,
  `sample_id` INT(11) NOT NULL ,
  PRIMARY KEY (`id`) ,
  INDEX `fk_sample_group_has_sample_sample1` (`sample_id` ASC) ,
  INDEX `fk_sample_group_has_sample_sample_group1` (`sample_group_id` ASC) ,
  CONSTRAINT `fk_sample_group_has_sample_sample1`
    FOREIGN KEY (`sample_id` )
    REFERENCES `sample` (`id` )
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `fk_sample_group_has_sample_sample_group1`
    FOREIGN KEY (`sample_group_id` )
    REFERENCES `sample_group` (`id` )
    ON DELETE NO ACTION
    ON UPDATE NO ACTION)
ENGINE = InnoDB
AUTO_INCREMENT = 1
DEFAULT CHARACTER SET = latin1;


-- -----------------------------------------------------
-- Table `detected_somatic_variant`
-- -----------------------------------------------------
CREATE  TABLE IF NOT EXISTS `detected_somatic_variant` (
  `id` INT(11) NOT NULL AUTO_INCREMENT ,
  `processed_sample_id_tumor` INT(11) NOT NULL ,
  `processed_sample_id_normal` INT(11) NOT NULL ,
  `variant_id` INT(11) NOT NULL ,
  `variant_frequency` FLOAT NOT NULL ,
  `depth` INT(11) NOT NULL ,
  `quality_snp` FLOAT NULL DEFAULT NULL ,
  `validated` ENUM('true positive','false positive','n/a') NOT NULL DEFAULT 'n/a' ,
  `comment` TEXT NULL ,
  PRIMARY KEY (`id`) ,
  INDEX `variant_id_INDEX` (`variant_id` ASC) ,
  INDEX `processed_sample_id_tumor_INDEX` (`processed_sample_id_tumor` ASC) ,
  INDEX `processed_sample_id_normal_INDEX` (`processed_sample_id_normal` ASC) ,
  UNIQUE INDEX `detected_somatic_variant_UNIQUE` (`processed_sample_id_tumor` ASC, `processed_sample_id_normal` ASC, `variant_id` ASC) ,
  INDEX `fk_dsv_has_ps1` (`processed_sample_id_tumor` ASC) ,
  INDEX `fk_dsv_has_ps2` (`processed_sample_id_normal` ASC) ,
  CONSTRAINT `fk_dsv_has_v`
    FOREIGN KEY (`variant_id` )
    REFERENCES `variant` (`id` )
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `fk_dsv_has_ps1`
    FOREIGN KEY (`processed_sample_id_tumor` )
    REFERENCES `processed_sample` (`id` )
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `fk_dsv_has_ps2`
    FOREIGN KEY (`processed_sample_id_normal` )
    REFERENCES `processed_sample` (`id` )
    ON DELETE NO ACTION
    ON UPDATE NO ACTION)
ENGINE = InnoDB
AUTO_INCREMENT = 1
DEFAULT CHARACTER SET = latin1;

-- -----------------------------------------------------
-- Table `diag_status`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `diag_status` (
  `processed_sample_id` INT NOT NULL,
  `status` ENUM('pending','in progress','sanger validation','done','repeat library and sequencing','repeat sequencing only') NULL,
  `user_id` INT NULL,
  `date` TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP,
  `outcome` ENUM('n/a','no significant findings','uncertain','significant findings') NOT NULL DEFAULT 'n/a',
  PRIMARY KEY (`processed_sample_id`),
  INDEX `user_id_idx` (`user_id` ASC),
  CONSTRAINT `user_id`
    FOREIGN KEY (`user_id`)
    REFERENCES `user` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION,
  CONSTRAINT `processed_sample`
    FOREIGN KEY (`processed_sample_id`)
    REFERENCES `processed_sample` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION)
ENGINE = InnoDB
DEFAULT CHARACTER SET = latin1;

-- -----------------------------------------------------
-- Table `kasp_status`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `kasp_status` (
  `processed_sample_id` INT NOT NULL,
  `random_error_prob` FLOAT UNSIGNED NOT NULL,
  `snps_evaluated` INT UNSIGNED NOT NULL,
  `snps_match` INT UNSIGNED NOT NULL,
  PRIMARY KEY (`processed_sample_id`),
  CONSTRAINT `processed_sample0`
    FOREIGN KEY (`processed_sample_id`)
    REFERENCES `processed_sample` (`id`)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION)
ENGINE = InnoDB
DEFAULT CHARACTER SET = latin1;


-- ----------------------------------------------------------------------------------------------------------
--                                                 INITIAL DATA
-- ----------------------------------------------------------------------------------------------------------


-- -----------------------------------------------------
-- Table `user`
-- -----------------------------------------------------
INSERT INTO user VALUES (NULL, 'admin', 'dd94709528bb1c83d08f3088d4043f4742891f4f', 'admin', 'Admin','no_valid@email.de', CURDATE(), NULL, TRUE);


-- -----------------------------------------------------
-- Table `species`
-- -----------------------------------------------------
INSERT INTO species VALUES (NULL, 'human');


-- -----------------------------------------------------
-- Table `genome`
-- -----------------------------------------------------
INSERT INTO genome VALUES (NULL, 'hg19');
INSERT INTO genome VALUES (NULL, 'hg38');

SET SQL_MODE=@OLD_SQL_MODE;
SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS;
SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS;
