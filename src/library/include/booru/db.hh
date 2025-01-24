#pragma once

#include "booru/common.hh"
#include "booru/db/types.hh"
#include "booru/result.hh"

namespace Booru::DB
{

class DatabasePreparedStatementInterface;
class DatabaseInterface
{
  public:
    virtual ~DatabaseInterface() {}
    virtual ExpectedOwning<DatabasePreparedStatementInterface>
    PrepareStatement( char const* _SQL )              = 0;
    virtual ResultCode ExecuteSQL( char const* _SQL ) = 0;

    virtual ResultCode BeginTransaction()    = 0;
    virtual ResultCode CommitTransaction()   = 0;
    virtual ResultCode RollbackTransaction() = 0;

    virtual ResultCode GetLastRowId( INTEGER& _Id ) = 0;
};

class TransactionGuard
{
  public:
    TransactionGuard( DatabaseInterface& _DB )
        : DB{ _DB }, IsCommited{ false }, IsValid{ false }
    {
        fprintf(stderr, "Transaction %p: Begin\n", this);
        IsValid = !ResultIsError( this->DB.BeginTransaction() );
    }

    ~TransactionGuard()
    {
        if ( !this->IsCommited )
        {
            fprintf(stderr, "Transaction %p: Rollback\n", this);
            auto result = this->DB.RollbackTransaction();
        }
    }

    void Commit()
    {
        if ( !IsCommited && IsValid )
        {
            fprintf(stderr, "Transaction %p: Commit\n", this);
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
