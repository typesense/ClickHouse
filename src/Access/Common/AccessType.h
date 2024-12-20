#pragma once

#include <Common/Exception.h>
#include <base/types.h>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>

namespace DB
{

namespace ErrorCodes
{
    extern const int LOGICAL_ERROR;
}

/// This namespace defines objects which can be used as a parameter in global_with_parameter access types.
/// Each such object should be an enum and have a APPLY_FOR_X(M) macro.
/// Macro M should be defined as M(name, aliases).
namespace AccessTypeObjects
{

enum class Source : uint8_t
{
#define APPLY_FOR_SOURCE(M) \
    M(FILE, "File") \
    M(URL, "") \
    M(REMOTE, "Distributed") \
    M(MONGO, "MongoDB") \
    M(REDIS, "Redis") \
    M(MYSQL, "MySQL") \
    M(POSTGRES, "PostgreSQL") \
    M(SQLITE, "SQLite") \
    M(ODBC, "") \
    M(JDBC, "") \
    M(HDFS, "") \
    M(S3, "") \
    M(HIVE, "Hive") \
    M(AZURE, "AzureBlobStorage") \
    M(KAFKA, "Kafka") \
    M(NATS, "") \
    M(RABBITMQ, "RabbitMQ")

#define DECLARE_ACCESS_TYPE_OBJECTS_ENUM_CONST(name, aliases) name,

