#ifndef _ROUTER_ROUTERADMIN_HPP
#define _ROUTER_ROUTERADMIN_HPP

#include <list>
#include <memory>
#include <shared_mutex>

#include <bk/icontroller.hpp>
#include <queue/iqueue.hpp>
#include <store/istore.hpp>

#include <builder/registry.hpp>
#include <parseEvent.hpp>

#include <router/iapi.hpp>
#include <router/types.hpp>

namespace router
{

namespace test
{
using TestingTuple = std::tuple<base::Event, Opt, std::function<void(Output&&)>>;
using QueueType = std::shared_ptr<TestingTuple>;
} // namespace test
using ProdQueueType = base::queue::iQueue<base::Event>;
using TestQueueType = base::queue::iQueue<test::QueueType>;

// Forward declarations
class Router;
class Tester;

struct Config
{
    int m_numThreads;

    std::weak_ptr<store::IStore> m_wStore; // TODO Change to weak_ptr
    std::weak_ptr<builder::internals::Registry<builder::internals::Builder>> m_wRegistry;
    std::shared_ptr<bk::IControllerMaker> m_controllerMaker;

    std::shared_ptr<ProdQueueType> m_prodQueue;
    std::shared_ptr<TestQueueType> m_testQueue;
};

class RouterAdmin
    : public IRouterAPI
    , public ITesterAPI
{

private:
    // Global
    std::atomic_bool m_isRunning;       ///< Flag to know if the router is running
    std::vector<std::thread> m_threads; ///< Vector of threads for the router (move router)

    // Production
    struct Production
    {
        std::list<std::shared_ptr<Router>> routers;
        mutable std::shared_mutex bussyMutex; ///< Mutex for updating the router (Only 1 request at a time)
        std::shared_ptr<ProdQueueType> queue;
        Production() = default;
    } m_production;

    struct Testing
    {
        std::list<std::shared_ptr<Tester>> tester; ///< List of testers
        mutable std::shared_mutex bussyMutex;      ///< Mutex for updating the testers (Only 1 request at a time)
        std::shared_ptr<TestQueueType> queue;
        Testing() = default;
    } m_testing;

    void validateConfig(const Config& config);

public:
    ~RouterAdmin() = default;
    RouterAdmin() = delete;

    RouterAdmin(const Config& config);

    /**
     * @brief Start the router
     *
     */
    void start();

    /**
     * @brief Stop the router
     *
     */
    void stop();

    void pushEvent(const std::string& eventStr)
    {
        base::Event event;
        try
        {
            event = base::parseEvent::parseWazuhEvent(eventStr);
        }
        catch (const std::exception& e)
        {
            LOG_WARNING("Error parsing event: '{}' (discarding...)", e.what());
            return;
        }
        m_production.queue->push(std::move(event));
    }

    /**************************************************************************
     * IRouterAPI
     *************************************************************************/

    /**
     * @copydoc router::IRouterAPI::postEnvironment
     */
    base::OptError postEntry(const prod::EntryPost& entry) override;

    /**
     * @copydoc router::IRouterAPI::deleteEnvironment
     */
    base::OptError deleteEntry(const std::string& name) override;

    /**
     * @copydoc router::IRouterAPI::getEnvironment
     */
    base::RespOrError<prod::Entry> getEntry(const std::string& name) const override;

    /**
     * @copydoc router::IRouterAPI::reloadEnvironment
     */
    base::OptError reloadEntry(const std::string& name) override;

    /**
     * @copydoc router::IRouterAPI::ChangeEnvironmentPriority
     */
    base::OptError changeEntryPriority(const std::string& name, size_t priority) override;

    /**
     * @copydoc router::IRouterAPI::getEntries
     */
    std::list<prod::Entry> getEntries() const override;

    /**
     * @copydoc router::IRouterAPI::postEvent
     */
    void postEvent(base::Event&& event) override { m_production.queue->push(std::move(event)); }

    /**
     * @copydoc router::IRouterAPI::postStrEvent
     */
    base::OptError postStrEvent(std::string_view event) override;

    /**************************************************************************
     * ITesterAPI
     *************************************************************************/

    /**
     * @copydoc router::ITesterAPI::postTestEnvironment
     */
    base::OptError postTestEntry(const test::EntryPost& entry) override;

    /**
     * @copydoc router::ITesterAPI::deleteTestEnvironment
     */
    base::OptError deleteTestEntry(const std::string& name) override;

    /**
     * @copydoc router::ITesterAPI::getTestEnvironment
     */
    base::RespOrError<test::Entry> getTestEntry(const std::string& name) const override;

    /**
     * @copydoc router::ITesterAPI::reloadTestEnvironment
     */
    base::OptError reloadTestEntry(const std::string& name) override;

    /**
     * @copydoc router::ITesterAPI::getTestEntries
     */
    std::list<test::Entry> getTestEntries() const override;

    /**
     * @copydoc router::ITesterAPI::ingestTest
     */
    base::RespOrError<std::future<test::Output>> ingestTest(base::Event&& event, const test::Opt& opt) override;

    /**
     * @copydoc router::ITesterAPI::ingestTest
     */
    base::RespOrError<std::future<test::Output>> ingestTest(std::string_view event, const test::Opt& opt) override;
};

} // namespace router

#endif // _ROUTER_ROUTERADMIN_HPP