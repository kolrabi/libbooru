#pragma once

#include <booru/db/entity.hh>

namespace Booru::DB::Entities
{

struct Site : public Entity
{
    static auto constexpr Table  = "Sites";
    static auto constexpr LOGGER = "booru.db.entites.sites";

    TEXT Name;
    TEXT Description;

    template <class Visitor> ResultCode IterateProperties(Visitor& _Visitor)
    {
        ENTITY_PROPERTY_KEY(Id);
        ENTITY_PROPERTY(Name);
        ENTITY_PROPERTY(Description);
        return ResultCode::OK;
    }
};

} // namespace Booru::DB::Entities
