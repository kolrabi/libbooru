#pragma once

#include <booru/db/entities/tag.hh>

static bool operator==(Booru::DB::Entities::Tag const &_A, Booru::DB::Entities::Tag const & _B) 
{
    return 
        _A.Id == _B.Id &&
        _A.Name == _B.Name &&
        _A.Description == _B.Description &&
        _A.Rating == _B.Rating &&
        _A.TagTypeId == _B.TagTypeId &&
        _A.RedirectId == _B.RedirectId;
}
