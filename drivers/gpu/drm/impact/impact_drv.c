/* SPDX-License-Identifier: GPL-2.0 */

#include <linux/module.h>
#include <linux/platform_device.h>

#include <drm/drm_atomic_helper.h>
#include <drm/drm_atomic_state_helper.h>
#include <drm/drm_connector.h>
#include <drm/drm_damage_helper.h>
#include <drm/drm_drv.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_file.h>
#include <drm/drm_format_helper.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_gem_shmem_helper.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_ioctl.h>
#include <drm/drm_modeset_helper.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_simple_kms_helper.h>
#include <drm/drm_vblank.h>

#include <video/impact.h>

#define ALPHA_WAR		33

struct impact_device {
	struct drm_device		dev;
	struct drm_simple_display_pipe	pipe;
	struct drm_connector		conn;
	void __iomem			*regs;
};

static inline struct impact_device* impact_device_of_dev(
	struct drm_device *dev)
{
	return container_of(dev, struct impact_device, dev);
}

static inline void impact_wait_cfifo_empty(void *regs)
{
	while (IMP_FIFOSTATUS(regs) & MSK_CFIFO_CNT)
		cpu_relax();
}

static void impact_hw_init(void *regs)
{
	/* CFIFO parameters */
	IMP_CFIFO_HW(regs) = VAL_CFIFO_HW;
	IMP_CFIFO_LW(regs) = VAL_CFIFO_LW;
	IMP_CFIFO_DELAY(regs) = VAL_CFIFO_DELAY;

	/* DFIFO parameters */
	IMP_DFIFO_HW(regs) = VAL_DFIFO_HW;
	IMP_DFIFO_LW(regs) = VAL_DFIFO_LW;
	IMP_DFIFO_DELAY(regs) = VAL_DFIFO_DELAY;

	/* Cursor Begone! (disable DISPLAY bit) */
	IMP_VC3_INDEXDATA(regs) = 0x1d000100;

	/* Transfer mask registers. */
	IMP_CFIFO(regs) = IMP_CMD_COLORMASKLSBSA(0xffffff);
	IMP_CFIFO(regs) = IMP_CMD_COLORMASKLSBSB(0xffffff);
	IMP_CFIFO(regs) = IMP_CMD_COLORMASKMSBS(0);
	IMP_CFIFO(regs) = IMP_CMD_XFRMASKLO(0xffffff);
	IMP_CFIFO(regs) = IMP_CMD_XFRMASKHI(0xffffff);

	/* Use the main plane. */
	IMP_CFIFO(regs) = IMP_CMD_DRBPOINTERS(0xc8240);

	/* Set the RE into vertical flip mode. */
	IMP_CFIFO(regs) = IMP_CMD_CONFIG(0xcac);
	IMP_CFIFO(regs) = IMP_CMD_XYWIN(0, 0x3ff);

	/* Set XMAP into 24-bpp mode. */
	IMP_XMAP_PP1SELECT(regs) = 0x01;
	IMP_XMAP_INDEX(regs) = 0x00;
	IMP_XMAP_MAIN_MODE(regs) = 0x07a4;
}

#if 0
/**
 * impact_fb_blank - blank the display.
 * @blank_mode: specifies the blanking mode.
 * @info: struct fb_info pointer to framebuffer data.
 */
static int
impact_fb_blank(int blank_mode, struct fb_info *info)
{
	switch (blank_mode) {
	case FB_BLANK_UNBLANK:		/* Unblanking */
		IMP_REG8(regs, 0x61811) = 4;
		IMP_REG8(regs, 0x61910) = 0xff;
		IMP_REG8(regs, 0x60d88) = 3;
		break;
	case FB_BLANK_NORMAL:		/* Normal blanking */
	case FB_BLANK_VSYNC_SUSPEND:	/* VESA blank (vsync off) */
	case FB_BLANK_HSYNC_SUSPEND:	/* VESA blank (hsync off) */
	case FB_BLANK_POWERDOWN:	/* Poweroff */
		IMP_REG8(regs, 0x61811) = 4;
		IMP_REG8(regs, 0x61910) = 0;
		IMP_REG8(regs, 0x60d88) = 1;
		break;
	}

	return 0;
}
#endif

static int impact_fb_blit_rect(struct drm_framebuffer *fb,
			       struct drm_rect *rect)
{
	struct impact_device *idev = impact_device_of_dev(fb->dev);
	unsigned long offset;
	int width, height;
	int i, u, v;
	void *regs = idev->regs;
	void *vmap;
	bool bit;
	u32 *dp;

	vmap = drm_gem_shmem_vmap(fb->obj[0]);
	if (!vmap)
		return -ENOMEM;

	width = rect->x2 - rect->x1;
	height = rect->y2 - rect->y1;
	offset = rect->y1 * fb->pitches[0] + rect->x1 * fb->format->cpp[0];

