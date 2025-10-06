#pragma once

#include "booru/db/entity.hh"

namespace Booru::DB::Entities
{

struct TagImplication : public Entity
{
    static char const constexpr* Table = "TagImplications";
    static char const constexpr* LOGGER = "booru.db.entites.tagimplications";

    enum
    {
        FLAG_REMOVE_TAG = 1 << 0
    };

    INTEGER TagId        = -1;
    INTEGER ImpliedTagId = -1;
    INTEGER Flags        = 0;

    template <class Visitor>
    ResultCode IterateProperties( Visitor & _Visitor )
    {
        ENTITY_PROPERTY_KEY( Id );
        ENTITY_PROPERTY( TagId );
        ENTITY_PROPERTY( ImpliedTagId );
        ENTITY_PROPERTY( Flags );
        return ResultCode::OK;
    }
};

} // namespace Booru::DB::Entities
