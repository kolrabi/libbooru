#include "sql.hh"

namespace Booru
{
    int64_t SQLGetSchemaVersion()
    {
        return 1ll;
    }

    StringView SQLGetBaseSchema()
    {
        return R"SQL(
    -- -----------------------------------------------------------------------------

    PRAGMA foreign_keys = ON;

    -- table containing per database application preference variables
    DROP TABLE IF EXISTS Config;
    CREATE TABLE Config
    (
        Name        VARCHAR(32) PRIMARY KEY     NOT NULL,
        Value       TEXT                                    DEFAULT NULL
    );

    INSERT INTO Config VALUES("db.version", 0);


    -- post types
    DROP TABLE IF EXISTS PostTypes;
    CREATE TABLE IF NOT EXISTS PostTypes
    (
        Id              INTEGER     PRIMARY KEY   NOT NULL,
        Name            TEXT                      NOT NULL DEFAULT "",
        Description     TEXT                      NOT NULL DEFAULT ""
    );

    INSERT OR IGNORE INTO PostTypes (Name, Description) VALUES("image", "A simple image file. jpg/png/...");
    INSERT OR IGNORE INTO PostTypes (Name, Description) VALUES("animation", "An animated image file. Animated gif, apng, ...");
    INSERT OR IGNORE INTO PostTypes (Name, Description) VALUES("archive", "A file archive containing images. zip/rar/cbr/...");
    INSERT OR IGNORE INTO PostTypes (Name, Description) VALUES("video", "A video file, with or without audio. avi/mp4/webm/...");


    -- post information
    CREATE TABLE IF NOT EXISTS Posts
    (
        Id                  INTEGER     PRIMARY KEY     NOT NULL,
        MD5Sum              BLOB(16)                    NOT NULL,
        Flags               INTEGER                     NOT NULL DEFAULT 0,
            -- 1: flagged for deletion
        PostTypeId          INTEGER                     NOT NULL DEFAULT -1, -- new
        Rating              INTEGER                     NOT NULL DEFAULT 0,
            -- 0: unrated
            -- 1: general
            -- 2: sensitive
            -- 3: questionable
            -- 4: explicit
        Score               INTEGER                     NOT NULL DEFAULT 0,
        Height              INTEGER                     NOT NULL DEFAULT 0,
        Width               INTEGER                     NOT NULL DEFAULT 0,
        AddedTime           INTEGER                     NOT NULL DEFAULT 0,
        UpdatedTime         INTEGER                     NOT NULL DEFAULT 0,
        OriginalFilename    TEXT                        NOT NULL DEFAULT "",

        FOREIGN KEY     (PostTypeId)              REFERENCES      PostTypes(Id)    ON DELETE SET DEFAULT,

        UNIQUE          (MD5Sum)
    );

    CREATE INDEX IF NOT EXISTS I_Post_MD5Sum ON Posts(MD5Sum);



    -- file information for each post
    CREATE TABLE IF NOT EXISTS PostFiles
    (
        Id              INTEGER     PRIMARY KEY   NOT NULL,
        Path            TEXT                      NOT NULL,
        PostId          INTEGER                   NOT NULL,

        FOREIGN KEY     (PostId)                  REFERENCES      Posts(Id)         ON DELETE CASCADE,
        UNIQUE          (Path)
    );

    CREATE INDEX IF NOT EXISTS I_PostFiles_PostId ON PostFiles(PostId);
    CREATE INDEX IF NOT EXISTS I_PostFiles_Path ON PostFiles(Path);



    -- information about each tag type
    CREATE TABLE IF NOT EXISTS TagTypes
    (
        Id              INTEGER     PRIMARY KEY   NOT NULL,
        Name            TEXT                      NOT NULL,
        Description     TEXT                      NOT NULL,
        Color           INTEGER                   NOT NULL DEFAULT 0xFFFFFFFF,

        UNIQUE          (Name)
    );

    INSERT OR IGNORE INTO TagTypes (Name, Description) VALUES("Normal", "");
    INSERT OR IGNORE INTO TagTypes (Name, Description) VALUES("Copyright", "Series/movie/comic/etc... a post is inspired by");
    INSERT OR IGNORE INTO TagTypes (Name, Description) VALUES("Character", "A characater that is featured in the post");
    INSERT OR IGNORE INTO TagTypes (Name, Description) VALUES("Artist", "Creator of the post");
    INSERT OR IGNORE INTO TagTypes (Name, Description) VALUES("Meta", "Internal management tag");
    INSERT OR IGNORE INTO TagTypes (Name, Description) VALUES("Censorship", "Indicates what kind (if any) of censorship was applied in the post");
    INSERT OR IGNORE INTO TagTypes (Name, Description) VALUES("Pose", "The pose of a character in the post");
    INSERT OR IGNORE INTO TagTypes (Name, Description) VALUES("Location", "Describes the location a post depicts");
    INSERT OR IGNORE INTO TagTypes (Name, Description) VALUES("Clothing", "Describes the clothing present in the post");
    INSERT OR IGNORE INTO TagTypes (Name, Description) VALUES("Process", "Describes a process happening in the post");
    INSERT OR IGNORE INTO TagTypes (Name, Description) VALUES("Feature", "Describes a feature that is present in the post");

    -- tag information
    CREATE TABLE IF NOT EXISTS Tags
    (
        Id              INTEGER     PRIMARY KEY   NOT NULL,
        Name            TEXT                      NOT NULL DEFAULT "",
        Description     TEXT                      NOT NULL DEFAULT "",
        TagTypeId       INTEGER                   NOT NULL DEFAULT -1,
        Rating              INTEGER                     NOT NULL DEFAULT 0,
            -- 0: unrated
            -- 1: general
            -- 2: sensitive
            -- 3: questionable
            -- 4: explicit
        RedirectId      INTEGER                            DEFAULT NULL,
        Flags           INTEGER                   NOT NULL DEFAULT 0,
        -- Flags:
        -- 1: new
        -- 2: obsolete

        UNIQUE          (Name),
        FOREIGN KEY     (TagTypeId)              REFERENCES      TagTypes(Id) ON DELETE SET DEFAULT
    );


    -- tag implications, tags to add/remove when a tag is added
    CREATE TABLE IF NOT EXISTS TagImplications
    (
        Id              INTEGER     PRIMARY KEY   NOT NULL,
        TagId           INTEGER                   NOT NULL,
        ImpliedTagId    INTEGER                   NOT NULL,                       -- the implied tag
        Flags           INTEGER                     NOT NULL DEFAULT 0,
            -- 1: remove implied tag instead of adding it

        UNIQUE          (TagId, ImpliedTagId),
        FOREIGN KEY     (TagId)                   REFERENCES Tags(Id)               ON DELETE CASCADE,
        FOREIGN KEY     (ImpliedTagId)            REFERENCES Tags(Id)               ON DELETE CASCADE
    );



    -- associations between posts and tags
    CREATE TABLE IF NOT EXISTS PostTags
    (
        Id              INTEGER     PRIMARY KEY   NOT NULL,
        PostId          INTEGER                   NOT NULL,
        TagId           INTEGER                   NOT NULL,

        FOREIGN KEY     (PostId)                  REFERENCES Posts(Id)              ON DELETE CASCADE,
        FOREIGN KEY     (TagId)                   REFERENCES Tags(Id)               ON DELETE CASCADE,

        UNIQUE          (PostId, TagId)
    );



    -- site information for imports
    CREATE TABLE IF NOT EXISTS Sites
    (
        Id              INTEGER     PRIMARY KEY   NOT NULL,
        Name            VARCHAR(32)               NOT NULL,
        Description     TEXT                      NOT NULL DEFAULT ""

        -- TODO: additional stuff like api information
    );

    -- associations between posts and other booru ids
    CREATE TABLE IF NOT EXISTS PostSiteIds
    (
        Id              INTEGER     PRIMARY KEY   NOT NULL,
        PostId          INTEGER                   NOT NULL,
        SiteId          INTEGER                   NOT NULL,
        SitePostId      INTEGER                   NOT NULL,

        FOREIGN KEY     (PostId)                  REFERENCES Posts(Id)              ON DELETE CASCADE,
        FOREIGN KEY     (SiteId)                  REFERENCES Sites(Id)              ON DELETE CASCADE,

        UNIQUE          (PostId, SiteId, SitePostId)
    );


        )SQL"sv;
    }

    StringView SQLGetUpdateSchema( int64_t _Version )
    {
        switch( _Version )
        {
            case 0: return R"SQL(
                UPDATE CONFIG SET Value = 1 WHERE Name == "db.version";
            )SQL"sv;
        }
        return ""sv;
    }

}