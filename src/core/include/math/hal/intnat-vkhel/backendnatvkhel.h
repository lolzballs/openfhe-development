//==================================================================================
// BSD 2-Clause License
//
// Copyright (c) 2014-2022, NJIT, Duality Technologies Inc. and other contributors
//
// All rights reserved.
//
// Author TPOC: contact@openfhe.org
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//==================================================================================

/*
  This file contains the definitions for the VKHEL accelerated native math backend
 */

#ifndef SRC_CORE_INCLUDE_MATH_HAL_INTNATVKHEL_BACKENDNAT_H_
#define SRC_CORE_INCLUDE_MATH_HAL_INTNATVKHEL_BACKENDNAT_H_

#include "math/hal/intnat-vkhel/ubintnatvkhel.h"
#include "math/hal/intnat-vkhel/mubintvecnatvkhel.h"
#include "math/hal/intnat-vkhel/transformnatvkhel.h"

#include "math/hal/basicint.h"

#if NATIVEINT != 64
    #error "Building with VKHEL optimizations requires NATIVE_SIZE == 64"
#endif

namespace lbcrypto {

using NativeInteger = intnatvkhel::NativeInteger;
using NativeVector  = intnatvkhel::NativeVector;

}  // namespace lbcrypto

// Promote to global namespace
using NativeInteger = lbcrypto::NativeInteger;
using NativeVector  = lbcrypto::NativeVector;

#endif /* SRC_CORE_INCLUDE_MATH_HAL_INTNATVKHEL_BACKENDNAT_H_ */
