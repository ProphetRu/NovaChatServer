## API
### General errors
**Endpoint not found / Invalid HTTP request type**
```json
{
    "code": "ENDPOINT_NOT_FOUND",
    "message": "Endpoint not found",
    "status": "error"
}
```

**Invalid json format / empty json**
```json
{
    "code": "INVALID_JSON",
    "message": "Invalid JSON body",
    "status": "error"
}
```

---

### 1. User registration
```http
POST /api/v1/auth/register
Content-Type: application/json

{
  "login": "username",
  "password": "SecurePass123!"
}
```

**Responses:**
**Success (201 Created):**
```json
{
    "data": {
        "login": "username",
        "user_id": "477ce6b7-ca31-4218-b017-b6ec6be47a54"
    },
    "message": "User registered successfully",
    "status": "success"
}
```

**Error (409 Conflict):**
```json
{
  "status": "error",
  "code": "LOGIN_EXISTS",
  "message": "User with this login already exists"
}
```

**Error (400 Bad Request):**
Invalid login
```json
{
    "code": "INVALID_LOGIN",
    "message": "Login must be 3-50 characters and contain only letters, numbers and underscores",
    "status": "error"
}
```

Invalid password
```json
{
    "code": "INVALID_PASSWORD",
    "message": "Password must be at least 6 characters and contain at least one letter and one digit",
    "status": "error"
}
```

---

### 2. User authentication
```http
POST /api/v1/auth/login
Content-Type: application/json

{
  "login": "username",
  "password": "SecurePass123!"
}
```

**Responses:**
**Success (200 OK):**
```json
{
    "data": {
        "access_token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpX...",
        "expires_in": 900,
        "login": "alice",
        "refresh_token": "eyJhbGciOiJIUzI1NiIsInR...",
        "token_type": "Bearer",
        "user_id": "cbb48acb-25e6-4c0b-b8c6-ebc779ecb941"
    },
    "message": "Login successful",
    "status": "success"
}
```

**Error (401 Unauthorized):**
```json
{
    "code": "INVALID_CREDENTIALS",
    "message": "Invalid login or password",
    "status": "error"
}
```

**Error (400 Bad Request):**
```json
{
    "code": "MISSING_FIELDS",
    "message": "Login and password are required",
    "status": "error"
}
```

---

### 3. Access Token Refresh
```http
POST /api/v1/auth/refresh
Content-Type: application/json

{
  "refresh_token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9..."
}
```

**Responses:**
**Success (200 OK):**
```json
{
    "data": {
        "access_token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpX...",
        "expires_in": 900,
        "refresh_token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVC...",
        "token_type": "Bearer",
        "user_id": "cbb48acb-25e6-4c0b-b8c6-ebc779ecb941"
    },
    "message": "Tokens refreshed successfully",
    "status": "success"
}
```

**Error (401 Unauthorized):**
```json
{
    "code": "INVALID_REFRESH_TOKEN",
    "message": "Refresh token not found or expired",
    "status": "error"
}
```

```json
{
    "code": "REFRESH_FAILED",
    "message": "Token refresh failed",
    "status": "error"
}
```

**Error (400 Bad Request):**
```json
{
  "status": "error",
  "code": "MISSING_TOKEN",
  "message": "Refresh token is required"
}
```

---

### 4. Logout
```http
POST /api/v1/auth/logout
Authorization: Bearer <access_token>
Content-Type: application/json

{
  "refresh_token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9..."
}
```

**Responses:**
**Success (200 OK):**
```json
{
    "message": "Successfully logged out",
    "status": "success"
}
```

**Error (400 Bad Request):**
```json
{
    "code": "MISSING_TOKEN",
    "message": "Refresh token is required",
    "status": "error"
}
```

**Error (401 Unauthorized):**
```json
{
    "code": "INVALID_TOKEN",
    "message": "Invalid access token",
    "status": "error"
}
```

---

### 5. Change password
```http
PUT /api/v1/auth/password
Authorization: Bearer <access_token>
Content-Type: application/json

{
  "old_password": "CurrentPass123!",
  "new_password": "NewSecurePass456!"
}
```

