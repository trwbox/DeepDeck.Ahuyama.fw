/**
 * @file nvs_funcs.c
 * @author ElectroNick (nick@dsd.dev)
 * @brief
 * @version 0.1
 * @date 2022-12-10
 *
 * @copyright Copyright (c) 2022
 * Based o the code f Gal Zaidenstein.
 *
 * MIT License
 * Copyright (c) 2022
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * DeepDeck, a product by DeepSea Developments.
 * More info on DeepDeck @ www.deepdeck.co
 * DeepseaDev.com
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "nvs_funcs.h"
#include "keymap.h"
#include <arr_conv.h>

#define NVS_TAG "NVS Storage"

#define KEYMAP_NAMESPACE "keymap_config"
#define ENCODER_NAMESPACE "encoder_config"
#define SLAVE_ENCODER_NAMESPACE "slave_encoder_config"

#include "key_definitions.h"
// #include "keyboard_config.h"
#include "keymap.h"
// #include "plugins.h"

// NameSpaces
#define LAYER_NAMESPACE "layers"

// Keys
#define LAYER_NUM_KEY "layer_num"

const static char *TAG = "NVS LAYERS";

dd_layer *key_layouts;
uint8_t layers_num = 0;

/**
 * @brief Check the number of available entries of the NVS
 *
 */
void nvs_check_memory_status(void)
{
	nvs_stats_t nvs_stats;
	nvs_get_stats(NULL, &nvs_stats);
	ESP_LOGI("NVS Status", "Count: UsedEntries = (%d), FreeEntries = (%d), AllEntries = (%d)",
			 nvs_stats.used_entries, nvs_stats.free_entries, nvs_stats.total_entries);
}

/**
 * @brief Read and returns the number of available layers stored
 *
 * @return uint8_t
 */
uint8_t nvs_read_num_layers(void)
{
	ESP_LOGV(TAG, "READ NUM LAYERS");
	nvs_handle_t nvs_handle;
	uint8_t layer_num = 0;
	esp_err_t error;

	// ESP_ERROR_CHECK(nvs_open(LAYER_NAMESPACE, NVS_READWRITE, &nvs_handle));
	error = nvs_open(LAYER_NAMESPACE, NVS_READWRITE, &nvs_handle);
	if (error == ESP_OK)
	{
		ESP_LOGI(TAG, "LAYER_NAMESPACE  open ---OK");
	}
	else
	{
		ESP_LOGE(TAG, "Error (%s) opening NVS Namespace!: \n", esp_err_to_name(error));
	}

#ifdef LAYER_MODIFICATION_MODE
	nvs_write_default_layers(nvs_handle);
#endif
	// LAYER_MODIFICATION_MODE

	error = nvs_get_u8(nvs_handle, LAYER_NUM_KEY, &layer_num);
	switch (error)
	{
	case ESP_ERR_NVS_NOT_FOUND:
		ESP_LOGE(TAG, "Value not set yet. Running routine to write default values");
		nvs_write_default_layers(nvs_handle);
		ESP_ERROR_CHECK(nvs_get_u8(nvs_handle, LAYER_NUM_KEY, &layer_num));
		break;
	case ESP_OK:
		ESP_LOGI(TAG, "Layers in memory: %d", layer_num);
		break;
	default:
		ESP_LOGE(TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(error));
		break;
	}

	nvs_close(nvs_handle);
	return layer_num;
}

/**
 * @brief Read all the available layers stored in the memory
 *
 * @param layers_array
 */
