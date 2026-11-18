<?php
/**
 * Central MySQL connection configuration.
 *
 * Loads optional local overrides from db_config.local.php (gitignored)
 * or environment variables (DB_HOST, DB_USER, DB_PASS, DB_NAME).
 */
$localConfig = __DIR__ . '/db_config.local.php';
if (file_exists($localConfig)) {
    require $localConfig;
}

$servername = $servername ?? (getenv('DB_HOST') ?: 'localhost');
$db_username = $db_username ?? (getenv('DB_USER') ?: 'root');
$db_password = $db_password ?? (getenv('DB_PASS') ?: '');
$dbname = $dbname ?? (getenv('DB_NAME') ?: 'deapc');
