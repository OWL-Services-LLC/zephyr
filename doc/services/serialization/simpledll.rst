.. _simpledll_reference:

Simple Data Link Layer (SimpleDLL)
##############################

SimpleDLL is a lightweight data serialization library designed to facilitate
efficient and reliable data frame synchronization between multiple devices.
Built on the principles of the HDLC (High-Level Data Link Control) protocol,
SimpleDLL simplifies the process of data transmission by focusing on core
functionality while maintaining a minimal footprint.

Unlike traditional HDLC, which incorporates built-in error detection and
correction mechanisms, SimpleDLL prioritizes flexibility by ensuring only the
correct transportation of frames. It achieves this by offering a prototype
function that allows users to customize the frame content verification
procedure (i.e. CRC, checksum, etc.) to suit their specific requirements.

Samples
*******

* :zephyr:code-sample:`simpledll` sample demonstrates the use of SimpleDLL
  for data transmission using CRC-16 as the frame verification method.

API Reference
*************

.. doxygengroup:: simpledll_api