	/* Setup PIO to RE. */
	impact_wait_cfifo_empty(regs);
	IMP_CFIFO(regs) = IMP_CMD_PP1FILLMODE(0x6300, IMP_LO_COPY);
	IMP_CFIFO(regs) = IMP_CMD_BLOCKXYSTARTI(rect->x1, rect->y1);
	IMP_CFIFO(regs) = IMP_CMD_BLOCKXYENDI(rect->x2 - 1, rect->y2 - 1);
	IMP_CFIFO(regs) = IMP_CMD_FILLMODE(0x00c00000);
	IMP_CFIFO(regs) = IMP_CMD_XFRMODE(0x00080);
	IMP_CFIFO(regs) = IMP_CMD_XFRSIZE(width, height);
	IMP_CFIFO(regs) = IMP_CMD_XFRCOUNTERS(width, height);
	IMP_CFIFO(regs) = IMP_CMD_GLINE_XSTARTF(1);
	IMP_CFIFO(regs) = IMP_CMD_IR_ALIAS(0x18);

	/* XXX: Another workaround, 33 writes to alpha. */
	for (i = 0; i < ALPHA_WAR; i++)
		IMP_CFIFO(regs) = IMP_CMD_ALPHA(0);
	IMP_CFIFO(regs) = IMP_CMD_XFRCONTROL(2);

	/* Pairs of pixels are sent in two writes to the RE. */
	bit = false;
	for (v = 0; v < height; v++) {
		dp = vmap + offset + (v * fb->pitches[0]);
		for (u = 0; u < width; u++) {
			if (bit)
				IMP_CFIFO(regs) = IMP_CMD_CHAR_L(dp[u]);
			else
				IMP_CFIFO(regs) = IMP_CMD_CHAR_H(dp[u]);
			bit ^= true;
		}
	}
	if (bit)
		IMP_CFIFO(regs) = IMP_CMD_CHAR_L(0);

	IMP_CFIFO(regs) = IMP_CMD_GLINE_XSTARTF(0);
	IMP_CFIFO(regs) = IMP_CMD_RE_TOGGLECNTX(0);
	IMP_CFIFO(regs) = IMP_CMD_XFRCOUNTERS(0, 0);

	drm_gem_shmem_vunmap(fb->obj[0], vmap);
	return 0;
}

static enum drm_mode_status impact_pipe_mode_valid(struct drm_simple_display_pipe *pipe,
						   const struct drm_display_mode *mode)
{
	// XXX
	return MODE_OK;
}

static int impact_pipe_check(struct drm_simple_display_pipe *pipe,
			     struct drm_plane_state *plane_state,
			     struct drm_crtc_state *crtc_state)
{
	struct drm_framebuffer *fb = plane_state->fb;

	if (!fb)
		return 0;

	return 0; // XXX
}

static void impact_pipe_enable(struct drm_simple_display_pipe *pipe,
			       struct drm_crtc_state *crtc_state,
			       struct drm_plane_state *plane_state)
{
	struct drm_rect fullscreen = {
		.x1 = 0,
		.x2 = plane_state->fb->width,
		.y1 = 0,
		.y2 = plane_state->fb->height,
	};

	impact_fb_blit_rect(plane_state->fb, &fullscreen);
}

static void impact_pipe_update(struct drm_simple_display_pipe *pipe,
			       struct drm_plane_state *old_state)
{
	struct drm_plane_state *state = pipe->plane.state;
	struct drm_crtc *crtc = &pipe->crtc;
	struct drm_rect rect;

	if (drm_atomic_helper_damage_merged(old_state, state, &rect))
		impact_fb_blit_rect(pipe->plane.state->fb, &rect);

	if (crtc->state->event) {
		spin_lock_irq(&crtc->dev->event_lock);
		drm_crtc_send_vblank_event(crtc, crtc->state->event);
		crtc->state->event = NULL;
		spin_unlock_irq(&crtc->dev->event_lock);
	}
}

static const struct drm_simple_display_pipe_funcs impact_pipe_funcs = {
	.mode_valid = impact_pipe_mode_valid,
	.check      = impact_pipe_check,
	.enable     = impact_pipe_enable,
	.update     = impact_pipe_update,
};

static int impact_conn_get_modes(struct drm_connector *conn)
{
	int count;

	count = drm_add_modes_noedid(conn,
				     conn->dev->mode_config.max_width,
				     conn->dev->mode_config.max_height);
	drm_set_preferred_mode(conn, 1280, 1024);
	return count;
}

static const struct drm_connector_helper_funcs impact_conn_helper_funcs = {
	.get_modes = impact_conn_get_modes,
};

static const struct drm_mode_config_funcs impact_mode_config_funcs = {
	.fb_create		= drm_gem_fb_create_with_dirty,
	.atomic_check		= drm_atomic_helper_check,
	.atomic_commit		= drm_atomic_helper_commit,
};

