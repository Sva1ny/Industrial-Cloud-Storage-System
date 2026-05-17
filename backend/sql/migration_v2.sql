-- CloudDisk Migration V2: File version history
CREATE TABLE IF NOT EXISTS tbl_file_history (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    file_id BIGINT NOT NULL,
    version INT NOT NULL DEFAULT 1,
    hashcode VARCHAR(64) NOT NULL DEFAULT '',
    size BIGINT NOT NULL DEFAULT 0,
    storage_path VARCHAR(512) NOT NULL DEFAULT '',
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_file_id (file_id),
    FOREIGN KEY (file_id) REFERENCES tbl_file_v2(id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
