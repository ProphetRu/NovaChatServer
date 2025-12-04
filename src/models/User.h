#ifndef USER_MODEL_H
#define USER_MODEL_H

#include "IModel.h"

namespace models
{
/**
 * @class User
 * @brief Represents a user entity in the system
 *
 * Implements the IModel interface for user data persistence and serialization.
 * This class encapsulates user authentication data including login, password hash,
 * unique identifier, and account creation timestamp.
 *
 * @note All setter methods perform validation and may throw exceptions
 * @see IModel
 */
class User final : public IModel
{
public:
    /**
     * @brief Default constructor
     * @note Creates an empty user with default values
     */
    User() noexcept = default;

    /**
     * @brief Parameterized constructor with login and password
     * @param login User login name
     * @param password Plain text password (will be hashed)
     * @throws std::invalid_argument if login or password validation fails
     */
    User(const std::string& login, const std::string& password);

    /**
     * @brief Constructor from JSON string
     * @param json JSON string representation of a user
     * @throws std::invalid_argument if JSON parsing fails
     */
    explicit User(const std::string& json);

    /**
     * @brief Virtual destructor
     */
    virtual ~User() noexcept override = default;

    /**
     * @brief Default copy constructor
     */
    User(const User&) = default;

    /**
     * @brief Default copy assignment operator
     */
    User& operator=(const User&) = default;

    /**
     * @brief Default move constructor
     */
    User(User&&) noexcept = default;

    /**
     * @brief Default move assignment operator
     */
    User& operator=(User&&) noexcept = default;

    // Getter methods

    /**
     * @brief Gets the unique user identifier
     * @return const std::string& User ID (UUID)
     */
    [[nodiscard]] const std::string& getUserId() const noexcept;

    /**
     * @brief Gets the user login name
     * @return const std::string& Login name
     */
    [[nodiscard]] const std::string& getLogin() const noexcept;

    /**
     * @brief Gets the password hash
     * @return const std::string& Hashed password
     * @warning Never expose this value in API responses
     */
    [[nodiscard]] const std::string& getPasswordHash() const noexcept;

    /**
     * @brief Gets the account creation timestamp
     * @return const std::string& Creation timestamp
     */
    [[nodiscard]] const std::string& getCreatedAt() const noexcept;

    // Setter methods

    /**
     * @brief Sets the user identifier
     * @param id New user ID (UUID)
     */
    void setUserId(const std::string& id) noexcept;

    /**
     * @brief Sets the user login name
     * @param login Login name
     * @throws std::invalid_argument if login validation fails
     */
    void setLogin(const std::string& login);

    /**
     * @brief Sets the user password (hashes the plain text)
     * @param password Plain text password
     * @throws std::invalid_argument if password validation fails
     */
    void setPassword(const std::string& password);

    /**
     * @brief Sets the password hash directly
     * @param hash Pre-computed password hash
     * @note Use with caution - no validation is performed
     */
    void setPasswordHash(const std::string& hash) noexcept;

    /**
     * @brief Sets the account creation timestamp
     * @param timestamp Creation timestamp
     */
    void setCreatedAt(const std::string& timestamp) noexcept;

    // IModel interface implementation

    /**
     * @brief Serializes the user to JSON format
     * @return nlohmann::json JSON representation of the user
     * @note Excludes sensitive data like password hash
     * @see IModel::toJson
     */
    [[nodiscard]] virtual nlohmann::json toJson() const noexcept override;

    /**
     * @brief Deserializes the user from JSON format
     * @param json JSON object containing user data
     * @return bool True if deserialization succeeded, false otherwise
     * @see IModel::fromJson
     */
    [[nodiscard]] virtual bool fromJson(const nlohmann::json& json) noexcept override;

    /**
     * @brief Validates the user's current state
     * @return bool True if the user is valid, false otherwise
     * @see IModel::isValid
     */
    [[nodiscard]] virtual bool isValid() const noexcept override;

    /**
     * @brief Gets the database table name for users
     * @return std::string Table name "users"
     * @see IModel::getTableName
     */
    [[nodiscard]] virtual std::string getTableName() const noexcept override;

    /**
     * @brief Gets the primary key column name
     * @return std::string Primary key column name "user_id"
     * @see IModel::getPrimaryKey
     */
    [[nodiscard]] virtual std::string getPrimaryKey() const noexcept override;

    /**
     * @brief Gets the primary key value for this user
     * @return std::string User ID value
     * @see IModel::getPrimaryKeyValue
     */
    [[nodiscard]] virtual std::string getPrimaryKeyValue() const noexcept override;

    /**
     * @brief Generates SQL INSERT statement for this user
     * @return std::string SQL INSERT statement
     * @see IModel::generateInsertSql
     */
    [[nodiscard]] virtual std::string generateInsertSql() const noexcept override;

    /**
     * @brief Generates SQL UPDATE statement for this user
     * @return std::string SQL UPDATE statement
     * @throws std::runtime_error if user ID is empty
     * @see IModel::generateUpdateSql
     */
    [[nodiscard]] virtual std::string generateUpdateSql() const override;

    /**
     * @brief Populates the user from a database row
     * @param row JSON representation of a database row
     * @throws std::runtime_error if data is invalid
     * @see IModel::fromDatabaseRow
     */
    virtual void fromDatabaseRow(const nlohmann::json& row) override;

    // User-specific operations

    /**
     * @brief Verifies if a plain text password matches the stored hash
     * @param password Plain text password to verify
     * @return bool True if password is valid, false otherwise
     */
    [[nodiscard]] bool isPasswordValid(const std::string& password) const noexcept;

    /**
     * @brief Factory method to create a new user from credentials
     * @param login User login name
     * @param password Plain text password
     * @return User New user instance with generated ID
     * @throws std::invalid_argument if credentials are invalid
     */
    static User createFromCredentials(const std::string& login, const std::string& password);

    // Static factory methods

    /**
     * @brief Creates a User instance from JSON string
     * @param jsonString JSON string representation
     * @return User User instance
     * @throws std::invalid_argument if parsing fails
     */
    static User fromJsonString(const std::string& jsonString);

    /**
     * @brief Creates a User instance from database row
     * @param row JSON representation of database row
     * @return User User instance
     * @throws std::runtime_error if data is invalid
     */
    static User fromDatabase(const nlohmann::json& row);

private:
    std::string id_;           ///< Unique user identifier (UUID)
    std::string login_;        ///< User login name
    std::string passwordHash_; ///< Hashed password (never stored in plain text)
    std::string createdAt_{ getCurrentTimestamp() }; ///< Account creation timestamp
};
}

#endif // USER_MODEL_H