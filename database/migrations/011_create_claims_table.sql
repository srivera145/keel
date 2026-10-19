-- Warranty claims for the Klaus demo page, app/Pages/claims.klaus. Each claim belongs to one organization, and
-- organization_id is what makes db.scoped("claims") limit every query to the current one.
CREATE TABLE IF NOT EXISTS claims (
    id INT AUTO_INCREMENT PRIMARY KEY,
    organization_id INT NOT NULL,
    ro_number VARCHAR(50) NOT NULL,
    customer VARCHAR(255) NOT NULL,
    vehicle VARCHAR(255) NOT NULL,
    amount DECIMAL(10, 2) NOT NULL,
    status VARCHAR(20) NOT NULL DEFAULT 'pending',
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (organization_id) REFERENCES organizations(id) ON DELETE CASCADE,
    INDEX idx_org_status (organization_id, status)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
