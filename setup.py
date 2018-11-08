from setuptools import setup, find_packages

setup(name='pyftsm',
      version='1.0.0',
      description='An implementation of a fault-tolerant state machine',
      url='https://github.com/ropod-project/ftsm',
      author='Alex Mitrevski',
      maintainer='Alex Mitrevski, Santosh Thoduka, Argentina Ortega Sainz',
      author_email='aleksandar.mitrevski@h-brs.de',
      maintainer_email='aleksandar.mitrevski@h-brs.de, santosh.thoduka@h-brs.de, argentina.ortega@h-brs.de',
      keywords='fault_tolerance robotics',
      packages=find_packages(exclude=['contrib', 'docs', 'tests']),
      project_urls={
          'Source': 'https://github.com/ropod-project/ftsm'
      })
