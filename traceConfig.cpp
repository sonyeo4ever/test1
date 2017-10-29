#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <iostream>
#include <list>
#include <string>
#include "traceReplay.h"
#include "cJSON.h"

static int trace_replay(char *config_name, int day);
static int parse_multimedia(cJSON *multi_obj, struct Config *config);
static int parse_basic_app(cJSON *basic_obj, struct Config *config);
static int parse_normal_app(cJSON *normal_obj, struct Config *config);
static int parse_apps(cJSON *apps_obj, struct App *apps, 
			double default_update, double default_loading);

int parse_config(char *config_name, struct Config *config)
{
	FILE *config_fp;
	long len = 0;
	char *config_data;
	int ret = 0;
	cJSON *root_obj;
	cJSON *init_filemap_obj;
	cJSON *multimedia_obj;
	cJSON *basic_app_obj;
	cJSON *normal_app_obj;
	cJSON *ps_name_obj;

	config_fp = fopen(config_name, "r");
	if (config_fp == NULL) {
		printf("error: cannot read %s\n", config_name);
		ret = -1;
		return -1;
	}
	
	fseek(config_fp, 0, SEEK_END);
	len = ftell(config_fp);
	fseek(config_fp, 0, SEEK_SET);

	config_data = (char *)malloc(len);
	if (config_data == NULL) {
		printf("error: failed malloc\n");
		fclose(config_fp);
		return -1;
	}
	fread(config_data, sizeof(char), len, config_fp);

	root_obj = cJSON_Parse(config_data);
	if (root_obj == NULL) {
		printf("error: %s\n", cJSON_GetErrorPtr());
		ret = -1;
		goto out;
	}

	// INIT_FILEMAP
	memset(config->INIT_FILEMAP, 0, PATH_MAX);
	init_filemap_obj = cJSON_GetObjectItem(root_obj, "INIT_FILEMAP");
	if (init_filemap_obj == NULL) {
		sprintf(config->INIT_FILEMAP, "NULL");
	} else {
		sprintf(config->INIT_FILEMAP, "%s", init_filemap_obj->valuestring);
	}
	printf("%s\n", config->INIT_FILEMAP);

	// MULTIMEDIA
	multimedia_obj = cJSON_GetObjectItem(root_obj, "MULTIMEDIA");
	if (multimedia_obj == NULL) {
		printf("error: Parse JSON file, No MULTIMEDIA\n");
		ret = -1;
		goto out;
	} else {
		ret = parse_multimedia(multimedia_obj, config);
		if (ret == -1)
			goto out;
	}

	basic_app_obj = cJSON_GetObjectItem(root_obj, "BASIC_APP");
	if (basic_app_obj == NULL) {
		printf("error: Parse JSON file, No BASIC_APP\n");
		ret = -1;
		goto out;
	} else {
		ret = parse_basic_app(basic_app_obj, config);
		if (ret == -1)
			goto out;
	}

	normal_app_obj = cJSON_GetObjectItem(root_obj, "NORMAL_APP");
	if (normal_app_obj == NULL) {
		printf("error: Parse JSON file, No NORMAL_APP\n");
		ret = -1;
		goto out;
	} else {
		ret = parse_normal_app(normal_app_obj, config);
		if (ret == -1)
			goto out;
	}

	ps_name_obj = cJSON_GetObjectItem(root_obj, "PROCESS");
	if (normal_app_obj == NULL) {
		printf("warning: Failed Parse JSON file, No PROCESS\n");
		ret = 0;
		goto out;
	} else {
		ret = parse_ps_name(ps_name_obj, config);
		if (ret == -1)
			goto out;
	}


out:
	fclose(config_fp);
	free(config_data);
	return ret;
}

