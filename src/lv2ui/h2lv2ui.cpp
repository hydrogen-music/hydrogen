/*
  LV2 Sampler Example Plugin UI
  Copyright 2011-2016 David Robillard <d@drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include "lv2/atom/atom.h"
#include "lv2/atom/forge.h"
#include "uris.h"

#include "lv2/atom/util.h"
#include "lv2/core/lv2.h"
#include "lv2/core/lv2_util.h"
#include "lv2/log/log.h"
#include "lv2/log/logger.h"
#include "lv2/midi/midi.h"
#include "lv2/ui/ui.h"
#include "lv2/urid/urid.h"

#include <QtGui>
#include <QtWidgets>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <hydrogen/helpers/filesystem.h>

#define H2_UI_URI "http://hydrogen-music.org/plugins/hydrogen#ui"

constexpr int minWidgetHeight = 400;
constexpr int minWidgetWidth = 300;

typedef struct {
	LV2_Atom_Forge       forge;
	LV2_URID_Map*        map;
	//LV2UI_Request_Value* request_value;
	LV2_Log_Logger       logger;
	SamplerURIs          uris;

	LV2UI_Write_Function write;
	LV2UI_Controller     controller;

	char*                filename;

	uint8_t forge_buf[1024];

	bool       did_init;
} H2_LV2UI;


static LV2UI_Handle
instantiate(const LV2UI_Descriptor*   descriptor,
            const char*               plugin_uri,
            const char*               bundle_path,
            LV2UI_Write_Function      write_function,
            LV2UI_Controller          controller,
            LV2UI_Widget*             widget,
            const LV2_Feature* const* features)
{
	H2_LV2UI* pUI = (H2_LV2UI*)calloc(1, sizeof(H2_LV2UI));
	if (!pUI) {
		return nullptr;
	}
	
	unsigned logLevelOpt = H2Core::Logger::Error;
	H2Core::Logger::create_instance();
	H2Core::Logger::set_bit_mask( logLevelOpt );
	H2Core::Filesystem::bootstrap(H2Core::Logger::get_instance());
	QStringList SystemDrumkits = H2Core::Filesystem::sys_drumkit_list();
	QStringList UserDrumkits = H2Core::Filesystem::usr_drumkit_list();

	pUI->write      = write_function;
	pUI->controller = controller;
	*widget        = nullptr;

	pUI->did_init   = false;

	// Get host features
	const char* missing = lv2_features_query(
		features,
		LV2_LOG__log,         &pUI->logger.log ,   false,
		LV2_URID__map,        &pUI->map,           true,
		nullptr);
	
	lv2_log_logger_set_map(&pUI->logger, pUI->map);
	if (missing) {
		lv2_log_error(&pUI->logger, "Missing feature <%s>\n", missing);
		free(pUI);
		return nullptr;
	}

	// Map URIs and initialise forge
	map_sampler_uris(pUI->map, &pUI->uris);
	lv2_atom_forge_init(&pUI->forge, pUI->map);
	
	QWidget *pMainWidget = new QWidget;

	 pMainWidget->setMinimumSize(minWidgetHeight, minWidgetWidth);
	 pMainWidget->setMaximumSize(minWidgetHeight, minWidgetWidth);
	 pMainWidget->setStyleSheet("background-image: url(/home/sebastian/src/hydrogen/data/img/gray/splash/splash.png)");

	 // create the GUI
	 QVBoxLayout* pVerticalLayout = new QVBoxLayout;
	 pMainWidget->setLayout(pVerticalLayout);
	 QLabel *label = new QLabel("Hello, user!");
	 label->setAlignment(Qt::AlignCenter|Qt::AlignHCenter);
	 pVerticalLayout->addWidget(label);
	 
	 QComboBox* pComboBox = new QComboBox();
	 
	 for(const auto& sysDkString : SystemDrumkits)
	 {
		pComboBox->addItem(sysDkString);
	 }
	 
	 for(const auto& usrDkString : UserDrumkits)
	 {
		pComboBox->addItem(usrDkString);
	 }
	 

	 pVerticalLayout->addWidget(pComboBox);

	// Request state (current drumkit) from plugin
	lv2_atom_forge_set_buffer(&pUI->forge, pUI->forge_buf, sizeof(pUI->forge_buf));
	LV2_Atom_Forge_Frame frame;
	LV2_Atom*            msg = (LV2_Atom*)lv2_atom_forge_object(
		&pUI->forge, &frame, 0, pUI->uris.patch_Get);
	lv2_atom_forge_pop(&pUI->forge, &frame);

	pUI->write(pUI->controller, 0, lv2_atom_total_size(msg),
	          pUI->uris.atom_eventTransfer,
	          msg);
	
	lv2_log_error(&pUI->logger, "Hallo!");

	*widget = pMainWidget;

	return pUI;
}

static void
cleanup(LV2UI_Handle handle)
{
	H2_LV2UI* ui = (H2_LV2UI*)handle;

	/*
	if (ui->window) {
		destroy_window(ui);
	}*/


	free(ui);
}

