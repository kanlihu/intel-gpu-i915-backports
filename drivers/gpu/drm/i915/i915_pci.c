/*
 * Copyright © 2016 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 */

#include <linux/vga_switcheroo.h>

#include <drm/drm_color_mgmt.h>
#include <drm/drm_drv.h>
#include <drm/i915_pciids.h>

#include "gt/intel_gt.h"
#include "gt/intel_gt_regs.h"
#include "gt/intel_sa_media.h"
#include "gem/i915_gem_object_types.h"

#include "i915_driver.h"
#include "i915_drv.h"
#include "i915_pci.h"
#include "i915_reg.h"
#include "intel_pci_config.h"

#define PLATFORM(x) .platform = (x)
#define GEN(x) \
	.graphics.ver = (x), \
	.media.ver = (x), \
	.display.ver = (x)

#define I845_PIPE_OFFSETS \
	.display.pipe_offsets = { \
		[TRANSCODER_A] = PIPE_A_OFFSET,	\
	}, \
	.display.trans_offsets = { \
		[TRANSCODER_A] = TRANSCODER_A_OFFSET, \
	}

#define I9XX_PIPE_OFFSETS \
	.display.pipe_offsets = { \
		[TRANSCODER_A] = PIPE_A_OFFSET,	\
		[TRANSCODER_B] = PIPE_B_OFFSET, \
	}, \
	.display.trans_offsets = { \
		[TRANSCODER_A] = TRANSCODER_A_OFFSET, \
		[TRANSCODER_B] = TRANSCODER_B_OFFSET, \
	}

#define IVB_PIPE_OFFSETS \
	.display.pipe_offsets = { \
		[TRANSCODER_A] = PIPE_A_OFFSET,	\
		[TRANSCODER_B] = PIPE_B_OFFSET, \
		[TRANSCODER_C] = PIPE_C_OFFSET, \
	}, \
	.display.trans_offsets = { \
		[TRANSCODER_A] = TRANSCODER_A_OFFSET, \
		[TRANSCODER_B] = TRANSCODER_B_OFFSET, \
		[TRANSCODER_C] = TRANSCODER_C_OFFSET, \
	}

#define HSW_PIPE_OFFSETS \
	.display.pipe_offsets = { \
		[TRANSCODER_A] = PIPE_A_OFFSET,	\
		[TRANSCODER_B] = PIPE_B_OFFSET, \
		[TRANSCODER_C] = PIPE_C_OFFSET, \
		[TRANSCODER_EDP] = PIPE_EDP_OFFSET, \
	}, \
	.display.trans_offsets = { \
		[TRANSCODER_A] = TRANSCODER_A_OFFSET, \
		[TRANSCODER_B] = TRANSCODER_B_OFFSET, \
		[TRANSCODER_C] = TRANSCODER_C_OFFSET, \
		[TRANSCODER_EDP] = TRANSCODER_EDP_OFFSET, \
	}

#define CHV_PIPE_OFFSETS \
	.display.pipe_offsets = { \
		[TRANSCODER_A] = PIPE_A_OFFSET, \
		[TRANSCODER_B] = PIPE_B_OFFSET, \
		[TRANSCODER_C] = CHV_PIPE_C_OFFSET, \
	}, \
	.display.trans_offsets = { \
		[TRANSCODER_A] = TRANSCODER_A_OFFSET, \
		[TRANSCODER_B] = TRANSCODER_B_OFFSET, \
		[TRANSCODER_C] = CHV_TRANSCODER_C_OFFSET, \
	}

#define I845_CURSOR_OFFSETS \
	.display.cursor_offsets = { \
		[PIPE_A] = CURSOR_A_OFFSET, \
	}

#define I9XX_CURSOR_OFFSETS \
	.display.cursor_offsets = { \
		[PIPE_A] = CURSOR_A_OFFSET, \
		[PIPE_B] = CURSOR_B_OFFSET, \
	}

#define CHV_CURSOR_OFFSETS \
	.display.cursor_offsets = { \
		[PIPE_A] = CURSOR_A_OFFSET, \
		[PIPE_B] = CURSOR_B_OFFSET, \
		[PIPE_C] = CHV_CURSOR_C_OFFSET, \
	}

#define IVB_CURSOR_OFFSETS \
	.display.cursor_offsets = { \
		[PIPE_A] = CURSOR_A_OFFSET, \
		[PIPE_B] = IVB_CURSOR_B_OFFSET, \
		[PIPE_C] = IVB_CURSOR_C_OFFSET, \
	}

#define TGL_CURSOR_OFFSETS \
	.display.cursor_offsets = { \
		[PIPE_A] = CURSOR_A_OFFSET, \
		[PIPE_B] = IVB_CURSOR_B_OFFSET, \
		[PIPE_C] = IVB_CURSOR_C_OFFSET, \
		[PIPE_D] = TGL_CURSOR_D_OFFSET, \
	}

#define I9XX_COLORS \
	.display.color = { .gamma_lut_size = 256 }
#define I965_COLORS \
	.display.color = { .gamma_lut_size = 129, \
		   .gamma_lut_tests = DRM_COLOR_LUT_NON_DECREASING, \
	}
#define ILK_COLORS \
	.display.color = { .gamma_lut_size = 1024 }
#define IVB_COLORS \
	.display.color = { .degamma_lut_size = 1024, .gamma_lut_size = 1024 }
#define CHV_COLORS \
	.display.color = { \
		.degamma_lut_size = 65, .gamma_lut_size = 257, \
		.degamma_lut_tests = DRM_COLOR_LUT_NON_DECREASING, \
		.gamma_lut_tests = DRM_COLOR_LUT_NON_DECREASING, \
	}
#define GLK_COLORS \
	.display.color = { \
		.degamma_lut_size = 33, .gamma_lut_size = 1024, \
		.degamma_lut_tests = DRM_COLOR_LUT_NON_DECREASING | \
				     DRM_COLOR_LUT_EQUAL_CHANNELS, \
	}
#define ICL_COLORS \
	.display.color = { \
		.degamma_lut_size = 33, .gamma_lut_size = 262145, \
		.degamma_lut_tests = DRM_COLOR_LUT_NON_DECREASING | \
				     DRM_COLOR_LUT_EQUAL_CHANNELS, \
		.gamma_lut_tests = DRM_COLOR_LUT_NON_DECREASING, \
	}

#define LEGACY_CACHELEVEL \
	.cachelevel_to_pat = { \
		[I915_CACHE_NONE]   = 0, \
		[I915_CACHE_LLC]    = 1, \
		[I915_CACHE_L3_LLC] = 2, \
		[I915_CACHE_WT]     = 3, \
	}

#define TGL_CACHELEVEL \
	.cachelevel_to_pat = { \
		[I915_CACHE_NONE]   = 3, \
		[I915_CACHE_LLC]    = 0, \
		[I915_CACHE_L3_LLC] = 0, \
		[I915_CACHE_WT]     = 2, \
	}

#define PVC_CACHELEVEL \
	.cachelevel_to_pat = { \
		[I915_CACHE_NONE]   = 0, \
		[I915_CACHE_LLC]    = 3, \
		[I915_CACHE_L3_LLC] = 3, \
		[I915_CACHE_WT]     = 2, \
	}

#define MTL_CACHELEVEL \
	.cachelevel_to_pat = { \
		[I915_CACHE_NONE]   = 2, \
		[I915_CACHE_LLC]    = 3, \
		[I915_CACHE_L3_LLC] = 3, \
		[I915_CACHE_WT]     = 1, \
	}

/* Keep in gen based order, and chronological order within a gen */

#define GEN_DEFAULT_PAGE_SIZES \
	.page_sizes = I915_GTT_PAGE_SIZE_4K

#define GEN_DEFAULT_REGIONS \
	.memory_regions = REGION_SMEM | REGION_STOLEN

