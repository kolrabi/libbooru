#pragma once

#include <booru/db/entities.hh>

namespace Booru::DB::Entities
{

    struct Tag : public Entity<Tag>
    {
        static char const constexpr *Table = "Tags";
        static char const constexpr *LOGGER = "booru.db.entites.tags";

        enum
        {
            FLAG_NEW = 1,
            FLAG_OBSOLETE = 2,
        };

        TEXT Name;
        TEXT Description;
        INTEGER TagTypeId = -1;
        INTEGER Rating = RATING_UNRATED;
        NULLABLE<INTEGER> RedirectId;
        INTEGER Flags = FLAG_NEW;

        template <class Visitor>
        ResultCode IterateProperties(Visitor &_Visitor)
        {
            ENTITY_PROPERTY_KEY(Id);
            ENTITY_PROPERTY(Name);
            ENTITY_PROPERTY(Description);
            ENTITY_PROPERTY(TagTypeId);
            ENTITY_PROPERTY(Rating);
            ENTITY_PROPERTY(RedirectId);
            ENTITY_PROPERTY(Flags);
            return ResultCode::OK;
        }
    };

} // namespace Booru::DB::Entities
