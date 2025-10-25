#pragma once

#include <booru/db.hh>
#include <booru/db/visitors.hh>

namespace Booru::DB::Query
{

    // Abstract where condition for queries.
    class Where
    {
    public:
        Where(StringView const &_A, StringView const &_B, StringView const &_Op) : A{_A}, B{_B}, Op{_Op} {}

        template <class TA, class TB>
        static Where Equal(TA const &_A, TB const &_B)
        {
            return Where(String(_A), String(_B), "=="sv);
        }

        template <class TA, class TB>
        static Where In(TA const &_A, TB const &_B)
        {
            return Where(String(_A), String(_B), "IN"sv);
        }

        operator String() const
        {
            return Strings::Join({"( ", A, ") ", Op, " ( ", B, " )"}, "");
        }

    private:
        String A;
        String B;
        String Op;
    };

    // Abstract query base. Where conditions are ANDed together.
    template <class Derived>
    class Query
    {
    public:
        static inline const String LOGGER = "booru.db.query";

        Query(StringView const &_Table) : Table{_Table} {}

        // Tries to prepare the query into a statement.
        ExpectedStmt Prepare(DBPtr _DB) const
        {
            if (_DB)
                return _DB->PrepareStatement(AsString());
            return ResultCode::InvalidArgument;
        }

        // Add a column to the query.
        Derived &Column(StringView const &_Name)
        {
            Columns.push_back(String(_Name));
            return static_cast<Derived &>(*this);
        }

        // Add a where condition.
        Derived &Where(String const &_Where)
        {
            WhereArgs.push_back(_Where);
            return static_cast<Derived &>(*this);
        }

        // Add a key where condition (WHERE key = $key). Bind "$key" in the prepared statement.
        Derived &Key(StringView const &_Key)
        {
            String keyString{_Key};
            return Where(Where::Equal(keyString, "$"s + keyString));
        }

        operator String() const { return AsString(); }

    protected:
        String Table;
        StringVector Columns;
        StringVector WhereArgs;

        String GetWhereString() const
        {
            if (WhereArgs.empty())
                return "";
            return " WHERE "s + Strings::Join(WhereArgs, " AND ");
        }

        virtual String AsString() const = 0;
    };

    // Simple select query. Queries given columns or all if none are specified.
    class Select : public Query<Select>
    {
    public:
        explicit Select(StringView const &_Table) : Query{_Table} {}

    protected:
        String AsString() const override
        {
            String sqlString = "SELECT ";

            if (Columns.empty())
                sqlString += "*";
            else
                sqlString += Strings::Join(Columns, ", ");

            if (Table != "")
                sqlString += " FROM " + Table;

            sqlString += GetWhereString();
            return sqlString;
        }
    };

    // Simple insert query. Inserts given columns.
    class Insert : public Query<Insert>
    {
    public:
        explicit Insert(StringView const &_Table) : Query{_Table} {}

    protected:
        String AsString() const override
        {
            return "INSERT INTO "s + Table + Values();
        }

        String Values() const
        {
            String sqlString = "\n(";
            sqlString += Strings::Join(Columns, ", ");
            sqlString += ")\nVALUES(";
            if (!Columns.empty())
            {
                sqlString += "$";
                sqlString += Strings::Join(Columns, ", $");
            }
            sqlString += ")\n";
            return sqlString;
        }
    };

    // Simple insert query. Inserts given columns.
    class Upsert : public Insert
    {
    public:
        explicit Upsert(StringView const &_Table) : Insert{_Table} {}

    protected:
        String AsString() const override
        {
            return "INSERT OR REPLACE INTO "s + Table + Values();
        }
    };

    // Simple update query. Updates given columns with values.
    class Update : public Query<Update>
    {
    public:
        explicit Update(StringView const &_Table) : Query{_Table} {}

    protected:
        String AsString() const override
        {
            String sqlString = "UPDATE " + Table + "\nSET ";

            bool first = true;
            for (auto const &col : Columns)
            {
                if (!first)
                {
                    sqlString += ", ";
                }
                sqlString += col + " = $" + col;
                first = false;
            }
            sqlString += " " + GetWhereString();
            return sqlString;
        }
    };

    // Simple delete query. Deletes rows that match keys and values. Will not delete all rows if no key is given.
    class Delete : public Query<Delete>
    {
    public:
        explicit Delete(StringView const &_Table) : Query{_Table} {}

    protected:
        String AsString() const override
        {
            if (WhereArgs.empty())
            {
                LOG_ERROR("Refusing to create a DELETE statement without a where clause!");
                // not doing a delete without a where
                return "--";
            }

            return "DELETE FROM "s + Table + " " + GetWhereString();
        }
    };

    // Query to insert a new entity into the database.
    template <class TValue>
    class InsertEntity : public Insert
    {
    public:
        InsertEntity(StringView const &_Table, TValue &_Entity) : Insert(_Table)
        {
            auto Visitor = Visitors::QueryNonPrimaryKeyColumnVisitor(*this);
            CHECK(_Entity.IterateProperties(Visitor));
        }
    };

    // Query to update a entity property values in the database.
    template <class TValue>
    class UpdateEntity : public Update
    {
    public:
        UpdateEntity(StringView const &_Table, TValue &_Entity) : Update(_Table)
        {
            auto Visitor = Visitors::QueryNonPrimaryKeyColumnVisitor(*this);
            CHECK(_Entity.IterateProperties(Visitor));
        }
    };

} // namespace Booru::DB
