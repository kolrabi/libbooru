#pragma once

#include "booru/db/entity.hh"
namespace Booru::DB::Entities
{

struct PostTag : public Entity
{
    static char const constexpr* Table = "PostTags";

    INTEGER PostId = -1;
    INTEGER TagId  = -1;

    template <class Visitor>
    ResultCode IterateProperties( Visitor & _Visitor )
    {
        ENTITY_PROPERTY_KEY( Id );
        ENTITY_PROPERTY( PostId );
        ENTITY_PROPERTY( TagId );
        return ResultCode::OK;
    }
};

} // namespace Booru::DB::Entities
