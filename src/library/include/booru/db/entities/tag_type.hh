#pragma once

#include <booru/db/entities.hh>

namespace Booru::DB::Entities
{

struct TagType : public Entity
{
    static auto constexpr Table  = "TagTypes";
    static auto constexpr LOGGER = "booru.db.entites.tagtypes";

    TEXT Name;
    TEXT Description;
    INTEGER Color = 0xFFFFFFFF;

    template <class Visitor> ResultCode IterateProperties(Visitor& _Visitor)
    {
        ENTITY_PROPERTY_KEY(Id);
        ENTITY_PROPERTY(Name);
        ENTITY_PROPERTY(Description);
        ENTITY_PROPERTY(Color);
        return ResultCode::OK;
    }
};

} // namespace Booru::DB::Entities
