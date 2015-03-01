// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

#include <mpi.h>
#include "ospray/common/OSPCommon.h"

// iw: for testing: whether to use a distributed frame buffer
// (DFB). if on, we use a DFB and accumulate where the tiles are
// owned; if not, we use a localfb at the host, and send tiles back in
// the load balancer
#define USE_DFB 1

/*! helper macro that checks the return value of all MPI_xxx(...)
    calls via MPI_CALL(xxx(...)).  */
#define MPI_CALL(a) { int rc = MPI_##a; if (rc != MPI_SUCCESS) throw std::runtime_error("MPI call returned error"); }

namespace ospray {

  /*! Helper class for MPI Programming */
  namespace mpi {
    //! abstraction for an MPI group. 
    /*! it's the responsiblity of the respective mpi setup routines to
      fill in the proper values */
    struct Group {
      /*! whether the current process/thread is a member of this
        gorup */
      bool     containsMe; 
      /*! communictor for this group. intercommunicator if i'm a
        member of this gorup; else it's an intracommunicator */
      MPI_Comm comm; 
      /*! my rank in this group if i'm a member; else set to
        MPI_ROOT */
      int rank; 
      /*! size of this group if i'm a member, else size of remote
        group this intracommunicaotr refers to */
      int size; 

      Group() : size(-1), rank(-1), comm(MPI_COMM_NULL), containsMe(false) {};
      void makeIntercomm() 
      { MPI_Comm_rank(comm,&rank); MPI_Comm_size(comm,&size); containsMe = true; }
      void makeIntracomm()
      { containsMe = false; rank = MPI_ROOT; MPI_Comm_remote_size(comm,&size); }

      /*! perform a MPI_barrier on this communicator */
      void barrier() { MPI_CALL(Barrier(comm)); }
    };

    extern Group world; //! MPI_COMM_WORLD
    extern Group app; /*! for workers: intracommunicator to app
                        for app: intercommunicator among app processes
                      */
    extern Group worker; /*!< group of all ospray workers (often the
                           world root is reserved for either app or
                           load balancing, and not part of the worker
                           group */

    void init(int *ac, const char **av);
  };

} // ::ospray