void nvs_read_layers(dd_layer *layers_array)
{
	ESP_LOGV(TAG, "READ LAYERS");
	nvs_handle_t nvs_layer_handle;
	uint8_t layer_num;
	char layer_num_key[10] = "layer_num";
	char layer_key[10];
	size_t dd_layer_size;

	esp_err_t res;

	ESP_ERROR_CHECK(nvs_open(LAYER_NAMESPACE, NVS_READWRITE, &nvs_layer_handle));

	res = nvs_get_u8(nvs_layer_handle, layer_num_key, &layer_num);
	switch (res)
	{
	case ESP_ERR_NVS_NOT_FOUND:
		ESP_LOGE(TAG, "Value not set yet. Running routine to write default values");
		nvs_write_default_layers(nvs_layer_handle);
		break;
	case ESP_OK:
		ESP_LOGI(TAG, "Layers in memory: %d", layer_num);
		break;
	default:
		ESP_LOGE(TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(res));
		break;
	}

	for (int i = 0; i < layer_num; i++)
	{
		sprintf(layer_key, "layer_%d", i);
		res = nvs_get_blob(nvs_layer_handle, layer_key, (void *)&layers_array[i], &dd_layer_size);
		if (res != ESP_OK)
		{
			ESP_LOGE(TAG, "Error (%s) reading layer %s!\n", esp_err_to_name(res), layer_key);
		}
	}
	nvs_close(nvs_layer_handle);
}

/**
 * @brief Write default layers
 *
 * @param nvs_handle
 */
void nvs_write_default_layers(nvs_handle_t nvs_handle)
{
	char layer_num_key[10] = "layer_num";
	char layer_key[10];
	uint8_t default_layer_num = LAYERS;

	ESP_ERROR_CHECK(nvs_set_u8(nvs_handle, layer_num_key, default_layer_num));

	for (int i = 0; i < default_layer_num; i++)
	{
		sprintf(layer_key, "layer_%d", i);

		ESP_ERROR_CHECK(nvs_set_blob(nvs_handle, layer_key, (void *)default_layouts[i], sizeof(dd_layer)));
		ESP_ERROR_CHECK(nvs_commit(nvs_handle));
	}
}

/**
 * @brief
 *
 * @return esp_err_t
 */
esp_err_t nvs_restore_default_layers()
{
	nvs_handle_t nvs_handle;
	esp_err_t error;
	// ESP_ERROR_CHECK(nvs_open(LAYER_NAMESPACE, NVS_READWRITE, &nvs_handle));
	error = nvs_open(LAYER_NAMESPACE, NVS_READWRITE, &nvs_handle);
	if (error == ESP_OK)
	{
		ESP_LOGI(TAG, "LAYER_NAMESPACE  open ---OK");
	}
	else
	{
		ESP_LOGE(TAG, "Error (%s) opening NVS Namespace!: \n", esp_err_to_name(error));
	}

	nvs_write_default_layers(nvs_handle);
	nvs_close(nvs_handle);
	nvs_load_layouts();
	return ESP_OK;
}

/**
 * @brief
 *
 * @param layer
 * @param layer_num
 * @return esp_err_t
 */
esp_err_t nvs_write_layer(dd_layer layer, uint8_t layer_num)
{
	nvs_handle_t nvs_layer_handle;
	char layer_key[10];

	ESP_ERROR_CHECK(nvs_open(LAYER_NAMESPACE, NVS_READWRITE, &nvs_layer_handle));

	sprintf(layer_key, "layer_%d", layer_num);
	ESP_ERROR_CHECK(nvs_set_blob(nvs_layer_handle, layer_key, (void *)&layer, sizeof(dd_layer)));
	ESP_ERROR_CHECK(nvs_commit(nvs_layer_handle));

	nvs_close(nvs_layer_handle);
	nvs_update_layout_position();
	nvs_load_layouts();

	return ESP_OK;
}

/**
 * @brief
 *
 * @param layer
 * @return esp_err_t
 */
// esp_err_t nvs_create_new_layer(dd_layer layer)
// {
// 	layers_num = nvs_read_num_layers();
// 	layers_num++;
// 	nvs_write_layer(layer, layers_num);
// 	return ESP_OK;
// }

