/*
 * Copyright (C) OpenTX
 *
 * Based on code named
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "opentx.h"

extern "C" {
#include <yajl/src/api/yajl_parse.h>
#include <yajl/src/yajl_parser.h>
}

enum StructFieldType {
  FIELD_BOOL,
  FIELD_NUMBER,
  FIELD_STRING,
  FIELD_ZSTRING,
  FIELD_CHOICE,
  FIELD_STRUCT,
};

struct StructField {
  const char * name;
  StructFieldType type;
  uint16_t offset;
  uint8_t size;
  const StructField * children;
  uint8_t children_max;
};

const char * countdown_values[] = { "silent", "beep", "voice", "haptic", NULL };

uint32_t getChoiceValue(const char ** values, const char * value, size_t len)
{
  for (uint32_t i=0; values[i]; i++) {
    if (strlen(values[i]) == len && !memcmp(values[i], value, len)) {
      return i;
    }
  }
  return 0;
}

const StructField timer_fields[] = {
  { "name", FIELD_ZSTRING, offsetof(TimerData, name), LEN_TIMER_NAME },
  { "mode", FIELD_NUMBER, offsetof(TimerData, mode), sizeof(TimerData::mode) },
  { "start", FIELD_NUMBER, offsetof(TimerData, start), sizeof(TimerData::start) },
  { "value", FIELD_NUMBER, offsetof(TimerData, value), sizeof(TimerData::value) },
  { "countdown-mode", FIELD_CHOICE, offsetof(TimerData, countdownBeep), sizeof(TimerData::countdownBeep), (const StructField *)countdown_values },
  { "countdown-start", FIELD_NUMBER, offsetof(TimerData, countdownStart), sizeof(TimerData::countdownStart) },
  { "minute-beep", FIELD_NUMBER, offsetof(TimerData, minuteBeep), sizeof(TimerData::minuteBeep) },
  { "countdownBeep", FIELD_NUMBER, offsetof(TimerData, countdownBeep), sizeof(TimerData::countdownBeep) },
  { NULL }
};

const StructField model_fields [] = {
  { "name", FIELD_ZSTRING, offsetof(ModelData, header.name), LEN_MODEL_NAME },
  { "bitmap", FIELD_STRING, offsetof(ModelData, header.bitmap), LEN_BITMAP_NAME },
  { "timers", FIELD_STRUCT, offsetof(ModelData, timers[0]), sizeof(TimerData), timer_fields, MAX_TIMERS },
  { NULL }
};

uint8_t * address = (uint8_t *)&g_model;
const StructField * field = NULL;
const StructField * fields = model_fields;
const StructField * parent_fields = NULL;
const StructField * parent_field = NULL;
uint8_t child_index = 0;

static int reformat_null(void * ctx)
{
  TRACE("null");
  return 0;
}

static int reformat_boolean(void * ctx, int boolean)
{
  if (field && field->type == FIELD_BOOL && boolean) {
    address[field->offset] = 1;
  }
  return 1;
}

static int reformat_number(void * ctx, const char * s, size_t l)
{
  if (field && (field->type == FIELD_NUMBER || field->type == FIELD_CHOICE)) {
    long long number = yajl_parse_integer((const unsigned char *) s, l);
    memcpy(address + field->offset, &number, field->size);
    // TRACE("%s = %d", field ? field->name : "?", number);
  }
  return 1;
}

static int reformat_string(void * ctx, const unsigned char * stringVal, size_t stringLen)
{
  if (field) {
    if (field->type == FIELD_STRING) {
      memcpy(address + field->offset, stringVal, stringLen);
    }
    else if (field->type == FIELD_ZSTRING) {
      str2zchar((char *) address + field->offset, (const char *)stringVal, stringLen);
    }
    else if (field->type == FIELD_CHOICE) {
      uint32_t value = getChoiceValue((const char **)field->children, (const char *)stringVal, stringLen);
      memcpy(address + field->offset, (uint8_t *)&value, field->size);
    }
    // TRACE("%s = %.*s", field ? field->name : "?", stringLen, stringVal);
  }
  return 1;
}

static int reformat_map_key(void * ctx, const unsigned char * stringVal, size_t stringLen)
{
  // TRACE("key %.*s", stringLen, stringVal);
  field = NULL;
  for (uint8_t i=0; fields[i].name; i++) {
    if (strlen(fields[i].name) == stringLen && !memcmp(fields[i].name, stringVal, stringLen)) {
      field = &fields[i];
      break;
    }
  }
  return 1;
}

static int reformat_start_map(void * ctx)
{
  if (field && field->type == FIELD_STRUCT && child_index < field->children_max) {
    parent_fields = fields;
    parent_field = field;
    fields = field->children;
    address += (field->offset + child_index*field->size);
    field = NULL;
  }
  return 1;
}

static int reformat_end_map(void * ctx)
{
  fields = parent_fields;
  field = parent_field;
  parent_fields = NULL;
  parent_field = NULL;
  if (field) {
    address -= (field->offset + child_index*field->size);
  }
  child_index++;
  return 1;
}

static int reformat_start_array(void * ctx)
{
  child_index = 0;
  return 1;
}

static int reformat_end_array(void * ctx)
{
  return 1;
}

static yajl_callbacks callbacks = {
  reformat_null,
  reformat_boolean,
  NULL,
  NULL,
  reformat_number,
  reformat_string,
  reformat_start_map,
  reformat_map_key,
  reformat_end_map,
  reformat_start_array,
  reformat_end_array
};

const char * loadJsonModel(const char * filename, ModelData & model)
{
  yajl_handle hand;
  static unsigned char fileData[64];
  yajl_status stat;
  FIL file;
  UINT read;

  memset(&model, 0, sizeof(ModelData));

  FRESULT result = f_open(&file, filename, FA_OPEN_EXISTING | FA_READ);
  if (result != FR_OK) {
    return SDCARD_ERROR(result);
  }

  /* ok.  open file.  let's read and parse */
  hand = yajl_alloc(&callbacks, NULL, (void *)NULL);
  /* and let's allow comments by default */
  yajl_config(hand, yajl_allow_comments, 1);

  for (;;) {
    result = f_read(&file, (uint8_t *)fileData, sizeof(fileData)-1, &read);
    if (result != FR_OK) {
      f_close(&file);
      return SDCARD_ERROR(result);
    }

    if (read == 0) {
      break;
    }
    fileData[read] = 0;

    stat = yajl_parse(hand, fileData, read);

    if (stat != yajl_status_ok) break;
  }

  stat = yajl_complete_parse(hand);

  if (stat != yajl_status_ok) {
    unsigned char * str = yajl_get_error(hand, 1, fileData, read);
    TRACE("%s", (const char *) str);
    yajl_free_error(hand, str);
    return "JSON ERROR"; // SDCARD_ERROR(-1);
  }

  yajl_free(hand);

  return NULL;
}