static const struct drm_connector_funcs impact_conn_funcs = {
	.fill_modes = drm_helper_probe_single_connector_modes,
	.destroy = drm_connector_cleanup,
	.reset = drm_atomic_helper_connector_reset,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
};

static int impact_device_init(struct impact_device *idev,
			      struct drm_driver *drv, struct device *dev)
{
	static const uint32_t formats[] = {
		DRM_FORMAT_XRGB8888,
	};
	static const uint64_t modifiers[] = {
		DRM_FORMAT_MOD_LINEAR,
		DRM_FORMAT_MOD_INVALID
	};
	int ret;

	ret = drm_dev_init(&idev->dev, drv, dev);
	if (ret)
		return ret;
	idev->dev.dev_private = idev;

	drm_mode_config_init(&idev->dev);
	idev->dev.mode_config.max_width = 1280;
	idev->dev.mode_config.max_height = 1024;
	idev->dev.mode_config.preferred_depth = 32;
	idev->dev.mode_config.prefer_shadow = 0;
	idev->dev.mode_config.funcs = &impact_mode_config_funcs;

	drm_connector_helper_add(&idev->conn, &impact_conn_helper_funcs);
	ret = drm_connector_init(&idev->dev, &idev->conn, &impact_conn_funcs,
				 DRM_MODE_CONNECTOR_VGA);
	if (ret)
		goto err_cleanup;

	ret = drm_simple_display_pipe_init(&idev->dev, &idev->pipe,
					   &impact_pipe_funcs,
					   formats, ARRAY_SIZE(formats),
					   modifiers, &idev->conn);
	if (ret)
		goto err_cleanup;

	drm_mode_config_reset(&idev->dev);

	ret = drm_dev_register(&idev->dev, 0);
	if (ret)
		goto err_cleanup;

	ret = drm_fbdev_generic_setup(&idev->dev, 0);
	if (ret)
		goto err_cleanup;

	return 0;

err_cleanup:
	/* cleans up all mode-setting structures */
	drm_mode_config_cleanup(&idev->dev);
	drm_dev_fini(&idev->dev);
	return ret;
}

static void impact_device_cleanup(struct impact_device *idev)
{
	struct drm_device *dev = &idev->dev;

	drm_mode_config_cleanup(dev);

	drm_dev_fini(dev);
	dev->dev_private = NULL;
}

DEFINE_DRM_GEM_FOPS(impact_file_operations);

static struct drm_driver impact_driver = {
	.driver_features = DRIVER_ATOMIC | DRIVER_GEM | DRIVER_MODESET,
	.name = "impact",
	.desc = "DRM driver for SGI Impact",
	.date = "20200213",
	.major = 0,
	.minor = 0,
	.patchlevel = 1,
	.fops = &impact_file_operations,
	DRM_GEM_SHMEM_DRIVER_OPS,
};

static void impact_remove_conflicting_framebuffers(struct platform_device *pdev)
{
	struct apertures_struct *ap;
	struct resource *res;
	bool primary = false;

	ap = alloc_apertures(1);
	if (!ap)
		return;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	ap->ranges[0].base = res->start;
	ap->ranges[0].size = resource_size(res);

#ifdef CONFIG_X86
	primary = pdev->resource[PCI_ROM_RESOURCE].flags &
		IORESOURCE_ROM_SHADOW;
#endif
	drm_fb_helper_remove_conflicting_framebuffers(ap, "impact",
						      primary);
	kfree(ap);
}

static int impact_drm_probe(struct platform_device *pdev)
{
	struct impact_device *idev;

	idev = devm_kzalloc(&pdev->dev, sizeof(*idev), GFP_KERNEL);
	if (!idev)
		return -ENOMEM;

        idev->regs = devm_platform_ioremap_resource(pdev, 0);
	if (!idev->regs)
		return -ENOMEM;

	platform_set_drvdata(pdev, idev);

	impact_hw_init(idev->regs);

	impact_remove_conflicting_framebuffers(pdev);

	return impact_device_init(idev, &impact_driver, &pdev->dev);
}

static int impact_drm_remove(struct platform_device *pdev)
{
	struct impact_device *idev = platform_get_drvdata(pdev);

	impact_device_cleanup(idev);
	return 0;
}

static struct platform_driver impact_drm_driver = {
	.probe = impact_drm_probe,
	.remove = impact_drm_remove,
	.driver = {
		.name = "impact",
	},
};

MODULE_ALIAS("platform:impact");

module_platform_driver(impact_drm_driver);

MODULE_AUTHOR("Thomas Bogendoerfer <tsbogend@alpha.franken.de>");
MODULE_DESCRIPTION("SGI Impact/MardiGras Video Driver");
MODULE_LICENSE("GPL");
