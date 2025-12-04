#ifndef MESSAGE_MODEL_H
#define MESSAGE_MODEL_H

#include "IModel.h"

namespace models
{
/**
 * @class Message
 * @brief Represents a message exchanged between users
 *
 * Implements the IModel interface for message data persistence and serialization.
 * This class encapsulates all message-related data including sender, recipient,
 * message content, read status, and timestamps.
 *
 * @note All setter methods perform validation and may throw exceptions
 * @see IModel
 */
class Message final : public IModel
{
public:
    /**
     * @brief Default constructor
     * @note Creates an empty message with default values
     */
    Message() noexcept = default;

    /**
     * @brief Parameterized constructor
     * @param fromUserId ID of the message sender
     * @param toUserId ID of the message recipient
     * @param text Message content text
     * @throws std::invalid_argument if parameters are invalid
     */
    Message(const std::string& fromUserId, const std::string& toUserId, const std::string& text);

    /**
     * @brief Constructor from JSON string
     * @param json JSON string representation of a message
     * @throws std::invalid_argument if JSON parsing fails
     */
    explicit Message(const std::string& json);

    /**
     * @brief Virtual destructor
     */
    virtual ~Message() noexcept override = default;

    /**
     * @brief Default copy constructor
     */
    Message(const Message&) = default;

    /**
     * @brief Default copy assignment operator
     */
    Message& operator=(const Message&) = default;

    /**
     * @brief Default move constructor
     */
    Message(Message&&) noexcept = default;

    /**
     * @brief Default move assignment operator
     */
    Message& operator=(Message&&) noexcept = default;

    // Getter methods

    /**
     * @brief Gets the unique message identifier
     * @return const std::string& Message ID
     */
    [[nodiscard]] const std::string& getMessageId() const noexcept;

    /**
     * @brief Gets the sender's user ID
     * @return const std::string& Sender user ID
     */
    [[nodiscard]] const std::string& getFromUserId() const noexcept;

    /**
     * @brief Gets the recipient's user ID
     * @return const std::string& Recipient user ID
     */
    [[nodiscard]] const std::string& getToUserId() const noexcept;

    /**
     * @brief Gets the sender's login name
     * @return const std::string& Sender login
     */
    [[nodiscard]] const std::string& getFromLogin() const noexcept;

    /**
     * @brief Gets the recipient's login name
     * @return const std::string& Recipient login
     */
    [[nodiscard]] const std::string& getToLogin() const noexcept;

    /**
     * @brief Gets the message text content
     * @return const std::string& Message text
     */
    [[nodiscard]] const std::string& getMessageText() const noexcept;

    /**
     * @brief Gets the read status of the message
     * @return bool True if message has been read, false otherwise
     */
    [[nodiscard]] bool getIsRead() const noexcept;

    /**
     * @brief Gets the message creation timestamp
     * @return const std::string& Creation timestamp
     */
    [[nodiscard]] const std::string& getCreatedAt() const noexcept;

    // Setter methods

    /**
     * @brief Sets the message identifier
     * @param id New message ID
     */
    void setMessageId(const std::string& id) noexcept;

    /**
     * @brief Sets the sender's user ID
     * @param id Sender user ID
     */
    void setFromUserId(const std::string& id) noexcept;

    /**
     * @brief Sets the recipient's user ID
     * @param id Recipient user ID
     */
    void setToUserId(const std::string& id) noexcept;

    /**
     * @brief Sets the sender's login name
     * @param login Sender login
     */
    void setFromLogin(const std::string& login) noexcept;

    /**
     * @brief Sets the recipient's login name
     * @param login Recipient login
     */
    void setToLogin(const std::string& login) noexcept;

    /**
     * @brief Sets the message text content
     * @param text Message text
     * @throws std::invalid_argument if text validation fails
     */
    void setMessageText(const std::string& text);

    /**
     * @brief Sets the read status of the message
     * @param isRead True to mark as read, false as unread
     */
    void setIsRead(bool isRead) noexcept;

    /**
     * @brief Sets the message creation timestamp
     * @param timestamp Creation timestamp
     */
    void setCreatedAt(const std::string& timestamp) noexcept;