**Responses:**
**Success (200 OK):**
```json
{
    "message": "Password changed successfully",
    "status": "success"
}
```

**Error (403 Forbidden):**
```json
{
    "code": "INVALID_PASSWORD",
    "message": "Current password is incorrect",
    "status": "error"
}
```

**Error (400 Bad Request):**
```json
{
  "status": "error",
  "code": "VALIDATION_ERROR",
  "message": "Password is required",
}
```

```json
{
    "code": "INVALID_PASSWORD",
    "message": "New password must be at least 6 characters and contain at least one letter and one digit",
    "status": "error"
}
```

**Error (401 Unauthorized):**
```json
{
    "code": "INVALID_TOKEN",
    "message": "Invalid access token",
    "status": "error"
}
```

---

### 6. Deleting an account
```http
DELETE /api/v1/auth/account
Authorization: Bearer <access_token>
```

**Responses:**
**Success (200 OK):**
```json
{
    "message": "Account deleted successfully",
    "status": "success"
}
```

**Error (401 Unauthorized):**
```json
{
    "code": "INVALID_TOKEN",
    "message": "Invalid access token",
    "status": "error"
}
```

---

### 7. Viewing the list of users
```http
GET /api/v1/users?page=1&limit=50&search=username
Authorization: Bearer <access_token>
```

**Параметры запроса:**
- `page` - page number (default: 1)
- `limit` - number of users per page (default: 50, maximum: 100)
- `search` - search by login (optional)

**Responses:**
**Success (200 OK):**
```json
{
    "data": {
        "pagination": {
            "has_next": false,
            "has_prev": false,
            "limit": 50,
            "page": 1,
            "total_count": 3,
            "total_pages": 1
        },
        "users": [
            {
                "login": "charlie",
                "user_id": "78eb80ee-fe46-4fc6-b640-3fa6b393a4b5"
            },
            {
                "login": "bob",
                "user_id": "7166634d-2ccd-407a-b8dd-e93597ff1f3e"
            },
            {
                "login": "alice",
                "user_id": "a2b9bd53-78d1-43a9-b398-29771930441d"
            }
        ]
    },
    "status": "success"
}
```

**Error (401 Unauthorized):**
```json
{
    "code": "INVALID_TOKEN",
    "message": "Invalid access token",
    "status": "error"
}
```

---

### 8. Search for a user
```http
GET /api/v1/users/search?query=username&limit=20
Authorization: Bearer <access_token>
```

**Параметры запроса:**
- `query` - search query (required)
- `limit` - limit the number of results (default: 20, maximum: 50)

**Responses:**
**Success (200 OK):**
```json
{
    "data": {
        "meta": {
            "count": 1,
            "limit": 20,
            "query": "bob"
        },
        "users": [
            {
                "login": "bob",
                "user_id": "7166634d-2ccd-407a-b8dd-e93597ff1f3e"
            }
        ]
    },
    "status": "success"
}
```

**Error (400 Bad Request):**
```json
{
    "code": "MISSING_QUERY",
    "message": "Search query is required",
    "status": "error"
}
```

**Error (401 Unauthorized):**
```json
{
  "status": "error",
  "code": "INVALID_TOKEN",
  "message": "Invalid access token"
}
```

---

### 9. Sending a message
```http
POST /api/v1/messages/send
Authorization: Bearer <access_token>
Content-Type: application/json

{
  "to_login": "recipient_username",
  "message": "Hello there!"
}
```

**Responses:**
**Success (201 Created):**
```json
{
    "data": {
        "message_id": "c17fa376-9834-4d25-a7eb-00e68e0db9ad",
        "sent_at": "2025-11-27 12:08:09.2341589"
    },
    "message": "Message sent successfully",
    "status": "success"
}
```

**Error (404 Not Found):**
```json
{
    "code": "USER_NOT_FOUND",
    "message": "Recipient user not found",
    "status": "error"
}
```

