# Data Center IEEE1588 PTP Profile 

(DC-PTP Profile)



## Introduction

Note: This document is work-in-progress and the DC-PTP Profile will evolve based on discussions and decisions with the extended team. 

The intent of this document is to define a new PTP (Precision Timing Protocol) profile, entitled *DC-PTP Profile*, for the purpose of distributing accurate and precise time within a data center infrastructure. Currently, the telecom (ITU-T), power (IEC), automotive (AVNU) and video (SMPTE) industries have each defined their own PTP profile.  To date, there is no PTP profile defined for Data Center. The proposal could eventually be submitted as a contribution to OCP and if the technology is demonstrated to be viable.

This document defines the PTP profile for time synchronization in a data center environment, providing time alignment across a set network elements, from a primary reference time clock to a large set of servers.

## 1. Profile Definition


A PTP profile is a set of required options, prohibited options, and the ranges and defaults of configurable attributes. According to IEEE 1588-2008, a profile should define the following:

* Best master clock algorithm options
* Configuration management options
* Path delay measurement option (delay request-response or peer delay)
* Range and default values of all configurable attributes and dataset members
* Transport mechanisms required, permitted, or prohibited
* Node types required, permitted, or prohibited
* It also allows to extend the standard


There are additional details that must be specified.  Some of those might be outside the scope of 1588-2008.

* End-to-end network architecture and network elements
* Clock requirements, time error budget
* End-to-end network limits and measurable metrics
* Failure scenarios and holdover accuracy
* Additional functions such syntonization and Synchronous Ethernet
* Classes of service and prioritization of PTP messages
* Operations and management of synchronization tree
* Procedures used for testing and verify conformance


A profile that defines the overall synchronization topology and architecture, required options and configurable attributes is also necessary.

## 2. High-Level Network Deployment Architecture


The figure below shows a Hypothetical Reference Model (HRM) for the purpose of:

* defining where the different clocks reside and protocol aspects
* establishing reasonable worst performance specification and network limits
* defining demarcation interfaces and measurable interfaces

Note1:  This is work-in-progress. A detailed budget of the network limits will be necessary
Note2:  There as been talk to deploy the DC-GM into HGRID

