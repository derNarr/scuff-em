======
README
======
This readme describes how the py.test framework is used in order to run fast
and slow tests on a compiled ``scuff-em`` version.

The tests should ensure that nothing unexpected happend during the development
of the code base.


Run Tests
---------
In order to run the test suite you need to install ``py.test`` and run in the
scuff-em top level directory the following command for very fast tests
(seconds)::

    py.test

or for slower tests (minutes to hours)::

    py.test --runslow

or in order to validate the ``scuff-em`` installation against known
semi-analytical results (hours -- at the moment 10 minutes on 4 cores)::

    py.test --validate-scuff


Work in progress
----------------
This test suite is work in progress and at the moment only some tests for
``scuff-caspol`` exist. You are welcome to contribute additional tests for
other parts of ``scuff-em``. Before contributing tests please read this
document and have a look at the tests in test_scuff-caspol.py.


Directories
-----------
unitTests/py_tests (this directory)
    This folder contains all tests that will be run by ``py.test``. The tests
    are written in legal python files that start with ``test_``.

unitTests/py_tests/data
    Files that will be copied to a temporary folder during the testing and that
    are needed in order to run ``scuff-em``.

unitTests/py_tests/geo
    geo files for the mesh files in data.


Files
-----
Files describing the tests:

test_scuff-caspol.py
    python script that states the different tests.

Files needed for these tests are:

dist.txt
    containing the distances that will be calculated. They are given in
    units of micro meters (microns).

freq.txt
    containing the imaginary frequencies that are used in the
    calculations. They are given in units of 3e14 rad/sec.

coin_144.msh
    the mesh for a simple plate

coin_pec_144.scuffgeo
    description of the geometry for scuff-caspol

coin.geo
    source file for the coin_144.msh file.

In order to generate ``coin_144.msh`` out of ``coin.geo`` one has to run::

    cd unitTests/py_tests/geo
    gmsh -clscale 1 -2 coin.geo
    mv coin.msh ../data/coin_144.msh

