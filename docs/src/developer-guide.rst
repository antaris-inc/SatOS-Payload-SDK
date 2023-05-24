SatOS Payload SDK Developer Guide
#################################


The SatOS Payload SDK provides libraries, tools and documentation that support developing satellite payloads for use with SatOS (TM) from Antaris, Inc.
This enables effective development and testing of payload applications, which handle communication with core spacecraft services as well as payload-specific devices.

Key Terminology
***************

**Antaris Cloud Platform**: Cloud-based SaaS platform developed and operated by Antaris, Inc. The cloud platform is used to drive development, simulation and operation of satellites.

**Payload Developer****: Individual or group that is working with the Antaris Cloud Platform to develop, deploy and operate a payload on-orbit.

**Payload SDK**: toolkit containing source code libraries and documentation used by Payload Developers to effectively develop a payload.

**Payload Server**: The physical onboard system with CPU, memory, storage and I/O connectivity for Payload Devices. It hosts Payload Applications and various system services.

**Payload Controller (PC)**: The software stack that manages operations on the Payload Server and acts as a gateway to all services provided to Payload Applications.

**Payload Interface**: API enabling bi-directional communication between Payload Application and Payload Controller.

**Payload Device**: A physical device provided by a Payload Developer that is physically connected to the Payload Server via one of the designated interfaces (Ethernet, I2C, UART, USB, SPI, etc).

**Device Driver**: A general term used to describe any software supporting access to or manipulation of a Payload Device. A driver typically supports one or both of the following:

  * **General Device I/O**: having generic knowledge about mechanisms to communicate with Hardware entities using a particular interface type (e.g. USB, UART etc.), or
  *  **Device-specific capabilities**: having knowledge of the Payload Device specifics and exposing abstracted interfaces for higher level software, while encapsulating the device details. Could be implemented as user - space software or kernel modules.

**Payload Application (PA)**: Software developed by a Payload Developer using the Payload SDK which operates its Payload Device, interacts with PC for flight services and executes payload mission business logic. It is developed by payload owners to execute a known set of Payload Sequences.

**Payload Sequence**: a discrete operation designed by Payload Developers to support mission objectives. Payload Sequences are triggered by Telecommands.

**Telecommand (TC)**: represents a command sent from an operator to a satellite, representing a logical action to be taken, for example "point towards the ground", "establish X-band radio connection", or "execute XYZ payload sequence". Telecommands may be executed on-demand during a ground contact, or they may be scheduled for execution over time.

System Architecture
*******************

A payload developer is responsible for a Payload Application (PA) and a Payload Device – effectively everything highlighted in blue below:

.. image:: images/satellite-architecture.png

Payload Applications contain payload-specific business logic and any device drivers necessary to interact with a Payload Device. Storage is made available to the application that persists across reboots or unexpected failures.

While active, a Payload Application has full control over its associated device and may interact with the Spacecraft Controller via the Payload Controller and the Payload Interface. This allows Payload Applications to coordinate with the onboard scheduler, emit telemetry/teledata for downlink, implement health checking, etc.

The Spacecraft Controller manages all communication with the Antaris Cloud Platform (i.e. mission control services), typically via S- and X-band radio links. This is the control and data path for onboard payloads.

So-called "massless" Payloads are also supported by this architecture. In this case, a Payload Application is simply developed without an associated Payload Device.

Application Design
******************

A Payload Application encompasses all software operating within the Payload Server for a specific payload. This includes application-specific software, system libraries, and even the encapsulating container or virtual machine.

This section of the SDK guide explains how a developer should think about design and implementation of a Payload Application.

Application Operation
=====================

When a Payload Application is scheduled to be active for a given period of time, a simple state machine is implemented:

1. Payload Device is powered on
2. Payload Application is booted up
3. Payload Application starts (via entrypoint script)
4. Payload Application initializes a Payload Interface connection
5. Payload Controller instructs Payload Application to execute necessary Payload Sequences
6. After sequences are executed, the PA shuts down
7. Payload Device is powered off

Application Modes
=================

On boot, the Payload Application has an opportunity to determine the "mode" of operation requested. This is used to instruct the Payload Application to start up in one or more states to facilitate actions such as upgrading application software or implementing a "factory reset" to recover from some failure. This "mode" check would happen within step 2) and alter steps 3) through 6) as described above.

Payload Sequences
=================