esp_err_t nvs_create_new_layer(dd_layer layer)
{
	int i = 0;
	int count_active = 0;
	nvs_handle_t nvs_handle;
	nvs_handle_t nvs_handle_new;
	esp_err_t error;
	uint8_t layer_num;
	char layer_key[10];

	dd_layer *temp_layout = malloc(layers_num * sizeof(dd_layer));
	nvs_read_layers(temp_layout);

	ESP_ERROR_CHECK(nvs_open(LAYER_NAMESPACE, NVS_READWRITE, &nvs_handle));
	error = nvs_get_u8(nvs_handle, LAYER_NUM_KEY, &layer_num);
	if (error == ESP_OK)
	{
		ESP_LOGI("TAG", "LAYER KEY FOUND ---OK");
		ESP_LOGI("TAG", "Layer QTY %d", layer_num);
	}
	else
	{
		ESP_LOGE("TAG", "Error (%s) READING KEY!: \n", esp_err_to_name(error));
		return ESP_FAIL;
	}

	layer_num++;
	ESP_LOGI("TAG", "New layer QTY %d", layer_num);
	ESP_LOGI("TAG", " Name:%s", layer.name);
	dd_layer *aux = malloc((layers_num + 1) * sizeof(dd_layer));

	// Copia las estructuras que tienen el atributo active en true al arreglo auxiliar
	for (i = 0; i < (layer_num - 1); i++)
	{
		if (temp_layout[i].active == true)
		{
			aux[count_active] = temp_layout[i];
			count_active++;
		}
	}
	aux[count_active] = layer;
	count_active++;
	for (i = 0; i < (layer_num - 1); i++)
	{
		if (temp_layout[i].active == false)
		{
			aux[count_active] = temp_layout[i];
			count_active++;
		}
	}
	ESP_ERROR_CHECK(nvs_open(LAYER_NAMESPACE, NVS_READWRITE, &nvs_handle));
	ESP_ERROR_CHECK(nvs_erase_all(nvs_handle));
	ESP_ERROR_CHECK(nvs_commit(nvs_handle));
	nvs_close(nvs_handle);

	ESP_ERROR_CHECK(nvs_open(LAYER_NAMESPACE, NVS_READWRITE, &nvs_handle_new));
	ESP_ERROR_CHECK(nvs_set_u8(nvs_handle_new, LAYER_NUM_KEY, layer_num));
	ESP_ERROR_CHECK(nvs_commit(nvs_handle_new));

	for (i = 0; i < layer_num; i++)
	{
		sprintf(layer_key, "layer_%d", i);

		ESP_ERROR_CHECK(nvs_set_blob(nvs_handle_new, layer_key, &aux[i], sizeof(dd_layer)));
		ESP_ERROR_CHECK(nvs_commit(nvs_handle_new));
		// ESP_LOGI("new layer", " Name:%s pos[%d]", aux[i].name, i);
	}

	free(temp_layout);
	free(aux);
	nvs_close(nvs_handle_new);
	nvs_load_layouts();
	
	return ESP_OK;
}




/**
 * @brief
 *
 * @param delete_layer_num
 * @return esp_err_t
 */
esp_err_t nvs_delete_layer(uint8_t delete_layer_num)
{
	esp_err_t error;
	uint8_t layer_num;
	char layer_key[10];

	nvs_handle_t nvs_handle;
	// ESP_ERROR_CHECK(nvs_open(LAYER_NAMESPACE, NVS_READWRITE, &nvs_handle));
	error = nvs_open(LAYER_NAMESPACE, NVS_READWRITE, &nvs_handle);
	if (error == ESP_OK)
	{
		ESP_LOGI(TAG, "LAYER_NAMESPACE  open ---OK");
	}
	else
	{
		ESP_LOGE(TAG, "Error (%s) opening NVS Namespace!: \n", esp_err_to_name(error));
	}

	error = nvs_get_u8(nvs_handle, LAYER_NUM_KEY, &layer_num);
	if (error == ESP_OK)
	{
		ESP_LOGI(TAG, "LAYER KEY FOUND ---OK");
	}
	else
	{
		ESP_LOGE(TAG, "Error (%s) READING KEY!: \n", esp_err_to_name(error));
	}

	dd_layer **new_layouts = malloc((layers_num - 1) * sizeof(dd_layer));

	// Copiar los elementos del array original al nuevo array, excepto el elemento a eliminar
	int j = 0;
	for (int i = 0; i < layer_num; i++)
	{
		if (i != delete_layer_num)
		{ // Si no es la posición a eliminar
			new_layouts[j++] = &key_layouts[i];
		}
	}

	ESP_ERROR_CHECK(nvs_set_u8(nvs_handle, LAYER_NUM_KEY, (layer_num - 1)));

	for (int i = 0; i < (layer_num - 1); i++)
	{
		sprintf(layer_key, "layer_%d", i);

		ESP_ERROR_CHECK(nvs_set_blob(nvs_handle, layer_key, (void *)new_layouts[i], sizeof(dd_layer)));
		ESP_ERROR_CHECK(nvs_commit(nvs_handle));
	}

	free(new_layouts);
	nvs_close(nvs_handle);
	nvs_load_layouts();
	if (current_layout > 0)
	{
		current_layout--;
	}

	return ESP_OK;
}

