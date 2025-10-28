#pragma once

#include <booru/db/entities.hh>

namespace Booru::DB::Entities
{

struct PostFile : public Entity
{
    static auto constexpr Table  = "PostFiles";
    static auto constexpr LOGGER = "booru.db.entites.postfiles";

    INTEGER PostId               = -1;
    INTEGER SiteId               = 1; // file
    TEXT Path;

    template <class Visitor> ResultCode IterateProperties(Visitor& _Visitor)
    {
        ENTITY_PROPERTY_KEY(Id);
        ENTITY_PROPERTY(PostId);
        ENTITY_PROPERTY(SiteId);
        ENTITY_PROPERTY(PostId);
        return ResultCode::OK;
    }
};

} // namespace Booru::DB::Entities
