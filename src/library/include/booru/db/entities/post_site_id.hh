#pragma once

#include <booru/db/entities.hh>

namespace Booru::DB::Entities
{

struct PostSiteId : public Entity
{
    static auto constexpr Table  = "PostSiteIds";
    static auto constexpr LOGGER = "booru.db.entites.postsiteids";

    INTEGER PostId               = -1;
    INTEGER SiteId               = -1;
    INTEGER SitePostId           = -1;

    template <class Visitor> ResultCode IterateProperties(Visitor& _Visitor)
    {
        ENTITY_PROPERTY_KEY(Id);
        ENTITY_PROPERTY(PostId);
        ENTITY_PROPERTY(SiteId);
        ENTITY_PROPERTY(SitePostId);
        return ResultCode::OK;
    }
};

} // namespace Booru::DB::Entities
