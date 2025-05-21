.. _sdll_test:

SDLL Subsystem Test Sample
##########################

Overview
********

This test sample is used to test the SDLL Subsystem API.

Building and Testing
********************

This application can be built and executed on native_sim as follows:

.. code-block:: console
   $ ./scripts/twister -p native_sim -T tests/subsys/sdll
To build for another board, change "native_sim" above to that board's name.

At the current stage, it only supports native_sim.