Payload Developers model executable routines as "Payload Sequences". A sequence represents a discrete unit of work, and usually maps to specific manipulation of a payload device in accordance with mission objectives.

Payload Sequences are scheduled using simple heuristics, all controlled by the Antaris Cloud Platform. Examples of this include "when the satellite is within range of X,Y coordinates" or "twice per orbit during the local day time".

There is no support for dynamic pointing at this time (i.e. manipulation of ADCS to point at targets determined by payload application at time of execution). Geographic triggers are considered by the Antaris Cloud Platform when generating spacecraft schedules.

A Payload Application is instructed to execute a sequence using the Payload Interface. Payload Applications are not "always on", and will only be booted up when its sequences are to be executed. Sequences are always given a duration within which they are expected to run, and are not able to run forever.

Dynamic/on-demand interaction for active debugging and diagnosis is supported directly by the Antaris Cloud Platform. This is not discussed further in this document.

Tasking & Scheduling
====================

A **Task** represents a higher-level operation, such as spacecraft station-keeping or payload manipulation. Tasks take the form of templates containing ordered sets of Telecommands. For example, a Task might be defined to point to a location on the ground and manipulate an earth observation payload:

+---+----------------------+-------------------------------------------------------------+------+
| *Example Task "exec_payload_imager"* (Duration = D)                                           |
+---+----------------------+-------------------------------------------------------------+------+
| # | Telecommand          | Parameters                                                  | Time |
+===+======================+=============================================================+======+
| 1 | adcs_point_nadir     |                                                             | T    |
+---+----------------------+-------------------------------------------------------------+------+
| 2 | power_on_payload     | Payload_HW_ID=7                                             | T1   |
+---+----------------------+-------------------------------------------------------------+------+
| 3 | boot_payload_app     | Payload_APP_ID=4, Mode=primary                              | T2   |
+---+----------------------+-------------------------------------------------------------+------+
| 4 | start_sequence       | Payload_APP_ID=4, Seq_ID=”B”, Seq_Params=“arg1”, Dur=D      | T3   |
+---+----------------------+-------------------------------------------------------------+------+
| 5 | shutdown_payload_app | Payload_APP_ID=4                                            | T3+D |
+---+----------------------+-------------------------------------------------------------+------+

An **Operator** uses Tasks to construct a **Schedule**. Schedules contain a series of Telecommands rendered from input Tasks. A schedule typically spans one or more days, beginning some number of hours or days in the future. This is used to instruct a satellite how to autonomously operate while outside of an active ground station contact.

An example Schedule could be created from the following tasks, taking the provided Start Time and Duration as input:

+---+------------------------+------------+----------+
| # | Task                   | Start Time | Duration |
+===+========================+============+==========+
| 1 | execute_payload_imager | 02:00:00   | 1200     |
+---+------------------------+------------+----------+
| 2 | ground_contact_alaska  | 02:24:40   | 600      |
+---+------------------------+------------+----------+

The rendered Schedule might look like so:


+---+----------------------+-------------------------------------------------------------+----------+
| # | Telecommand          | Parameters                                                  | Time     |
+===+======================+=============================================================+==========+
| **execute_payload_imager(D=1200)**                                                                |
+---+----------------------+-------------------------------------------------------------+----------+
| 1 | adcs_point_nadir     |                                                             | 02:00:00 |
+---+----------------------+-------------------------------------------------------------+----------+
| 2 | power_on_payload     | Payload_HW_ID=7                                             | 02:02:30 |
+---+----------------------+-------------------------------------------------------------+----------+
| 3 | boot_payload_app     | Payload_APP_ID=4, Mode=primary                              | 02:04:00 |
+---+----------------------+-------------------------------------------------------------+----------+
| 4 | start_sequence       | Payload_APP_ID=4, Seq_ID=”B”, Seq_Params=“arg1”, Dur=1200   | 02:04:30 |
+---+----------------------+-------------------------------------------------------------+----------+
| 5 | shutdown_payload_app | Payload_APP_ID=4                                            | 02:24:30 |
+---+----------------------+-------------------------------------------------------------+----------+
| **ground_contact_alaska(D=600)**                                                                  |
+---+----------------------+-------------------------------------------------------------+----------+
| 6 | adcs_point_lat_lng   | lat_lng=61,-147                                             | 02:24:40 |
+---+----------------------+-------------------------------------------------------------+----------+
| 7 | exec_ground_contact  | bands=s,x                                                   | 02:26:00 |
+---+----------------------+-------------------------------------------------------------+----------+

