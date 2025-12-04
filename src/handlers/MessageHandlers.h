#ifndef MESSAGE_HANDLERS_H
#define MESSAGE_HANDLERS_H

#include "IHandler.h"
#include "../models/Message.h"
#include "../auth/JWTManager.h"
#include "../database/DatabaseManager.h"

namespace handlers
{
/**
 * @class MessageHandlers
 * @brief Handles message-related HTTP endpoints
 *
 * Implements message sending, retrieval, and management operations.
 * This class provides endpoints for sending messages, retrieving message history,
 * marking messages as read, and filtering conversations.
 *
 * @note All methods are thread-safe and exception-safe unless otherwise specified.
 * @see IHandler
 */
class MessageHandlers final : public IHandler
{
public:
    /**
     * @brief Constructs a MessageHandlers instance with required dependencies
     * @param jwtManager Shared pointer to JWT token manager for authentication
     * @param dbManager Shared pointer to database manager for data persistence
     * @note Both dependencies must be non-null for proper operation
     * @throws std::invalid_argument if any parameter is null
     */
    MessageHandlers(std::shared_ptr<auth::JWTManager> jwtManager, std::shared_ptr<database::DatabaseManager> dbManager) noexcept;

    /**
     * @brief Default virtual destructor
     * @note Ensures proper cleanup of inherited resources
     */
    virtual ~MessageHandlers() noexcept override = default;

    /**
     * @brief Deleted copy constructor
     * @note MessageHandlers should not be copied
     */
    MessageHandlers(const MessageHandlers&) = delete;

    /**
     * @brief Deleted copy assignment operator
     * @note MessageHandlers should not be copied
     */
    MessageHandlers& operator=(const MessageHandlers&) = delete;

    /**
     * @brief Default move constructor
     * @note MessageHandlers can be moved
     */
    MessageHandlers(MessageHandlers&&) noexcept = default;

    /**
     * @brief Default move assignment operator
     * @note MessageHandlers can be moved
     */
    MessageHandlers& operator=(MessageHandlers&&) noexcept = default;

    /**
     * @brief Main request handler for message endpoints
     * @param request HTTP request to process
     * @return boost::beast::http::response<boost::beast::http::string_body> HTTP response
     * @note Routes requests to appropriate handler methods based on endpoint and HTTP method
     * @warning This method never throws exceptions; errors are returned as HTTP error responses
     * @see handleSendMessage
     * @see handleGetMessages
     * @see handleMarkAsRead
     */
    [[nodiscard]] virtual boost::beast::http::response<boost::beast::http::string_body> handleRequest(
        const boost::beast::http::request<boost::beast::http::string_body>& request) noexcept override;

    /**
     * @brief Returns HTTP methods supported by message endpoints
     * @return std::vector<boost::beast::http::verb> List of supported HTTP methods
     * @note Message endpoints support GET and POST methods
     */
    [[nodiscard]] virtual std::vector<boost::beast::http::verb> getSupportedMethods() const noexcept override;

private:
    /**
     * @brief Handles message sending endpoint
     * @param request HTTP POST request with message data
     * @return HTTP response indicating send success or failure
     * @details Expected JSON body: {"to_login": string, "message": string}
     * @note Requires Bearer token in Authorization header
     * @see models::Message::createMessage
     */
    [[nodiscard]] boost::beast::http::response<boost::beast::http::string_body> handleSendMessage(const boost::beast::http::request<boost::beast::http::string_body>& request) const noexcept;

    /**
     * @brief Handles message retrieval endpoint
     * @param request HTTP GET request with optional query parameters
     * @return HTTP response with message list and metadata
     * @details Supported query parameters:
     * - unread_only (bool): Return only unread messages
     * - after_message_id (string): Return messages after specified ID
     * - before_message_id (string): Return messages before specified ID
     * - limit (int): Maximum number of messages to return (1-200, default 50)
     * - conversation_with (string): Filter messages to specific user
     * @note Requires Bearer token in Authorization header
     * @see getMessagesForUser
     */
    [[nodiscard]] boost::beast::http::response<boost::beast::http::string_body> handleGetMessages(const boost::beast::http::request<boost::beast::http::string_body>& request) const noexcept;

    /**
     * @brief Handles message read status update endpoint
     * @param request HTTP POST request with message IDs to mark as read
     * @return HTTP response with count of messages marked as read
     * @details Expected JSON body: {"message_ids": array of string}
     * @note Requires Bearer token in Authorization header
     * @see markMessagesAsRead
     */
    [[nodiscard]] boost::beast::http::response<boost::beast::http::string_body> handleMarkAsRead(const boost::beast::http::request<boost::beast::http::string_body>& request) const noexcept;

    /**
     * @brief Validates JWT access tokens for message operations
     * @param token JWT access token to validate
     * @param[out] userId User ID extracted from valid token
     * @return bool True if token is valid, false otherwise
     * @note Implements the pure virtual method from IHandler
     * @see auth::JWTManager::verifyAndDecode
     */
    [[nodiscard]] virtual bool isAuthTokenValid(const std::string& token, std::string& userId) const noexcept override;

    /**
     * @brief Retrieves user ID by login name
     * @param login User login name to lookup
     * @return string User ID if found, empty string otherwise
     * @note Logs errors to the system logger
     */
    [[nodiscard]] std::string getUserIdByLogin(const std::string& login) const noexcept;

    /**
     * @brief Retrieves messages for a specific user with various filters
     * @param userId ID of the user to retrieve messages for
     * @param isUnreadOnly If true, returns only unread messages (default: false)
     * @param afterMessageId Return messages after this message ID (default: empty)
     * @param beforeMessageId Return messages before this message ID (default: empty)
     * @param limit Maximum number of messages to return (default: 50)
     * @param conversationWith Filter messages to conversation with specific user (default: empty)
     * @return std::vector<models::Message> List of messages matching criteria
     * @throws std::exception on database errors
     * @note Messages are returned in descending chronological order (newest first)
     */
    [[nodiscard]] std::vector<models::Message> getMessagesForUser(const std::string& userId,
        bool isUnreadOnly = false,
        const std::string& afterMessageId = "",
        const std::string& beforeMessageId = "",
        int limit = 50,
        const std::string& conversationWith = "") const;

    /**
     * @brief Marks specified messages as read for a user
     * @param messageIds Vector of message IDs to mark as read
     * @param userId ID of the user marking messages as read
     * @return int Number of messages successfully marked as read
     * @throws std::exception on database errors
     * @note Only marks messages where the user is the recipient
     */
    [[nodiscard]] int markMessagesAsRead(const std::vector<std::string>& messageIds, const std::string& userId) const;

    /**
     * @brief Retrieves count of unread messages for a user
     * @param userId ID of the user to check
     * @return int Number of unread messages
     * @note Logs errors to the system logger
     */
    [[nodiscard]] int getUnreadMessagesCount(const std::string& userId) const noexcept;

private:
    std::shared_ptr<auth::JWTManager> jwtManager_;      ///< JWT token manager for authentication
    std::shared_ptr<database::DatabaseManager> dbManager_; ///< Database manager for message storage
};
}

#endif // MESSAGE_HANDLERS_H