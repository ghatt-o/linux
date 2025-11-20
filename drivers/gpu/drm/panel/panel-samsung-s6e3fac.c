// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2024, Linaro Limited

#include <linux/backlight.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/regulator/consumer.h>
#include <linux/module.h>
#include <linux/of.h>

#include <drm/display/drm_dsc.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>
#include <drm/display/drm_dsc.h>
#include <drm/display/drm_dsc_helper.h>

#include <video/mipi_display.h>

struct samsung_s6e3fac {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	struct drm_dsc_config dsc;
	struct gpio_desc *reset_gpio;
	struct regulator_bulk_data *supplies;
};

static const struct regulator_bulk_data samsung_s6e3fac_supplies[] = {
	{ .supply = "vddio" },
	{ .supply = "vci" },
	{ .supply = "vdd" },
};

static inline struct samsung_s6e3fac *to_samsung_s6e3fac(struct drm_panel *panel)
{
	return container_of(panel, struct samsung_s6e3fac, panel);
}

static void samsung_s6e3fac_reset(struct samsung_s6e3fac *ctx)
{
	// gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(10000, 11000);
	// gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	usleep_range(10000, 11000);
	// gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(10000, 11000);
}

static int samsung_s6e3fac_on(struct samsung_s6e3fac *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct mipi_dsi_multi_context dsi_ctx = { .dsi = dsi };
	struct drm_dsc_picture_parameter_set pps;

	dev_info(&dsi->dev, "%s\n", __func__);

	return 0;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	mipi_dsi_dcs_write_long_multi(&dsi_ctx, 0xb0, 0x04);
	mipi_dsi_dcs_write_long_multi(&dsi_ctx, 0xe8, 0x00, 0x02);
	mipi_dsi_dcs_write_long_multi(&dsi_ctx, 0xe4, 0x00, 0x08);
	mipi_dsi_dcs_write_long_multi(&dsi_ctx, 0xb4, 0x20, 0x1c);
	mipi_dsi_dcs_write_long_multi(&dsi_ctx, 0xb6, 0x6c, 0x00, 0x06, 0x23, 0xaf, 0x13, 0x1a, 0x05, 0x04, 0xfa, 0x05, 0x20);
	mipi_dsi_dcs_write_long_multi(&dsi_ctx, 0xb0, 0x00);
	mipi_dsi_dcs_write_long_multi(&dsi_ctx, 0xc4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x29, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
	mipi_dsi_dcs_write_long_multi(&dsi_ctx, 0xd0, 0x44, 0x44, 0xb2, 0x28, 0x00, 0x28, 0x5a, 0x00, 0x5a, 0x03, 0x0d, 0x01);
	mipi_dsi_dcs_write_long_multi(&dsi_ctx, 0xd3, 0x49, 0x00, 0x00, 0x01, 0x1a, 0x15, 0x00, 0x15, 0x07, 0x0f, 0x77, 0x77, 0x77, 0x37, 0xb2, 0x11, 0x00, 0xa0, 0x3c, 0x9a);
	mipi_dsi_dcs_write_long_multi(&dsi_ctx, 0xd8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x30, 0x00, 0x30, 0x00, 0x30, 0x00, 0x30, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x2f, 0x00, 0x0f, 0x00, 0x20);
	mipi_dsi_dcs_write_long_multi(&dsi_ctx, 0xdf, 0x50, 0x42, 0x58, 0x81, 0x2d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0f, 0xff, 0xd4, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x53, 0x18, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
	mipi_dsi_dcs_write_long_multi(&dsi_ctx, 0xeb, 0x8b, 0x8b);
	mipi_dsi_dcs_write_long_multi(&dsi_ctx, 0xf7, 0x01);
	mipi_dsi_dcs_write_long_multi(&dsi_ctx, 0xb0, 0x80);
	mipi_dsi_dcs_write_long_multi(&dsi_ctx, 0xe4, 0x34, 0xb4, 0x00, 0x00, 0x00, 0x30, 0x04, 0x0c, 0xe2);
	mipi_dsi_dcs_write_long_multi(&dsi_ctx, 0xe6, 0x00);
	mipi_dsi_dcs_write_long_multi(&dsi_ctx, 0xb0, 0x04);
	mipi_dsi_dcs_write_long_multi(&dsi_ctx, 0xdf, 0x50, 0x40);
	mipi_dsi_dcs_write_long_multi(&dsi_ctx, 0xf3, 0x50, 0x00, 0x00, 0x00, 0x00);
	mipi_dsi_dcs_write_long_multi(&dsi_ctx, 0xf2, 0x11);
	mipi_dsi_dcs_write_long_multi(&dsi_ctx, 0xf3, 0x01, 0x00, 0x00, 0x00, 0x01);
	mipi_dsi_dcs_write_long_multi(&dsi_ctx, 0xf4, 0x00, 0x02);
	mipi_dsi_dcs_write_long_multi(&dsi_ctx, 0xf2, 0x19);
	mipi_dsi_dcs_write_long_multi(&dsi_ctx, 0xdf, 0x50, 0x42);
	mipi_dsi_dcs_set_tear_on_multi(&dsi_ctx, MIPI_DSI_DCS_TEAR_MODE_VBLANK);
	mipi_dsi_dcs_set_column_address_multi(&dsi_ctx, 0x0000, 1079);
	mipi_dsi_dcs_set_page_address_multi(&dsi_ctx, 0x0000, 2339);
	mipi_dsi_dcs_exit_sleep_mode_multi(&dsi_ctx);
	// mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x11);
	mipi_dsi_msleep(&dsi_ctx, 78);
	// ff 78
	mipi_dsi_dcs_set_display_on_multi(&dsi_ctx);
	// 05 29

	mipi_dsi_msleep(&dsi_ctx, 50);

	drm_dsc_pps_payload_pack(&pps, ctx->dsi->dsc);

	mipi_dsi_picture_parameter_set_multi(&dsi_ctx, &pps);

	// try 1 ?
	mipi_dsi_compression_mode_ext_multi(&dsi_ctx, true,
					    MIPI_DSI_COMPRESSION_DSC, 0);

	return dsi_ctx.accum_err;
}

static void samsung_s6e3fac_off(struct samsung_s6e3fac *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct mipi_dsi_multi_context dsi_ctx = { .dsi = dsi };

	dev_info(&dsi->dev, "%s\n", __func__);

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	// mipi_dsi_dcs_set_display_off_multi(&dsi_ctx);
	// mipi_dsi_msleep(&dsi_ctx, 20);

	// mipi_dsi_dcs_enter_sleep_mode_multi(&dsi_ctx);
	// mipi_dsi_msleep(&dsi_ctx, 120);
}

static int samsung_s6e3fac_prepare(struct drm_panel *panel)
{
	struct samsung_s6e3fac *ctx = to_samsung_s6e3fac(panel);
	int ret;

	dev_info(panel->dev, "%s\n", __func__);

	// ret = regulator_bulk_enable(ARRAY_SIZE(samsung_s6e3fac_supplies),
	// 			    ctx->supplies);
	// if (ret < 0)
	// 	return ret;

	samsung_s6e3fac_reset(ctx);

	ret = samsung_s6e3fac_on(ctx);
	if (ret < 0) {
		// gpiod_set_value_cansleep(ctx->reset_gpio, 1);
		// regulator_bulk_disable(ARRAY_SIZE(samsung_s6e3fac_supplies),
		// 		       ctx->supplies);
		return ret;
	}

	return 0;
}

static int samsung_s6e3fac_unprepare(struct drm_panel *panel)
{
	struct samsung_s6e3fac *ctx = to_samsung_s6e3fac(panel);

	dev_info(panel->dev, "%s\n", __func__);

	samsung_s6e3fac_off(ctx);

	// gpiod_set_value_cansleep(ctx->reset_gpio, 1);

	// regulator_bulk_disable(ARRAY_SIZE(samsung_s6e3fac_supplies),
	// 		       ctx->supplies);

	return 0;
}

static const struct drm_display_mode samsung_s6e3fac_mode = {
	.clock = (1080 + 96 + 40 + 32) * (2340 + 25 + 4 + 4) * 120 / 1000,
	.hdisplay = 1080,
	.hsync_start = 1080 + 96,
	.hsync_end = 1080 + 96 + 40,
	.htotal = 1080 + 96 + 40 + 32,
	.vdisplay = 2340,
	.vsync_start = 2340 + 25,
	.vsync_end = 2340 + 25 + 4,
	.vtotal = 2340 + 25 + 4 + 4,
	.width_mm = 64,
	.height_mm = 140,
	.type = DRM_MODE_TYPE_DRIVER,
};

static int samsung_s6e3fac_get_modes(struct drm_panel *panel,
				       struct drm_connector *connector)
{
	struct drm_display_mode *mode;

	mode = drm_mode_duplicate(connector->dev, &samsung_s6e3fac_mode);
	if (!mode)
		return -ENOMEM;

	drm_mode_set_name(mode);

	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	connector->display_info.width_mm = mode->width_mm;
	connector->display_info.height_mm = mode->height_mm;
	drm_mode_probed_add(connector, mode);

	return 1;
}

static const struct drm_panel_funcs samsung_s6e3fac_panel_funcs = {
	.prepare = samsung_s6e3fac_prepare,
	.unprepare = samsung_s6e3fac_unprepare,
	.get_modes = samsung_s6e3fac_get_modes,
};

static int samsung_s6e3fac_bl_update_status(struct backlight_device *bl)
{
	struct mipi_dsi_device *dsi = bl_get_data(bl);
	u16 brightness = backlight_get_brightness(bl);

	return mipi_dsi_dcs_set_display_brightness_large(dsi, brightness);
}

static const struct backlight_ops samsung_s6e3fac_bl_ops = {
	.update_status = samsung_s6e3fac_bl_update_status,
};

static struct backlight_device *
samsung_s6e3fac_create_backlight(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	const struct backlight_properties props = {
		.type = BACKLIGHT_RAW,
		.brightness = 2050,
		.max_brightness = 4095,
	};

	return devm_backlight_device_register(dev, dev_name(dev), dev, dsi,
					      &samsung_s6e3fac_bl_ops, &props);
}

static int samsung_s6e3fac_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct samsung_s6e3fac *ctx;
	int ret;

	dev_info(dev, "%s\n", __func__);

	ctx = devm_kzalloc(dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	// ret = devm_regulator_bulk_get_const(&dsi->dev,
	// 				    ARRAY_SIZE(samsung_s6e3fac_supplies),
	// 				    samsung_s6e3fac_supplies,
	// 				    &ctx->supplies);
	// if (ret < 0)
	// 	return ret;

	// ret = regulator_bulk_enable(ARRAY_SIZE(samsung_s6e3fac_supplies),
	// 			    ctx->supplies);
	// if (ret < 0)
	// 	return ret;

	// ctx->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_LOW);
	// if (IS_ERR(ctx->reset_gpio))
	// 	return dev_err_probe(dev, PTR_ERR(ctx->reset_gpio),
	// 			     "Failed to get reset-gpios\n");

	ctx->dsi = dsi;
	mipi_dsi_set_drvdata(dsi, ctx);

	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	// dsi->mode_flags = MIPI_DSI_MODE_NO_EOT_PACKET |
	// 		  MIPI_DSI_CLOCK_NON_CONTINUOUS;
	ctx->panel.prepare_prev_first = true;

	drm_panel_init(&ctx->panel, dev, &samsung_s6e3fac_panel_funcs,
		       DRM_MODE_CONNECTOR_DSI);

	ctx->panel.backlight = samsung_s6e3fac_create_backlight(dsi);
	if (IS_ERR(ctx->panel.backlight))
		return dev_err_probe(dev, PTR_ERR(ctx->panel.backlight),
				     "Failed to create backlight\n");

	drm_panel_add(&ctx->panel);

	/* The panel works only in the DSC mode. Set DSC params. */
	ctx->dsc.dsc_version_major = 0x1;
	ctx->dsc.dsc_version_minor = 0x1;

	/* slice_count * slice_width == width */
	ctx->dsc.slice_height = 117;
	ctx->dsc.slice_width = 540;
	ctx->dsc.slice_count = 2;
	ctx->dsc.bits_per_component = 8;
	ctx->dsc.bits_per_pixel = 8 << 4;
	ctx->dsc.block_pred_enable = true;

	dsi->dsc = &ctx->dsc;

	ret = mipi_dsi_attach(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to attach to DSI host: %d\n", ret);
		drm_panel_remove(&ctx->panel);
		return ret;
	}

	return 0;
}

static void samsung_s6e3fac_remove(struct mipi_dsi_device *dsi)
{
	struct samsung_s6e3fac *ctx = mipi_dsi_get_drvdata(dsi);
	int ret;

	ret = mipi_dsi_detach(dsi);
	if (ret < 0)
		dev_err(&dsi->dev, "Failed to detach from DSI host: %d\n", ret);

	drm_panel_remove(&ctx->panel);
}

static const struct of_device_id samsung_s6e3fac_of_match[] = {
	{ .compatible = "samsung,s6e3fac" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, samsung_s6e3fac_of_match);

static struct mipi_dsi_driver samsung_s6e3fac_driver = {
	.probe = samsung_s6e3fac_probe,
	.remove = samsung_s6e3fac_remove,
	.driver = {
		.name = "panel-samsung-s6e3fac",
		.of_match_table = samsung_s6e3fac_of_match,
	},
};
module_mipi_dsi_driver(samsung_s6e3fac_driver);

MODULE_AUTHOR("Neil Armstrong <neil.armstrong@linaro.org>");
MODULE_DESCRIPTION("Panel driver for the samsung s6e3fac AMOLED DSI panel");
MODULE_LICENSE("GPL");
