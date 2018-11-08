#ifndef FTSM_BASE_H
#define FTSM_BASE_H

#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <string>
#include <vector>
#include <map>

namespace ftsm
{
    /**
     * Defines constants for the states of a fault-tolerant state machine
     *
     * @author Alex Mitrevski
     * @maintainer Alex Mitrevski, Santosh Thoduka, Argentina Ortega Sainz
     * @contact aleksandar.mitrevski@h-brs.de, santosh.thoduka@h-brs.de, argentina.ortega@h-brs.de
     */
    struct FTSMStates
    {
        static std::string START;
        static std::string INITIALISING;
        static std::string CONFIGURING;
        static std::string READY;
        static std::string RUNNING;
        static std::string RECOVERING;
        static std::string STOPPED;
    };

    /**
     * Defines constants for the state transitions of a fault-tolerant state machine
     *
     * @author Alex Mitrevski
     * @maintainer Alex Mitrevski, Santosh Thoduka, Argentina Ortega Sainz
     * @contact aleksandar.mitrevski@h-brs.de, santosh.thoduka@h-brs.de, argentina.ortega@h-brs.de
     */
    struct FTSMTransitions
    {
        static std::string INITIALISED;
        static std::string INIT_FAILED;

        static std::string DONE_CONFIGURING;
        static std::string RECONFIGURE;
        static std::string DONE_RECONFIGURING;
        static std::string RETRY_CONFIG;
        static std::string FAILED_CONFIG;

        static std::string WAIT;
        static std::string RUN;
        static std::string CONTINUE;
        static std::string DONE;

        static std::string RECOVER;
        static std::string DONE_RECOVERING;
        static std::string FAILED_RECOVERY;
    };

    /**
     * Base class of a fault-tolerant state machine
     *
     * @author Alex Mitrevski
     * @maintainer Alex Mitrevski, Santosh Thoduka, Argentina Ortega Sainz
     * @contact aleksandar.mitrevski@h-brs.de, santosh.thoduka@h-brs.de, argentina.ortega@h-brs.de
     */
    class FTSMBase
    {
    public:
        FTSMBase(std::string name, std::vector<std::string> dependencies, int max_recovery_attempts=1);

        /**
         * Starts the state machine on a background thread
         */
        void run();

        /**
         * Stops the state machine
         */
        void stop();

        /**
         * Abstract method for component initialisation
         */
        virtual std::string init() = 0;

        /**
         * Abstract method for component configuration/reconfiguration
         */
        virtual std::string configuring() = 0;

        /**
         * Abstract method for the behaviour of a component when it is ready for operation, but not active
         */
        virtual std::string ready() = 0;

        /**
         * Abstract method for the behaviour of a component during active operation
         */
        virtual std::string running() = 0;

        /**
         * Abstract method for component recovery
         */
        virtual std::string recovering() = 0;

        /**
         * Indicates whether the state machine is running
         */
        bool is_running;

        /**
         * Indicates whether the state machine thread is alive
         */
        bool is_alive;
    protected:
        /**
         * The name of the component
         */
        std::string name;

        /**
         * The current state of the component
         */
        std::string current_state;

        /**
         * A list of component dependencies
         */
        std::vector<std::string> dependencies;

        /**
         * Indicates whether the component has been configured
         */
        bool configured;

        /**
         * The maximum number of times the recovery behaviour should be repeated before we consider the component to have failed
         */
        int max_recovery_attempts;
    private:
        /**
         * Performs component initialisation; calls the virtual this->init method and returns a transition constant
         */
        std::string __init();

        /**
         * Performs component configuration; calls the virtual this->configuring method and returns a transition constant
         */
        std::string __configuring();

        /**
         * Indicates component readiness; calls the virtual this->ready method and returns a transition constant when an operation request is received
         */
        std::string __ready();

        /**
         * Performs component operation; calls the virtual this->running method and returns a transition constant
         */
        std::string __running();

        /**
         * Performs component recovery; calls the virtual this->recovering method and returns a transition constant
         */
        std::string __recovering();

        /**
         * Manages the operation of the state machine by calling the appropriate state methods and
         * performing state machine transitions based on the results of the states
         */
        void __manage_sm();

        /**
         * Performs a state machine transition as indicated by the given transition constant
         *
         * @param transition an FTSMTransitions constant indicating the transition that should be taken
         */
        void __transition(std::string transition);

        /**
         * The previous state of the state machine (used for back-transitioning in case of reconfiguration and recovery)
         */
        std::string previous_state;

        /**
         * Thread for running the state machine
         */
        std::thread sm_thread;

        /**
         * A map of the possible transitions from each state
         */
        static std::map<std::string, std::map<std::string, std::string>> transition_map;
    };
}

#endif