    APPLY_FOR_SOURCE(DECLARE_ACCESS_TYPE_OBJECTS_ENUM_CONST)
#undef DECLARE_ACCESS_TYPE_OBJECTS_ENUM_CONST
};


#define ACCESS_TYPE_OBJECT_ADD_TO_MAPPING(name, aliases) \
        addToMapping(Source::name, #name); \
        addAliases(Source::name, aliases);

#define ENUM_ACCESS_OBJECT(NAME, M) \
class EnumHolder##NAME \
{ \
public: \
    static const EnumHolder##NAME & instance() \
    { \
        static const EnumHolder##NAME res; \
        return res; \
    } \
    \
    std::string_view toString(NAME type) const { return entity_enum_to_string[static_cast<size_t>(type)]; } \
    \
    NAME fromString(const String & name) const \
    { \
        if (auto it = aliases.find(name); it != aliases.end()) \
            return it->second; \
        \
        throw Exception(ErrorCodes::LOGICAL_ERROR, "Unable to find AccessTypeObject {} by name {}", #NAME, name); \
    } \
    \
    bool validate(const String & name) const { return aliases.find(name) != aliases.end(); } \
    \
    const std::map<String, NAME> & getAliasesMap() const { return aliases; } \
    \
private: \
    EnumHolder##NAME() \
    { \
        M(ACCESS_TYPE_OBJECT_ADD_TO_MAPPING) \
    } \
    \
    void addToMapping(NAME type, std::string_view str) \
    { \
        String str2{str}; \
        size_t index = static_cast<size_t>(type); \
        entity_enum_to_string.resize(std::max(index + 1, entity_enum_to_string. size())); \
        entity_enum_to_string[index] = str2; \
    } \
    \
    void addAliases(NAME type, std::string_view str) \
    { \
        String str2{str}; \
        std::vector<String> type_aliases; \
        \
        boost::split(type_aliases, str2, [](char c) { return c == ','; }); \
        for (auto & alias : type_aliases) \
        { \
            boost::trim(alias); \
            aliases[alias] = type; \
        } \
        \
        aliases[String{toString(type)}] = type; \
    } \
    \
    std::vector<String> entity_enum_to_string; \
    std::map<String, NAME> aliases; \
}; \
\
inline auto toString##NAME(NAME type) \
{ \
    return EnumHolder##NAME::instance().toString(type); \
} \
\
inline auto fromString##NAME(const String & name) \
{ \
    return EnumHolder##NAME::instance().fromString(name); \
} \
inline auto unify##NAME(const String & name) \
{ \
    return toString##NAME(fromString##NAME(name)); \
} \
\
inline auto & getAliasesMap##NAME() \
{ \
    return EnumHolder##NAME::instance().getAliasesMap(); \
} \
\
inline auto validate##NAME(const String & name) \
{ \
    return EnumHolder##NAME::instance().validate(name); \
}

ENUM_ACCESS_OBJECT(Source, APPLY_FOR_SOURCE)

#undef ENUM_ACCESS_OBJECT
#undef ACCESS_TYPE_OBJECT_ADD_TO_MAPPING
} // namespace AccessTypeObjects


/// Represents an access type which can be granted on databases, tables, columns, etc.
enum class AccessType : uint8_t
{
/// Macro M should be defined as M(name, aliases, node_type, parent_group_name)
/// where name is identifier with underscores (instead of spaces);
/// aliases is a string containing comma-separated list;
/// node_type either specifies access type's level (GLOBAL/NAMED_COLLECTION/USER_NAME/SOURCE/DATABASE/TABLE/DICTIONARY/VIEW/COLUMNS),
/// or specifies that the access type is a GROUP of other access types;
/// parent_group_name is the name of the group containing this access type (or NONE if there is no such group).
/// NOTE A parent group must be declared AFTER all its children.
#define APPLY_FOR_ACCESS_TYPES(M) \
    M(SHOW_DATABASES, "", DATABASE, SHOW) /* allows to execute SHOW DATABASES, SHOW CREATE DATABASE, USE <database>;
                                             implicitly enabled by any grant on the database */\
    M(SHOW_TABLES, "", TABLE, SHOW) /* allows to execute SHOW TABLES, EXISTS <table>, CHECK <table>;
                                       implicitly enabled by any grant on the table */\
    M(SHOW_COLUMNS, "", COLUMN, SHOW) /* allows to execute SHOW CREATE TABLE, DESCRIBE;
                                         implicitly enabled with any grant on the column */\
    M(SHOW_DICTIONARIES, "", DICTIONARY, SHOW) /* allows to execute SHOW DICTIONARIES, SHOW CREATE DICTIONARY, EXISTS <dictionary>;
                                                  implicitly enabled by any grant on the dictionary */\
    M(SHOW, "", GROUP, ALL) /* allows to execute SHOW, USE, EXISTS, CHECK, DESCRIBE */\
    M(SHOW_FILESYSTEM_CACHES, "", GROUP, ALL) \
    \
    M(SELECT, "", COLUMN, ALL) \
    M(INSERT, "", COLUMN, ALL) \
    M(ALTER_UPDATE, "UPDATE", COLUMN, ALTER_TABLE) /* allows to execute ALTER UPDATE */\
    M(ALTER_DELETE, "DELETE", COLUMN, ALTER_TABLE) /* allows to execute ALTER DELETE */\
    \
    M(ALTER_ADD_COLUMN, "ADD COLUMN", COLUMN, ALTER_COLUMN) \
    M(ALTER_MODIFY_COLUMN, "MODIFY COLUMN", COLUMN, ALTER_COLUMN) \
    M(ALTER_DROP_COLUMN, "DROP COLUMN", COLUMN, ALTER_COLUMN) \
    M(ALTER_COMMENT_COLUMN, "COMMENT COLUMN", COLUMN, ALTER_COLUMN) \
    M(ALTER_CLEAR_COLUMN, "CLEAR COLUMN", COLUMN, ALTER_COLUMN) \
    M(ALTER_RENAME_COLUMN, "RENAME COLUMN", COLUMN, ALTER_COLUMN) \
    M(ALTER_MATERIALIZE_COLUMN, "MATERIALIZE COLUMN", COLUMN, ALTER_COLUMN) \
    M(ALTER_COLUMN, "", GROUP, ALTER_TABLE) /* allow to execute ALTER {ADD|DROP|MODIFY...} COLUMN */\
    M(ALTER_MODIFY_COMMENT, "MODIFY COMMENT", TABLE, ALTER_TABLE) /* modify table comment */\
    \
    M(ALTER_ORDER_BY, "ALTER MODIFY ORDER BY, MODIFY ORDER BY", TABLE, ALTER_INDEX) \
    M(ALTER_SAMPLE_BY, "ALTER MODIFY SAMPLE BY, MODIFY SAMPLE BY", TABLE, ALTER_INDEX) \
    M(ALTER_ADD_INDEX, "ADD INDEX", TABLE, ALTER_INDEX) \
    M(ALTER_DROP_INDEX, "DROP INDEX", TABLE, ALTER_INDEX) \
    M(ALTER_MATERIALIZE_INDEX, "MATERIALIZE INDEX", TABLE, ALTER_INDEX) \
    M(ALTER_CLEAR_INDEX, "CLEAR INDEX", TABLE, ALTER_INDEX) \
    M(ALTER_INDEX, "INDEX", GROUP, ALTER_TABLE) /* allows to execute ALTER ORDER BY or ALTER {ADD|DROP...} INDEX */\
    \
    M(ALTER_ADD_STATISTICS, "ALTER ADD STATISTIC", TABLE, ALTER_STATISTICS) \
    M(ALTER_DROP_STATISTICS, "ALTER DROP STATISTIC", TABLE, ALTER_STATISTICS) \
    M(ALTER_MODIFY_STATISTICS, "ALTER MODIFY STATISTIC", TABLE, ALTER_STATISTICS) \
    M(ALTER_MATERIALIZE_STATISTICS, "ALTER MATERIALIZE STATISTIC", TABLE, ALTER_STATISTICS) \
    M(ALTER_STATISTICS, "STATISTIC", GROUP, ALTER_TABLE) /* allows to execute ALTER STATISTIC */\
    \
    M(ALTER_ADD_PROJECTION, "ADD PROJECTION", TABLE, ALTER_PROJECTION) \
    M(ALTER_DROP_PROJECTION, "DROP PROJECTION", TABLE, ALTER_PROJECTION) \
    M(ALTER_MATERIALIZE_PROJECTION, "MATERIALIZE PROJECTION", TABLE, ALTER_PROJECTION) \
    M(ALTER_CLEAR_PROJECTION, "CLEAR PROJECTION", TABLE, ALTER_PROJECTION) \
    M(ALTER_PROJECTION, "PROJECTION", GROUP, ALTER_TABLE) /* allows to execute ALTER ORDER BY or ALTER {ADD|DROP...} PROJECTION */\
    \
    M(ALTER_ADD_CONSTRAINT, "ADD CONSTRAINT", TABLE, ALTER_CONSTRAINT) \
    M(ALTER_DROP_CONSTRAINT, "DROP CONSTRAINT", TABLE, ALTER_CONSTRAINT) \
    M(ALTER_CONSTRAINT, "CONSTRAINT", GROUP, ALTER_TABLE) /* allows to execute ALTER {ADD|DROP} CONSTRAINT */\
    \
    M(ALTER_TTL, "ALTER MODIFY TTL, MODIFY TTL", TABLE, ALTER_TABLE) /* allows to execute ALTER MODIFY TTL */\
    M(ALTER_MATERIALIZE_TTL, "MATERIALIZE TTL", TABLE, ALTER_TABLE) /* allows to execute ALTER MATERIALIZE TTL;
                                                                       enabled implicitly by the grant ALTER_TABLE */\
    M(ALTER_SETTINGS, "ALTER SETTING, ALTER MODIFY SETTING, MODIFY SETTING, RESET SETTING", TABLE, ALTER_TABLE) /* allows to execute ALTER MODIFY SETTING */\
    M(ALTER_MOVE_PARTITION, "ALTER MOVE PART, MOVE PARTITION, MOVE PART", TABLE, ALTER_TABLE) \
    M(ALTER_FETCH_PARTITION, "ALTER FETCH PART, FETCH PARTITION", TABLE, ALTER_TABLE) \
    M(ALTER_FREEZE_PARTITION, "FREEZE PARTITION, UNFREEZE", TABLE, ALTER_TABLE) \
    \
    M(ALTER_DATABASE_SETTINGS, "ALTER DATABASE SETTING, ALTER MODIFY DATABASE SETTING, MODIFY DATABASE SETTING", DATABASE, ALTER_DATABASE) /* allows to execute ALTER MODIFY SETTING */\
    M(ALTER_NAMED_COLLECTION, "", NAMED_COLLECTION, NAMED_COLLECTION_ADMIN) /* allows to execute ALTER NAMED COLLECTION */\
    \
    M(ALTER_TABLE, "", GROUP, ALTER) \
    M(ALTER_DATABASE, "", GROUP, ALTER) \
    \
    M(ALTER_VIEW_MODIFY_QUERY, "ALTER TABLE MODIFY QUERY", VIEW, ALTER_VIEW) \
    M(ALTER_VIEW_MODIFY_REFRESH, "ALTER TABLE MODIFY QUERY", VIEW, ALTER_VIEW) \
    M(ALTER_VIEW_MODIFY_SQL_SECURITY, "ALTER TABLE MODIFY SQL SECURITY", VIEW, ALTER_VIEW) \
    M(ALTER_VIEW, "", GROUP, ALTER) /* allows to execute ALTER VIEW REFRESH, ALTER VIEW MODIFY QUERY, ALTER VIEW MODIFY REFRESH;
                                       implicitly enabled by the grant ALTER_TABLE */\
    \
    M(ALTER, "", GROUP, ALL) /* allows to execute ALTER TABLE */\
    \
    M(CREATE_DATABASE, "", DATABASE, CREATE) /* allows to execute {CREATE|ATTACH} DATABASE */\
    M(CREATE_TABLE, "", TABLE, CREATE) /* allows to execute {CREATE|ATTACH} {TABLE|VIEW} */\
    M(CREATE_VIEW, "", VIEW, CREATE) /* allows to execute {CREATE|ATTACH} VIEW;
                                        implicitly enabled by the grant CREATE_TABLE */\
    M(CREATE_DICTIONARY, "", DICTIONARY, CREATE) /* allows to execute {CREATE|ATTACH} DICTIONARY */\
    M(CREATE_TEMPORARY_TABLE, "", GLOBAL, CREATE_ARBITRARY_TEMPORARY_TABLE) /* allows to create and manipulate temporary tables;
                                                     implicitly enabled by the grant CREATE_TABLE on any table */ \
    M(CREATE_ARBITRARY_TEMPORARY_TABLE, "", GLOBAL, CREATE)  /* allows to create  and manipulate temporary tables
                                                                with arbitrary table engine */\
    M(CREATE_FUNCTION, "", GLOBAL, CREATE) /* allows to execute CREATE FUNCTION */ \
    M(CREATE_WORKLOAD, "", GLOBAL, CREATE) /* allows to execute CREATE WORKLOAD */ \
    M(CREATE_RESOURCE, "", GLOBAL, CREATE) /* allows to execute CREATE RESOURCE */ \
    M(CREATE_NAMED_COLLECTION, "", NAMED_COLLECTION, NAMED_COLLECTION_ADMIN) /* allows to execute CREATE NAMED COLLECTION */ \
    M(CREATE, "", GROUP, ALL) /* allows to execute {CREATE|ATTACH} */ \
    \
    M(DROP_DATABASE, "", DATABASE, DROP) /* allows to execute {DROP|DETACH|TRUNCATE} DATABASE */\
    M(DROP_TABLE, "", TABLE, DROP) /* allows to execute {DROP|DETACH} TABLE */\
    M(DROP_VIEW, "", VIEW, DROP) /* allows to execute {DROP|DETACH} TABLE for views;
                                    implicitly enabled by the grant DROP_TABLE */\
    M(DROP_DICTIONARY, "", DICTIONARY, DROP) /* allows to execute {DROP|DETACH} DICTIONARY */\
    M(DROP_FUNCTION, "", GLOBAL, DROP) /* allows to execute DROP FUNCTION */\
    M(DROP_WORKLOAD, "", GLOBAL, DROP) /* allows to execute DROP WORKLOAD */\
    M(DROP_RESOURCE, "", GLOBAL, DROP) /* allows to execute DROP RESOURCE */\
    M(DROP_NAMED_COLLECTION, "", NAMED_COLLECTION, NAMED_COLLECTION_ADMIN) /* allows to execute DROP NAMED COLLECTION */\
    M(DROP, "", GROUP, ALL) /* allows to execute {DROP|DETACH} */\
    \
    M(UNDROP_TABLE, "", TABLE, ALL) /* allows to execute {UNDROP} TABLE */\
    \
    M(TRUNCATE, "TRUNCATE TABLE", TABLE, ALL) \
    M(OPTIMIZE, "OPTIMIZE TABLE", TABLE, ALL) \
    M(BACKUP, "", TABLE, ALL) /* allows to backup tables */\
    \
    M(KILL_QUERY, "", GLOBAL, ALL) /* allows to kill a query started by another user
                                      (anyone can kill his own queries) */\
    M(KILL_TRANSACTION, "", GLOBAL, ALL) \
    \
    M(MOVE_PARTITION_BETWEEN_SHARDS, "", GLOBAL, ALL) /* required to be able to move a part/partition to a table
                                                         identified by its ZooKeeper path */\
    \
    M(CREATE_USER, "", USER_NAME, ACCESS_MANAGEMENT) \
    M(ALTER_USER, "", USER_NAME, ACCESS_MANAGEMENT) \
    M(DROP_USER, "", USER_NAME, ACCESS_MANAGEMENT) \
    M(CREATE_ROLE, "", USER_NAME, ACCESS_MANAGEMENT) \
    M(ALTER_ROLE, "", USER_NAME, ACCESS_MANAGEMENT) \
    M(DROP_ROLE, "", USER_NAME, ACCESS_MANAGEMENT) \
    M(ROLE_ADMIN, "", GLOBAL, ACCESS_MANAGEMENT) /* allows to grant and revoke the roles which are not granted to the current user with admin option */\
    M(CREATE_ROW_POLICY, "CREATE POLICY", TABLE, ACCESS_MANAGEMENT) \
    M(ALTER_ROW_POLICY, "ALTER POLICY", TABLE, ACCESS_MANAGEMENT) \
    M(DROP_ROW_POLICY, "DROP POLICY", TABLE, ACCESS_MANAGEMENT) \
    M(CREATE_QUOTA, "", GLOBAL, ACCESS_MANAGEMENT) \
    M(ALTER_QUOTA, "", GLOBAL, ACCESS_MANAGEMENT) \
    M(DROP_QUOTA, "", GLOBAL, ACCESS_MANAGEMENT) \
    M(CREATE_SETTINGS_PROFILE, "CREATE PROFILE", GLOBAL, ACCESS_MANAGEMENT) \
    M(ALTER_SETTINGS_PROFILE, "ALTER PROFILE", GLOBAL, ACCESS_MANAGEMENT) \
    M(DROP_SETTINGS_PROFILE, "DROP PROFILE", GLOBAL, ACCESS_MANAGEMENT) \
    M(ALLOW_SQL_SECURITY_NONE, "CREATE SQL SECURITY NONE, ALLOW SQL SECURITY NONE, SQL SECURITY NONE, SECURITY NONE", GLOBAL, ACCESS_MANAGEMENT) \
    M(SHOW_USERS, "SHOW CREATE USER", GLOBAL, SHOW_ACCESS) \
    M(SHOW_ROLES, "SHOW CREATE ROLE", GLOBAL, SHOW_ACCESS) \
    M(SHOW_ROW_POLICIES, "SHOW POLICIES, SHOW CREATE ROW POLICY, SHOW CREATE POLICY", TABLE, SHOW_ACCESS) \
    M(SHOW_QUOTAS, "SHOW CREATE QUOTA", GLOBAL, SHOW_ACCESS) \
    M(SHOW_SETTINGS_PROFILES, "SHOW PROFILES, SHOW CREATE SETTINGS PROFILE, SHOW CREATE PROFILE", GLOBAL, SHOW_ACCESS) \
    M(SHOW_ACCESS, "", GROUP, ACCESS_MANAGEMENT) \
    M(ACCESS_MANAGEMENT, "", GROUP, ALL) \
    M(SHOW_NAMED_COLLECTIONS, "SHOW NAMED COLLECTIONS", NAMED_COLLECTION, NAMED_COLLECTION_ADMIN) \
    M(SHOW_NAMED_COLLECTIONS_SECRETS, "SHOW NAMED COLLECTIONS SECRETS", NAMED_COLLECTION, NAMED_COLLECTION_ADMIN) \
    M(NAMED_COLLECTION, "NAMED COLLECTION USAGE, USE NAMED COLLECTION", NAMED_COLLECTION, NAMED_COLLECTION_ADMIN) \
    M(NAMED_COLLECTION_ADMIN, "NAMED COLLECTION CONTROL", NAMED_COLLECTION, ALL) \
    M(SET_DEFINER, "", USER_NAME, ALL) \
    \
    M(TABLE_ENGINE, "TABLE ENGINE", TABLE_ENGINE, ALL) \
    \
    M(SYSTEM_SHUTDOWN, "SYSTEM KILL, SHUTDOWN", GLOBAL, SYSTEM) \
    M(SYSTEM_DROP_DNS_CACHE, "SYSTEM DROP DNS, DROP DNS CACHE, DROP DNS", GLOBAL, SYSTEM_DROP_CACHE)  \
    M(SYSTEM_DROP_CONNECTIONS_CACHE, "SYSTEM DROP CONNECTIONS CACHE, DROP CONNECTIONS CACHE", GLOBAL, SYSTEM_DROP_CACHE)  \
    M(SYSTEM_PREWARM_MARK_CACHE, "SYSTEM PREWARM MARK, PREWARM MARK CACHE, PREWARM MARKS", GLOBAL, SYSTEM_DROP_CACHE) \
    M(SYSTEM_DROP_MARK_CACHE, "SYSTEM DROP MARK, DROP MARK CACHE, DROP MARKS", GLOBAL, SYSTEM_DROP_CACHE) \
    M(SYSTEM_PREWARM_PRIMARY_INDEX_CACHE, "SYSTEM PREWARM PRIMARY INDEX, PREWARM PRIMARY INDEX CACHE, PREWARM PRIMARY INDEX", GLOBAL, SYSTEM_DROP_CACHE) \
    M(SYSTEM_DROP_PRIMARY_INDEX_CACHE, "SYSTEM DROP PRIMARY INDEX, DROP PRIMARY INDEX CACHE, DROP PRIMARY INDEX", GLOBAL, SYSTEM_DROP_CACHE) \
    M(SYSTEM_DROP_UNCOMPRESSED_CACHE, "SYSTEM DROP UNCOMPRESSED, DROP UNCOMPRESSED CACHE, DROP UNCOMPRESSED", GLOBAL, SYSTEM_DROP_CACHE) \
    M(SYSTEM_DROP_MMAP_CACHE, "SYSTEM DROP MMAP, DROP MMAP CACHE, DROP MMAP", GLOBAL, SYSTEM_DROP_CACHE) \
    M(SYSTEM_DROP_QUERY_CACHE, "SYSTEM DROP QUERY, DROP QUERY CACHE, DROP QUERY", GLOBAL, SYSTEM_DROP_CACHE) \
    M(SYSTEM_DROP_COMPILED_EXPRESSION_CACHE, "SYSTEM DROP COMPILED EXPRESSION, DROP COMPILED EXPRESSION CACHE, DROP COMPILED EXPRESSIONS", GLOBAL, SYSTEM_DROP_CACHE) \
    M(SYSTEM_DROP_FILESYSTEM_CACHE, "SYSTEM DROP FILESYSTEM CACHE, DROP FILESYSTEM CACHE", GLOBAL, SYSTEM_DROP_CACHE) \
    M(SYSTEM_DROP_DISTRIBUTED_CACHE, "SYSTEM DROP DISTRIBUTED CACHE, DROP DISTRIBUTED CACHE", GLOBAL, SYSTEM_DROP_CACHE) \
    M(SYSTEM_SYNC_FILESYSTEM_CACHE, "SYSTEM REPAIR FILESYSTEM CACHE, REPAIR FILESYSTEM CACHE, SYNC FILESYSTEM CACHE", GLOBAL, SYSTEM) \
    M(SYSTEM_DROP_PAGE_CACHE, "SYSTEM DROP PAGE CACHE, DROP PAGE CACHE", GLOBAL, SYSTEM_DROP_CACHE) \
    M(SYSTEM_DROP_SCHEMA_CACHE, "SYSTEM DROP SCHEMA CACHE, DROP SCHEMA CACHE", GLOBAL, SYSTEM_DROP_CACHE) \
    M(SYSTEM_DROP_FORMAT_SCHEMA_CACHE, "SYSTEM DROP FORMAT SCHEMA CACHE, DROP FORMAT SCHEMA CACHE", GLOBAL, SYSTEM_DROP_CACHE) \
    M(SYSTEM_DROP_S3_CLIENT_CACHE, "SYSTEM DROP S3 CLIENT, DROP S3 CLIENT CACHE", GLOBAL, SYSTEM_DROP_CACHE) \
    M(SYSTEM_DROP_CACHE, "DROP CACHE", GROUP, SYSTEM) \
    M(SYSTEM_RELOAD_CONFIG, "RELOAD CONFIG", GLOBAL, SYSTEM_RELOAD) \
    M(SYSTEM_RELOAD_USERS, "RELOAD USERS", GLOBAL, SYSTEM_RELOAD) \
    M(SYSTEM_RELOAD_DICTIONARY, "SYSTEM RELOAD DICTIONARIES, RELOAD DICTIONARY, RELOAD DICTIONARIES", GLOBAL, SYSTEM_RELOAD) \
    M(SYSTEM_RELOAD_MODEL, "SYSTEM RELOAD MODELS, RELOAD MODEL, RELOAD MODELS", GLOBAL, SYSTEM_RELOAD) \
    M(SYSTEM_RELOAD_FUNCTION, "SYSTEM RELOAD FUNCTIONS, RELOAD FUNCTION, RELOAD FUNCTIONS", GLOBAL, SYSTEM_RELOAD) \
    M(SYSTEM_RELOAD_EMBEDDED_DICTIONARIES, "RELOAD EMBEDDED DICTIONARIES", GLOBAL, SYSTEM_RELOAD) /* implicitly enabled by the grant SYSTEM_RELOAD_DICTIONARY ON *.* */\
    M(SYSTEM_RELOAD_ASYNCHRONOUS_METRICS, "RELOAD ASYNCHRONOUS METRICS", GLOBAL, SYSTEM_RELOAD) \
    M(SYSTEM_RELOAD, "", GROUP, SYSTEM) \
    M(SYSTEM_RESTART_DISK, "SYSTEM RESTART DISK", GLOBAL, SYSTEM) \
    M(SYSTEM_MERGES, "SYSTEM STOP MERGES, SYSTEM START MERGES, STOP MERGES, START MERGES", TABLE, SYSTEM) \
    M(SYSTEM_TTL_MERGES, "SYSTEM STOP TTL MERGES, SYSTEM START TTL MERGES, STOP TTL MERGES, START TTL MERGES", TABLE, SYSTEM) \
    M(SYSTEM_FETCHES, "SYSTEM STOP FETCHES, SYSTEM START FETCHES, STOP FETCHES, START FETCHES", TABLE, SYSTEM) \
    M(SYSTEM_MOVES, "SYSTEM STOP MOVES, SYSTEM START MOVES, STOP MOVES, START MOVES", TABLE, SYSTEM) \
    M(SYSTEM_PULLING_REPLICATION_LOG, "SYSTEM STOP PULLING REPLICATION LOG, SYSTEM START PULLING REPLICATION LOG", TABLE, SYSTEM) \
    M(SYSTEM_CLEANUP, "SYSTEM STOP CLEANUP, SYSTEM START CLEANUP", TABLE, SYSTEM) \
    M(SYSTEM_VIEWS, "SYSTEM REFRESH VIEW, SYSTEM START VIEWS, SYSTEM STOP VIEWS, SYSTEM START VIEW, SYSTEM STOP VIEW, SYSTEM CANCEL VIEW, REFRESH VIEW, START VIEWS, STOP VIEWS, START VIEW, STOP VIEW, CANCEL VIEW", VIEW, SYSTEM) \
    M(SYSTEM_DISTRIBUTED_SENDS, "SYSTEM STOP DISTRIBUTED SENDS, SYSTEM START DISTRIBUTED SENDS, STOP DISTRIBUTED SENDS, START DISTRIBUTED SENDS", TABLE, SYSTEM_SENDS) \
    M(SYSTEM_REPLICATED_SENDS, "SYSTEM STOP REPLICATED SENDS, SYSTEM START REPLICATED SENDS, STOP REPLICATED SENDS, START REPLICATED SENDS", TABLE, SYSTEM_SENDS) \
    M(SYSTEM_SENDS, "SYSTEM STOP SENDS, SYSTEM START SENDS, STOP SENDS, START SENDS", GROUP, SYSTEM) \
    M(SYSTEM_REPLICATION_QUEUES, "SYSTEM STOP REPLICATION QUEUES, SYSTEM START REPLICATION QUEUES, STOP REPLICATION QUEUES, START REPLICATION QUEUES", TABLE, SYSTEM) \
    M(SYSTEM_VIRTUAL_PARTS_UPDATE, "SYSTEM STOP VIRTUAL PARTS UPDATE, SYSTEM START VIRTUAL PARTS UPDATE, STOP VIRTUAL PARTS UPDATE, START VIRTUAL PARTS UPDATE", TABLE, SYSTEM) \
    M(SYSTEM_REDUCE_BLOCKING_PARTS, "SYSTEM STOP REDUCE BLOCKING PARTS, SYSTEM START REDUCE BLOCKING PARTS, STOP REDUCE BLOCKING PARTS, START REDUCE BLOCKING PARTS", TABLE, SYSTEM) \
    M(SYSTEM_DROP_REPLICA, "DROP REPLICA", TABLE, SYSTEM) \
    M(SYSTEM_SYNC_REPLICA, "SYNC REPLICA", TABLE, SYSTEM) \
    M(SYSTEM_REPLICA_READINESS, "SYSTEM REPLICA READY, SYSTEM REPLICA UNREADY", GLOBAL, SYSTEM) \
    M(SYSTEM_RESTART_REPLICA, "RESTART REPLICA", TABLE, SYSTEM) \
    M(SYSTEM_RESTORE_REPLICA, "RESTORE REPLICA", TABLE, SYSTEM) \
    M(SYSTEM_WAIT_LOADING_PARTS, "WAIT LOADING PARTS", TABLE, SYSTEM) \
    M(SYSTEM_SYNC_DATABASE_REPLICA, "SYNC DATABASE REPLICA", DATABASE, SYSTEM) \
    M(SYSTEM_SYNC_TRANSACTION_LOG, "SYNC TRANSACTION LOG", GLOBAL, SYSTEM) \
    M(SYSTEM_SYNC_FILE_CACHE, "SYNC FILE CACHE", GLOBAL, SYSTEM) \
    M(SYSTEM_FLUSH_DISTRIBUTED, "FLUSH DISTRIBUTED", TABLE, SYSTEM_FLUSH) \
    M(SYSTEM_FLUSH_LOGS, "FLUSH LOGS", GLOBAL, SYSTEM_FLUSH) \
    M(SYSTEM_FLUSH_ASYNC_INSERT_QUEUE, "FLUSH ASYNC INSERT QUEUE", GLOBAL, SYSTEM_FLUSH) \
    M(SYSTEM_FLUSH, "", GROUP, SYSTEM) \
    M(SYSTEM_THREAD_FUZZER, "SYSTEM START THREAD FUZZER, SYSTEM STOP THREAD FUZZER, START THREAD FUZZER, STOP THREAD FUZZER", GLOBAL, SYSTEM) \
    M(SYSTEM_UNFREEZE, "SYSTEM UNFREEZE", GLOBAL, SYSTEM) \
    M(SYSTEM_FAILPOINT, "SYSTEM ENABLE FAILPOINT, SYSTEM DISABLE FAILPOINT, SYSTEM WAIT FAILPOINT", GLOBAL, SYSTEM) \
    M(SYSTEM_LISTEN, "SYSTEM START LISTEN, SYSTEM STOP LISTEN", GLOBAL, SYSTEM) \
    M(SYSTEM_JEMALLOC, "SYSTEM JEMALLOC PURGE, SYSTEM JEMALLOC ENABLE PROFILE, SYSTEM JEMALLOC DISABLE PROFILE, SYSTEM JEMALLOC FLUSH PROFILE", GLOBAL, SYSTEM) \
    M(SYSTEM_LOAD_PRIMARY_KEY, "SYSTEM LOAD PRIMARY KEY", TABLE, SYSTEM) \
    M(SYSTEM_UNLOAD_PRIMARY_KEY, "SYSTEM UNLOAD PRIMARY KEY", TABLE, SYSTEM) \
    M(SYSTEM, "", GROUP, ALL) /* allows to execute SYSTEM {SHUTDOWN|RELOAD CONFIG|...} */ \
    \
    M(dictGet, "dictHas, dictGetHierarchy, dictIsIn", DICTIONARY, ALL) /* allows to execute functions dictGet(), dictHas(), dictGetHierarchy(), dictIsIn() */\
    M(displaySecretsInShowAndSelect, "", GLOBAL, ALL) /* allows to show plaintext secrets in SELECT and SHOW queries. display_secrets_in_show_and_select format and server settings must be turned on */\
    \
    M(addressToLine, "", GLOBAL, INTROSPECTION) /* allows to execute function addressToLine() */\
    M(addressToLineWithInlines, "", GLOBAL, INTROSPECTION) /* allows to execute function addressToLineWithInlines() */\
    M(addressToSymbol, "", GLOBAL, INTROSPECTION) /* allows to execute function addressToSymbol() */\
    M(demangle, "", GLOBAL, INTROSPECTION) /* allows to execute function demangle() */\
    M(INTROSPECTION, "INTROSPECTION FUNCTIONS", GROUP, ALL) /* allows to execute functions addressToLine(), addressToSymbol(), demangle()*/\
    \
    M(SOURCE_READ, "READ", SOURCE, SOURCES) \
    M(SOURCE_WRITE, "WRITE", SOURCE, SOURCES) \
    M(SOURCES, "", GROUP, ALL) \
    \
    M(CLUSTER, "", GLOBAL, ALL) /* ON CLUSTER queries */ \
    \
    /* Deprecated */ \
    M(FILE, "", GLOBAL, ALL) \
    M(URL, "", GLOBAL, ALL) \
    M(REMOTE, "", GLOBAL, ALL) \
    M(MONGO, "", GLOBAL, ALL) \
    M(REDIS, "", GLOBAL, ALL) \
    M(MYSQL, "", GLOBAL, ALL) \
    M(POSTGRES, "", GLOBAL, ALL) \
    M(SQLITE, "", GLOBAL, ALL) \
    M(ODBC, "", GLOBAL, ALL) \
    M(JDBC, "", GLOBAL, ALL) \
    M(HDFS, "", GLOBAL, ALL) \
    M(S3, "", GLOBAL, ALL) \
    M(HIVE, "", GLOBAL, ALL) \
    M(AZURE, "", GLOBAL, ALL) \
    M(KAFKA, "", GLOBAL, ALL) \
    M(NATS, "", GLOBAL, ALL) \
    M(RABBITMQ, "", GLOBAL, ALL) \
    M(ALL, "ALL PRIVILEGES", GROUP, NONE) /* full access */ \
    M(NONE, "USAGE, NO PRIVILEGES", GROUP, NONE) /* no access */

#define DECLARE_ACCESS_TYPE_ENUM_CONST(name, aliases, node_type, parent_group_name) \
    name,

    APPLY_FOR_ACCESS_TYPES(DECLARE_ACCESS_TYPE_ENUM_CONST)
#undef DECLARE_ACCESS_TYPE_ENUM_CONST
};

/// This macro allows to override deprecated global access types with parameterized ones.
#define ADD_OVERRIDE_FOR_DEPRECATED_ACCESS_TYPES(R) \
    R(FILE, SOURCES, "FILE") \
    R(URL, SOURCES, "URL") \
    R(REMOTE, SOURCES, "REMOTE") \
    R(MONGO, SOURCES, "MONGO") \
    R(REDIS, SOURCES, "REDIS") \
    R(MYSQL, SOURCES, "MYSQL") \
    R(POSTGRES, SOURCES, "POSTGRES") \
    R(SQLITE, SOURCES, "SQLITE") \
    R(ODBC, SOURCES, "ODBC") \
    R(JDBC, SOURCES, "JDBC") \
    R(HDFS, SOURCES, "HDFS") \
    R(S3, SOURCES, "S3") \
    R(HIVE, SOURCES, "HIVE") \
    R(AZURE, SOURCES, "AZURE") \
    R(KAFKA, SOURCES, "KAFKA") \
    R(NATS, SOURCES, "NATS") \
    R(RABBITMQ, SOURCES, "RABBITMQ")


std::string_view toString(AccessType type);

}
