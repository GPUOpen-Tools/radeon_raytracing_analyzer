These panes show the overview of the trace and the system.

Summary
-------
The summary pane presents a high-level view of the acceleration structures in
a trace.

At the top, a small table will show the number of acceleration structures by type
and if there are any acceleration structures that are empty or missing.

The **Total Memory** is the memory used for all the acceleration structures.

The **TLAS List** shows a list of all Top-level acceleration structures in
the trace. It is split into several sections:

* The left section shows a high level diagram of the TLAS, showing the number
  of instances and BLASes. Also shown is the memory used by the TLAS and the total
  memory used. The total here is the combined memory used by the TLAS and all the BLASes
  it references. TLASes can, and do, share BLASes so the totals for all the TLASes can
  sometimes be larger that the total memory displayed above.

* The right section displays some useful statistics for each TLAS are shown so each
  can be easily identified.

.. image:: media/overview/summary_1.png

Clicking on the TLAS name or address (text in blue) will navigate to the TLAS
Viewer pane.

Device configuration
--------------------
This pane will show some of the parameters of the video hardware on which the
memory trace was taken, showing such things as the name of the video card and
the memory bandwidth.

.. image:: media/overview/device_config_1.png
