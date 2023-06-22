#include <fmt/format.h>

#include <logging/logging.hpp>

#include <kvdb/kvdbHandlerCollection.hpp>

namespace kvdbManager
{

std::shared_ptr<IKVDBHandler> KVDBHandlerCollection::getKVDBHandler(rocksdb::DB* db,
                                                                    rocksdb::ColumnFamilyHandle* cfHandle,
                                                                    const std::string& dbName,
                                                                    const std::string& scopeName)
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    const auto it = m_mapInstances.find(dbName);
    if (it != m_mapInstances.end())
    {
        auto& instance = it->second;
        instance->addScope(scopeName);
    }
    else
    {
        auto spInstance = std::make_shared<KVDBHandlerInstance>();
        spInstance->addScope(scopeName);
        m_mapInstances.emplace(dbName, std::move(spInstance));
    }

    return std::make_shared<KVDBSpace>(m_handleManager, db, cfHandle, dbName, scopeName);
}

void KVDBHandlerCollection::removeKVDBHandler(const std::string& dbName, const std::string& scopeName)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);

    const auto it = m_mapInstances.find(dbName);
    if (it != m_mapInstances.end())
    {
        auto& instance = it->second;
        instance->removeScope(scopeName);
        if (instance->emptyScopes())
        {
            m_mapInstances.erase(it);
        }
    }
}

std::vector<std::string> KVDBHandlerCollection::getDBNames()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    std::vector<std::string> dbNames;
    dbNames.reserve(m_mapInstances.size());

    for (const auto& instance : m_mapInstances)
    {
        dbNames.push_back(instance.first);
    }

    return dbNames;
}

std::map<std::string, uint32_t> KVDBHandlerCollection::getRefMap(const std::string& dbName)
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    const auto it = m_mapInstances.find(dbName);
    if (it != m_mapInstances.end())
    {
        return it->second->getRefMap();
    }

<<<<<<< Updated upstream
    return std::map<std::string, uint32_t>();
=======
    return {};
>>>>>>> Stashed changes
}

void KVDBHandlerInstance::addScope(const std::string& scopeName)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);

    m_scopeCounter.addRef(scopeName);
}

void KVDBHandlerInstance::removeScope(const std::string& scopeName)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);

    m_scopeCounter.removeRef(scopeName);
}

bool KVDBHandlerInstance::emptyScopes()
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);

    return m_scopeCounter.empty();
}

std::vector<std::string> KVDBHandlerInstance::getRefNames()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    return m_scopeCounter.getRefNames();
}

std::map<std::string, uint32_t> KVDBHandlerInstance::getRefMap()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    return m_scopeCounter.getRefMap();
}

} // namespace kvdbManager