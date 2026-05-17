-- CloudDisk Migration V3: Full-text search index
CREATE TABLE IF NOT EXISTS tbl_file_text (
    file_id    BIGINT PRIMARY KEY,
    text       MEDIUMTEXT NOT NULL,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (file_id) REFERENCES tbl_file_v2(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS tbl_inverted_index (
    id        BIGINT AUTO_INCREMENT PRIMARY KEY,
    term      VARCHAR(128) NOT NULL,
    file_id   BIGINT NOT NULL,
    freq      INT NOT NULL DEFAULT 1,
    positions TEXT,
    INDEX idx_term (term),
    INDEX idx_file (file_id),
    FOREIGN KEY (file_id) REFERENCES tbl_file_v2(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
