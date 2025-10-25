#pragma once

#include <booru/common.hh>
#include <booru/result.hh>

#include <booru/db/types.hh>

namespace Booru::DB
{

    static constexpr String LOGGER = "booru.db";

    /// @brief Common interface for database connections.
    class DatabaseInterface
    {
    public:
        virtual ~DatabaseInterface() {}

        /// @brief Prepare a statement from SQL string.
        virtual ExpectedStmt PrepareStatement(StringView const &_SQL) = 0;

        /// @brief Execute SQL directly.
        virtual ResultCode ExecuteSQL(StringView const &_SQL) = 0;

        // Transaction control.
        virtual bool IsInTransaction() const = 0;

        /// @brief Enter a transaction. Recursively nests transactions if one already is active.
        virtual ResultCode BeginTransaction() = 0;

        /// @brief If in toplevel transaction, commit to database and leave transaction. Otherwise just leave transaction.
        virtual ResultCode CommitTransaction() = 0;

        /// @brief If in toplevel transaction, roll back transaction. Otherwise mark current transaction as failed and roll it back on final rollback or commit.
        virtual ResultCode RollbackTransaction() = 0;

        /// @brief Get unique id for last inserted database row.
        virtual Expected<INTEGER> GetLastRowId() = 0;
    };

    /// @brief RAII transaction guard that handles transaction scope.
    class TransactionGuard
    {
    public:
        /// @brief C'tor, enter transaction
        explicit TransactionGuard(DBPtr _DB)
            : DB{_DB}, m_IsCommited{false}, m_IsValid{false}
        {
            LOG_DEBUG("Transaction Guard {}: BEGIN", static_cast<void *>(this));
            m_IsValid = !ResultIsError(DB->BeginTransaction());
        }

        ~TransactionGuard()
        {
            if (!m_IsCommited && m_IsValid)
            {
                LOG_DEBUG("Transaction Guard {}: ROLLBACK", static_cast<void *>(this));
                CHECK(DB->RollbackTransaction());
            }
        }

        void Commit()
        {
            if (!m_IsCommited && m_IsValid)
            {
                LOG_DEBUG("Transaction Guard {}: COMMIT", static_cast<void *>(this));
                CHECK(DB->CommitTransaction());
                m_IsCommited = true;
            }
        }

        void Rollback()
        {
            if (!m_IsCommited && m_IsValid)
            {
                LOG_DEBUG("Transaction Guard {}: ROLLBACK", static_cast<void *>(this));
                CHECK(DB->RollbackTransaction());
                m_IsCommited = true;
            }
        }

        bool GetIsValid() const { return m_IsValid; }

    private:
        DBPtr DB;
        bool m_IsCommited;
        bool m_IsValid;
    };

} // namespace Booru::DB