static int parse_multimedia(cJSON *multi_obj, struct Config *config)
{
	cJSON *camera_obj;
	cJSON *camera_path_obj;
	cJSON *camera_multipath_obj;
	cJSON *camera_takecount_obj;
	cJSON *camera_deletecount_obj;

	camera_obj = cJSON_GetObjectItem(multi_obj, "CAMERA");

	if (camera_obj == NULL) {
		printf("error: No exist CAMERA in MULTIMEDIA");
		return -1;
	}

	camera_path_obj = cJSON_GetObjectItem(camera_obj, "PATH");
	if (camera_path_obj == NULL) {
		printf("error: No exist PATH in CAMERA");
		return -1;
	}
	memset(config->multi.mul_camera.path, 0, PATH_MAX + 1);
	sprintf(config->multi.mul_camera.path, "%s", camera_path_obj->valuestring);

	camera_multipath_obj = cJSON_GetObjectItem(camera_obj, "MULTIMEDIA_PATH");
	if (camera_multipath_obj == NULL) {
		printf("error: No exist MULTIMEDIA_PATH in CAMERA");
		return -1;
	}
	memset(config->multi.mul_camera.multimedia_path, 0, PATH_MAX + 1);
	sprintf(config->multi.mul_camera.multimedia_path, "%s", camera_multipath_obj->valuestring);

	camera_takecount_obj = cJSON_GetObjectItem(camera_obj, "TAKE_COUNT");
	if (camera_takecount_obj == NULL) {
		printf("error: No exist TAKE_COUNT in CAMERA");
		return -1;
	}
	config->multi.mul_camera.take_count = camera_takecount_obj->valueint;

	camera_deletecount_obj = cJSON_GetObjectItem(camera_obj, "DELETE_COUNT");
	if (camera_deletecount_obj == NULL) {
		printf("error: No exist DELETE_COUNT in CAMERA");
		return -1;
	}
	config->multi.mul_camera.delete_count = camera_deletecount_obj->valueint;

	return 0;
}

static int parse_basic_app(cJSON *basic_obj, struct Config *config)
{
	cJSON *default_update_obj;
	cJSON *default_loading_obj;
	cJSON *apps_obj;
	int app_count;

	default_update_obj = cJSON_GetObjectItem(basic_obj, "DEFAULT_UPDATE_CYCLE");
	if (default_update_obj == NULL) {
		printf("error: No exist DEFAULT_UPDATE_CYCLE in BASIC_APP");
		return -1;
	}
	config->basic_app.default_update_cycle = default_update_obj->valuedouble;

	default_loading_obj = cJSON_GetObjectItem(basic_obj, "DEFAULT_LOADING_CYCLE");
	if (default_loading_obj == NULL) {
		printf("error: No exist DEFAULT_LOADING_CYCLE in BASIC_APP");
		return -1;
	}
	config->basic_app.default_loading_cycle = default_loading_obj->valuedouble;

	apps_obj = cJSON_GetObjectItem(basic_obj, "APPS");
	if (apps_obj == NULL) {
		printf("error: No exist APPS in BASIC_APP");
		return -1;
	}
	app_count = cJSON_GetArraySize(apps_obj);
	config->basic_app.app_count = app_count;
	if (app_count > 0) {
		config->basic_app.apps = (struct App*) malloc(sizeof(struct App) * app_count);
		parse_apps(apps_obj, config->basic_app.apps, config->basic_app.default_update_cycle, config->basic_app.default_loading_cycle);
	}
	
	return 0;
}

static int parse_normal_app(cJSON *normal_obj, struct Config *config)
{
	cJSON *default_update_obj;
	cJSON *default_loading_obj;
	cJSON *install_obj;
	cJSON *uninstall_obj;
	cJSON *apps_obj;
	int app_count;

	default_update_obj = cJSON_GetObjectItem(normal_obj, "DEFAULT_UPDATE_CYCLE");
	if (default_update_obj == NULL) {
		printf("error: No exist DEFAULT_UPDATE_CYCLE in NORMAL_APP");
		return -1;
	}
	config->normal_app.default_update_cycle = default_update_obj->valuedouble;

	default_loading_obj = cJSON_GetObjectItem(normal_obj, "DEFAULT_LOADING_CYCLE");
	if (default_loading_obj == NULL) {
		printf("error: No exist DEFAULT_LOADING_CYCLE in NORMAL_APP");
		return -1;
	}
	config->normal_app.default_loading_cycle = default_loading_obj->valuedouble;

	install_obj = cJSON_GetObjectItem(normal_obj, "APP_INSTALL_CYCLE");
	if (install_obj == NULL) {
		printf("error: No exist APP_INSTALL_CYCLE in NORMAL_APP");
		return -1;
	}
	config->normal_app.install_cycle = install_obj->valuedouble;

	uninstall_obj = cJSON_GetObjectItem(normal_obj, "APP_UNINSTALL_CYCLE");
	if (uninstall_obj == NULL) {
		printf("error: No exist APP_UNINSTALL_CYCLE in NORMAL_APP");
		return -1;
	}
	config->normal_app.uninstall_cycle = uninstall_obj->valuedouble;

	apps_obj = cJSON_GetObjectItem(normal_obj, "APPS");
	if (apps_obj == NULL) {
		printf("error: No exist APPS in BASIC_APP");
		return -1;
	}
	app_count = cJSON_GetArraySize(apps_obj);
	config->normal_app.app_count = app_count;
	if (app_count > 0) {
		config->normal_app.apps = (struct App*) malloc(sizeof(struct App) * app_count);
		parse_apps(apps_obj, config->normal_app.apps, config->normal_app.default_update_cycle, config->normal_app.default_loading_cycle);
	}

	return 0;
}

