import time
from pyftsm.ftsm_base import FTSMBase, FTSMTransitions

class Component(FTSMBase):
    def __init__(self, name, dependencies, max_recovery_attempts=1):
        super(Component, self).__init__(name, dependencies, max_recovery_attempts)

    def init(self):
        print('Initialising')
        return FTSMTransitions.INITIALISED

    def configuring(self):
        print('Configuring')
        time.sleep(0.2)
        return FTSMTransitions.RETRY_CONFIG

    def ready(self):
        print('Ready')
        time.sleep(0.25)
        return FTSMTransitions.RUN

    def running(self):
        print('Running')
        time.sleep(1.)
        return FTSMTransitions.DONE

    def recovering(self):
        print('Recovering')
        time.sleep(0.5)
        return FTSMTransitions.DONE_RECOVERING

if __name__ == '__main__':
    component = Component('component_that_cannot_configure', ['component_1', 'component_2'], 3)
    try:
        component.run()
        while component.is_running:
            time.sleep(0.1)
    except (KeyboardInterrupt, SystemExit):
        print('{0} interrupted; exiting...'.format(component.name))
        component.stop()
