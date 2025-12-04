## Database schema
### Tables
#### Users table
```sql
CREATE TABLE users (
    user_id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    login VARCHAR(50) UNIQUE NOT NULL,
    password_hash VARCHAR(60) NOT NULL, -- bcrypt hash
    created_at TIMESTAMPTZ DEFAULT NOW(),
    
    CONSTRAINT login_length CHECK (LENGTH(login) >= 3 AND LENGTH(login) <= 50),
    CONSTRAINT login_format CHECK (login ~ '^[a-zA-Z0-9_]+$')
);

CREATE INDEX idx_users_login ON users(login);
CREATE INDEX idx_users_created_at ON users(created_at);
```

#### Messages table
```sql
CREATE TABLE messages (
    message_id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    from_user_id UUID NOT NULL REFERENCES users(user_id) ON DELETE CASCADE,
    to_user_id UUID NOT NULL REFERENCES users(user_id) ON DELETE CASCADE,
    message_text TEXT NOT NULL,
    is_read BOOLEAN DEFAULT FALSE,
    created_at TIMESTAMPTZ DEFAULT NOW(),
    
    CONSTRAINT message_length CHECK (LENGTH(message_text) > 0 AND LENGTH(message_text) <= 4096)
);

CREATE INDEX idx_messages_to_user_id ON messages(to_user_id, created_at);
CREATE INDEX idx_messages_from_user_id ON messages(from_user_id, created_at);
CREATE INDEX idx_messages_is_read ON messages(is_read) WHERE NOT is_read;
```

#### Refresh tokens table
```sql
CREATE TABLE refresh_tokens (
    token_id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id UUID NOT NULL REFERENCES users(user_id) ON DELETE CASCADE,
    token_hash VARCHAR(64) NOT NULL, -- SHA-256 hash
    expires_at TIMESTAMPTZ NOT NULL,
    created_at TIMESTAMPTZ DEFAULT NOW()
);

CREATE INDEX idx_refresh_tokens_token_hash ON refresh_tokens(token_hash);
CREATE INDEX idx_refresh_tokens_user_id ON refresh_tokens(user_id);
CREATE INDEX idx_refresh_tokens_expires_at ON refresh_tokens(expires_at);
```

---

### Functions
#### A function for automatically removing expired refresh tokens
```sql
CREATE OR REPLACE FUNCTION cleanup_expired_tokens()
RETURNS void AS $$
BEGIN
    DELETE FROM refresh_tokens WHERE expires_at < NOW();
END;
$$ LANGUAGE plpgsql;
```

#### A function for deleting expired refresh tokens, which we will call on a schedule
```sql
CREATE OR REPLACE FUNCTION scheduled_cleanup()
RETURNS void AS $$
BEGIN
    PERFORM cleanup_expired_tokens();
END;
$$ LANGUAGE plpgsql;
```

#### Function to check that the user exists and is not sending a message to himself
```sql
CREATE OR REPLACE FUNCTION check_user_exists()
RETURNS TRIGGER AS $$
BEGIN
    IF NEW.from_user_id = NEW.to_user_id THEN
        RAISE EXCEPTION 'Cannot send message to yourself';
    END IF;
    
    IF NOT EXISTS (SELECT 1 FROM users WHERE user_id = NEW.from_user_id) THEN
        RAISE EXCEPTION 'Sender user does not exist';
    END IF;
    
    IF NOT EXISTS (SELECT 1 FROM users WHERE user_id = NEW.to_user_id) THEN
        RAISE EXCEPTION 'Recipient user does not exist';
    END IF;
    
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;
```

---

### Triggers
#### Trigger to check user when sending a message
```sql
CREATE TRIGGER trigger_check_users_before_message
    BEFORE INSERT ON messages
    FOR EACH ROW
    EXECUTE FUNCTION check_user_exists();
```
