// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#define ARROW_VERSION_MAJOR 12
#define ARROW_VERSION_MINOR 0
#define ARROW_VERSION_PATCH 1
#define ARROW_VERSION ((ARROW_VERSION_MAJOR * 1000) + ARROW_VERSION_MINOR) * 1000 + ARROW_VERSION_PATCH

#define ARROW_VERSION_STRING "12.0.1"

#define ARROW_SO_VERSION "1200"
#define ARROW_FULL_SO_VERSION "1200.1.0"

#define ARROW_CXX_COMPILER_ID "GNU"
#define ARROW_CXX_COMPILER_VERSION "8.3.0"
#define ARROW_CXX_COMPILER_FLAGS " -fdiagnostics-color=always"

#define ARROW_BUILD_TYPE "RELEASE"

#define ARROW_GIT_ID "ac8331f7b1f8b7efdb890c4dc50ce0efd1637778"
#define ARROW_GIT_DESCRIPTION "v8-9.0.257.17-83-gac8331f"

#define ARROW_PACKAGE_KIND ""

#define ARROW_COMPUTE
#define ARROW_CSV
/* #undef ARROW_CUDA */
#define ARROW_DATASET
#define ARROW_FILESYSTEM
/* #undef ARROW_FLIGHT */
/* #undef ARROW_FLIGHT_SQL */
#define ARROW_IPC
/* #undef ARROW_JEMALLOC */
/* #undef ARROW_JEMALLOC_VENDORED */
#define ARROW_JSON
/* #undef ARROW_ORC */
#define ARROW_PARQUET
/* #undef ARROW_SUBSTRAIT */

#define ARROW_GCS
#define ARROW_S3
#define ARROW_USE_NATIVE_INT128
/* #undef ARROW_WITH_MUSL */
/* #undef ARROW_WITH_OPENTELEMETRY */
/* #undef ARROW_WITH_UCX */

/* #undef GRPCPP_PP_INCLUDE */
