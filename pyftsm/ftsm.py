import time
from abc import abstractmethod
import threading

class FTSMStates(object):
    '''Defines constants for the states of a fault-tolerant state machine;
    the constants corresponding to non-terminal states correspond
    to the names of the state-implementing methods

    @author Alex Mitrevski
    @maintainer Alex Mitrevski, Santosh Thoduka, Argentina Ortega Sainz
    @contact aleksandar.mitrevski@h-brs.de, santosh.thoduka@h-brs.de, argentina.ortega@h-brs.de
    '''
    START = 'start'
    INITIALISING = 'init'
    CONFIGURING = 'configuring'
    READY = 'ready'
    RUNNING = 'running'
    RECOVERING = 'recovering'
    STOPPED = 'stopped'

class FTSMTransitions(object):
    '''Defines constants for the state transitions of a fault-tolerant state machine

    @author Alex Mitrevski
    @maintainer Alex Mitrevski, Santosh Thoduka, Argentina Ortega Sainz
    @contact aleksandar.mitrevski@h-brs.de, santosh.thoduka@h-brs.de, argentina.ortega@h-brs.de
    '''
    INITIALISED = 'initialised'
    INIT_FAILED = 'initialisation_failed'

    DONE_CONFIGURING = 'config_successful'
    RECONFIGURE = 'configure'
    DONE_RECONFIGURING = 'reconfig_successful'
    RETRY_CONFIG = 'retry_config'
    FAILED_CONFIG = 'config_failure'

    WAIT = 'wait'
    RUN = 'run'
    CONTINUE = 'continue_running'
    DONE = 'done'

    RECOVER = 'recover'
    DONE_RECOVERING = 'recovery_successful'
    FAILED_RECOVERY = 'failed_recovery'

