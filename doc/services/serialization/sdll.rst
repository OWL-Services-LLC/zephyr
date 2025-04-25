.. _sdll_reference:

Simple Data Link Layer (SDLL)
##############################

SDLL is a lightweight data serialization library designed to facilitate
efficient and reliable data frame synchronization between multiple devices.
Built on the principles of the HDLC (High-Level Data Link Control) protocol,
SDLL simplifies the process of data transmission by focusing on core
functionality while maintaining a minimal footprint.

Unlike traditional HDLC, which incorporates built-in error detection and
correction mechanisms, SDLL prioritizes flexibility by ensuring only the
correct transportation of frames. It achieves this by offering a prototype
function that allows users to customize the frame content verification
procedure (i.e. CRC, checksum, etc.) to suit their specific requirements.

Samples
*******

* :zephyr:code-sample:`sdll-basic` sample demonstrates the use of SDLL
  for data transmission using CRC-16 as the frame verification method.

API Reference
*************

.. doxygengroup:: sdll_api