static void
port_event(LV2UI_Handle handle,
           uint32_t     port_index,
           uint32_t     buffer_size,
           uint32_t     format,
           const void*  buffer)
{
/*
	SamplerUI* ui = (SamplerUI*)handle;
	if (format == ui->uris.atom_eventTransfer) {
		const LV2_Atom* atom = (const LV2_Atom*)buffer;
		if (lv2_atom_forge_is_object_type(&ui->forge, atom->type)) {
			const LV2_Atom_Object* obj = (const LV2_Atom_Object*)atom;
			if (obj->body.otype == ui->uris.patch_Set) {
				const char* path = read_set_file(&ui->uris, obj);
				if (path && (!ui->filename || strcmp(path, ui->filename))) {
					g_free(ui->filename);
					ui->filename = g_strdup(path);
					gtk_file_chooser_set_filename(
						GTK_FILE_CHOOSER(ui->file_button), path);
					peaks_receiver_clear(&ui->precv);
					ui->requested_n_peaks = 0;
					request_peaks(ui, ui->width / 2 * 2);
				} else if (!path) {
					lv2_log_warning(&ui->logger, "Set message has no path\n");
				}
			} else if (obj->body.otype == ui->precv.uris.peaks_PeakUpdate) {
				if (!peaks_receiver_receive(&ui->precv, obj)) {
					gtk_widget_queue_draw(ui->canvas);
				}
			}
		} else {
			lv2_log_error(&ui->logger, "Unknown message type\n");
		}
	} else {
		lv2_log_warning(&ui->logger, "Unknown port event format\n");
	}
*/
}

/* Optional non-embedded UI show interface. */
static int
ui_show(LV2UI_Handle handle)
{
	H2_LV2UI* ui = (H2_LV2UI*)handle;
	/*
	if (ui->window) {
		return 0;
	}

	if (!ui->did_init) {
		int argc = 0;
		gtk_init_check(&argc, NULL);
		g_object_ref(ui->box);
		ui->did_init = true;
	}

	ui->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_container_add(GTK_CONTAINER(ui->window), ui->box);

	g_signal_connect(G_OBJECT(ui->window),
	                 "delete-event",
	                 G_CALLBACK(on_window_closed),
	                 handle);

	gtk_widget_show_all(ui->window);
	gtk_window_present(GTK_WINDOW(ui->window));
	*/
	return 0;
}

/* Optional non-embedded UI hide interface. */
static int
ui_hide(LV2UI_Handle handle)
{
	H2_LV2UI* ui = (H2_LV2UI*)handle;

	/*
	if (ui->window) {
		destroy_window(ui);
	}
	*/

	return 0;
}

/* Idle interface for optional non-embedded UI. */
static int
ui_idle(LV2UI_Handle handle)
{
	H2_LV2UI* ui = (H2_LV2UI*)handle;
	
	/*
	if (ui->window) {
		gtk_main_iteration_do(false);
	}
	*/
	return 0;
	
}

static const void*
extension_data(const char* uri)
{
	static const LV2UI_Show_Interface show = { ui_show, ui_hide };
	static const LV2UI_Idle_Interface idle = { ui_idle };
	if (!strcmp(uri, LV2_UI__showInterface)) {
		return &show;
	} else if (!strcmp(uri, LV2_UI__idleInterface)) {
		return &idle;
	}
	return nullptr;
}

static const LV2UI_Descriptor descriptor = {
	H2_UI_URI,
	instantiate,
	cleanup,
	port_event,
	extension_data
};

LV2_SYMBOL_EXPORT
const LV2UI_Descriptor*
lv2ui_descriptor(uint32_t index)
{
	switch (index) {
	case 0:
		return &descriptor;
	default:
		return nullptr;
	}
}
