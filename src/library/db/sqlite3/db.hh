#pragma once

#include <booru/db.hh>

struct sqlite3;

namespace Booru::DB::Sqlite3
{

class Backend : public DB::IBackend
{
  public:
    static ExpectedDB OpenDatabase(StringView const& _Path);

    explicit Backend(sqlite3* _Handle);
    virtual ~Backend() override;

    virtual ExpectedStmt PrepareStatement(StringView const& _SQL) override;
    virtual ResultCode ExecuteSQL(StringView const& _SQL) override;

    virtual bool IsInTransaction() const override;
    virtual ResultCode BeginTransaction() override;
    virtual ResultCode CommitTransaction() override;
    virtual ResultCode RollbackTransaction() override;

    virtual Expected<DB::INTEGER> GetLastRowId() override;

  private:
    sqlite3* m_Handle        = nullptr;

    int m_TransactionDepth   = 0;
    bool m_TransactionFailed = false;
};

} // namespace Booru::DB::Sqlite3