#define I830_FEATURES \
	GEN(2), \
	.is_mobile = 1, \
	.display.pipe_mask = BIT(PIPE_A) | BIT(PIPE_B), \
	.display.cpu_transcoder_mask = BIT(TRANSCODER_A) | BIT(TRANSCODER_B), \
	.display.has_overlay = 1, \
	.display.cursor_needs_physical = 1, \
	.display.overlay_needs_physical = 1, \
	.display.has_gmch = 1, \
	.gpu_reset_clobbers_display = true, \
	.has_3d_pipeline = 1, \
	.hws_needs_physical = 1, \
	.unfenced_needs_alignment = 1, \
	.platform_engine_mask = BIT(RCS0), \
	.has_snoop = true, \
	.has_coherent_ggtt = false, \
	.dma_mask_size = 32, \
	I9XX_PIPE_OFFSETS, \
	I9XX_CURSOR_OFFSETS, \
	I9XX_COLORS, \
	GEN_DEFAULT_PAGE_SIZES, \
	GEN_DEFAULT_REGIONS, \
	LEGACY_CACHELEVEL

#define I845_FEATURES \
	GEN(2), \
	.display.pipe_mask = BIT(PIPE_A), \
	.display.cpu_transcoder_mask = BIT(TRANSCODER_A), \
	.display.has_overlay = 1, \
	.display.overlay_needs_physical = 1, \
	.display.has_gmch = 1, \
	.has_3d_pipeline = 1, \
	.gpu_reset_clobbers_display = true, \
	.hws_needs_physical = 1, \
	.unfenced_needs_alignment = 1, \
	.platform_engine_mask = BIT(RCS0), \
	.has_snoop = true, \
	.has_coherent_ggtt = false, \
	.dma_mask_size = 32, \
	I845_PIPE_OFFSETS, \
	I845_CURSOR_OFFSETS, \
	I9XX_COLORS, \
	GEN_DEFAULT_PAGE_SIZES, \
	GEN_DEFAULT_REGIONS, \
	LEGACY_CACHELEVEL

static const struct intel_device_info i830_info = {
	I830_FEATURES,
	PLATFORM(INTEL_I830),
};

static const struct intel_device_info i845g_info = {
	I845_FEATURES,
	PLATFORM(INTEL_I845G),
};

static const struct intel_device_info i85x_info = {
	I830_FEATURES,
	PLATFORM(INTEL_I85X),
	.display.fbc_mask = BIT(INTEL_FBC_A),
};

static const struct intel_device_info i865g_info = {
	I845_FEATURES,
	PLATFORM(INTEL_I865G),
	.display.fbc_mask = BIT(INTEL_FBC_A),
};

#define GEN3_FEATURES \
	GEN(3), \
	.display.pipe_mask = BIT(PIPE_A) | BIT(PIPE_B), \
	.display.cpu_transcoder_mask = BIT(TRANSCODER_A) | BIT(TRANSCODER_B), \
	.display.has_gmch = 1, \
	.gpu_reset_clobbers_display = true, \
	.platform_engine_mask = BIT(RCS0), \
	.has_3d_pipeline = 1, \
	.has_snoop = true, \
	.has_coherent_ggtt = true, \
	.dma_mask_size = 32, \
	I9XX_PIPE_OFFSETS, \
	I9XX_CURSOR_OFFSETS, \
	I9XX_COLORS, \
	GEN_DEFAULT_PAGE_SIZES, \
	GEN_DEFAULT_REGIONS, \
	LEGACY_CACHELEVEL

static const struct intel_device_info i915g_info = {
	GEN3_FEATURES,
	PLATFORM(INTEL_I915G),
	.has_coherent_ggtt = false,
	.display.cursor_needs_physical = 1,
	.display.has_overlay = 1,
	.display.overlay_needs_physical = 1,
	.hws_needs_physical = 1,
	.unfenced_needs_alignment = 1,
};

static const struct intel_device_info i915gm_info = {
	GEN3_FEATURES,
	PLATFORM(INTEL_I915GM),
	.is_mobile = 1,
	.display.cursor_needs_physical = 1,
	.display.has_overlay = 1,
	.display.overlay_needs_physical = 1,
	.display.supports_tv = 1,
	.display.fbc_mask = BIT(INTEL_FBC_A),
	.hws_needs_physical = 1,
	.unfenced_needs_alignment = 1,
};

static const struct intel_device_info i945g_info = {
	GEN3_FEATURES,
	PLATFORM(INTEL_I945G),
	.display.has_hotplug = 1,
	.display.cursor_needs_physical = 1,
	.display.has_overlay = 1,
	.display.overlay_needs_physical = 1,
	.hws_needs_physical = 1,
	.unfenced_needs_alignment = 1,
};

static const struct intel_device_info i945gm_info = {
	GEN3_FEATURES,
	PLATFORM(INTEL_I945GM),
	.is_mobile = 1,
	.display.has_hotplug = 1,
	.display.cursor_needs_physical = 1,
	.display.has_overlay = 1,
	.display.overlay_needs_physical = 1,
	.display.supports_tv = 1,
	.display.fbc_mask = BIT(INTEL_FBC_A),
	.hws_needs_physical = 1,
	.unfenced_needs_alignment = 1,
};

static const struct intel_device_info g33_info = {
	GEN3_FEATURES,
	PLATFORM(INTEL_G33),
	.display.has_hotplug = 1,
	.display.has_overlay = 1,
	.dma_mask_size = 36,
};

static const struct intel_device_info pnv_g_info = {
	GEN3_FEATURES,
	PLATFORM(INTEL_PINEVIEW),
	.display.has_hotplug = 1,
	.display.has_overlay = 1,
	.dma_mask_size = 36,
};

static const struct intel_device_info pnv_m_info = {
	GEN3_FEATURES,
	PLATFORM(INTEL_PINEVIEW),
	.is_mobile = 1,
	.display.has_hotplug = 1,
	.display.has_overlay = 1,
	.dma_mask_size = 36,
};

#define GEN4_FEATURES \
	GEN(4), \
	.display.pipe_mask = BIT(PIPE_A) | BIT(PIPE_B), \
	.display.cpu_transcoder_mask = BIT(TRANSCODER_A) | BIT(TRANSCODER_B), \
	.display.has_hotplug = 1, \
	.display.has_gmch = 1, \
	.gpu_reset_clobbers_display = true, \
	.platform_engine_mask = BIT(RCS0), \
	.has_3d_pipeline = 1, \
	.has_snoop = true, \
	.has_coherent_ggtt = true, \
	.dma_mask_size = 36, \
	I9XX_PIPE_OFFSETS, \
	I9XX_CURSOR_OFFSETS, \
	I965_COLORS, \
	GEN_DEFAULT_PAGE_SIZES, \
	GEN_DEFAULT_REGIONS, \
	LEGACY_CACHELEVEL

static const struct intel_device_info i965g_info = {
	GEN4_FEATURES,
	PLATFORM(INTEL_I965G),
	.display.has_overlay = 1,
	.hws_needs_physical = 1,
	.has_snoop = false,
};

static const struct intel_device_info i965gm_info = {
	GEN4_FEATURES,
	PLATFORM(INTEL_I965GM),
	.is_mobile = 1,
	.display.fbc_mask = BIT(INTEL_FBC_A),
	.display.has_overlay = 1,
	.display.supports_tv = 1,
	.hws_needs_physical = 1,
	.has_snoop = false,
};

static const struct intel_device_info g45_info = {
	GEN4_FEATURES,
	PLATFORM(INTEL_G45),
	.platform_engine_mask = BIT(RCS0) | BIT(VCS0),
	.gpu_reset_clobbers_display = false,
};

static const struct intel_device_info gm45_info = {
	GEN4_FEATURES,
	PLATFORM(INTEL_GM45),
	.is_mobile = 1,
	.display.fbc_mask = BIT(INTEL_FBC_A),
	.display.supports_tv = 1,
	.platform_engine_mask = BIT(RCS0) | BIT(VCS0),
	.gpu_reset_clobbers_display = false,
};

#define GEN5_FEATURES \
	GEN(5), \
	.display.pipe_mask = BIT(PIPE_A) | BIT(PIPE_B), \
	.display.cpu_transcoder_mask = BIT(TRANSCODER_A) | BIT(TRANSCODER_B), \
	.display.has_hotplug = 1, \
	.platform_engine_mask = BIT(RCS0) | BIT(VCS0), \
	.has_3d_pipeline = 1, \
	.has_snoop = true, \
	.has_coherent_ggtt = true, \
	/* ilk does support rc6, but we do not implement [power] contexts */ \
	.has_rc6 = 0, \
	.dma_mask_size = 36, \
	I9XX_PIPE_OFFSETS, \
	I9XX_CURSOR_OFFSETS, \
	ILK_COLORS, \
	GEN_DEFAULT_PAGE_SIZES, \
	GEN_DEFAULT_REGIONS, \
	LEGACY_CACHELEVEL

