#include "ftsm.h"

#include <csignal>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

namespace ftsm_tests
{
    class FailedConfigComponent : public ftsm::FTSM
    {
    public:
        FailedConfigComponent(std::string name, std::vector<std::string> dependencies, int max_recovery_attempts=1)
         : ftsm::FTSM(name, dependencies, max_recovery_attempts) { }

        virtual std::string init()
        {
            std::cout << "initialising..." << std::endl;
            return ftsm::FTSMTransitions::INITIALISED;
        }

        virtual std::string configuring()
        {
            std::cout << "configuring..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            return ftsm::FTSMTransitions::RETRY_CONFIG;
        }

        virtual std::string ready()
        {
            std::cout << "waiting..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
            return ftsm::FTSMTransitions::RUN;
        }

        virtual std::string running()
        {
            std::cout << "running..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            return ftsm::FTSMTransitions::DONE;
        }

        virtual std::string recovering()
        {
            std::cout << "recovering..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            return ftsm::FTSMTransitions::DONE_RECOVERING;
        }
    };
}

ftsm_tests::FailedConfigComponent component("component_that_cannot_configure", {"component1", "component2"}, 3);

void checkTermination(int signal)
{
    component.stop();
}

int main()
{
    component.run();
    signal(SIGINT, checkTermination);
    while (component.is_running)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (component.is_alive)
    {
        component.stop();
    }
    return 0;
}
