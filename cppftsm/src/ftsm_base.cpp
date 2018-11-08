#include "ftsm_base.h"

namespace ftsm
{
    //////////////////////////////
    // State constant definitions
    //////////////////////////////
    std::string FTSMStates::START = "start";
    std::string FTSMStates::INITIALISING = "initialising";
    std::string FTSMStates::CONFIGURING = "configuring";
    std::string FTSMStates::READY = "ready";
    std::string FTSMStates::RUNNING = "running";
    std::string FTSMStates::RECOVERING = "recovering";
    std::string FTSMStates::STOPPED = "stopped";


    ///////////////////////////////////
    // Transition constant definitions
    ///////////////////////////////////
    std::string FTSMTransitions::INITIALISED = "initialised";
    std::string FTSMTransitions::INIT_FAILED = "initialisation_failed";

    std::string FTSMTransitions::DONE_CONFIGURING = "config_successful";
    std::string FTSMTransitions::RECONFIGURE = "configure";
    std::string FTSMTransitions::DONE_RECONFIGURING = "reconfig_successful";
    std::string FTSMTransitions::RETRY_CONFIG = "retry_config";
    std::string FTSMTransitions::FAILED_CONFIG = "config_failure";

    std::string FTSMTransitions::WAIT = "wait";
    std::string FTSMTransitions::RUN = "run";
    std::string FTSMTransitions::CONTINUE = "continue_running";
    std::string FTSMTransitions::DONE = "done";

    std::string FTSMTransitions::RECOVER = "recover";
    std::string FTSMTransitions::DONE_RECOVERING = "recovery_successful";
    std::string FTSMTransitions::FAILED_RECOVERY = "failed_recovery";


    /////////////////////////////
    // Transition map definition
    /////////////////////////////
    std::map<std::string, std::map<std::string, std::string>> FTSMBase::transition_map =
    {
        {FTSMStates::INITIALISING,
            {
                {FTSMTransitions::INITIALISED, FTSMStates::CONFIGURING},
                {FTSMTransitions::INIT_FAILED, FTSMStates::RECOVERING}
            }
        },
        {FTSMStates::CONFIGURING,
            {
                {FTSMTransitions::DONE_CONFIGURING, FTSMStates::READY},
                {FTSMTransitions::RETRY_CONFIG, FTSMStates::CONFIGURING},
                {FTSMTransitions::DONE_RECONFIGURING, ""}, // depends on the previous state; only performed if the component was already configured before
                {FTSMTransitions::FAILED_CONFIG, FTSMStates::STOPPED}
            }
        },
        {FTSMStates::READY,
            {
                {FTSMTransitions::RUN, FTSMStates::RUNNING},
                {FTSMTransitions::WAIT, FTSMStates::READY},
                {FTSMTransitions::RECONFIGURE, FTSMStates::CONFIGURING}
            }
        },
        {FTSMStates::RUNNING,
            {
                {FTSMTransitions::DONE, FTSMStates::READY},
                {FTSMTransitions::CONTINUE, FTSMStates::RUNNING},
                {FTSMTransitions::RECOVER, FTSMStates::RECOVERING},
                {FTSMTransitions::RECONFIGURE, FTSMStates::CONFIGURING}
            }
        },
        {FTSMStates::RECOVERING,
            {
                {FTSMTransitions::DONE_RECOVERING, ""}, // depends on the previous state
                {FTSMTransitions::FAILED_RECOVERY, FTSMStates::STOPPED}
            }
        }
    };


    ///////////////////////////////
    // FTSMBase method definitions
    ///////////////////////////////
    FTSMBase::FTSMBase(std::string name, std::vector<std::string> dependencies, int max_recovery_attempts)
        : current_state(FTSMStates::START)
    {
        this->name = name;
        this->dependencies = dependencies;
        this->max_recovery_attempts = max_recovery_attempts;
        this->configured = false;
        this->is_running = true;
        this->is_alive = false;
    }

    /**
     * Starts the state machine on a background thread
     */
    void FTSMBase::run()
    {
        if (!this->is_alive)
        {
            this->current_state = FTSMStates::INITIALISING;
            this->is_alive = true;
            this->sm_thread = std::thread(&FTSMBase::__manage_sm, this);
        }
        else
        {
            std::cout << this->name << " already running" << std::endl;
        }
    }