class FTSM(object):
    '''Base class of a fault-tolerant state machine

    @author Alex Mitrevski
    @maintainer Alex Mitrevski, Santosh Thoduka, Argentina Ortega Sainz
    @contact aleksandar.mitrevski@h-brs.de, santosh.thoduka@h-brs.de, argentina.ortega@h-brs.de
    '''
    def __init__(self, name, dependencies, max_recovery_attempts=1):
        ## name of the component
        self.name = name

        ## a list of strings indicating the component dependencies
        self.dependencies = dependencies

        ## maximum number of attempts in case recovery is necessary
        self.max_recovery_attempts = max_recovery_attempts

        ## thread on which the state machine is running
        self.sm_thread = None

        ## indicates whether the state machine is running
        self.is_running = True

        ## indicates whether the state machine thread is alive
        self.is_alive = False

        ## indicates whether the component has been configured
        self.configured = False

        ## the current state of the state machine
        self.current_state = ''

        ## the previous state of the state machine
        self.previous_state = ''

        ## a map of the possible transitions from each state
        self.transition_map =\
        {
            FTSMStates.INITIALISING:
            {
                FTSMTransitions.INITIALISED: FTSMStates.CONFIGURING,
                FTSMTransitions.INIT_FAILED: FTSMStates.RECOVERING
            },
            FTSMStates.CONFIGURING:
            {
                FTSMTransitions.DONE_CONFIGURING: FTSMStates.READY,
                FTSMTransitions.RETRY_CONFIG: FTSMStates.CONFIGURING,
                FTSMTransitions.DONE_RECONFIGURING: '', # depends on the previous state; only performed if the component was already configured before
                FTSMTransitions.FAILED_CONFIG: FTSMStates.STOPPED
            },
            FTSMStates.READY:
            {
                FTSMTransitions.RUN: FTSMStates.RUNNING,
                FTSMTransitions.WAIT: FTSMStates.READY,
                FTSMTransitions.RECONFIGURE: FTSMStates.CONFIGURING
            },
            FTSMStates.RUNNING:
            {
                FTSMTransitions.DONE: FTSMStates.READY,
                FTSMTransitions.CONTINUE: FTSMStates.RUNNING,
                FTSMTransitions.RECOVER: FTSMStates.RECOVERING,
                FTSMTransitions.RECONFIGURE: FTSMStates.CONFIGURING
            },
            FTSMStates.RECOVERING:
            {
                FTSMTransitions.DONE_RECOVERING: "", # depends on the previous state
                FTSMTransitions.FAILED_RECOVERY: FTSMStates.STOPPED
            }
        }

    def run(self):
        '''Starts the state machine on a background thread
        '''
        if not self.is_alive:
            self.current_state = FTSMStates.INITIALISING
            self.sm_thread = threading.Thread(target=self.__manage_sm)
            self.sm_thread.daemon = True
            self.sm_thread.start()
            self.is_alive = True
        else:
            print('{0} already running'.format(self.name))

    def stop(self):
        '''Stops the state machine
        '''
        if self.is_alive:
            self.is_running = False
            self.is_alive = False
            self.sm_thread.join()
        else:
            print('{0} cannot be stopped because it is not running yet'.format(self.name))

    def init(self):
        '''Method for component initialisation; returns FTSMTransitions.INITIALISED by default
        '''
        return FTSMTransitions.INITIALISED

    def configuring(self):
        '''Abstract method for component configuration/reconfiguration;
        returns FTSMTransitions.DONE_CONFIGURING by default
        '''
        return FTSMTransitions.DONE_CONFIGURING

    def ready(self):
        '''Abstract method for the behaviour of a component when it is ready
        for operation, but not active; returns FTSMTransitions.RUN by default
        '''
        return FTSMTransitions.RUN

    @abstractmethod
    def running(self):
        '''Abstract method for the behaviour of a component during active operation
        '''
        pass

    @abstractmethod
    def recovering(self):
        '''Abstract method for component recovery
        '''
        pass

    def _init(self):
        '''Performs component initialisation; calls the virtual self.init method
        and returns a transition constant
        '''
        result = self.init()
        return result

    def _configuring(self):
        '''Performs component configuration; calls the virtual self.configuring method
        and returns a transition constant
        '''
        attempt_counter = 0
        result = ''
        while attempt_counter < self.max_recovery_attempts and \
        result != FTSMTransitions.DONE_CONFIGURING:
            print("Configuring {0}; attempt number {1}".format(self.name, attempt_counter+1))
            result = self.configuring()
            attempt_counter += 1

        if attempt_counter == self.max_recovery_attempts and \
        result != FTSMTransitions.DONE_CONFIGURING:
            print("Could not configure {0} after a maximum of {1} attempts".format(self.name, self.max_recovery_attempts))
            return FTSMTransitions.FAILED_CONFIG

        if not self.configured:
            self.configured = True
            return FTSMTransitions.DONE_CONFIGURING
        else:
            return FTSMTransitions.DONE_RECONFIGURING

    def _ready(self):
        '''Indicates component readiness; calls the virtual self.ready method
        and returns a transition constant when an operation request is received
        '''
        result = self.ready()
        return result

    def _running(self):
        '''Performs component operation; calls the virtual self.running method
        and returns a transition constant
        '''
        result = self.running()
        return result

    def _recovering(self):
        '''Performs component recovery; calls the virtual self.recovering method
        and returns a transition constant
        '''
        attempt_counter = 0
        result = ''
        while attempt_counter < self.max_recovery_attempts and \
        result != FTSMTransitions.DONE_RECOVERING:
            print("Attempting recovery of {0}; attempt number {1}".format(self.name, attempt_counter+1))
            result = self.recovering()
            attempt_counter += 1

        if attempt_counter == self.max_recovery_attempts and \
        result != FTSMTransitions.DONE_RECOVERING:
            print("Could not recover {0} after a maximum of {1} attempts".format(self.name, self.max_recovery_attempts))
            return FTSMTransitions.FAILED_RECOVERY
        return result

    def __manage_sm(self):
        '''Manages the operation of the state machine by calling the appropriate state methods and
        performing state machine transitions based on the results of the states
        '''
        transition_constant = ''
        while self.current_state != FTSMStates.STOPPED and self.is_running:
            transition_constant = getattr(self, '_{0}'.format(self.current_state))()
            self.__transition(transition_constant)
            time.sleep(0.2)

        if self.is_running:
            self.is_running = False

    def __transition(self, transition):
        '''Performs a state machine transition as indicated by the given transition constant

        @param transition an FTSMTransitions constant indicating the transition that should be taken
        '''
        new_state = self.transition_map[self.current_state][transition]
        if new_state == '':
            new_state = self.previous_state

        if new_state != self.current_state:
            print('State machine transitioning: {0} -> {1}'.format(self.current_state, new_state))

        self.previous_state = self.current_state
        self.current_state = new_state
