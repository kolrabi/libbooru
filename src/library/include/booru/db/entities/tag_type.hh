#pragma once

#include <booru/db/entity.hh>

namespace Booru::DB::Entities
{

struct TagType : public Entity
{
    static char const constexpr* Table = "TagTypes";
    static char const constexpr* LOGGER = "booru.db.entites.tagtypes";

    TEXT Name;
    TEXT Description;
    INTEGER Color = 0xFFFFFFFF;

    template <class Visitor>
    ResultCode IterateProperties( Visitor & _Visitor )
    {
        ENTITY_PROPERTY_KEY( Id );
        ENTITY_PROPERTY( Name );
        ENTITY_PROPERTY( Description );
        ENTITY_PROPERTY( Color );
        return ResultCode::OK;
    }
};

} // namespace Booru::DB::Entities
