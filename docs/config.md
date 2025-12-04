# Server config file
`config.json`
```json
{
    "server": {
        "address": "0.0.0.0",
        "port": 8443,
        "threads": 4
    },
    "ssl": {
        "certificate_file": "sslCerts/server.crt",
        "private_key_file": "sslCerts/server.key",
        "dh_params_file": "sslCerts/dhparams.pem"
    },
    "database": {
		"address": "192.168.50.37",
		"port": 5432,
		"username": "chat_user",
		"password": "chat_user",
		"db_name": "chat_db",
        "max_connections": 10,
		"connection_timeout": 10
    },
    "jwt": {
        "secret_key": "MJ1IdWHzDpT7VfGZQFRScabPuxEs1EEP",
        "access_token_expiry_minutes": 15,
        "refresh_token_expiry_days": 7
    },
    "logging": {
        "level": "info",
        "access_log": "access.log",
        "error_log": "error.log",
        "console_output": true,
        "log_access": true
    }
}
```

## Configuration parameters
### Server section
* **`server.address`** (string) - IP address to bind the server to. `0.0.0` means listening on all network interfaces
* **`server.port`** (integer) - Port for HTTPS connections (8443 is the standard alternative HTTPS port)
* **`server.threads`** (integer) - Number of worker threads for processing requests

### SSL section
* **`ssl.certificate_file`** (string) - Path to the SSL certificate (usually in PEM format)
* **`ssl.private_key_file`** (string) - Path to the private key for SSL
* **`ssl.dh_params_file`** (string) - Diffie-Hellman parameters file for perfect forward secrecy (PFS)

### Database section
* **`database.address`** (string) - IP address or domain name of the PostgreSQL server
* **`database.port`** (integer) - PostgreSQL connection port (5432 is the standard port)
* **`database.username`** (string) - Username for connecting to the database
* **`database.password`** (string) - Database user password (secure storage should be used in production)
* **`database.db_name`** (string) - Database name
* **`database.max_connections`** (integer) - Maximum number of connections in the pool
* **`database.connection_timeout`** (integer) - Connection timeout in seconds

### JWT section
* **`jwt.secret_key`** (string) - Secret key for signing JWT tokens (must be stored securely)
* **`jwt.access_token_expiry_minutes`** (integer) - Access token lifetime in minutes
* **`jwt.refresh_token_expiry_days`** (integer) - Refresh token lifetime in days

### Logging section
* **`logging.level`** (string) - Logging level (`debug`, `info`, `warning`, `error`, `critical`)
* **`logging.access_log`** (string) - File name for access logs
* **`logging.error_log`** (string) - File name for error logs
* **`logging.console_output`** (boolean) - Output logs to console (true/false)
* **`logging.log_access`** (boolean) - Enable logging of access requests (true/false)
