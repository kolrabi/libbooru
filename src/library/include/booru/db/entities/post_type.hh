#pragma once

#include "booru/db/entity.hh"

namespace Booru::DB::Entities
{

struct PostType : public Entity
{
    static char const constexpr* Table = "PostTypes";

    TEXT Name;
    TEXT Description;

    template <class Visitor>
    ResultCode IterateProperties( Visitor & _Visitor )
    {
        ENTITY_PROPERTY_KEY( Id );
        ENTITY_PROPERTY( Name );
        ENTITY_PROPERTY( Description );
        return ResultCode::OK;
    }
};

} // namespace Booru::DB::Entities
