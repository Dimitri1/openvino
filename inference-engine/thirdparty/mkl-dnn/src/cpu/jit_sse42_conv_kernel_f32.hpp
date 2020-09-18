/*******************************************************************************
* Copyright 2017-2018 Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

#ifndef JIT_SSE42_CONV_KERNEL_F32_HPP
#define JIT_SSE42_CONV_KERNEL_F32_HPP

#include "c_types_map.hpp"
#include "cpu_memory.hpp"
#include "jit_generator.hpp"
#include "jit_primitive_conf.hpp"
#include "jit_uni_eltwise.hpp"
#include "jit_uni_depthwise.hpp"

namespace mkldnn {
namespace impl {
namespace cpu {

struct jit_sse42_conv_fwd_kernel_f32: public jit_generator {
    jit_sse42_conv_fwd_kernel_f32(jit_conv_conf_t ajcp, jit_conv_conf_t ajcp_dw,
            const primitive_attr_t &attr)
        : jcp(ajcp), jcp_dw(ajcp_dw), attr_(attr)
    {
        this->generate();
        jit_ker = (void (*)(jit_conv_call_s *))this->getCode();
    }

    ~jit_sse42_conv_fwd_kernel_f32() {
        for (auto inj : eltwise_injectors)
            delete inj;
        eltwise_injectors.clear();

        for (auto inj : depthwise_injectors)
            delete inj;
        depthwise_injectors.clear();
    }

    static bool post_ops_ok(jit_conv_conf_t &jcp,
            const primitive_attr_t &attr);

    static status_t init_conf(jit_conv_conf_t &jcp,
            const convolution_desc_t &cd, const memory_desc_wrapper &src_d,
            const memory_desc_wrapper &weights_d,
            const memory_desc_wrapper &dst_d, const primitive_attr_t &attr);
    static void init_scratchpad(memory_tracking::registrar_t &scratchpad,
            const jit_conv_conf_t &jcp, const jit_conv_conf_t &jcp_dw = jit_conv_conf_t());

    DECLARE_CPU_JIT_AUX_FUNCTIONS(jit_sse42_conv_fwd_kernel_f32)
    jit_conv_conf_t jcp;
    jit_conv_conf_t jcp_dw;
    const primitive_attr_t &attr_;
    void (*jit_ker)(jit_conv_call_s *);

private:
    using reg64_t = const Xbyak::Reg64;
    reg64_t reg_input = rax;
    reg64_t aux_reg_input = r8;
    reg64_t reg_kernel = rdx;
    reg64_t aux_reg_kernel = r9;
    reg64_t reg_output = rsi;
    reg64_t reg_bias = rbx;

    reg64_t kj = r10;
    reg64_t oi_iter = r11;
    reg64_t ki_iter = r12;
    reg64_t reg_kh = abi_not_param1;
    reg64_t simd_iter = r15;
    reg64_t reg_oc_blocks = r14;
    reg64_t imm_addr64 = reg_oc_blocks;
    Xbyak::Reg32 reg_ci_flag = r13d;

    reg64_t reg_d_weights = imm_addr64;
    reg64_t reg_d_bias = ki_iter;
    reg64_t reg_oc_off = abi_param1;

    nstl::vector<jit_uni_eltwise_injector_f32<sse42>*> eltwise_injectors;
    nstl::vector<jit_uni_depthwise_injector_f32<sse42>*> depthwise_injectors;

    inline void oh_step_unroll_kw(int ur_w, int pad_l, int pad_r,
            int oc_blocks);
    inline void oh_step_nopad(int ur_w, int pad_l, int pad_r, int oc_blocks);
    inline void width_blk_step(int ur_w, int pad_l, int pad_r, int oc_blocks);
    inline void solve_common(int oc_blocks);

    void generate();
};

}
}
}

#endif