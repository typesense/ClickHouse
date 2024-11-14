#include <Core/BaseSettings.h>
#include <Core/BaseSettingsFwdMacrosImpl.h>
#include <Parsers/ASTCreateQuery.h>
#include <Parsers/ASTFunction.h>
#include <Parsers/ASTSetQuery.h>
#include <Storages/ObjectStorage/StorageObjectStorage.h>
#include <Storages/ObjectStorage/StorageObjectStorageSettings.h>
#include <Storages/System/MutableColumnsAndConstraints.h>
#include <Common/Exception.h>

namespace DB
{

#define STORAGE_OBJECT_STORAGE_RELATED_SETTINGS(DECLARE, ALIAS) DECLARE(Bool, allow_dynamic_metadata_for_data_lakes, false, "", 0)

#define LIST_OF_STORAGE_OBJECT_STORAGE_SETTINGS(M, ALIAS) \
    STORAGE_OBJECT_STORAGE_RELATED_SETTINGS(M, ALIAS) \
    LIST_OF_ALL_FORMAT_SETTINGS(M, ALIAS)

DECLARE_SETTINGS_TRAITS(StorageObjectStorageSettingsTraits, LIST_OF_STORAGE_OBJECT_STORAGE_SETTINGS)
IMPLEMENT_SETTINGS_TRAITS(StorageObjectStorageSettingsTraits, LIST_OF_STORAGE_OBJECT_STORAGE_SETTINGS)

struct StorageObjectStorageSettingsImpl : public BaseSettings<StorageObjectStorageSettingsTraits>
{
};

#define INITIALIZE_SETTING_EXTERN(TYPE, NAME, DEFAULT, DESCRIPTION, FLAGS) \
    StorageObjectStorageSettings##TYPE NAME = &StorageObjectStorageSettingsImpl ::NAME;

namespace StorageObjectStorageSetting
{
LIST_OF_STORAGE_OBJECT_STORAGE_SETTINGS(INITIALIZE_SETTING_EXTERN, SKIP_ALIAS)
}

#undef INITIALIZE_SETTING_EXTERN

StorageObjectStorageSettings::StorageObjectStorageSettings() : impl(std::make_unique<StorageObjectStorageSettingsImpl>())
{
}

StorageObjectStorageSettings::StorageObjectStorageSettings(const StorageObjectStorageSettings & settings)
    : impl(std::make_unique<StorageObjectStorageSettingsImpl>(*settings.impl))
{
}

StorageObjectStorageSettings::StorageObjectStorageSettings(StorageObjectStorageSettings && settings) noexcept
    : impl(std::make_unique<StorageObjectStorageSettingsImpl>(std::move(*settings.impl)))
{
}


void StorageObjectStorageSettings::dumpToSystemEngineSettingsColumns(
    MutableColumnsAndConstraints & params,
    const std::string & table_name,
    const std::string & database_name,
    const StorageObjectStorage & storage) const
{
    MutableColumns & res_columns = params.res_columns;

    /// We cannot use setting.isValueChanged(), because we do not store initial settings in storage.
    /// Therefore check if the setting was changed via table metadata.
    const auto & settings_changes = storage.getInMemoryMetadataPtr()->settings_changes->as<ASTSetQuery>()->changes;
    auto is_changed = [&](const std::string & setting_name) -> bool
    {
        return settings_changes.end()
            != std::find_if(
                   settings_changes.begin(),
                   settings_changes.end(),
                   [&](const SettingChange & change) { return change.name == setting_name; });
    };

    for (const auto & change : impl->all())
    {
        size_t i = 0;
        res_columns[i++]->insert(database_name);
        res_columns[i++]->insert(table_name);
        res_columns[i++]->insert(change.getName());
        res_columns[i++]->insert(convertFieldToString(change.getValue()));
        res_columns[i++]->insert(change.getTypeName());
        res_columns[i++]->insert(is_changed(change.getName()));
        res_columns[i++]->insert(change.getDescription());
        res_columns[i++]->insert(false);
    }
}

StorageObjectStorageSettings::~StorageObjectStorageSettings() = default;

STORAGE_OBJECT_STORAGE_SETTINGS_SUPPORTED_TYPES(StorageObjectStorageSettings, IMPLEMENT_SETTING_SUBSCRIPT_OPERATOR)


void StorageObjectStorageSettings::loadFromQuery(ASTStorage & storage_def)
{
    if (storage_def.settings)
    {
        impl->applyChanges(storage_def.settings->changes);
    }
}

Field StorageObjectStorageSettings::get(const std::string & name)
{
    return impl->get(name);
}

}
