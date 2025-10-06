#pragma once

#include "booru/db/entity.hh"

namespace Booru::DB::Entities
{

struct Post : public Entity
{
    static char const constexpr* Table = "Posts";
    static char const constexpr* LOGGER = "booru.db.entites.posts";

    enum
    {
        FLAG_DELETEME = 1
    };

    MD5BLOB MD5Sum;
    INTEGER Flags       = 0;
    INTEGER PostTypeId  = -1;
    INTEGER Rating      = RATING_UNRATED;
    INTEGER Score       = 0;
    INTEGER Height      = 0;
    INTEGER Width       = 0;
    INTEGER AddedTime   = 0;
    INTEGER UpdatedTime = 0;
    TEXT OriginalFileName;

    template <class Visitor>
    ResultCode IterateProperties( Visitor& _Visitor )
    {
        ENTITY_PROPERTY_KEY( Id );
        ENTITY_PROPERTY( MD5Sum );
        ENTITY_PROPERTY( Flags );
        ENTITY_PROPERTY( PostTypeId );
        ENTITY_PROPERTY( Rating );
        ENTITY_PROPERTY( Score );
        ENTITY_PROPERTY( Height );
        ENTITY_PROPERTY( Width );
        ENTITY_PROPERTY( AddedTime );
        ENTITY_PROPERTY( UpdatedTime );
        ENTITY_PROPERTY( OriginalFileName );
        return ResultCode::OK;
    }
};

} // namespace Booru::DB::Entities
