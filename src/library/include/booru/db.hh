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
    virtual ExpectedOwning<DatabasePreparedStatementInterface>
    PrepareStatement( StringView const & _SQL )              = 0;

    /// @brief Execute SQL directly. 
    virtual ResultCode ExecuteSQL( StringView const & _SQL ) = 0;

    // Transaction control.

    /// @brief Enter a transaction. Recursively nests transactions if one already is active.
    virtual ResultCode BeginTransaction()    = 0;

    /// @brief If in toplevel transaction, commit to database and leave transaction. Otherwise just leave transaction.
    virtual ResultCode CommitTransaction()   = 0;

    /// @brief If in toplevel transaction, roll back transaction. Otherwise mark current transaction as failed and roll it back on final rollback or commit.
    virtual ResultCode RollbackTransaction() = 0;

    /// @brief Get unique id for last inserted database row.
    virtual ResultCode GetLastRowId( INTEGER& _Id ) = 0;
};

/// @brief RAII transaction guard that handles transaction scope.
class TransactionGuard
{
  public:

    /// @brief C'tor, enter transaction
    TransactionGuard( DatabaseInterface& _DB )
        : DB{ _DB }, IsCommited{ false }, IsValid{ false }
    {
        LOG_DEBUG("Transaction Guard {}: BEGIN", static_cast<void*>(this));
        IsValid = !ResultIsError( this->DB.BeginTransaction() );
    }

    ~TransactionGuard()
    {
        if ( !this->IsCommited )
        {
            LOG_DEBUG("Transaction Guard {}: ROLLBACK", static_cast<void*>(this));
            auto result = this->DB.RollbackTransaction();
        }
    }

    void Commit()
    {
        if ( !IsCommited && IsValid )
        {
            LOG_DEBUG("Transaction Guard {}: COMMIT", static_cast<void*>(this));
            auto result      = this->DB.CommitTransaction();
            this->IsCommited = true;
        }
    }

    bool GetIsValid() const { return IsValid; }

  private:
    DatabaseInterface& DB;
    bool IsCommited;
    bool IsValid;
};

} // namespace Booru::DB