static const struct intel_device_info ilk_d_info = {
	GEN5_FEATURES,
	PLATFORM(INTEL_IRONLAKE),
};

static const struct intel_device_info ilk_m_info = {
	GEN5_FEATURES,
	PLATFORM(INTEL_IRONLAKE),
	.is_mobile = 1,
	.has_rps = true,
	.display.fbc_mask = BIT(INTEL_FBC_A),
};

#define GEN6_FEATURES \
	GEN(6), \
	.display.pipe_mask = BIT(PIPE_A) | BIT(PIPE_B), \
	.display.cpu_transcoder_mask = BIT(TRANSCODER_A) | BIT(TRANSCODER_B), \
	.display.has_hotplug = 1, \
	.display.fbc_mask = BIT(INTEL_FBC_A), \
	.platform_engine_mask = BIT(RCS0) | BIT(VCS0) | BIT(BCS0), \
	.has_3d_pipeline = 1, \
	.has_coherent_ggtt = true, \
	.has_llc = 1, \
	.has_rc6 = 1, \
	.has_rc6p = 1, \
	.has_rps = true, \
	.dma_mask_size = 40, \
	.ppgtt_type = INTEL_PPGTT_ALIASING, \
	.ppgtt_size = 31, \
	.ppgtt_msb = 31, \
	I9XX_PIPE_OFFSETS, \
	I9XX_CURSOR_OFFSETS, \
	ILK_COLORS, \
	GEN_DEFAULT_PAGE_SIZES, \
	GEN_DEFAULT_REGIONS, \
	LEGACY_CACHELEVEL

#define SNB_D_PLATFORM \
	GEN6_FEATURES, \
	PLATFORM(INTEL_SANDYBRIDGE)

static const struct intel_device_info snb_d_gt1_info = {
	SNB_D_PLATFORM,
	.gt = 1,
};

static const struct intel_device_info snb_d_gt2_info = {
	SNB_D_PLATFORM,
	.gt = 2,
};

#define SNB_M_PLATFORM \
	GEN6_FEATURES, \
	PLATFORM(INTEL_SANDYBRIDGE), \
	.is_mobile = 1


static const struct intel_device_info snb_m_gt1_info = {
	SNB_M_PLATFORM,
	.gt = 1,
};

static const struct intel_device_info snb_m_gt2_info = {
	SNB_M_PLATFORM,
	.gt = 2,
};

#define GEN7_FEATURES  \
	GEN(7), \
	.display.pipe_mask = BIT(PIPE_A) | BIT(PIPE_B) | BIT(PIPE_C), \
	.display.cpu_transcoder_mask = BIT(TRANSCODER_A) | BIT(TRANSCODER_B) | BIT(TRANSCODER_C), \
	.display.has_hotplug = 1, \
	.display.fbc_mask = BIT(INTEL_FBC_A), \
	.platform_engine_mask = BIT(RCS0) | BIT(VCS0) | BIT(BCS0), \
	.has_3d_pipeline = 1, \
	.has_coherent_ggtt = true, \
	.has_llc = 1, \
	.has_rc6 = 1, \
	.has_rc6p = 1, \
	.has_reset_engine = true, \
	.has_rps = true, \
	.dma_mask_size = 40, \
	.ppgtt_type = INTEL_PPGTT_ALIASING, \
	.ppgtt_size = 31, \
	.ppgtt_msb = 31, \
	IVB_PIPE_OFFSETS, \
	IVB_CURSOR_OFFSETS, \
	IVB_COLORS, \
	GEN_DEFAULT_PAGE_SIZES, \
	GEN_DEFAULT_REGIONS, \
	LEGACY_CACHELEVEL

#define IVB_D_PLATFORM \
	GEN7_FEATURES, \
	PLATFORM(INTEL_IVYBRIDGE), \
	.has_l3_dpf = 1

static const struct intel_device_info ivb_d_gt1_info = {
	IVB_D_PLATFORM,
	.gt = 1,
};

static const struct intel_device_info ivb_d_gt2_info = {
	IVB_D_PLATFORM,
	.gt = 2,
};

#define IVB_M_PLATFORM \
	GEN7_FEATURES, \
	PLATFORM(INTEL_IVYBRIDGE), \
	.is_mobile = 1, \
	.has_l3_dpf = 1

static const struct intel_device_info ivb_m_gt1_info = {
	IVB_M_PLATFORM,
	.gt = 1,
};

static const struct intel_device_info ivb_m_gt2_info = {
	IVB_M_PLATFORM,
	.gt = 2,
};

static const struct intel_device_info ivb_q_info = {
	GEN7_FEATURES,
	PLATFORM(INTEL_IVYBRIDGE),
	.gt = 2,
	.display.pipe_mask = 0, /* legal, last one wins */
	.display.cpu_transcoder_mask = 0,
	.has_l3_dpf = 1,
};

static const struct intel_device_info vlv_info = {
	PLATFORM(INTEL_VALLEYVIEW),
	GEN(7),
	.is_lp = 1,
	.display.pipe_mask = BIT(PIPE_A) | BIT(PIPE_B),
	.display.cpu_transcoder_mask = BIT(TRANSCODER_A) | BIT(TRANSCODER_B),
	.has_runtime_pm = 1,
	.has_rc6 = 1,
	.has_reset_engine = true,
	.has_rps = true,
	.display.has_gmch = 1,
	.display.has_hotplug = 1,
	.dma_mask_size = 40,
	.ppgtt_type = INTEL_PPGTT_ALIASING,
	.ppgtt_size = 31,
	.ppgtt_msb = 31,
	.has_snoop = true,
	.has_coherent_ggtt = false,
	.platform_engine_mask = BIT(RCS0) | BIT(VCS0) | BIT(BCS0),
	.display.mmio_offset = VLV_DISPLAY_BASE,
	I9XX_PIPE_OFFSETS,
	I9XX_CURSOR_OFFSETS,
	I965_COLORS,
	GEN_DEFAULT_PAGE_SIZES,
	GEN_DEFAULT_REGIONS,
	LEGACY_CACHELEVEL,
};

#define G75_FEATURES  \
	GEN7_FEATURES, \
	.platform_engine_mask = BIT(RCS0) | BIT(VCS0) | BIT(BCS0) | BIT(VECS0), \
	.display.cpu_transcoder_mask = BIT(TRANSCODER_A) | BIT(TRANSCODER_B) | \
		BIT(TRANSCODER_C) | BIT(TRANSCODER_EDP), \
	.display.has_ddi = 1, \
	.display.has_fpga_dbg = 1, \
	.display.has_dp_mst = 1, \
	.has_rc6p = 0 /* RC6p removed-by HSW */, \
	HSW_PIPE_OFFSETS, \
	.has_runtime_pm = 1

#define HSW_PLATFORM \
	G75_FEATURES, \
	PLATFORM(INTEL_HASWELL), \
	.has_l3_dpf = 1

static const struct intel_device_info hsw_gt1_info = {
	HSW_PLATFORM,
	.gt = 1,
};

static const struct intel_device_info hsw_gt2_info = {
	HSW_PLATFORM,
	.gt = 2,
};

static const struct intel_device_info hsw_gt3_info = {
	HSW_PLATFORM,
	.gt = 3,
};

#define GEN8_FEATURES \
	G75_FEATURES, \
	GEN(8), \
	.has_logical_ring_contexts = 1, \
	.dma_mask_size = 39, \
	.ppgtt_type = INTEL_PPGTT_FULL, \
	.ppgtt_size = 48, \
	.ppgtt_msb = 47, \
	.has_64bit_reloc = 1

#define BDW_PLATFORM \
	GEN8_FEATURES, \
	PLATFORM(INTEL_BROADWELL)

static const struct intel_device_info bdw_gt1_info = {
	BDW_PLATFORM,
	.gt = 1,
};

static const struct intel_device_info bdw_gt2_info = {
	BDW_PLATFORM,
	.gt = 2,
};

static const struct intel_device_info bdw_rsvd_info = {
	BDW_PLATFORM,
	.gt = 3,
	/* According to the device ID those devices are GT3, they were
	 * previously treated as not GT3, keep it like that.
	 */
};

