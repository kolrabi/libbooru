#pragma once

#include <booru/db/entity.hh>

namespace Booru::DB::Entities
{

struct PostSiteId : public Entity<PostSiteId>
{
    static char const constexpr* Table = "PostSiteIds";
    static char const constexpr* LOGGER = "booru.db.entites.postsiteids";

    INTEGER PostId     = -1;
    INTEGER SiteId     = -1;
    INTEGER SitePostId = -1;

    template <class Visitor>
    ResultCode IterateProperties( Visitor & _Visitor )
    {
        ENTITY_PROPERTY_KEY( Id );
        ENTITY_PROPERTY( PostId );
        ENTITY_PROPERTY( SiteId );
        ENTITY_PROPERTY( SitePostId );
        return ResultCode::OK;
    }
};

} // namespace Booru::DB::Entities