esp_err_t nvs_update_layout_position(void)
{
	int i = 0;
	int count_active = 0;
	nvs_handle_t nvs_handle;
	nvs_handle_t nvs_handle_new;
	esp_err_t error;
	uint8_t layer_num;
	char layer_key[10];
	dd_layer *temp_layout = malloc(layers_num * sizeof(dd_layer));
	nvs_read_layers(temp_layout);

	ESP_ERROR_CHECK(nvs_open(LAYER_NAMESPACE, NVS_READWRITE, &nvs_handle));
	error = nvs_get_u8(nvs_handle, LAYER_NUM_KEY, &layer_num);

	if (error != ESP_OK)
	{
		ESP_LOGE("TAG", "Error (%s) READING KEY!: \n", esp_err_to_name(error));
		return ESP_FAIL;
	}

	dd_layer *aux = malloc((layers_num) * sizeof(dd_layer));

	// Copia las estructuras que tienen el atributo active en true al arreglo auxiliar
	for (i = 0; i < (layer_num); i++)
	{
		if (temp_layout[i].active == true)
		{
			aux[count_active] = temp_layout[i];
			count_active++;
		}
	}
	for (i = 0; i < (layer_num); i++)
	{
		if (temp_layout[i].active == false)
		{
			aux[count_active] = temp_layout[i];
			count_active++;
		}
	}
	ESP_ERROR_CHECK(nvs_open(LAYER_NAMESPACE, NVS_READWRITE, &nvs_handle));
	ESP_ERROR_CHECK(nvs_erase_all(nvs_handle));
	ESP_ERROR_CHECK(nvs_commit(nvs_handle));
	nvs_close(nvs_handle);

	ESP_ERROR_CHECK(nvs_open(LAYER_NAMESPACE, NVS_READWRITE, &nvs_handle_new));
	ESP_ERROR_CHECK(nvs_set_u8(nvs_handle_new, LAYER_NUM_KEY, layer_num));
	ESP_ERROR_CHECK(nvs_commit(nvs_handle_new));

	for (i = 0; i < layer_num; i++)
	{
		sprintf(layer_key, "layer_%d", i);

		ESP_ERROR_CHECK(nvs_set_blob(nvs_handle_new, layer_key, &aux[i], sizeof(dd_layer)));
		ESP_ERROR_CHECK(nvs_commit(nvs_handle_new));
		// ESP_LOGI("new layer", " Name:%s pos[%d]", aux[i].name, i);
	}

	free(temp_layout);
	free(aux);
	nvs_close(nvs_handle_new);
	return ESP_OK;
}

/**
 * @brief load the layouts from nvs
 *
 */
void nvs_load_layouts(void)
{

	ESP_LOGI("NVS_TAG", "LOADING LAYOUTS");
	layers_num = nvs_read_num_layers();
	key_layouts = malloc(layers_num * sizeof(dd_layer));

	nvs_read_layers(key_layouts);
	nvs_check_memory_status();

	for (int i = 0; i < layers_num; i++)
	{
		ESP_LOGI("NVS_TAG", "LAYER NAME %s", key_layouts[i].name);
	}
	// ESP_LOGI("NVS_TAG","Layer names %s, %s, %s",key_layouts[0].name,key_layouts[1].name,key_layouts[2].name);
	ESP_LOGI(NVS_TAG, "Layouts loaded");
}
