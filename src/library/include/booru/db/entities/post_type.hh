#pragma once

#include <booru/db/entities.hh>

namespace Booru::DB::Entities
{

struct PostType : public Entity
{
    static auto constexpr Table  = "PostTypes";
    static auto constexpr LOGGER = "booru.db.entites.posttypes";

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
