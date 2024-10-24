/* main.c - Application main entry point */

/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <stddef.h>
#include <ztest.h>

#include <bluetooth/buf.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/gatt.h>

/* Custom Service Variables */
static struct bt_uuid_128 test_uuid = BT_UUID_INIT_128(
	0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);
static struct bt_uuid_128 test_chrc_uuid = BT_UUID_INIT_128(
	0xf2, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);

static u8_t test_value[] = { 'T', 'e', 's', 't', '\0' };

static struct bt_uuid_128 test1_uuid = BT_UUID_INIT_128(
	0xf4, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);

static const struct bt_uuid_128 test1_nfy_uuid = BT_UUID_INIT_128(
	0xf5, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);

static struct bt_gatt_ccc_cfg test1_ccc_cfg[BT_GATT_CCC_MAX] = {};
static u8_t nfy_enabled;

static void test1_ccc_cfg_changed(const struct bt_gatt_attr *attr, u16_t value)
{
	nfy_enabled = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
}

static ssize_t read_test(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, u16_t len, u16_t offset)
{
	const char *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 strlen(value));
}

static ssize_t write_test(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 const void *buf, u16_t len, u16_t offset,
			 u8_t flags)
{
	u8_t *value = attr->user_data;

	if (offset + len > sizeof(test_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf, len);

	return len;
}

static struct bt_gatt_attr test_attrs[] = {
	/* Vendor Primary Service Declaration */
	BT_GATT_PRIMARY_SERVICE(&test_uuid),

	BT_GATT_CHARACTERISTIC(&test_chrc_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ_AUTHEN |
			       BT_GATT_PERM_WRITE_AUTHEN,
			       read_test, write_test, test_value),
};

static struct bt_gatt_service test_svc = BT_GATT_SERVICE(test_attrs);

static struct bt_gatt_attr test1_attrs[] = {
	/* Vendor Primary Service Declaration */
	BT_GATT_PRIMARY_SERVICE(&test1_uuid),

	BT_GATT_CHARACTERISTIC(&test1_nfy_uuid.uuid,
			       BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_NONE,
			       NULL, NULL, &nfy_enabled),
	BT_GATT_CCC(test1_ccc_cfg, test1_ccc_cfg_changed),
};

static struct bt_gatt_service test1_svc = BT_GATT_SERVICE(test1_attrs);

void test_gatt_register(void)
{
	/* Attempt to register services */
	zassert_false(bt_gatt_service_register(&test_svc),
		     "Test service registration failed");
	zassert_false(bt_gatt_service_register(&test1_svc),
		     "Test service1 registration failed");

	/* Attempt to register already registered services */
	zassert_true(bt_gatt_service_register(&test_svc),
		     "Test service duplicate succeeded");
	zassert_true(bt_gatt_service_register(&test1_svc),
		     "Test service1 duplicate succeeded");
}

void test_gatt_unregister(void)
{
	/* Attempt to unregister last */
	zassert_false(bt_gatt_service_unregister(&test1_svc),
		     "Test service1 unregister failed");
	zassert_false(bt_gatt_service_register(&test1_svc),
		     "Test service1 re-registration failed");

	/* Attempt to unregister first/middle */
	zassert_false(bt_gatt_service_unregister(&test_svc),
		     "Test service unregister failed");
	zassert_false(bt_gatt_service_register(&test_svc),
		     "Test service re-registration failed");

	/* Attempt to unregister all reverse order */
	zassert_false(bt_gatt_service_unregister(&test1_svc),
		     "Test service1 unregister failed");
	zassert_false(bt_gatt_service_unregister(&test_svc),
		     "Test service unregister failed");

	zassert_false(bt_gatt_service_register(&test_svc),
		     "Test service registration failed");
	zassert_false(bt_gatt_service_register(&test1_svc),
		     "Test service1 registration failed");

	/* Attempt to unregister all same order */
	zassert_false(bt_gatt_service_unregister(&test_svc),
		     "Test service1 unregister failed");
	zassert_false(bt_gatt_service_unregister(&test1_svc),
		     "Test service unregister failed");
}

static u8_t count_attr(const struct bt_gatt_attr *attr, void *user_data)
{
	u16_t *count = user_data;

	(*count)++;

	return BT_GATT_ITER_CONTINUE;
}

static u8_t find_attr(const struct bt_gatt_attr *attr, void *user_data)
{
	const struct bt_gatt_attr **tmp = user_data;

	*tmp = attr;

	return BT_GATT_ITER_CONTINUE;
}

void test_gatt_foreach(void)
{
	const struct bt_gatt_attr *attr;
	u16_t num = 0;

	/* Attempt to register services */
	zassert_false(bt_gatt_service_register(&test_svc),
		     "Test service registration failed");
	zassert_false(bt_gatt_service_register(&test1_svc),
		     "Test service1 registration failed");

	/* Iterate attributes */
	bt_gatt_foreach_attr(test_attrs[0].handle, 0xffff, count_attr, &num);
	zassert_equal(num, 7, "Number of attributes don't match");

	/* Iterate 1 attribute */
	num = 0;
	bt_gatt_foreach_attr_type(test_attrs[0].handle, 0xffff, NULL, NULL, 1,
				  count_attr, &num);
	zassert_equal(num, 1, "Number of attributes don't match");

	/* Find attribute by UUID */
	attr = NULL;
	bt_gatt_foreach_attr_type(test_attrs[0].handle, 0xffff,
				  &test_chrc_uuid.uuid, NULL, 0, find_attr,
				  &attr);
	zassert_not_null(attr, "Attribute don't match");
	if (attr) {
		zassert_equal(attr->uuid, &test_chrc_uuid.uuid,
			      "Attribute UUID don't match");
	}

	/* Find attribute by DATA */
	attr = NULL;
	bt_gatt_foreach_attr_type(test_attrs[0].handle, 0xffff, NULL,
				  test_value, 0, find_attr, &attr);
	zassert_not_null(attr, "Attribute don't match");
	if (attr) {
		zassert_equal(attr->user_data, test_value,
			      "Attribute value don't match");
	}

	/* Find all characteristics */
	num = 0;
	bt_gatt_foreach_attr_type(test_attrs[0].handle, 0xffff,
				  BT_UUID_GATT_CHRC, NULL, 0, count_attr, &num);
	zassert_equal(num, 2, "Number of attributes don't match");

	/* Find 1 characteristic */
	attr = NULL;
	bt_gatt_foreach_attr_type(test_attrs[0].handle, 0xffff,
				  BT_UUID_GATT_CHRC, NULL, 1, find_attr, &attr);
	zassert_not_null(attr, "Attribute don't match");

	/* Find attribute by UUID and DATA */
	attr = NULL;
	bt_gatt_foreach_attr_type(test_attrs[0].handle, 0xffff,
				  &test1_nfy_uuid.uuid, &nfy_enabled, 1,
				  find_attr, &attr);
	zassert_not_null(attr, "Attribute don't match");
	if (attr) {
		zassert_equal(attr->uuid, &test1_nfy_uuid.uuid,
			      "Attribute UUID don't match");
		zassert_equal(attr->user_data, &nfy_enabled,
			      "Attribute value don't match");
	}
}

void test_gatt_read(void)
{
	const struct bt_gatt_attr *attr;
	u8_t buf[256];
	ssize_t ret;

	/* Find attribute by UUID */
	attr = NULL;
	bt_gatt_foreach_attr_type(test_attrs[0].handle, 0xffff,
				  &test_chrc_uuid.uuid, NULL, 0, find_attr,
				  &attr);
	zassert_not_null(attr, "Attribute don't match");
	zassert_equal(attr->uuid, &test_chrc_uuid.uuid,
			      "Attribute UUID don't match");

	ret = attr->read(NULL, attr, (void *)buf, sizeof(buf), 0);
	zassert_equal(ret, strlen(test_value),
		      "Attribute read unexpected return");
	zassert_mem_equal(buf, test_value, ret,
			  "Attribute read value don't match");
}

void test_gatt_write(void)
{
	const struct bt_gatt_attr *attr;
	char *value = "    ";
	ssize_t ret;

	/* Find attribute by UUID */
	attr = NULL;
	bt_gatt_foreach_attr_type(test_attrs[0].handle, 0xffff,
				  &test_chrc_uuid.uuid, NULL, 0, find_attr,
				  &attr);
	zassert_not_null(attr, "Attribute don't match");

	ret = attr->write(NULL, attr, (void *)value, strlen(value), 0, 0);
	zassert_equal(ret, strlen(value), "Attribute write unexpected return");
	zassert_mem_equal(value, test_value, ret,
			  "Attribute write value don't match");
}

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_gatt,
			 ztest_unit_test(test_gatt_register),
			 ztest_unit_test(test_gatt_unregister),
			 ztest_unit_test(test_gatt_foreach),
			 ztest_unit_test(test_gatt_read),
			 ztest_unit_test(test_gatt_write));
	ztest_run_test_suite(test_gatt);
}
