-- ============================================================
-- CloudDisk Migration V1: Hierarchical files, recycle bin, sharing
-- ============================================================

-- 1. Extend tbl_user with storage quota
ALTER TABLE tbl_user
  ADD COLUMN total_storage BIGINT DEFAULT 1073741824 AFTER salt,  -- 1 GB
  ADD COLUMN used_storage BIGINT DEFAULT 0 AFTER total_storage;

-- 2. New tbl_file with tree structure (parent_id)
CREATE TABLE IF NOT EXISTS tbl_file_v2 (
    id            BIGINT AUTO_INCREMENT PRIMARY KEY,
    uid           INT NOT NULL,
    parent_id     BIGINT NOT NULL DEFAULT 0,
    filename      VARCHAR(255) NOT NULL,
    is_folder     TINYINT(1) NOT NULL DEFAULT 0,
    hashcode      VARCHAR(64) NOT NULL DEFAULT '',
    size          BIGINT NOT NULL DEFAULT 0,
    mime_type     VARCHAR(128) NOT NULL DEFAULT '',
    storage_path  VARCHAR(512) NOT NULL DEFAULT '',
    created_at    DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at    DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    tomb          TINYINT(1) NOT NULL DEFAULT 0,
    deleted_at    DATETIME DEFAULT NULL,

    INDEX idx_uid_parent (uid, parent_id),
    INDEX idx_uid_tomb   (uid, tomb),
    INDEX idx_hashcode   (hashcode),
    UNIQUE KEY uk_uid_parent_filename (uid, parent_id, filename, tomb),

    FOREIGN KEY (uid) REFERENCES tbl_user(id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 3. Recycle bin
CREATE TABLE IF NOT EXISTS tbl_trash (
    id                  BIGINT AUTO_INCREMENT PRIMARY KEY,
    file_id             BIGINT NOT NULL,
    uid                 INT NOT NULL,
    original_parent_id  BIGINT NOT NULL DEFAULT 0,
    original_filename   VARCHAR(255) NOT NULL,
    deleted_at          DATETIME DEFAULT CURRENT_TIMESTAMP,
    expire_at           DATETIME,

    INDEX idx_uid (uid),
    INDEX idx_expire (expire_at),

    FOREIGN KEY (uid) REFERENCES tbl_user(id),
    FOREIGN KEY (file_id) REFERENCES tbl_file_v2(id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 4. Share links
CREATE TABLE IF NOT EXISTS tbl_share (
    id            BIGINT AUTO_INCREMENT PRIMARY KEY,
    uid           INT NOT NULL,
    file_id       BIGINT NOT NULL,
    share_token   VARCHAR(64) NOT NULL UNIQUE,
    share_type    TINYINT(1) NOT NULL DEFAULT 0,
    password_hash VARCHAR(128) NOT NULL DEFAULT '',
    expire_at     DATETIME DEFAULT NULL,
    download_count INT NOT NULL DEFAULT 0,
    created_at    DATETIME DEFAULT CURRENT_TIMESTAMP,
    is_active     TINYINT(1) NOT NULL DEFAULT 1,

    INDEX idx_token (share_token),
    INDEX idx_uid (uid),

    FOREIGN KEY (uid) REFERENCES tbl_user(id),
    FOREIGN KEY (file_id) REFERENCES tbl_file_v2(id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
