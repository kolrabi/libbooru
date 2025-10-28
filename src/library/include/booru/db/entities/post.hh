#pragma once

#include <booru/db/entities.hh>

namespace Booru::DB::Entities
{

struct Post : public Entity
{
    static auto constexpr Table  = "Posts";
    static auto constexpr LOGGER = "booru.db.entites.posts";

    enum
    {
        FLAG_DELETEME = 1
    };

    MD5BLOB MD5Sum;
    INTEGER Flags      = 0;
    INTEGER PostTypeId = -1;
    TEXT MimeType      = "application/octet-stream";
    INTEGER Rating     = RATING_UNRATED;
    INTEGER Score      = 0;
    INTEGER Height     = 0;
    INTEGER Width      = 0;
    INTEGER AddedTime  = 0;

    template <class Visitor> ResultCode IterateProperties(Visitor& _Visitor)
    {
        ENTITY_PROPERTY_KEY(Id);
        ENTITY_PROPERTY(MD5Sum);
        ENTITY_PROPERTY(Flags);
        ENTITY_PROPERTY(PostTypeId);
        ENTITY_PROPERTY(MimeType);
        ENTITY_PROPERTY(Rating);
        ENTITY_PROPERTY(Score);
        ENTITY_PROPERTY(Height);
        ENTITY_PROPERTY(Width);
        ENTITY_PROPERTY(AddedTime);
        return ResultCode::OK;
    }
};

} // namespace Booru::DB::Entities