**Error (400 Bad Request):**
```json
{
    "code": "EMPTY_MESSAGE",
    "message": "Message cannot be empty",
    "status": "error"
}
```

```json
{
    "code": "MESSAGE_TOO_LONG",
    "message": "Message exceeds maximum length of 5000 characters",
    "status": "error"
}
```

```json
{
    "code": "SELF_MESSAGE",
    "message": "Cannot send message to yourself",
    "status": "error"
}
```

**Error (401 Unauthorized):**
```json
{
  "status": "error",
  "code": "INVALID_TOKEN",
  "message": "Invalid access token"
}
```

---

### 10. Receiving messages
```http
GET /api/v1/messages?unread_only=true&after_message_id=last_id&limit=50
Authorization: Bearer <access_token>
```

**Параметры запроса:**
- `unread_only` - unread messages only (default: false)
- `after_message_id` - receive messages after the specified ID
- `before_message_id` - receive messages up to the specified ID
- `limit` - message limit (default: 50, maximum: 200)
- `conversation_with` - filter by specific user (optional)

**Responses:**
**Success (200 OK):**
```json
{
    "data": {
        "messages": [
            {
                "from_login": "alice",
                "from_user_id": "8caf53c4-0507-4c9c-b5e9-095b3188304a",
                "is_read": false,
                "message_id": "956f52da-2655-4d09-a5e2-bffa0138ae7c",
                "message_text": "alice to bob 3",
                "timestamp": "2025-11-27 13:39:35.868799+01",
                "to_login": "bob",
                "to_user_id": "ffdb8ebd-be03-49c5-b59e-a2911f6b5af8"
            },
            {
                "from_login": "alice",
                "from_user_id": "8caf53c4-0507-4c9c-b5e9-095b3188304a",
                "is_read": false,
                "message_id": "88fc4e08-6671-48b4-8b58-3e0ed7a07ec6",
                "message_text": "alice to bob 2",
                "timestamp": "2025-11-27 13:39:31.665435+01",
                "to_login": "bob",
                "to_user_id": "ffdb8ebd-be03-49c5-b59e-a2911f6b5af8"
            },
            {
                "from_login": "alice",
                "from_user_id": "8caf53c4-0507-4c9c-b5e9-095b3188304a",
                "is_read": false,
                "message_id": "122576c6-ab86-4915-b972-5aec581dbefc",
                "message_text": "alice to bob 1",
                "timestamp": "2025-11-27 13:39:29.518177+01",
                "to_login": "bob",
                "to_user_id": "ffdb8ebd-be03-49c5-b59e-a2911f6b5af8"
            }
        ],
        "meta": {
            "has_more": false,
            "last_message_id": "122576c6-ab86-4915-b972-5aec581dbefc",
            "total_count": 3,
            "unread_count": 0
        }
    },
    "status": "success"
}
```

**Error (401 Unauthorized):**
```json
{
  "status": "error",
  "code": "INVALID_TOKEN",
  "message": "Invalid access token"
}
```

**Error (400 Bad Request):**
```json
{
  "status": "error",
  "code": "INVALID_LIMIT",
  "message": "Limit must be between 1 and 200"
}
```

---

### 11. Marking messages as read
```http
POST /api/v1/messages/read
Authorization: Bearer <access_token>
Content-Type: application/json

{
  "message_ids": [
    "660e8400-e29b-41d4-a716-446655440000",
    "660e8400-e29b-41d4-a716-446655440001"
  ]
}
```

**Responses:**
**Success (200 OK):**
```json
{
    "data": {
        "read_count": 3
    },
    "message": "Messages marked as read",
    "status": "success"
}
```

**Error (400 Bad Request):**
```json
{
  "status": "error",
  "code": "EMPTY_MESSAGE_IDS",
  "message": "Message IDs array cannot be empty"
}
```

```json
{
  "status": "error",
  "code": "INVALID_MESSAGE_IDS",
  "message": "One or more message IDs are invalid"
}
```

**Error (401 Unauthorized):**
```json
{
  "status": "error",
  "code": "INVALID_TOKEN",
  "message": "Invalid access token"
}
```