static const struct intel_device_info bdw_gt3_info = {
	BDW_PLATFORM,
	.gt = 3,
	.platform_engine_mask =
		BIT(RCS0) | BIT(VCS0) | BIT(BCS0) | BIT(VECS0) | BIT(VCS1),
};

static const struct intel_device_info chv_info = {
	PLATFORM(INTEL_CHERRYVIEW),
	GEN(8),
	.display.pipe_mask = BIT(PIPE_A) | BIT(PIPE_B) | BIT(PIPE_C),
	.display.cpu_transcoder_mask = BIT(TRANSCODER_A) | BIT(TRANSCODER_B) | BIT(TRANSCODER_C),
	.display.has_hotplug = 1,
	.is_lp = 1,
	.platform_engine_mask = BIT(RCS0) | BIT(VCS0) | BIT(BCS0) | BIT(VECS0),
	.has_64bit_reloc = 1,
	.has_runtime_pm = 1,
	.has_rc6 = 1,
	.has_rps = true,
	.has_logical_ring_contexts = 1,
	.display.has_gmch = 1,
	.dma_mask_size = 39,
	.ppgtt_type = INTEL_PPGTT_FULL,
	.ppgtt_size = 32,
	.ppgtt_msb = 47,
	.has_reset_engine = 1,
	.has_snoop = true,
	.has_coherent_ggtt = false,
	.display.mmio_offset = VLV_DISPLAY_BASE,
	CHV_PIPE_OFFSETS,
	CHV_CURSOR_OFFSETS,
	CHV_COLORS,
	GEN_DEFAULT_PAGE_SIZES,
	GEN_DEFAULT_REGIONS,
};

#define GEN9_DEFAULT_PAGE_SIZES \
	.page_sizes = I915_GTT_PAGE_SIZE_4K | \
		      I915_GTT_PAGE_SIZE_64K

#define GEN9_FEATURES \
	GEN8_FEATURES, \
	GEN(9), \
	GEN9_DEFAULT_PAGE_SIZES, \
	.display.has_dmc = 1, \
	.has_gt_uc = 1, \
	.display.has_hdcp = 1, \
	.display.has_ipc = 1, \
	.display.has_psr = 1, \
	.display.has_psr_hw_tracking = 1, \
	.display.dbuf.size = 896 - 4, /* 4 blocks for bypass path allocation */ \
	.display.dbuf.slice_mask = BIT(DBUF_S1)

#define SKL_PLATFORM \
	GEN9_FEATURES, \
	PLATFORM(INTEL_SKYLAKE)

static const struct intel_device_info skl_gt1_info = {
	SKL_PLATFORM,
	.gt = 1,
};

static const struct intel_device_info skl_gt2_info = {
	SKL_PLATFORM,
	.gt = 2,
};

#define SKL_GT3_PLUS_PLATFORM \
	SKL_PLATFORM, \
	.platform_engine_mask = \
		BIT(RCS0) | BIT(VCS0) | BIT(BCS0) | BIT(VECS0) | BIT(VCS1)


static const struct intel_device_info skl_gt3_info = {
	SKL_GT3_PLUS_PLATFORM,
	.gt = 3,
};

static const struct intel_device_info skl_gt4_info = {
	SKL_GT3_PLUS_PLATFORM,
	.gt = 4,
};

#define GEN9_LP_FEATURES \
	GEN(9), \
	.is_lp = 1, \
	.display.dbuf.slice_mask = BIT(DBUF_S1), \
	.display.has_hotplug = 1, \
	.platform_engine_mask = BIT(RCS0) | BIT(VCS0) | BIT(BCS0) | BIT(VECS0), \
	.display.pipe_mask = BIT(PIPE_A) | BIT(PIPE_B) | BIT(PIPE_C), \
	.display.cpu_transcoder_mask = BIT(TRANSCODER_A) | BIT(TRANSCODER_B) | \
		BIT(TRANSCODER_C) | BIT(TRANSCODER_EDP) | \
		BIT(TRANSCODER_DSI_A) | BIT(TRANSCODER_DSI_C), \
	.has_3d_pipeline = 1, \
	.has_64bit_reloc = 1, \
	.display.has_ddi = 1, \
	.display.has_fpga_dbg = 1, \
	.display.fbc_mask = BIT(INTEL_FBC_A), \
	.display.has_hdcp = 1, \
	.display.has_psr = 1, \
	.display.has_psr_hw_tracking = 1, \
	.has_runtime_pm = 1, \
	.display.has_dmc = 1, \
	.has_rc6 = 1, \
	.has_rps = true, \
	.display.has_dp_mst = 1, \
	.has_logical_ring_contexts = 1, \
	.has_gt_uc = 1, \
	.dma_mask_size = 39, \
	.ppgtt_type = INTEL_PPGTT_FULL, \
	.ppgtt_size = 48, \
	.ppgtt_msb = 47, \
	.has_reset_engine = 1, \
	.has_snoop = true, \
	.has_coherent_ggtt = false, \
	.display.has_ipc = 1, \
	HSW_PIPE_OFFSETS, \
	IVB_CURSOR_OFFSETS, \
	IVB_COLORS, \
	GEN9_DEFAULT_PAGE_SIZES, \
	GEN_DEFAULT_REGIONS, \
	LEGACY_CACHELEVEL

static const struct intel_device_info bxt_info = {
	GEN9_LP_FEATURES,
	PLATFORM(INTEL_BROXTON),
	.display.dbuf.size = 512 - 4, /* 4 blocks for bypass path allocation */
};

static const struct intel_device_info glk_info = {
	GEN9_LP_FEATURES,
	PLATFORM(INTEL_GEMINILAKE),
	.display.ver = 10,
	.display.dbuf.size = 1024 - 4, /* 4 blocks for bypass path allocation */
	GLK_COLORS,
};

#define KBL_PLATFORM \
	GEN9_FEATURES, \
	PLATFORM(INTEL_KABYLAKE)

static const struct intel_device_info kbl_gt1_info = {
	KBL_PLATFORM,
	.gt = 1,
};

static const struct intel_device_info kbl_gt2_info = {
	KBL_PLATFORM,
	.gt = 2,
};

static const struct intel_device_info kbl_gt3_info = {
	KBL_PLATFORM,
	.gt = 3,
	.platform_engine_mask =
		BIT(RCS0) | BIT(VCS0) | BIT(BCS0) | BIT(VECS0) | BIT(VCS1),
};

#define CFL_PLATFORM \
	GEN9_FEATURES, \
	PLATFORM(INTEL_COFFEELAKE)

static const struct intel_device_info cfl_gt1_info = {
	CFL_PLATFORM,
	.gt = 1,
};

static const struct intel_device_info cfl_gt2_info = {
	CFL_PLATFORM,
	.gt = 2,
};

static const struct intel_device_info cfl_gt3_info = {
	CFL_PLATFORM,
	.gt = 3,
	.platform_engine_mask =
		BIT(RCS0) | BIT(VCS0) | BIT(BCS0) | BIT(VECS0) | BIT(VCS1),
};

#define CML_PLATFORM \
	GEN9_FEATURES, \
	PLATFORM(INTEL_COMETLAKE)

static const struct intel_device_info cml_gt1_info = {
	CML_PLATFORM,
	.gt = 1,
};

static const struct intel_device_info cml_gt2_info = {
	CML_PLATFORM,
	.gt = 2,
};

#define GEN11_DEFAULT_PAGE_SIZES \
	.page_sizes = I915_GTT_PAGE_SIZE_4K | \
		      I915_GTT_PAGE_SIZE_64K | \
		      I915_GTT_PAGE_SIZE_2M | \
		      I915_GTT_PAGE_SIZE_1G