    /**
     * Stops the state machine
     */
    void FTSMBase::stop()
    {
        if (this->is_alive)
        {
            this->is_running = false;
            this->is_alive = false;
            this->sm_thread.join();
        }
        else
        {
            std::cout << this->name << " cannot be stopped because it is not running yet" << std::endl;
        }
    }

    /**
     * Performs component initialisation; calls the virtual this->init method and returns a transition constant
     */
    std::string FTSMBase::__init()
    {
        std::string result = this->init();
        return result;
    }

    /**
     * Performs component configuration; calls the virtual this->configuring method and returns a transition constant
     */
    std::string FTSMBase::__configuring()
    {
        int attempt_counter = 0;
        std::string result = "";
        while (attempt_counter < this->max_recovery_attempts && result != FTSMTransitions::DONE_CONFIGURING)
        {
            std::cout << "Configuring " << this->name << "; attempt number " << (attempt_counter + 1) << std::endl;
            result = this->configuring();
            attempt_counter += 1;
        }

        if (attempt_counter == this->max_recovery_attempts && result != FTSMTransitions::DONE_CONFIGURING)
        {
            std::cout << "Could not configure " << this->name << " after a maximum of " << this->max_recovery_attempts << " attempts" << std::endl;
            return FTSMTransitions::FAILED_CONFIG;
        }

        if (!this->configured)
        {
            this->configured = true;
            return FTSMTransitions::DONE_CONFIGURING;
        }
        else
        {
            return FTSMTransitions::DONE_RECONFIGURING;
        }
    }

    /**
     * Indicates component readiness; calls the virtual this->ready method and returns a transition constant when an operation request is received
     */
    std::string FTSMBase::__ready()
    {
        std::string result = this->ready();
        return result;
    }

    /**
     * Performs component operation; calls the virtual this->running method and returns a transition constant
     */
    std::string FTSMBase::__running()
    {
        std::string result = this->running();
        return result;
    }

    /**
     * Performs component recovery; calls the virtual this->recovering method and returns a transition constant
     */
    std::string FTSMBase::__recovering()
    {
        int attempt_counter = 0;
        std::string result = "";
        while (attempt_counter < this->max_recovery_attempts && result != FTSMTransitions::DONE_RECOVERING)
        {
            std::cout << "Attempting recovery of " << this->name << "; attempt number " << (attempt_counter + 1) << std::endl;
            result = this->recovering();
            attempt_counter += 1;
        }

        if (attempt_counter == this->max_recovery_attempts && result != FTSMTransitions::DONE_RECOVERING)
        {
            std::cout << "Could not recover " << this->name << " after a maximum of " << this->max_recovery_attempts << " recovery attempts" << std::endl;
            return FTSMTransitions::FAILED_RECOVERY;
        }
        return result;
    }

    /**
     * Manages the operation of the state machine by calling the appropriate state methods and
     * performing state machine transitions based on the results of the states
     */
    void FTSMBase::__manage_sm()
    {
        std::string transition_constant = "";
        while (this->current_state != FTSMStates::STOPPED and this->is_running)
        {
            if (this->current_state == FTSMStates::INITIALISING)
            {
                transition_constant = this->__init();
            }
            else if (this->current_state == FTSMStates::CONFIGURING)
            {
                transition_constant = this->__configuring();
            }
            else if (this->current_state == FTSMStates::READY)
            {
                transition_constant = this->__ready();
            }
            else if (this->current_state == FTSMStates::RUNNING)
            {
                transition_constant = this->__running();
            }
            else if (this->current_state == FTSMStates::RECOVERING)
            {
                transition_constant = this->__recovering();
            }
            this->__transition(transition_constant);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        if (this->is_running)
        {
            this->is_running = false;
        }
    }

    /**
     * Performs a state machine transition as indicated by the given transition constant
     *
     * @param transition an FTSMTransitions constant indicating the transition that should be taken
     */
    void FTSMBase::__transition(std::string transition)
    {
        std::string new_state = FTSMBase::transition_map[current_state][transition];
        if (new_state == "")
        {
            new_state = this->previous_state;
        }

        if (new_state != this->current_state)
        {
            std::cout << "State machine transitioning: " << this->current_state << " -> " << new_state << std::endl;
        }

        this->previous_state = this->current_state;
        this->current_state = new_state;
    }
}
