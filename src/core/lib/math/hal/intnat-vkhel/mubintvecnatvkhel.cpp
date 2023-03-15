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
  This code provides basic arithmetic functionality for vectors of VKHEL accelerated native integers
 */

//==================================================================================
// This file is included only if WITH_VKHEL is set to ON in CMakeLists.txt
//==================================================================================
#ifdef WITH_VKHEL

    #include "math/hal.h"
    #include "math/hal/intnat-vkhel/mubintvecnatvkhel.h"
    #include "math/nbtheory.h"
    #include "utils/debug.h"
    #include "utils/serializable.h"

namespace intnatvkhel {

// CONSTRUCTORS

template <class IntegerType>
NativeVectorT<IntegerType>::NativeVectorT() : m_vkVector(nullptr), m_dirtyVulkan(false), m_dirtyHost(false) {
    this->m_vkCtx = lbcrypto::VkhelCtxManager::getContext();
}

template <class IntegerType>
NativeVectorT<IntegerType>::NativeVectorT(usint length) : m_dirtyVulkan(false), m_dirtyHost(false) {
    this->m_data.resize(length);
    this->m_vkCtx    = lbcrypto::VkhelCtxManager::getContext();
    this->m_vkVector = vkhel_vector_create(m_vkCtx.get(), length);
}

template <class IntegerType>
NativeVectorT<IntegerType>::NativeVectorT(usint length, const IntegerType& modulus)
    : m_dirtyVulkan(false), m_dirtyHost(false) {
    if (modulus.GetMSB() > MAX_MODULUS_SIZE) {
        OPENFHE_THROW(lbcrypto::not_available_error,
                      "NativeVectorT supports only modulus size <=  " + std::to_string(MAX_MODULUS_SIZE) + " bits");
    }
    this->SetModulus(modulus);
    this->m_data.resize(length);

    this->m_vkCtx    = lbcrypto::VkhelCtxManager::getContext();
    this->m_vkVector = vkhel_vector_create(m_vkCtx.get(), length);
}

template <class IntegerType>
NativeVectorT<IntegerType>::NativeVectorT(const NativeVectorT& bigVector) : m_dirtyVulkan(false), m_dirtyHost(false) {
    // update host/vk vectors before we copy
    bigVector.updateHost();
    bigVector.updateVulkan();

    m_modulus  = bigVector.m_modulus;
    m_data     = bigVector.m_data;
    m_vkCtx    = bigVector.m_vkCtx;
    m_vkVector = vkhel_vector_dup(bigVector.m_vkVector);
}

template <class IntegerType>
NativeVectorT<IntegerType>::NativeVectorT(NativeVectorT&& bigVector)
    : m_dirtyVulkan(bigVector.m_dirtyVulkan), m_dirtyHost(bigVector.m_dirtyHost) {
    m_data               = std::move(bigVector.m_data);
    m_modulus            = bigVector.m_modulus;
    m_vkCtx              = std::move(bigVector.m_vkCtx);
    m_vkVector           = bigVector.m_vkVector;
    bigVector.m_vkVector = nullptr;
}

template <class IntegerType>
NativeVectorT<IntegerType>::NativeVectorT(usint length, const IntegerType& modulus,
                                          std::initializer_list<std::string> rhs)
    : m_dirtyVulkan(false), m_dirtyHost(false) {
    this->SetModulus(modulus);
    this->m_data.resize(length);
    usint len = rhs.size();
    for (usint i = 0; i < m_data.size(); i++) {  // this loops over each entry
        if (i < len) {
            m_data[i] = IntegerType(*(rhs.begin() + i)) % m_modulus;
        }
        else {
            m_data[i] = IntegerType(0);
        }
    }

    this->m_vkCtx    = lbcrypto::VkhelCtxManager::getContext();
    this->m_vkVector = vkhel_vector_create(m_vkCtx.get(), length);
    this->updateVkVector();
}

template <class IntegerType>
NativeVectorT<IntegerType>::NativeVectorT(usint length, const IntegerType& modulus, std::initializer_list<uint64_t> rhs)
    : m_dirtyVulkan(false), m_dirtyHost(false) {
    this->SetModulus(modulus);
    this->m_data.resize(length);
    usint len = rhs.size();
    for (usint i = 0; i < m_data.size(); i++) {  // this loops over each entry
        if (i < len) {
            m_data[i] = IntegerType(*(rhs.begin() + i)) % m_modulus;
        }
        else {
            m_data[i] = IntegerType(0);
        }
    }

    this->m_vkCtx    = lbcrypto::VkhelCtxManager::getContext();
    this->m_vkVector = vkhel_vector_create(m_vkCtx.get(), length);
    this->updateVkVector();
}

template <class IntegerType>
NativeVectorT<IntegerType>::~NativeVectorT() {
    if (m_vkVector) {
        vkhel_vector_destroy(m_vkVector);
    }
}

// ASSIGNMENT OPERATORS

template <class IntegerType>
const NativeVectorT<IntegerType>& NativeVectorT<IntegerType>::operator=(const NativeVectorT& rhs) {
    if (this != &rhs) {
        rhs.updateHost();
        rhs.updateVulkan();

        if (this->m_data.size() == rhs.m_data.size()) {
            for (usint i = 0; i < m_data.size(); i++) {
                this->m_data[i] = rhs.m_data[i];
            }
        }
        else {
            m_data = rhs.m_data;

            if (m_vkVector != nullptr) {
                vkhel_vector_destroy(m_vkVector);
                m_vkVector = nullptr;
            }
        }

        if (m_vkVector != nullptr) {
            // TODO: copy from rhs instead of reallocating
            vkhel_vector_destroy(m_vkVector);
        }
        m_vkVector    = vkhel_vector_dup(rhs.m_vkVector);
        m_vkCtx       = rhs.m_vkCtx;
        m_modulus     = rhs.m_modulus;
        m_dirtyVulkan = false;
        m_dirtyHost   = false;
    }

    return *this;
}

template <class IntegerType>
NativeVectorT<IntegerType>& NativeVectorT<IntegerType>::operator=(NativeVectorT&& rhs) {
    if (this != &rhs) {
        m_data    = std::move(rhs.m_data);
        m_modulus = rhs.m_modulus;

        m_vkCtx = std::move(rhs.m_vkCtx);
        if (m_vkVector) {
            vkhel_vector_destroy(m_vkVector);
        }

        m_vkVector     = rhs.m_vkVector;
        rhs.m_vkVector = nullptr;
        m_dirtyVulkan  = rhs.m_dirtyVulkan;
        m_dirtyHost    = rhs.m_dirtyHost;
    }
    return *this;
}

template <class IntegerType>
const NativeVectorT<IntegerType>& NativeVectorT<IntegerType>::operator=(std::initializer_list<std::string> rhs) {
    usint len = rhs.size();
    for (usint i = 0; i < m_data.size(); i++) {  // this loops over each tower
        if (i < len) {
            if (m_modulus != 0) {
                m_data[i] = IntegerType(*(rhs.begin() + i)) % m_modulus;
            }
            else {
                m_data[i] = IntegerType(*(rhs.begin() + i));
            }
        }
        else {
            m_data[i] = 0;
        }
    }
    m_dirtyVulkan = true;
    return *this;
}

template <class IntegerType>
const NativeVectorT<IntegerType>& NativeVectorT<IntegerType>::operator=(std::initializer_list<uint64_t> rhs) {
    usint len = rhs.size();
    for (usint i = 0; i < m_data.size(); i++) {  // this loops over each tower
        if (i < len) {
            if (m_modulus != 0) {
                m_data[i] = IntegerType(*(rhs.begin() + i)) % m_modulus;
            }
            else {
                m_data[i] = IntegerType(*(rhs.begin() + i));
            }
        }
        else {
            m_data[i] = 0;
        }
    }
    m_dirtyVulkan = true;
    return *this;
}

// ACCESSORS

template <class IntegerType>
void NativeVectorT<IntegerType>::SetModulus(const IntegerType& value) {
    if (value.GetMSB() > MAX_MODULUS_SIZE) {
        OPENFHE_THROW(lbcrypto::not_available_error,
                      "NativeVectorT supports only modulus size <=  " + std::to_string(MAX_MODULUS_SIZE) + " bits");
    }
    this->m_modulus = value;
}

/**Switches the integers in the vector to values corresponding to the new
 * modulus.
 * Algorithm: Integer i, Old Modulus om, New Modulus nm,
 * delta = abs(om-nm):
 *  Case 1: om < nm
 *    if i > om/2
 *      i' = i + delta
 *  Case 2: om > nm
 *    i > om/2 i' = i-delta
 */
template <class IntegerType>
void NativeVectorT<IntegerType>::SwitchModulus(const IntegerType& newModulus) {
    updateVulkan();
    IntegerType oldModulus(this->m_modulus);
    IntegerType oldModulusByTwo(oldModulus >> 1);
    IntegerType diff((oldModulus > newModulus) ? (oldModulus - newModulus) : (newModulus - oldModulus));

    if (newModulus > oldModulus) {
        vkhel_vector_elemgtadd(this->m_vkVector, this->m_vkVector, oldModulusByTwo.ConvertToInt(), diff.ConvertToInt());
    }
    else {  // newModulus <= oldModulus
        vkhel_vector_elemgtsub(this->m_vkVector, this->m_vkVector, oldModulusByTwo.ConvertToInt(),
                               diff.ConvertToInt() % newModulus.ConvertToInt(), newModulus.ConvertToInt());
    }
    this->SetModulus(newModulus);
    m_dirtyHost = true;
}

template <class IntegerType>
const IntegerType& NativeVectorT<IntegerType>::GetModulus() const {
    return this->m_modulus;
}

// MODULAR ARITHMETIC OPERATIONS

template <class IntegerType>
NativeVectorT<IntegerType> NativeVectorT<IntegerType>::Mod(const IntegerType& modulus) const {
    NativeVectorT ans(*this);
    vkhel_vector_elemmod(this->m_vkVector, ans.m_vkVector, modulus.ConvertToInt(), this->GetModulus().ConvertToInt());
    ans.m_dirtyHost = true;
    return ans;
}

template <class IntegerType>
const NativeVectorT<IntegerType>& NativeVectorT<IntegerType>::ModEq(const IntegerType& modulus) {
    updateVulkan();
    vkhel_vector_elemmod(this->m_vkVector, this->m_vkVector, modulus.ConvertToInt(), this->GetModulus().ConvertToInt());
    m_dirtyHost = true;
    return *this;
}

template <class IntegerType>
NativeVectorT<IntegerType> NativeVectorT<IntegerType>::ModAdd(const IntegerType& b) const {
    IntegerType modulus = this->m_modulus;
    IntegerType bLocal  = b;
    NativeVectorT ans(*this);
    if (bLocal > m_modulus) {
        bLocal.ModEq(modulus);
    }
    ans.updateHost();
    // TODO: replace with an vk elemadd kernel
    for (usint i = 0; i < this->m_data.size(); i++) {
        ans.m_data[i].ModAddFastEq(bLocal, modulus);
    }
    ans.m_dirtyVulkan = true;
    return ans;
}

template <class IntegerType>
const NativeVectorT<IntegerType>& NativeVectorT<IntegerType>::ModAddEq(const IntegerType& b) {
    IntegerType modulus = this->m_modulus;
    IntegerType bLocal  = b;
    if (bLocal > m_modulus) {
        bLocal.ModEq(modulus);
    }
    this->updateHost();
    // TODO: replace with an vk elemadd kernel
    for (usint i = 0; i < this->m_data.size(); i++) {
        this->m_data[i].ModAddFastEq(bLocal, modulus);
    }
    this->m_dirtyVulkan = true;
    return *this;
}

template <class IntegerType>
NativeVectorT<IntegerType> NativeVectorT<IntegerType>::ModAddAtIndex(usint i, const IntegerType& b) const {
    if (i > this->GetLength() - 1) {
        std::string errMsg = "ubintnat::ModAddAtIndex. Index is out of range. i = " + std::to_string(i);
        OPENFHE_THROW(lbcrypto::math_error, errMsg);
    }
    NativeVectorT ans(*this);
    ans.updateHost();
    ans.m_data[i].ModAddEq(b, this->m_modulus);
    ans.m_dirtyVulkan = true;
    return ans;
}

template <class IntegerType>
const NativeVectorT<IntegerType>& NativeVectorT<IntegerType>::ModAddAtIndexEq(usint i, const IntegerType& b) {
    if (i > this->GetLength() - 1) {
        std::string errMsg = "ubintnat::ModAddAtIndex. Index is out of range. i = " + std::to_string(i);
        OPENFHE_THROW(lbcrypto::math_error, errMsg);
    }
    this->updateHost();
    this->m_data[i].ModAddEq(b, this->m_modulus);
    this->m_dirtyVulkan = true;
    return *this;
}

template <class IntegerType>
NativeVectorT<IntegerType> NativeVectorT<IntegerType>::ModAdd(const NativeVectorT& b) const {
    if ((this->m_data.size() != b.m_data.size()) || this->m_modulus != b.m_modulus) {
        OPENFHE_THROW(lbcrypto::math_error, "ModAdd called on NativeVectorT's with different parameters.");
    }
    NativeVectorT ans(*this);
    b.updateHost();
    IntegerType modulus = this->m_modulus;
    // TODO: replace with an vk elemadd kernel
    for (usint i = 0; i < ans.m_data.size(); i++) {
        ans.m_data[i].ModAddFastEq(b[i], modulus);
    }
    ans.m_dirtyVulkan = true;
    return ans;
}

template <class IntegerType>
const NativeVectorT<IntegerType>& NativeVectorT<IntegerType>::ModAddEq(const NativeVectorT& b) {
    if ((this->m_data.size() != b.m_data.size()) || this->m_modulus != b.m_modulus) {
        OPENFHE_THROW(lbcrypto::math_error, "ModAddEq called on NativeVectorT's with different parameters.");
    }
    IntegerType modulus = this->m_modulus;
    this->updateHost();
    b.updateHost();
    // TODO: replace with an vk elemadd kernel
    for (usint i = 0; i < this->m_data.size(); i++) {
        this->m_data[i].ModAddFastEq(b[i], modulus);
    }
    this->m_dirtyVulkan = true;
    return *this;
}

template <class IntegerType>
NativeVectorT<IntegerType> NativeVectorT<IntegerType>::ModSub(const IntegerType& b) const {
    NativeVectorT ans(*this);
    ans.updateHost();
    // TODO: replace with an vk elemsub kernel
    for (usint i = 0; i < this->m_data.size(); i++) {
        ans.m_data[i].ModSubEq(b, this->m_modulus);
    }
    ans.m_dirtyVulkan = true;
    return ans;
}

template <class IntegerType>
const NativeVectorT<IntegerType>& NativeVectorT<IntegerType>::ModSubEq(const IntegerType& b) {
    this->updateHost();
    // TODO: replace with an vk elemsub kernel
    for (usint i = 0; i < this->m_data.size(); i++) {
        this->m_data[i].ModSubEq(b, this->m_modulus);
    }
    this->m_dirtyVulkan = true;
    return *this;
}

template <class IntegerType>
NativeVectorT<IntegerType> NativeVectorT<IntegerType>::ModSub(const NativeVectorT& b) const {
    if ((this->m_data.size() != b.m_data.size()) || this->m_modulus != b.m_modulus) {
        OPENFHE_THROW(lbcrypto::math_error, "ModSub called on NativeVectorT's with different parameters.");
    }
    NativeVectorT ans(*this);
    b.updateHost();
    // TODO: replace with an vk elemsub kernel
    for (usint i = 0; i < ans.m_data.size(); i++) {
        ans.m_data[i].ModSubFastEq(b.m_data[i], this->m_modulus);
    }
    ans.m_dirtyVulkan = true;
    return ans;
}

template <class IntegerType>
const NativeVectorT<IntegerType>& NativeVectorT<IntegerType>::ModSubEq(const NativeVectorT& b) {
    if ((this->m_data.size() != b.m_data.size()) || this->m_modulus != b.m_modulus) {
        OPENFHE_THROW(lbcrypto::math_error, "ModSubEq called on NativeVectorT's with different parameters.");
    }
    this->updateHost();
    b.updateHost();
    // TODO: replace with an vk elemsub kernel
    for (usint i = 0; i < this->m_data.size(); i++) {
        this->m_data[i].ModSubFastEq(b.m_data[i], this->m_modulus);
    }
    this->m_dirtyVulkan = true;
    return *this;
}

template <class IntegerType>
NativeVectorT<IntegerType> NativeVectorT<IntegerType>::ModMul(const IntegerType& b) const {
    NativeVectorT ans(*this);
    IntegerType modulus = this->m_modulus;
    IntegerType bLocal  = b;
    if (bLocal >= modulus) {
        bLocal.ModEq(modulus);
    }
    IntegerType bPrec = bLocal.PrepModMulConst(modulus);
    ans.updateHost();
    // TODO: replace with an vk_elemmul kernel
    for (usint i = 0; i < this->m_data.size(); i++) {
        ans.m_data[i].ModMulFastConstEq(bLocal, modulus, bPrec);
    }
    ans.m_dirtyVulkan = true;
    return ans;
}

template <class IntegerType>
const NativeVectorT<IntegerType>& NativeVectorT<IntegerType>::ModMulEq(const IntegerType& b) {
    IntegerType modulus = this->m_modulus;
    IntegerType bLocal  = b;
    if (bLocal >= modulus) {
        bLocal.ModEq(modulus);
    }
    IntegerType bPrec = bLocal.PrepModMulConst(modulus);
    this->updateHost();
    // TODO: replace with an vk_elemmul kernel
    for (usint i = 0; i < this->m_data.size(); i++) {
        this->m_data[i].ModMulFastConstEq(bLocal, modulus, bPrec);
    }
    this->m_dirtyVulkan = true;
    return *this;
}

template <class IntegerType>
NativeVectorT<IntegerType> NativeVectorT<IntegerType>::ModMul(const NativeVectorT& b) const {
    if ((this->m_data.size() != b.m_data.size()) || this->m_modulus != b.m_modulus) {
        OPENFHE_THROW(lbcrypto::math_error, "ModMul called on NativeVectorT's with different parameters.");
    }
    NativeVectorT ans(*this);
    b.updateVulkan();

    vkhel_vector_elemmul(this->m_vkVector, b.m_vkVector, ans.m_vkVector, m_modulus.ConvertToInt());
    ans.m_dirtyHost = true;
    return ans;
}

template <class IntegerType>
const NativeVectorT<IntegerType>& NativeVectorT<IntegerType>::ModMulEq(const NativeVectorT& b) {
    if ((this->m_data.size() != b.m_data.size()) || this->m_modulus != b.m_modulus) {
        OPENFHE_THROW(lbcrypto::math_error, "ModMulEq called on NativeVectorT's with different parameters.");
    }
    this->updateVulkan();
    b.updateVulkan();

    vkhel_vector_elemmul(this->m_vkVector, b.m_vkVector, this->m_vkVector, m_modulus.ConvertToInt());
    this->m_dirtyHost = true;
    return *this;
}

template <class IntegerType>
NativeVectorT<IntegerType> NativeVectorT<IntegerType>::ModByTwo() const {
    return this->Mod(2);
}

template <class IntegerType>
const NativeVectorT<IntegerType>& NativeVectorT<IntegerType>::ModByTwoEq() {
    return this->ModEq(2);
}

template <class IntegerType>
NativeVectorT<IntegerType> NativeVectorT<IntegerType>::ModExp(const IntegerType& b) const {
    NativeVectorT ans(*this);
    // TODO: replace with elemexp
    for (usint i = 0; i < this->m_data.size(); i++) {
        ans.m_data[i].ModExpEq(b, this->m_modulus);
    }
    ans.m_dirtyVulkan = true;
    return ans;
}

template <class IntegerType>
const NativeVectorT<IntegerType>& NativeVectorT<IntegerType>::ModExpEq(const IntegerType& b) {
    this->updateHost();
    // TODO: replace with elemexp
    for (usint i = 0; i < this->m_data.size(); i++) {
        this->m_data[i].ModExpEq(b, this->m_modulus);
    }
    this->m_dirtyVulkan = true;
    return *this;
}

template <class IntegerType>
NativeVectorT<IntegerType> NativeVectorT<IntegerType>::ModInverse() const {
    NativeVectorT ans(*this);
    // TODO: replace with eleminv
    for (usint i = 0; i < this->m_data.size(); i++) {
        ans.m_data[i].ModInverseEq(this->m_modulus);
    }
    ans.m_dirtyVulkan = true;
    return ans;
}

template <class IntegerType>
const NativeVectorT<IntegerType>& NativeVectorT<IntegerType>::ModInverseEq() {
    this->updateHost();
    // TODO: replace with eleminv
    for (usint i = 0; i < this->m_data.size(); i++) {
        this->m_data[i].ModInverseEq(this->m_modulus);
    }
    this->m_dirtyVulkan = true;
    return *this;
}

template <class IntegerType>
NativeVectorT<IntegerType> NativeVectorT<IntegerType>::MultWithOutMod(const NativeVectorT& b) const {
    if ((this->m_data.size() != b.m_data.size()) || this->m_modulus != b.m_modulus) {
        OPENFHE_THROW(lbcrypto::math_error, "ModMul called on NativeVectorT's with different parameters.");
    }
    NativeVectorT ans(*this);
    b.updateHost();
    // TODO: replace with elmmul (nomod)
    for (usint i = 0; i < ans.m_data.size(); i++) {
        ans.m_data[i].MulEq(b.m_data[i]);
    }
    ans.m_dirtyVulkan = true;
    return ans;
}

template <class IntegerType>
NativeVectorT<IntegerType> NativeVectorT<IntegerType>::MultiplyAndRound(const IntegerType& p,
                                                                        const IntegerType& q) const {
    NativeVectorT ans(*this);
    IntegerType halfQ(this->m_modulus >> 1);
    // TODO: implement kernel for this
    for (usint i = 0; i < this->m_data.size(); i++) {
        if (ans.m_data[i] > halfQ) {
            IntegerType temp = this->m_modulus - ans.m_data[i];
            ans.m_data[i]    = this->m_modulus - temp.MultiplyAndRound(p, q);
        }
        else {
            ans.m_data[i].MultiplyAndRoundEq(p, q);
            ans.m_data[i].ModEq(this->m_modulus);
        }
    }
    ans.m_dirtyVulkan = true;
    return ans;
}

template <class IntegerType>
const NativeVectorT<IntegerType>& NativeVectorT<IntegerType>::MultiplyAndRoundEq(const IntegerType& p,
                                                                                 const IntegerType& q) {
    this->updateHost();
    IntegerType halfQ(this->m_modulus >> 1);
    // TODO: implement kernel for this
    for (usint i = 0; i < this->m_data.size(); i++) {
        if (this->m_data[i] > halfQ) {
            IntegerType temp = this->m_modulus - this->m_data[i];
            this->m_data[i]  = this->m_modulus - temp.MultiplyAndRound(p, q);
        }
        else {
            this->m_data[i].MultiplyAndRoundEq(p, q);
            this->ModEq(this->m_modulus);
        }
    }
    this->m_dirtyVulkan = true;
    return *this;
}

template <class IntegerType>
NativeVectorT<IntegerType> NativeVectorT<IntegerType>::DivideAndRound(const IntegerType& q) const {
    NativeVectorT ans(*this);
    IntegerType halfQ(this->m_modulus >> 1);
    // TODO: implement kernel for this
    for (usint i = 0; i < this->m_data.size(); i++) {
        if (ans.m_data[i] > halfQ) {
            IntegerType temp = this->m_modulus - ans.m_data[i];
            ans.m_data[i]    = this->m_modulus - temp.DivideAndRound(q);
        }
        else {
            ans.m_data[i].DivideAndRoundEq(q);
        }
    }
    ans.m_dirtyVulkan = true;
    return ans;
}

template <class IntegerType>
const NativeVectorT<IntegerType>& NativeVectorT<IntegerType>::DivideAndRoundEq(const IntegerType& q) {
    this->updateHost();
    IntegerType halfQ(this->m_modulus >> 1);
    // TODO: implement kernel for this
    for (usint i = 0; i < this->m_data.size(); i++) {
        if (this->m_data[i] > halfQ) {
            IntegerType temp = this->m_modulus - this->m_data[i];
            this->m_data[i]  = this->m_modulus - temp.DivideAndRound(q);
        }
        else {
            this->m_data[i].DivideAndRoundEq(q);
        }
    }
    this->m_dirtyVulkan = true;
    return *this;
}

// OTHER FUNCTIONS

// Gets the ind
template <class IntegerType>
NativeVectorT<IntegerType> NativeVectorT<IntegerType>::GetDigitAtIndexForBase(usint index, usint base) const {
    NativeVectorT ans(*this);
    for (usint i = 0; i < this->m_data.size(); i++) {
        ans.m_data[i] = IntegerType(ans.m_data[i].GetDigitAtIndexForBase(index, base));
    }
    ans.m_dirtyVulkan = true;
    return ans;
}

template class NativeVectorT<NativeInteger>;

}  // namespace intnatvkhel

#endif  // WITH_VKHEL