#define GEN11_FEATURES \
	GEN9_FEATURES, \
	GEN11_DEFAULT_PAGE_SIZES, \
	.display.abox_mask = BIT(0), \
	.display.cpu_transcoder_mask = BIT(TRANSCODER_A) | BIT(TRANSCODER_B) | \
		BIT(TRANSCODER_C) | BIT(TRANSCODER_EDP) | \
		BIT(TRANSCODER_DSI_0) | BIT(TRANSCODER_DSI_1), \
	.display.pipe_offsets = { \
		[TRANSCODER_A] = PIPE_A_OFFSET, \
		[TRANSCODER_B] = PIPE_B_OFFSET, \
		[TRANSCODER_C] = PIPE_C_OFFSET, \
		[TRANSCODER_EDP] = PIPE_EDP_OFFSET, \
		[TRANSCODER_DSI_0] = PIPE_DSI0_OFFSET, \
		[TRANSCODER_DSI_1] = PIPE_DSI1_OFFSET, \
	}, \
	.display.trans_offsets = { \
		[TRANSCODER_A] = TRANSCODER_A_OFFSET, \
		[TRANSCODER_B] = TRANSCODER_B_OFFSET, \
		[TRANSCODER_C] = TRANSCODER_C_OFFSET, \
		[TRANSCODER_EDP] = TRANSCODER_EDP_OFFSET, \
		[TRANSCODER_DSI_0] = TRANSCODER_DSI0_OFFSET, \
		[TRANSCODER_DSI_1] = TRANSCODER_DSI1_OFFSET, \
	}, \
	GEN(11), \
	ICL_COLORS, \
	.display.dbuf.size = 2048, \
	.display.dbuf.slice_mask = BIT(DBUF_S1) | BIT(DBUF_S2), \
	.display.has_dsc = 1, \
	.has_coherent_ggtt = false, \
	.has_logical_ring_elsq = 1

static const struct intel_device_info icl_info = {
	GEN11_FEATURES,
	PLATFORM(INTEL_ICELAKE),
	.platform_engine_mask =
		BIT(RCS0) | BIT(BCS0) | BIT(VECS0) | BIT(VCS0) | BIT(VCS2),
};

static const struct intel_device_info ehl_info = {
	GEN11_FEATURES,
	PLATFORM(INTEL_ELKHARTLAKE),
	.platform_engine_mask = BIT(RCS0) | BIT(BCS0) | BIT(VCS0) | BIT(VECS0),
	.ppgtt_size = 36,
};

static const struct intel_device_info jsl_info = {
	GEN11_FEATURES,
	PLATFORM(INTEL_JASPERLAKE),
	.platform_engine_mask = BIT(RCS0) | BIT(BCS0) | BIT(VCS0) | BIT(VECS0),
	.ppgtt_size = 36,
};

#define GEN12_FEATURES \
	GEN11_FEATURES, \
	GEN(12), \
	.display.abox_mask = GENMASK(2, 1), \
	.display.pipe_mask = BIT(PIPE_A) | BIT(PIPE_B) | BIT(PIPE_C) | BIT(PIPE_D), \
	.display.cpu_transcoder_mask = BIT(TRANSCODER_A) | BIT(TRANSCODER_B) | \
		BIT(TRANSCODER_C) | BIT(TRANSCODER_D) | \
		BIT(TRANSCODER_DSI_0) | BIT(TRANSCODER_DSI_1), \
	.display.pipe_offsets = { \
		[TRANSCODER_A] = PIPE_A_OFFSET, \
		[TRANSCODER_B] = PIPE_B_OFFSET, \
		[TRANSCODER_C] = PIPE_C_OFFSET, \
		[TRANSCODER_D] = PIPE_D_OFFSET, \
		[TRANSCODER_DSI_0] = PIPE_DSI0_OFFSET, \
		[TRANSCODER_DSI_1] = PIPE_DSI1_OFFSET, \
	}, \
	.display.trans_offsets = { \
		[TRANSCODER_A] = TRANSCODER_A_OFFSET, \
		[TRANSCODER_B] = TRANSCODER_B_OFFSET, \
		[TRANSCODER_C] = TRANSCODER_C_OFFSET, \
		[TRANSCODER_D] = TRANSCODER_D_OFFSET, \
		[TRANSCODER_DSI_0] = TRANSCODER_DSI0_OFFSET, \
		[TRANSCODER_DSI_1] = TRANSCODER_DSI1_OFFSET, \
	}, \
	TGL_CURSOR_OFFSETS, \
	TGL_CACHELEVEL, \
	.has_global_mocs = 1, \
	.has_null_page = 1, \
	.has_pxp = 1, \
	.display.has_dsb = 0 /* FIXME: LUT load is broken with DSB */

static const struct intel_device_info tgl_info = {
	GEN12_FEATURES,
	PLATFORM(INTEL_TIGERLAKE),
	.display.has_modular_fia = 1,
	.platform_engine_mask =
		BIT(RCS0) | BIT(BCS0) | BIT(VECS0) | BIT(VCS0) | BIT(VCS2),
	.has_sriov = 1,
};

static const struct intel_device_info rkl_info = {
	GEN12_FEATURES,
	PLATFORM(INTEL_ROCKETLAKE),
	.display.abox_mask = BIT(0),
	.display.pipe_mask = BIT(PIPE_A) | BIT(PIPE_B) | BIT(PIPE_C),
	.display.cpu_transcoder_mask = BIT(TRANSCODER_A) | BIT(TRANSCODER_B) |
		BIT(TRANSCODER_C),
	.display.has_hti = 1,
	.display.has_psr_hw_tracking = 0,
	.platform_engine_mask =
		BIT(RCS0) | BIT(BCS0) | BIT(VECS0) | BIT(VCS0),
};

#define DGFX_FEATURES \
	.memory_regions = REGION_SMEM | REGION_LMEM | REGION_STOLEN, \
	.has_llc = 0, \
	.has_pxp = 0, \
	.has_snoop = 1, \
	.is_dgfx = 1, \
	.has_heci_gscfi = 1

static const struct intel_device_info dg1_info = {
	GEN12_FEATURES,
	DGFX_FEATURES,
	.graphics.rel = 10,
	PLATFORM(INTEL_DG1),
	.display.pipe_mask = BIT(PIPE_A) | BIT(PIPE_B) | BIT(PIPE_C) | BIT(PIPE_D),
	.platform_engine_mask =
		BIT(RCS0) | BIT(BCS0) | BIT(VECS0) |
		BIT(VCS0) | BIT(VCS2),
	/* Wa_16011227922 */
	.ppgtt_size = 47,
};

static const struct intel_device_info adl_s_info = {
	GEN12_FEATURES,
	PLATFORM(INTEL_ALDERLAKE_S),
	.display.pipe_mask = BIT(PIPE_A) | BIT(PIPE_B) | BIT(PIPE_C) | BIT(PIPE_D),
	.display.has_hti = 1,
	.display.has_psr_hw_tracking = 0,
	.platform_engine_mask =
		BIT(RCS0) | BIT(BCS0) | BIT(VECS0) | BIT(VCS0) | BIT(VCS2),
	.dma_mask_size = 39,
	.has_sriov =1,
};

#define XE_LPD_FEATURES \
	.display.abox_mask = GENMASK(1, 0),					\
	.display.color = {							\
		.degamma_lut_size = 128, .gamma_lut_size = 1024,		\
		.degamma_lut_tests = DRM_COLOR_LUT_NON_DECREASING |		\
				     DRM_COLOR_LUT_EQUAL_CHANNELS,		\
	},									\
	.display.dbuf.size = 4096,						\
	.display.dbuf.slice_mask = BIT(DBUF_S1) | BIT(DBUF_S2) | BIT(DBUF_S3) |	\
		BIT(DBUF_S4),							\
	.display.has_ddi = 1,							\
	.display.has_dmc = 1,							\
	.display.has_dp_mst = 1,						\
	.display.has_dsb = 1,							\
	.display.has_dsc = 1,							\
	.display.fbc_mask = BIT(INTEL_FBC_A),					\
	.display.has_fpga_dbg = 1,						\
	.display.has_hdcp = 1,							\
	.display.has_hotplug = 1,						\
	.display.has_ipc = 1,							\
	.display.has_psr = 1,							\
	.display.ver = 13,							\
	.display.pipe_mask = BIT(PIPE_A) | BIT(PIPE_B) | BIT(PIPE_C) | BIT(PIPE_D),	\
	.display.pipe_offsets = {						\
		[TRANSCODER_A] = PIPE_A_OFFSET,					\
		[TRANSCODER_B] = PIPE_B_OFFSET,					\
		[TRANSCODER_C] = PIPE_C_OFFSET,					\
		[TRANSCODER_D] = PIPE_D_OFFSET,					\
		[TRANSCODER_DSI_0] = PIPE_DSI0_OFFSET,				\
		[TRANSCODER_DSI_1] = PIPE_DSI1_OFFSET,				\
	},									\
	.display.trans_offsets = {						\
		[TRANSCODER_A] = TRANSCODER_A_OFFSET,				\
		[TRANSCODER_B] = TRANSCODER_B_OFFSET,				\
		[TRANSCODER_C] = TRANSCODER_C_OFFSET,				\
		[TRANSCODER_D] = TRANSCODER_D_OFFSET,				\
		[TRANSCODER_DSI_0] = TRANSCODER_DSI0_OFFSET,			\
		[TRANSCODER_DSI_1] = TRANSCODER_DSI1_OFFSET,			\
	},									\
	TGL_CURSOR_OFFSETS

