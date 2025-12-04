#ifndef IMODEL_H
#define IMODEL_H

#include <string>
#include <nlohmann/json.hpp>

namespace models
{
/**
 * @class IModel
 * @brief Abstract base class for all data models in the system
 *
 * Defines the interface for model classes that represent database entities.
 * Provides methods for JSON serialization/deserialization, database operations,
 * and validation. All concrete model classes must implement this interface.
 *
 * @note This interface follows the CRUD (Create, Read, Update, Delete) pattern
 *       and supports both JSON and database representations.
 * @see User
 * @see Message
 */
class IModel
{
public:
    /**
     * @brief Default constructor
     * @note noexcept ensures no exceptions are thrown
     */
    IModel() noexcept = default;

    /**
     * @brief Virtual destructor for proper polymorphism
     * @note noexcept ensures no exceptions are thrown during destruction
     */
    virtual ~IModel() noexcept = default;

    /**
     * @brief Default copy constructor
     */
    IModel(const IModel&) = default;

    /**
     * @brief Default copy assignment operator
     */
    IModel& operator=(const IModel&) = default;

    /**
     * @brief Default move constructor
     * @note noexcept ensures no exceptions are thrown during move
     */
    IModel(IModel&&) noexcept = default;

    /**
     * @brief Default move assignment operator
     * @note noexcept ensures no exceptions are thrown during move
     */
    IModel& operator=(IModel&&) noexcept = default;

    /**
     * @brief Serializes the model to JSON format
     * @return nlohmann::json JSON representation of the model
     * @note This method must be implemented by derived classes
     * @note noexcept ensures no exceptions are thrown
     */
    [[nodiscard]] virtual nlohmann::json toJson() const noexcept = 0;

    /**
     * @brief Deserializes the model from JSON format
     * @param json JSON object containing model data
     * @return bool True if deserialization succeeded, false otherwise
     * @note This method must be implemented by derived classes
     * @note noexcept ensures no exceptions are thrown
     */
    [[nodiscard]] virtual bool fromJson(const nlohmann::json& json) noexcept = 0;

    /**
     * @brief Validates the model's current state
     * @return bool True if the model is valid, false otherwise
     * @note This method must be implemented by derived classes
     * @note noexcept ensures no exceptions are thrown
     */
    [[nodiscard]] virtual bool isValid() const noexcept = 0;

    /**
     * @brief Gets the name of the database table associated with the model
     * @return std::string Name of the database table
     * @note This method must be implemented by derived classes
     * @note noexcept ensures no exceptions are thrown
     */
    [[nodiscard]] virtual std::string getTableName() const noexcept = 0;

    /**
     * @brief Gets the name of the primary key column
     * @return std::string Name of the primary key column
     * @note This method must be implemented by derived classes
     * @note noexcept ensures no exceptions are thrown
     */
    [[nodiscard]] virtual std::string getPrimaryKey() const noexcept = 0;

    /**
     * @brief Gets the value of the primary key for the current model instance
     * @return std::string Value of the primary key
     * @note This method must be implemented by derived classes
     * @note noexcept ensures no exceptions are thrown
     */
    [[nodiscard]] virtual std::string getPrimaryKeyValue() const noexcept = 0;

    /**
     * @brief Generates SQL INSERT statement for the current model
     * @return std::string SQL INSERT statement
     * @note This method must be implemented by derived classes
     * @note noexcept ensures no exceptions are thrown
     */
    [[nodiscard]] virtual std::string generateInsertSql() const noexcept = 0;

    /**
     * @brief Generates SQL UPDATE statement for the current model
     * @return std::string SQL UPDATE statement
     * @note This method must be implemented by derived classes
     * @note May throw exceptions on error
     */
    [[nodiscard]] virtual std::string generateUpdateSql() const = 0;

    /**
     * @brief Populates the model from a database row
     * @param row JSON representation of a database row
     * @note This method must be implemented by derived classes
     * @note May throw exceptions on error
     */
    virtual void fromDatabaseRow(const nlohmann::json& row) = 0;

protected:
    /**
     * @brief Gets current timestamp in database-friendly format
     * @return std::string Current timestamp as "YYYY-MM-DD HH:MM:SS"
     * @note Used by derived classes for timestamp generation
     * @note noexcept ensures no exceptions are thrown (returns fallback on error)
     */
    [[nodiscard]] std::string getCurrentTimestamp() noexcept;
};
}

#endif // IMODEL_H