Task definition and scheduling is a collaborative, ongoing exercise. During initial payload development, it is helpful to keep the following dimensions in mind:

* **Task ID**: an alphanumeric value assigned by payload developer (e.g. "execute_payload_imager" above)
* **Task Duration**: the amount of time required to run the Task
* **Trigger Conditions**: the geographic location, absolute/relative time that Tasks should be executed
* **Executions per Orbit/Day**: the number of times a Task should be invoked within a given time period, likely per orbit or per 24-hour period
* **Payload Device Power State**: the expected payload device power state before and during Task execution
* **Power Requirements**: the average and max power requirements required for the Task

File Upload & Download
======================
File uploads are handled facilitated by the Antaris Cloud Platform. Uploaded files are made available at a pre-determined location in a Payload Application’s storage space. Keep in mind that radio uplink bandwidth is relatively limited, so it is wise to minimize upload file size and to consider piecemeal update processes during Payload Application development.

File downloads are typically initiated in response to creation of some mission-oriented data by the Payload Application and/or Payload Device. A PA must inform the satellite that files are ready to be downloaded using the Payload Interface. Files can then be automatically downlinked to the ground and distributed to mission operators. 

Application Upgrades
====================

Payload Applications are expected to upgrade themselves, typically using package-based processes (i.e. deb/rpm).

An alternate PA mode should be used to trigger an upgrade. This explicit approach is preferred as it allows for upgrade/recovery in the event the PA is unable to operate normally.

A typical upgrade flow would look like so:

1. Payload Developer uses Antaris Cloud Platform to uplink needed file(s) to Payload Application storage
2. A Telecommand is issued to Payload Controller to boot the Payload Application into an “upgrade” mode
3. On boot, the PA entrypoint.sh script detects the alternate mode of operation and applies any software/filesystem changes necessary within the VM
4. A Telecommand is issued to Payload Controller to shut the Payload Application down
5. Later Telecommands would then boot the PA into a nominal mode and resume normal operations.

The above flow would be modeled and scheduled as an "upgrade" Task.

If an upgrade needs to be applied to the entrypoint.sh script, out-of-band update processes can also be applied.

Using the Payload SDK
*********************

Proper Payload Application development relies heavily on the Payload SDK, which contains necessary libraries, tools, documentation, and other assets.
Specifically, the SDK includes:

* Documentation regarding how to use the SDK
* Fully-functional example Payload Applications
* Base Payload Application VM and container images
* Automation tools for Payload Application packaging
* Python and C++ Payload Interface bindings
* Template Payload Application entrypoint.sh scripts
* Sample Payload Application configs and runtime artifacts

A developer MUST use the SDK libraries and base images to automate many lower level details of PA operation. Current language support is limited to Python and C++, but support for other languages is planned.

Developer Deliverables
======================

Developers are expected to deliver fully-packaged Payload Applications to the Antaris Cloud Platform for incorporation into SatOS.
During the operational phase of a satellite, developers may deliver software packages instead of full images to support modular upgrades.

Application Environment
******************************

Configuration
=============

Application configuration is provided via the readonly ``/opt/antaris/app/`` directory. These files include:

* **config.json** contains a JSON-encoded config file, constructed by the PC to help automate PA configuration
* **mode** contains the current application mode. This file is typically read by the PA entrypoint script to influence PA startup behavior

Both of these files are managed by the system and are readonly to the running application processes.

Compute & Storage
=================

All Payload Applications are deployed as Virtual Machines (VMs). CPU and memory resources are configured within the Antaris Cloud Platform during satellite configuration. Storage capacity is also pre-configured.

All storage is persistent and will maintain state across reboots. Access to storage is provided via the following filesystem mounts:

* ``/opt/antaris/outbound/``: contains files produced by the PA that are intended for downlink during a ground station contact
* ``/opt/antaris/inbound/``: contains files uplinked and made available to the PA. The PA has readonly access of this folder.
* ``/opt/antaris/workspace/``: available to be used as a scratch space or sandbox, supporting active operation of a PA. The PA has full read/write permissions.

Network
=======
Each Payload Application receives a unique IP Address, as do any associated Payload Devices. The Payload Controller and an NTP server are also available over this network. The values assigned to these resources are defined in the PA config file, and should be accessed via the SDK library.