static const struct intel_device_info adl_p_info = {
	GEN12_FEATURES,
	XE_LPD_FEATURES,
	PLATFORM(INTEL_ALDERLAKE_P),
	.display.cpu_transcoder_mask = BIT(TRANSCODER_A) | BIT(TRANSCODER_B) |
			       BIT(TRANSCODER_C) | BIT(TRANSCODER_D) |
			       BIT(TRANSCODER_DSI_0) | BIT(TRANSCODER_DSI_1),
	.display.has_cdclk_crawl = 1,
	.display.has_modular_fia = 1,
	.display.has_psr_hw_tracking = 0,
	.platform_engine_mask =
		BIT(RCS0) | BIT(BCS0) | BIT(VECS0) | BIT(VCS0) | BIT(VCS2),
	.ppgtt_size = 48,
	.dma_mask_size = 39,
	.has_sriov = 1,
};

#undef GEN

#define XE_HP_PAGE_SIZES \
	.page_sizes = I915_GTT_PAGE_SIZE_4K | \
		      I915_GTT_PAGE_SIZE_64K | \
		      I915_GTT_PAGE_SIZE_2M | \
		      I915_GTT_PAGE_SIZE_1G

#define XE_HP_FEATURES \
	.graphics.ver = 12, \
	.graphics.rel = 50, \
	XE_HP_PAGE_SIZES, \
	.dma_mask_size = 46, \
	.has_3d_pipeline = 1, \
	.has_64bit_reloc = 1, \
	.has_flat_ccs = 1, \
	.has_4tile = 1, \
	.has_global_mocs = 1, \
	.has_gt_uc = 1, \
	.has_llc = 1, \
	.has_logical_ring_contexts = 1, \
	.has_logical_ring_elsq = 1, \
	.has_mslice_steering = 1, \
	.has_oa_bpc_reporting = 1, \
	.has_oa_buf_128m = 1, \
	.has_oa_mmio_trigger = 1, \
	.has_oa_slice_contrib_limits = 1, \
	.has_null_page = 1, \
	.has_rc6 = 1, \
	.has_reset_engine = 1, \
	.has_rps = 1, \
	.has_runtime_pm = 1, \
	.has_selective_tlb_invalidation = 1, \
	.has_semaphore_xehpsdv = 1, \
	.ppgtt_msb = 47, \
	.ppgtt_size = 48, \
	.ppgtt_type = INTEL_PPGTT_FULL, \
	.has_oam = 1, \
	.oam_uses_vdbox0_channel = 1, \
	TGL_CACHELEVEL

#define XE_HPM_FEATURES \
	.media.ver = 12, \
	.media.rel = 50

#define REMOTE_TILE_FEATURES \
	.has_remote_tiles = 1, \
	.memory_regions = (REGION_SMEM | REGION_STOLEN | REGION_LMEM)

#define XE_HP_SDV_ENGINES \
	BIT(BCS0) | \
	BIT(VECS0) | BIT(VECS1) | BIT(VECS2) | BIT(VECS3) | \
	BIT(VCS0) | BIT(VCS1) | BIT(VCS2) | BIT(VCS3) | \
	BIT(VCS4) | BIT(VCS5) | BIT(VCS6) | BIT(VCS7) | \
	BIT(CCS0) | BIT(CCS1) | BIT(CCS2) | BIT(CCS3) \

static const struct intel_gt_definition xehp_sdv_extra_gt[] = {
	{
		.type = GT_TILE,
		.name = "Remote Tile GT 1",
		.mapping_base = SZ_16M,
		.engine_mask = XE_HP_SDV_ENGINES,

	},
	{
		.type = GT_TILE,
		.name = "Remote Tile GT 2",
		.mapping_base = SZ_16M * 2,
		.engine_mask = XE_HP_SDV_ENGINES,

	},
	{
		.type = GT_TILE,
		.name = "Remote Tile GT 3",
		.mapping_base = SZ_16M * 3,
		.engine_mask = XE_HP_SDV_ENGINES,

	},
	{}
};

__maybe_unused
static const struct intel_device_info xehpsdv_info = {
	XE_HP_FEATURES,
	XE_HPM_FEATURES,
	DGFX_FEATURES,
	REMOTE_TILE_FEATURES,
	PLATFORM(INTEL_XEHPSDV),
	.display = { },
	.extra_gt_list = xehp_sdv_extra_gt,
	.has_64k_pages = 1,
	.has_media_ratio_mode = 1,
	.has_sriov = 1,
	.has_iov_memirq = 1,
	.has_mem_sparing = 1,
	.platform_engine_mask = XE_HP_SDV_ENGINES,
	.require_force_probe = 1,
};

#define DG2_FEATURES \
	XE_HP_FEATURES, \
	XE_HPM_FEATURES, \
	DGFX_FEATURES, \
	.graphics.rel = 55, \
	.media.rel = 55, \
	PLATFORM(INTEL_DG2), \
	.has_64k_pages = 1, \
	.has_guc_deprivilege = 1, \
	.has_heci_pxp = 1, \
	.has_media_ratio_mode = 1, \
	.has_iov_memirq = 1, \
	.has_oac = 1, \
	.has_sriov = 1, \
	.platform_engine_mask = \
		BIT(RCS0) | BIT(BCS0) | \
		BIT(VECS0) | BIT(VECS1) | \
		BIT(VCS0) | BIT(VCS2) | \
		BIT(CCS0) | BIT(CCS1) | BIT(CCS2) | BIT(CCS3)

static const struct intel_device_info dg2_info = {
	DG2_FEATURES,
	XE_LPD_FEATURES,
	.display.cpu_transcoder_mask = BIT(TRANSCODER_A) | BIT(TRANSCODER_B) |
			       BIT(TRANSCODER_C) | BIT(TRANSCODER_D),
};

static const struct intel_device_info ats_m_info = {
	DG2_FEATURES,
	.display = { 0 },
	.tuning_thread_rr_after_dep = 1,
	.has_csc_uid = 1,
	.has_lmem_max_bandwidth = 1,
};

#define XE_HPC_FEATURES \
	XE_HP_FEATURES, \
	.dma_mask_size = 52, \
	.has_3d_pipeline = 0, \
	/* FIXME: remove as soon as PVC support for LMEM 4K pages is working */ \
	.has_64k_pages = 1, \
	.has_access_counter = 1, \
	.has_asid_tlb_invalidation = 1, \
	.has_cache_clos = 1, \
	.has_eu_stall_sampling = 1, \
	.has_full_ps64 = 1, \
	.has_gt_error_vectors = 1, \
	.has_guc_deprivilege = 1, \
	.has_guc_programmable_mocs = 1, \
	.has_iaf = 1, \
	.has_iov_memirq = 1, \
	.has_l3_ccs_read = 1, \
	.has_link_copy_engines = 1, \
	.has_lmtt_lvl2 = 1, \
	.has_media_ratio_mode = 1, \
	.has_mem_sparing = 1, \
	.has_mslice_steering = 0, \
	.has_oac = 1, \
	.has_one_eu_per_fuse_bit = 1, \
	.has_recoverable_page_fault = 1, \
	.has_slim_vdbox = 1, \
	.has_sriov = 1, \
	.has_um_queues = 1, \
	.ppgtt_msb = 56, \
	.ppgtt_size = 57

#define PVC_ENGINES \
	BIT(BCS0) | BIT(BCS1) | BIT(BCS2) | BIT(BCS3) | \
	BIT(BCS4) | BIT(BCS5) | BIT(BCS6) | BIT(BCS7) | \
	BIT(BCS8) | \
	BIT(VCS0) | BIT(VCS1) | BIT(VCS2) | \
	BIT(CCS0) | BIT(CCS1) | BIT(CCS2) | BIT(CCS3)

