# Install script for directory: /Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "RelWithDebInfo")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set path to fallback-tool for dependency-resolution.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "devel" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/workflow" TYPE FILE RENAME "workflow-config.cmake" FILES "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/mac_build/config.toinstall.cmake")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "devel" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/workflow" TYPE FILE FILES "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/mac_build/workflow-config-version.cmake")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "devel" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/workflow" TYPE FILE FILES
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/protocol/ProtocolMessage.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/protocol/http_parser.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/protocol/HttpMessage.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/protocol/HttpUtil.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/protocol/redis_parser.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/protocol/RedisMessage.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/protocol/mysql_stream.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/protocol/MySQLMessage.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/protocol/MySQLMessage.inl"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/protocol/MySQLResult.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/protocol/MySQLResult.inl"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/protocol/MySQLUtil.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/protocol/mysql_parser.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/protocol/mysql_types.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/protocol/mysql_byteorder.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/protocol/PackageWrapper.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/protocol/SSLWrapper.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/protocol/dns_types.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/protocol/dns_parser.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/protocol/DnsMessage.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/protocol/DnsUtil.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/protocol/TLVMessage.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/protocol/ConsulDataTypes.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/server/WFServer.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/server/WFDnsServer.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/server/WFHttpServer.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/server/WFRedisServer.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/server/WFMySQLServer.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/client/WFHttpChunkedClient.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/client/WFMySQLConnection.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/client/WFRedisSubscriber.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/client/WFConsulClient.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/client/WFDnsClient.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/manager/DnsCache.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/manager/WFGlobal.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/manager/UpstreamManager.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/manager/RouteManager.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/manager/EndpointParams.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/manager/WFFuture.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/manager/WFFacilities.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/manager/WFFacilities.inl"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/util/json_parser.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/util/EncodeStream.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/util/LRUCache.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/util/StringUtil.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/util/URIParser.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/factory/WFConnection.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/factory/WFTask.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/factory/WFTask.inl"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/factory/WFGraphTask.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/factory/WFTaskError.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/factory/WFTaskFactory.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/factory/WFTaskFactory.inl"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/factory/WFAlgoTaskFactory.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/factory/WFAlgoTaskFactory.inl"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/factory/Workflow.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/factory/WFOperator.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/factory/WFResourcePool.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/factory/WFMessageQueue.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/factory/HttpTaskImpl.inl"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/factory/RedisTaskImpl.inl"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/nameservice/WFNameService.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/nameservice/WFDnsResolver.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/nameservice/WFServiceGovernance.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/nameservice/UpstreamPolicies.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/kernel/CommRequest.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/kernel/CommScheduler.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/kernel/Communicator.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/kernel/SleepRequest.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/kernel/ExecRequest.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/kernel/IORequest.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/kernel/Executor.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/kernel/list.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/kernel/mpoller.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/kernel/poller.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/kernel/msgqueue.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/kernel/rbtree.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/kernel/SubTask.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/kernel/thrdpool.h"
    "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/src/kernel/IOService_thread.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "devel" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/doc/workflow-1.0.0" TYPE FILE FILES "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/README.md")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/mac_build/src/cmake_install.cmake")

endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/mac_build/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
if(CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_COMPONENT MATCHES "^[a-zA-Z0-9_.+-]+$")
    set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
  else()
    string(MD5 CMAKE_INST_COMP_HASH "${CMAKE_INSTALL_COMPONENT}")
    set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INST_COMP_HASH}.txt")
    unset(CMAKE_INST_COMP_HASH)
  endif()
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/Users/lee1y/Desktop/Industrial-Cloud-Storage-System/workflow/mac_build/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