    // IModel interface implementation

    /**
     * @brief Serializes the message to JSON format
     * @return nlohmann::json JSON representation of the message
     * @see IModel::toJson
     */
    [[nodiscard]] virtual nlohmann::json toJson() const noexcept override;

    /**
     * @brief Deserializes the message from JSON format
     * @param json JSON object containing message data
     * @return bool True if deserialization succeeded, false otherwise
     * @see IModel::fromJson
     */
    [[nodiscard]] virtual bool fromJson(const nlohmann::json& json) noexcept override;

    /**
     * @brief Validates the message's current state
     * @return bool True if the message is valid, false otherwise
     * @see IModel::isValid
     */
    [[nodiscard]] virtual bool isValid() const noexcept override;

    /**
     * @brief Gets the database table name for messages
     * @return std::string Table name "messages"
     * @see IModel::getTableName
     */
    [[nodiscard]] virtual std::string getTableName() const noexcept override;

    /**
     * @brief Gets the primary key column name
     * @return std::string Primary key column name "message_id"
     * @see IModel::getPrimaryKey
     */
    [[nodiscard]] virtual std::string getPrimaryKey() const noexcept override;

    /**
     * @brief Gets the primary key value for this message
     * @return std::string Message ID value
     * @see IModel::getPrimaryKeyValue
     */
    [[nodiscard]] virtual std::string getPrimaryKeyValue() const noexcept override;

    /**
     * @brief Generates SQL INSERT statement for this message
     * @return std::string SQL INSERT statement
     * @see IModel::generateInsertSql
     */
    [[nodiscard]] virtual std::string generateInsertSql() const noexcept override;

    /**
     * @brief Generates SQL UPDATE statement for this message
     * @return std::string SQL UPDATE statement
     * @throws std::runtime_error if message ID is empty
     * @see IModel::generateUpdateSql
     */
    [[nodiscard]] virtual std::string generateUpdateSql() const override;

    /**
     * @brief Populates the message from a database row
     * @param row JSON representation of a database row
     * @throws std::runtime_error if data is invalid
     * @see IModel::fromDatabaseRow
     */
    virtual void fromDatabaseRow(const nlohmann::json& row) override;

    // Message-specific operations

    /**
     * @brief Marks the message as read
     */
    void markAsRead() noexcept;

    /**
     * @brief Checks if the message was sent by a specific user
     * @param userId User ID to check
     * @return bool True if message was sent by the specified user
     */
    [[nodiscard]] bool isFromUser(const std::string& userId) const noexcept;

    /**
     * @brief Checks if the message was sent to a specific user
     * @param userId User ID to check
     * @return bool True if message was sent to the specified user
     */
    [[nodiscard]] bool isToUser(const std::string& userId) const noexcept;

    // Static factory methods

    /**
     * @brief Creates a Message instance from JSON string
     * @param jsonString JSON string representation
     * @return Message Message instance
     * @throws std::invalid_argument if parsing fails
     */
    static Message fromJsonString(const std::string& jsonString);

    /**
     * @brief Creates a Message instance from database row
     * @param row JSON representation of database row
     * @return Message Message instance
     * @throws std::runtime_error if data is invalid
     */
    static Message fromDatabase(const nlohmann::json& row);

    /**
     * @brief Factory method to create a new message
     * @param fromUserId Sender user ID
     * @param toUserId Recipient user ID
     * @param text Message text content
     * @return Message New message instance with generated ID
     * @throws std::invalid_argument if parameters are invalid
     */
    static Message createMessage(const std::string& fromUserId, const std::string& toUserId, const std::string& text);

private:
    std::string id_;         ///< Unique message identifier (UUID)
    std::string fromUserID_; ///< Sender user ID
    std::string toUserID_;   ///< Recipient user ID
    std::string fromLogin_;  ///< Sender login name (for display purposes)
    std::string toLogin_;    ///< Recipient login name (for display purposes)
    std::string text_;       ///< Message text content (sanitized)
    bool isRead_{ false };   ///< Read status flag
    std::string createdAt_{ getCurrentTimestamp() }; ///< Creation timestamp
};
}

#endif // MESSAGE_MODEL_H