static const struct intel_gt_definition pvc_extra_gt[] = {
	{
		.type = GT_TILE,
		.name = "Remote Tile GT",
		.mapping_base = SZ_16M,
		.engine_mask = PVC_ENGINES,

	},
	{}
};

static const struct intel_device_info pvc_info = {
	XE_HPC_FEATURES,
	XE_HPM_FEATURES,
	DGFX_FEATURES,
	REMOTE_TILE_FEATURES,
	.graphics.rel = 60,
	.media.rel = 60,
	PLATFORM(INTEL_PONTEVECCHIO),
	.display = { 0 },
	.has_flat_ccs = 0,
	.extra_gt_list = pvc_extra_gt,
	.platform_engine_mask = PVC_ENGINES,

	/*
	 * Runtime PM is not a PVC requirement, few PVC platforms
	 * has ended up with DPC and Internal fabric error when entered to
	 * Runtime Suspend D3, therefore disabling Runtime PM.
	 */
	.has_runtime_pm = 0,
	.require_force_probe = 1,
	PVC_CACHELEVEL,
};

#define XE_LPDP_FEATURES	\
	XE_LPD_FEATURES,	\
	.display.ver = 14,	\
	.display.has_cdclk_crawl = 1, \
	.display.fbc_mask = BIT(INTEL_FBC_A) | BIT(INTEL_FBC_B)

static const struct intel_gt_definition xelpmp_extra_gt[] = {
	{
		.type = GT_MEDIA,
		.name = "Standalone Media GT",
		.gsi_offset = MTL_MEDIA_GSI_BASE,
		.engine_mask = BIT(VECS0) | BIT(VCS0) | BIT(VCS2) | BIT(GSC0),
	},
	{}
};

__maybe_unused
static const struct intel_device_info mtl_info = {
	XE_HP_FEATURES,
	XE_LPDP_FEATURES,
	/*
	 * Real graphics IP version will be obtained from hardware GMD_ID
	 * register.  Value provided here is just for sanity checking.
	 */
	.graphics.ver = 12,
	.graphics.rel = 70,
	.media.ver = 13,
	PLATFORM(INTEL_METEORLAKE),
	.display.has_modular_fia = 1,
	.extra_gt_list = xelpmp_extra_gt,
	.has_flat_ccs = 0,
	.has_gmd_id = 1,
	.has_guc_deprivilege = 1,
	.has_iov_memirq = 1,
	.has_llc = 0,
	.has_mslice_steering = 0,
	.has_snoop = 1,
	.has_sriov = 1,
	.memory_regions = REGION_SMEM | REGION_STOLEN,
	MTL_CACHELEVEL,
	.platform_engine_mask = BIT(RCS0) | BIT(BCS0) | BIT(CCS0),
	.require_force_probe = 1,
	.needs_driver_flr = 0, /* FIXME: IFWI still has issues with FLR */
};

#undef PLATFORM

/*
 * Make sure any device matches here are from most specific to most
 * general.  For example, since the Quanta match is based on the subsystem
 * and subvendor IDs, we need it to come before the more general IVB
 * PCI ID matches, otherwise we'll use the wrong info struct above.
 */
static const struct pci_device_id pciidlist[] = {
	INTEL_I830_IDS(&i830_info),
	INTEL_I845G_IDS(&i845g_info),
	INTEL_I85X_IDS(&i85x_info),
	INTEL_I865G_IDS(&i865g_info),
	INTEL_I915G_IDS(&i915g_info),
	INTEL_I915GM_IDS(&i915gm_info),
	INTEL_I945G_IDS(&i945g_info),
	INTEL_I945GM_IDS(&i945gm_info),
	INTEL_I965G_IDS(&i965g_info),
	INTEL_G33_IDS(&g33_info),
	INTEL_I965GM_IDS(&i965gm_info),
	INTEL_GM45_IDS(&gm45_info),
	INTEL_G45_IDS(&g45_info),
	INTEL_PINEVIEW_G_IDS(&pnv_g_info),
	INTEL_PINEVIEW_M_IDS(&pnv_m_info),
	INTEL_IRONLAKE_D_IDS(&ilk_d_info),
	INTEL_IRONLAKE_M_IDS(&ilk_m_info),
	INTEL_SNB_D_GT1_IDS(&snb_d_gt1_info),
	INTEL_SNB_D_GT2_IDS(&snb_d_gt2_info),
	INTEL_SNB_M_GT1_IDS(&snb_m_gt1_info),
	INTEL_SNB_M_GT2_IDS(&snb_m_gt2_info),
	INTEL_IVB_Q_IDS(&ivb_q_info), /* must be first IVB */
	INTEL_IVB_M_GT1_IDS(&ivb_m_gt1_info),
	INTEL_IVB_M_GT2_IDS(&ivb_m_gt2_info),
	INTEL_IVB_D_GT1_IDS(&ivb_d_gt1_info),
	INTEL_IVB_D_GT2_IDS(&ivb_d_gt2_info),
	INTEL_HSW_GT1_IDS(&hsw_gt1_info),
	INTEL_HSW_GT2_IDS(&hsw_gt2_info),
	INTEL_HSW_GT3_IDS(&hsw_gt3_info),
	INTEL_VLV_IDS(&vlv_info),
	INTEL_BDW_GT1_IDS(&bdw_gt1_info),
	INTEL_BDW_GT2_IDS(&bdw_gt2_info),
	INTEL_BDW_GT3_IDS(&bdw_gt3_info),
	INTEL_BDW_RSVD_IDS(&bdw_rsvd_info),
	INTEL_CHV_IDS(&chv_info),
	INTEL_SKL_GT1_IDS(&skl_gt1_info),
	INTEL_SKL_GT2_IDS(&skl_gt2_info),
	INTEL_SKL_GT3_IDS(&skl_gt3_info),
	INTEL_SKL_GT4_IDS(&skl_gt4_info),
	INTEL_BXT_IDS(&bxt_info),
	INTEL_GLK_IDS(&glk_info),
	INTEL_KBL_GT1_IDS(&kbl_gt1_info),
	INTEL_KBL_GT2_IDS(&kbl_gt2_info),
	INTEL_KBL_GT3_IDS(&kbl_gt3_info),
	INTEL_KBL_GT4_IDS(&kbl_gt3_info),
	INTEL_AML_KBL_GT2_IDS(&kbl_gt2_info),
	INTEL_CFL_S_GT1_IDS(&cfl_gt1_info),
	INTEL_CFL_S_GT2_IDS(&cfl_gt2_info),
	INTEL_CFL_H_GT1_IDS(&cfl_gt1_info),
	INTEL_CFL_H_GT2_IDS(&cfl_gt2_info),
	INTEL_CFL_U_GT2_IDS(&cfl_gt2_info),
	INTEL_CFL_U_GT3_IDS(&cfl_gt3_info),
	INTEL_WHL_U_GT1_IDS(&cfl_gt1_info),
	INTEL_WHL_U_GT2_IDS(&cfl_gt2_info),
	INTEL_AML_CFL_GT2_IDS(&cfl_gt2_info),
	INTEL_WHL_U_GT3_IDS(&cfl_gt3_info),
	INTEL_CML_GT1_IDS(&cml_gt1_info),
	INTEL_CML_GT2_IDS(&cml_gt2_info),
	INTEL_CML_U_GT1_IDS(&cml_gt1_info),
	INTEL_CML_U_GT2_IDS(&cml_gt2_info),
	INTEL_ICL_11_IDS(&icl_info),
	INTEL_EHL_IDS(&ehl_info),
	INTEL_JSL_IDS(&jsl_info),
#if 0
	INTEL_TGL_12_IDS(&tgl_info),
	INTEL_RKL_IDS(&rkl_info),
	INTEL_ADLS_IDS(&adl_s_info),
	INTEL_ADLP_IDS(&adl_p_info),
	INTEL_ADLN_IDS(&adl_p_info),
#endif
	INTEL_DG1_IDS(&dg1_info),
#if 0
	INTEL_RPLS_IDS(&adl_s_info),
	INTEL_RPLP_IDS(&adl_p_info),
#endif
	INTEL_DG2_IDS(&dg2_info),
	INTEL_ATS_M_IDS(&ats_m_info),
#if 0
	INTEL_MTL_IDS(&mtl_info),
#endif
	INTEL_PVC_IDS(&pvc_info),
	{0, 0, 0}
};
MODULE_DEVICE_TABLE(pci, pciidlist);

