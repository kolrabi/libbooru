#pragma once

#include <booru/db/entity.hh>
#include <booru/db/visitors.hh>

namespace Booru::DB::Entities
{
    // forward declaration of entities
    struct PostFile;
    struct PostSiteId;
    struct PostTag;
    struct PostType;
    struct Post;
    struct Site;
    struct TagImplication;
    struct TagType;
    struct Tag;

    // A post can be suited for different audiences, such as adults or children. The rating of a post can be used to determine whether it should be displayed.
    enum
    {
        RATING_UNRATED = 0,
        RATING_GENERAL = 1,
        RATING_SENSITIVE = 2,
        RATING_QUESTIONABLE = 3,
        RATING_EXPLICIT = 4
    };

    /// (Debug tool) Convert an entity to a printable string.
    template <class TEntity>
    static inline String EntityToString(TEntity const &_Item)
    {
        Visitors::ToStringPropertyVisitor Visitor;
        auto result = const_cast<TEntity &>(_Item).IterateProperties(Visitor);
        return Visitor.m_String;
    }
}