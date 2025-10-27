#pragma once

#include <booru/db/entities.hh>

namespace Booru::DB::Entities
{

struct PostFile : public Entity
{
    static auto constexpr Table  = "PostFiles";
    static auto constexpr LOGGER = "booru.db.entites.postfiles";

    TEXT Path;
    INTEGER PostId = -1;

    template <class Visitor> ResultCode IterateProperties(Visitor& _Visitor)
    {
        ENTITY_PROPERTY_KEY(Id);
        ENTITY_PROPERTY(Path);
        ENTITY_PROPERTY(PostId);
        return ResultCode::OK;
    }
};

} // namespace Booru::DB::Entities