static void i915_pci_remove(struct pci_dev *pdev)
{
	struct drm_i915_private *i915;

	i915 = pci_get_drvdata(pdev);
	if (!i915) /* driver load aborted, nothing to cleanup */
		return;

	if (IS_SRIOV_PF(i915))
		i915_sriov_pf_disable_vfs(i915);

	i915_driver_remove(i915);
	pci_set_drvdata(pdev, NULL);

	intel_memory_regions_remove(i915);
}

/* is device_id present in comma separated list of ids */
static bool force_probe(u16 device_id, const char *devices)
{
	char *s, *p, *tok;
	bool ret;

	if (!devices || !*devices)
		return false;

	/* match everything */
	if (strcmp(devices, "*") == 0)
		return true;

	s = kstrdup(devices, GFP_KERNEL);
	if (!s)
		return false;

	for (p = s, ret = false; (tok = strsep(&p, ",")) != NULL; ) {
		u16 val;

		if (kstrtou16(tok, 16, &val) == 0 && val == device_id) {
			ret = true;
			break;
		}
	}

	kfree(s);

	return ret;
}

bool i915_pci_resource_valid(struct pci_dev *pdev, int bar)
{
	if (!pci_resource_flags(pdev, bar))
		return false;

	if (pci_resource_flags(pdev, bar) & IORESOURCE_UNSET)
		return false;

	if (!pci_resource_len(pdev, bar))
		return false;

	return true;
}

static int device_set_offline(struct device *dev, void *data)
{
	dev->offline = true;
	return 0;
}

void i915_pci_set_offline(struct pci_dev *pdev)
{
	device_for_each_child(&pdev->dev, NULL, device_set_offline);
}

static bool intel_mmio_bar_valid(struct pci_dev *pdev, struct intel_device_info *intel_info)
{
	int gttmmaddr_bar = intel_info->graphics.ver == 2 ? GEN2_GTTMMADR_BAR : GTTMMADR_BAR;

	return i915_pci_resource_valid(pdev, gttmmaddr_bar);
}

static int i915_pci_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	struct intel_device_info *intel_info =
		(struct intel_device_info *) ent->driver_data;
	struct drm_i915_private *i915;
	intel_wakeref_t wakeref;
	int err;

	/* If we've already injected a fault into an earlier device, bail */
	if (i915_error_injected() && !i915_modparams.inject_probe_failure)
		return -ENODEV;

	if (intel_info->require_force_probe &&
	    !force_probe(pdev->device, i915_modparams.force_probe)) {
		dev_info(&pdev->dev,
			 "Your graphics device %04x is not properly supported by the driver in this\n"
			 "kernel version. To force driver probe anyway, use i915.force_probe=%04x\n"
			 "module parameter or CPTCFG_DRM_I915_FORCE_PROBE=%04x configuration option,\n"
			 "or (recommended) check for kernel updates.\n",
			 pdev->device, pdev->device, pdev->device);
		return -ENODEV;
	}

	/*
	 * Don't bind to non-zero function, unless it is a virtual function.
	 * Early generations used function 1 as a placeholder for multi-head.
	 * This causes us confusion instead, especially on the systems where
	 * both functions have the same PCI-ID!
	 */
	if (PCI_FUNC(pdev->devfn) && !pdev->is_virtfn)
		return -ENODEV;

	if (!intel_mmio_bar_valid(pdev, intel_info))
		return -ENXIO;

	/*
	 * apple-gmux is needed on dual GPU MacBook Pro
	 * to probe the panel if we're the inactive GPU.
	 */
	if (vga_switcheroo_client_probe_defer(pdev))
		return -EPROBE_DEFER;

	err = i915_driver_probe(pdev, ent);
	if (err)
		return err;

	pvc_wa_disallow_rc6(pdev_to_i915(pdev));

	i915 = pdev_to_i915(pdev);
	with_intel_runtime_pm(&i915->runtime_pm, wakeref)
		i915_driver_register(i915);

	if (i915_inject_probe_failure(pci_get_drvdata(pdev))) {
		err = -ENODEV;
		goto err_remove;
	}

	err = i915_live_selftests(pdev);
	if (err)
		goto err_remove;

	err = i915_wip_selftests(pdev);
	if (err)
		goto err_remove;

	err = i915_perf_selftests(pdev);
	if (err)
		goto err_remove;

	if (i915_save_pci_state(pdev))
		pci_restore_state(pdev);

	pvc_wa_allow_rc6(i915);
	return 0;

err_remove:
	pvc_wa_allow_rc6(i915);
	i915_pci_remove(pdev);
	return err > 0 ? -ENOTTY : err;
}

static void i915_pci_shutdown(struct pci_dev *pdev)
{
	struct drm_i915_private *i915 = pci_get_drvdata(pdev);

	if (IS_SRIOV_PF(i915))
		i915_sriov_pf_disable_vfs(i915);

	i915_driver_shutdown(i915);

	/*
	 * Shutdown is fast and dirty, just enough to make the system safe, and
	 * may leave the driver in an inconsistent state. Make sure we no longer
	 * access the device again.
	 */
	i915->do_release = IS_SRIOV_VF(i915);
	pci_set_drvdata(pdev, NULL);
}

/**
 * i915_pci_sriov_configure - Configure SR-IOV (enable/disable VFs).
 * @pdev: pci_dev struct
 * @num_vfs: number of VFs to enable (or zero to disable all)
 *
 * This function will be called when user requests SR-IOV configuration via the
 * sysfs interface. Note that VFs configuration can be done only on the PF and
 * after successful PF initialization.
 *
 * Return: number of configured VFs or a negative error code on failure.
 */
static int i915_pci_sriov_configure(struct pci_dev *pdev, int num_vfs)
{
	struct drm_device *dev = pci_get_drvdata(pdev);
	struct drm_i915_private *i915 = to_i915(dev);
	int ret;

	/* handled in drivers/pci/pci-sysfs.c */
	GEM_BUG_ON(num_vfs < 0);
	GEM_BUG_ON(num_vfs > U16_MAX);
	GEM_BUG_ON(num_vfs > pci_sriov_get_totalvfs(pdev));
	GEM_BUG_ON(num_vfs && pci_num_vf(pdev));
	GEM_BUG_ON(!num_vfs && !pci_num_vf(pdev));

	if (!IS_SRIOV_PF(i915))
		return -ENODEV;

	if (num_vfs > 0)
		ret = i915_sriov_pf_enable_vfs(i915, num_vfs);
	else
		ret = i915_sriov_pf_disable_vfs(i915);

	return ret;
}

extern const struct pci_error_handlers i915_pci_err_handlers;

static struct pci_driver i915_pci_driver = {
	.name = DRIVER_NAME,
	.id_table = pciidlist,
	.probe = i915_pci_probe,
	.remove = i915_pci_remove,
	.shutdown = i915_pci_shutdown,
	.driver.pm = &i915_pm_ops,
	.sriov_configure = i915_pci_sriov_configure,
	.err_handler = &i915_pci_err_handlers,
};

#ifdef CONFIG_PCI_IOV
/* our Gen12 SR-IOV platforms are simple */
#define GEN12_VF_OFFSET 1
#define GEN12_VF_STRIDE 1
#define GEN12_VF_ROUTING_OFFSET(id) (GEN12_VF_OFFSET + ((id) - 1) * GEN12_VF_STRIDE)

struct pci_dev *i915_pci_pf_get_vf_dev(struct pci_dev *pdev, unsigned int id)
{
	u16 vf_devid = pci_dev_id(pdev) + GEN12_VF_ROUTING_OFFSET(id);

	GEM_BUG_ON(!dev_is_pf(&pdev->dev));
	GEM_BUG_ON(!id);
	GEM_BUG_ON(id > pci_num_vf(pdev));

	/* caller must use pci_dev_put() */
	return pci_get_domain_bus_and_slot(pci_domain_nr(pdev->bus),
					   PCI_BUS_NUM(vf_devid),
					   PCI_DEVFN(PCI_SLOT(vf_devid),
						     PCI_FUNC(vf_devid)));
}
#endif

int i915_pci_register_driver(void)
{
	return pci_register_driver(&i915_pci_driver);
}

void i915_pci_unregister_driver(void)
{
	pci_unregister_driver(&i915_pci_driver);
}