static int parse_apps(cJSON *apps_obj, struct App *apps, 
			double default_update, double default_loading)
{
	int array_size = cJSON_GetArraySize(apps_obj);
	int i;
	if (array_size == 0) {
		return 0;
	}
	
	for (i = 0; i < array_size; i++)
	{
		cJSON* app_obj = cJSON_GetArrayItem(apps_obj, i);
		cJSON* app_name_obj = cJSON_GetObjectItem(app_obj, "NAME");
		cJSON* app_path_obj = cJSON_GetObjectItem(app_obj, "PATH");
		cJSON* app_update_obj = cJSON_GetObjectItem(app_obj, "UPDATE_CYCLE");
		cJSON* app_loading_obj = cJSON_GetObjectItem(app_obj, "LOADING_CYCLE");
		if (app_name_obj == NULL || app_path_obj == NULL) 
			continue;
		memset(apps[i].name, 0, MAX_NAME);
		sprintf(apps[i].name, "%s", app_name_obj->valuestring);
		memset(apps[i].path, 0, PATH_MAX + 1);
		sprintf(apps[i].path, "%s", app_path_obj->valuestring);
		if (app_update_obj == NULL)
			apps[i].update_cycle = default_update;
		else
			apps[i].update_cycle = app_update_obj->valuedouble;
		if (app_loading_obj == NULL)
			apps[i].loading_cycle = default_loading;
		else
			apps[i].loading_cycle = app_loading_obj->valuedouble;
	}

	return 0;
}


static int parse_ps_name(cJSON *ps_name_obj, struct Config *config)
{
	cJSON *install_obj;
	cJSON *update_obj;
	cJSON *loading_obj;
	cJSON *uninstall_obj;

	install_obj = cJSON_GetObjectItem(ps_name_obj, "INSTALL");
	if (install_obj != NULL) {
		int array_size = cJSON_GetArraySize(install_obj);
		for (int i = 0; i < array_size; i++)
		{
			cJSON* app_obj = cJSON_GetArrayItem(install_obj, i);
			String ps_name(app_obj->valuestring);
			config->ps_install.push_back(ps_name);
		}
	}

	update_obj = cJSON_GetObjectItem(ps_name_obj, "UPDATE");
	if (update_obj != NULL) {
		int array_size = cJSON_GetArraySize(update_obj);
		for (int i = 0; i < array_size; i++)
		{
			cJSON* app_obj = cJSON_GetArrayItem(update_obj, i);
			String ps_name(app_obj->valuestring);
			config->ps_update.push_back(ps_name);
		}
	}

	loading_obj = cJSON_GetObjectItem(ps_name_obj, "INSTALL");
	if (loading_obj != NULL) {
		int array_size = cJSON_GetArraySize(loading_obj);
		for (int i = 0; i < array_size; i++)
		{
			cJSON* app_obj = cJSON_GetArrayItem(loading_obj, i);
			String ps_name(app_obj->valuestring);
			config->ps_loading.push_back(ps_name);
		}
	}

	uninstall_obj = cJSON_GetObjectItem(ps_name_obj, "INSTALL");
	if (uninstall_obj != NULL) {
		int array_size = cJSON_GetArraySize(uninstall_obj);
		for (int i = 0; i < array_size; i++)
		{
			cJSON* app_obj = cJSON_GetArrayItem(uninstall_obj, i);
			String ps_name(app_obj->valuestring);
			config->ps_uninstall.push_back(ps_name);
		}
	}

	return 0;
}

/*
int trace_merge(char *config_name)
{
	FILE *config_fp;
	char *config_data;

    config_fp = fopen(config_name, "r");
    if (config_fp == NULL) {
		printf("error: cannot read %s\n", config_name);
		ret = -1;
		return -1;
	}

	fseek(config_fp, 0, SEEK_END);
	len = ftell(config_fp);
	fseek(config_fp, 0, SEEK_SET);

	config_data = (char *)malloc(len);
	if (config_data == NULL) {
		printf("error: failed malloc\n");
		fclose(config_fp);
		return -1;
	}
	fread(config_data, sizeof(char), len, config_fp);

	root_obj = cJSON_Parse(config_data);
	if (root_obj == NULL) {
		printf("error: %s\n", cJSON_GetErrorPtr());
		ret = -1;
		goto out;
	}

out:
	free(config_data);
	fclose(config_fp);
}
*/

