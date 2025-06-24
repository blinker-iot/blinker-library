/*
  This file is part of the ArduinoBLE library.
  Copyright (c) 2018 Arduino SA. All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

// #include <stdint.h>

#ifndef _BLE_PROPERTY_H_
#define _BLE_PROPERTY_H_

enum BLEProperty {
  BLEBroadcast            = 0x01,
  BLERead                 = 0x02,
  BLEWriteWithoutResponse = 0x04,
  BLEWrite                = 0x08,
  BLENotify               = 0x10,
  BLEIndicate             = 0x20,
  BLEAuthSignedWrite      = 1 << 6,
  BLEExtProp              = 1 << 7,
};

enum BLEPermission {
  BLEEncryption         = 1 << 9,
  BLEAuthentication     = 1 << 10,
  BLEAuthorization      = 1 << 11,
  // BLEWriteEncryption        = 1 << 11,
  // BLEWriteAuthentication    = 1 << 12,
  // BLEWriteAuthorization     = 1 << 13,
};

#define    ESP_GATT_CHAR_PROP_BIT_BROADCAST    (1 << 0)       /* 0x01 */    /* relate to BTA_GATT_CHAR_PROP_BIT_BROADCAST in bta/bta_gatt_api.h */
#define    ESP_GATT_CHAR_PROP_BIT_READ         (1 << 1)       /* 0x02 */    /* relate to BTA_GATT_CHAR_PROP_BIT_READ in bta/bta_gatt_api.h */
#define    ESP_GATT_CHAR_PROP_BIT_WRITE_NR     (1 << 2)       /* 0x04 */    /* relate to BTA_GATT_CHAR_PROP_BIT_WRITE_NR in bta/bta_gatt_api.h */
#define    ESP_GATT_CHAR_PROP_BIT_WRITE        (1 << 3)       /* 0x08 */    /* relate to BTA_GATT_CHAR_PROP_BIT_WRITE in bta/bta_gatt_api.h */
#define    ESP_GATT_CHAR_PROP_BIT_NOTIFY       (1 << 4)       /* 0x10 */    /* relate to BTA_GATT_CHAR_PROP_BIT_NOTIFY in bta/bta_gatt_api.h */
#define    ESP_GATT_CHAR_PROP_BIT_INDICATE     (1 << 5)       /* 0x20 */    /* relate to BTA_GATT_CHAR_PROP_BIT_INDICATE in bta/bta_gatt_api.h */
#define    ESP_GATT_CHAR_PROP_BIT_AUTH         (1 << 6)       /* 0x40 */    /* relate to BTA_GATT_CHAR_PROP_BIT_AUTH in bta/bta_gatt_api.h */
#define    ESP_GATT_CHAR_PROP_BIT_EXT_PROP     (1 << 7)       /* 0x80 */    /* relate to BTA_GATT_CHAR_PROP_BIT_EXT_PROP in bta/bta_gatt_api.h */

#define    ESP_GATT_PERM_READ                  (1 << 0)   /* bit 0 -  0x0001 */    /* relate to BTA_GATT_PERM_READ in bta/bta_gatt_api.h */
#define    ESP_GATT_PERM_READ_ENCRYPTED        (1 << 1)   /* bit 1 -  0x0002 */    /* relate to BTA_GATT_PERM_READ_ENCRYPTED in bta/bta_gatt_api.h */
#define    ESP_GATT_PERM_READ_ENC_MITM         (1 << 2)   /* bit 2 -  0x0004 */    /* relate to BTA_GATT_PERM_READ_ENC_MITM in bta/bta_gatt_api.h */
#define    ESP_GATT_PERM_WRITE                 (1 << 4)   /* bit 4 -  0x0010 */    /* relate to BTA_GATT_PERM_WRITE in bta/bta_gatt_api.h */
#define    ESP_GATT_PERM_WRITE_ENCRYPTED       (1 << 5)   /* bit 5 -  0x0020 */    /* relate to BTA_GATT_PERM_WRITE_ENCRYPTED in bta/bta_gatt_api.h */
#define    ESP_GATT_PERM_WRITE_ENC_MITM        (1 << 6)   /* bit 6 -  0x0040 */    /* relate to BTA_GATT_PERM_WRITE_ENC_MITM in bta/bta_gatt_api.h */
#define    ESP_GATT_PERM_WRITE_SIGNED          (1 << 7)   /* bit 7 -  0x0080 */    /* relate to BTA_GATT_PERM_WRITE_SIGNED in bta/bta_gatt_api.h */
#define    ESP_GATT_PERM_WRITE_SIGNED_MITM     (1 << 8)   /* bit 8 -  0x0100 */    /* relate to BTA_GATT_PERM_WRITE_SIGNED_MITM in bta/bta_gatt_api.h */
#define    ESP_GATT_PERM_READ_AUTHORIZATION    (1 << 9)   /* bit 9 -  0x0200 */
#define    ESP_GATT_PERM_WRITE_AUTHORIZATION   (1 << 10)  /* bit 10 - 0x0400 */

enum BLE_GATT_PERM_ {
  BLE_GATT_READ       = 1 << 0,
  READ_ENCRYPTED      = 1 << 1,
  READ_ENC_MITM       = 1 << 2,
  BLE_GATT_WRITE      = 1 << 4,
  WRITE_ENCRYPTED     = 1 << 5,
  WRITE_ENC_MITM      = 1 << 6,
  WRITE_SIGNED        = 1 << 7,
  WRITE_SIGNED_MITM   = 1 << 8,
  READ_AUTHORIZATION  = 1 << 9,
  WRITE_AUTHORIZATION = 1 << 10,
};


#endif
