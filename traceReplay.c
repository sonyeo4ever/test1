#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "cJSON.h"
#include "traceReplay.h"

struct Config *config;

int parse_config(char *config_name);
static int trace_replay(char *config_name, int day);
static int parse_multimedia(cJSON *multi_obj);
static int parse_basic_app(cJSON *basic_obj);
static int parse_normal_app(cJSON *normal_obj);
static parse_apps(cJSON *apps_obj, struct App *apps, 
			double default_update, double default_loading);

void print_help()
{
	printf("traceReplay 0.1 Version\n");
}

int main (int argc, char *argv[])
{
	FILE* init_fp;
	int opt, i;
	int day = DEFALUT_DAY;

	while ((opt = getopt(argc, argv, "hd:")) != EOF) {
		switch (opt) {
		case 'h':
			print_help();
			goto out;
		case 'd': 
			day = atoi(optarg);
			break;
		default:
			print_help();
			goto out;
		}
	}

	if ((argc-optind) != 1) {
		print_help();
		goto out;
	}

	trace_replay(argv[optind], day);

out:
	return 0;
}

static int trace_replay(char *config_name, int day)
{
	int app_count;
	config = malloc(sizeof(struct Config));

	if (parse_config(config_name) < 0)
		goto out;

	trace_init();

	do_trace_replay((double)day);
	
	app_count = config->basic_app.app_count;
	if (app_count > 0) {
		free(config->basic_app.apps);
	}
	app_count = config->normal_app.app_count;
	if (app_count > 0) {
		free(config->normal_app.apps);
	}
out:
	free(config);
}

int trace_init(void)
{
	if (strcmp(config->INIT_FILEMAP, "NULL") == 0)
		return -1;

	execlp("./simpleReplay", "./simpleReplay", "-i", config->INIT_FILEMAP, NULL);

	return 0;
}

int do_trace_replay(double day)
{
	struct ReplayJob* ReplayJobBasic_array;
	struct ReplayJob* ReplayJobNormal_array
	struct Node* ReplayJob_queue;
	struct Node* InstallApp_list;
	struct Node* UninstallApp_list;

	if (init_replayjob(&ReplayJobBasic_array, &ReplayJobNormal_array, &ReplayJob_queue) < 0)
		return -1;

	while (1)
	{
		struct ReplayJob* replay = ll_remove(ReplayJob_queue, 0);
		double curTime;

		if (curTime > day)
			break;
	}

	free(ReplayJob_array);






}

int job_compare(void *data1, void *data2)
{
	struct ReplayJob *job1 = (struct ReplayJob*) data1;
	struct ReplayJob *job2 = (struct ReplayJob*) data2;

	if (job1->curTime > job2->curTime)
		return -1;
	else if (job1->curTime == job2->curTime)
		return 0;
	else
		return 1;
}

int init_replayjob(struct ReplayJob** basic_array, struct ReplayJob** normal_array, struct Node** queue)
{
	int size = (2 * config->basic_app.app_count) + 2 + 2; // 2 = CAMERA, 2 = NORMAL(INSTALL+UNINSTALL)
	int cnt, i;
	*basic_array = malloc(sizeof(ReplayJob) * size);
	*queue = NULL;

	if (*basic_array == NULL)
		return -1;
	
	*normal_array = malloc(sizeof(ReplayJob) * config->normal_app.app_count * 2);

	if (*normal_array == NULL) {
		free(basic_array);
		return -1;
	}

	set_replayjob(&((*basic_array)[0]), REPLAY_CAMERA, "camera", config->multi.camera.path, 1/conifg->multi.camera.take_count);
	set_replayjob(&((*basic_array)[1]), REPLAY_CAMERA_DELETE, "camera", config->multi.camera.multimedia_path, 1/conifg->multi.camera.delete_count);
	set_replayjob(&((*basic_array)[2]), REPLAY_INSTALL, "install", "NULL", conifig->normal_app.install_cycle);
	set_replayjob(&((*basic_array)[3]), REPLAY_UNINSTALL, "uninstall", "NULL", conifg->normal_app.uninstall_cycle);

	cnt = 4;
	for (i = 0; i < config->basic_app.app_count; i++)
	{
		set_replayjob(&((*basic_array)[cnt]), REPLAY_LOADING, config->basic_app.apps[i].name, config->basic_app.apps[i].path, config->basic_app.apps[i].loading_cycle);
		cnt++;
		set_replayjob(&((*basic_array)[cnt]), REPLAY_UPDATE, config->basic_app.apps[i].name, config->basic_app.apps[i].path, config->basic_app.apps[i].update_cycle);
		cnt++;
	}
	for (i = 0; i < size; i++)
	{
		ll_insert_priority(queue, &((*array)[cnt]), job_compare);
	}

	cnt = 0;
	for (i = 0; i < config->normal_app.app_count; i++)
	{
		set_replayjob(&((*normal_array)[cnt]), REPLAY_LOADING, config->normal_app.apps[i].name, config->normal_app.apps[i].path, config->normal_app.apps[i].loading_cycle);
		cnt++;
		set_replayjob(&((*normal_array)[cnt]), REPLAY_UPDATE, config->normal_app.apps[i].name, config->normal_app.apps[i].path, config->normal_app.apps[i].update_cycle);
		cnt++;
	}
}

int set_replayjob(struct ReplayJob* job, enum REPLAY_TYPE type, char *name, char *path, double cycle)
{
	job->type = type;
	memset(job->name, 0, MAX_NAME);
	sprintf(job->name, "%s", name);
	memset(job->path, 0, PATH_MAX + 1);
	sprintf(job->path, "%s", path);
	job->cycle = cycle;
	job->curTime = cycle;
}

int parse_config(char *config_name)
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
		ret = parse_multimedia(multimedia_obj);
		if (ret == -1)
			goto out;
	}

	basic_app_obj = cJSON_GetObjectItem(root_obj, "BASIC_APP");
	if (basic_app_obj == NULL) {
		printf("error: Parse JSON file, No BASIC_APP\n");
		ret = -1;
		goto out;
	} else {
		ret = parse_basic_app(basic_app_obj);
		if (ret == -1)
			goto out;
	}

	normal_app_obj = cJSON_GetObjectItem(root_obj, "NORMAL_APP");
	if (normal_app_obj == NULL) {
		printf("error: Parse JSON file, No NORMAL_APP\n");
		ret = -1;
		goto out;
	} else {
		ret = parse_normal_app(normal_app_obj);
		if (ret == -1)
			goto out;
	}

out:
	fclose(config_fp);
	free(config_data);
	return ret;
}

static int parse_multimedia(cJSON *multi_obj)
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
	memset(config->multi.camera.path, 0, PATH_MAX + 1);
	sprintf(config->multi.camera.path, "%s", camera_path_obj->valuestring);

	camera_multipath_obj = cJSON_GetObjectItem(camera_obj, "MULTIMEDIA_PATH");
	if (camera_multipath_obj == NULL) {
		printf("error: No exist MULTIMEDIA_PATH in CAMERA");
		return -1;
	}
	memset(config->multi.camera.multimedia_path, 0, PATH_MAX + 1);
	sprintf(config->multi.camera.multimedia_path, "%s", camera_path_obj->valuestring);

	camera_takecount_obj = cJSON_GetObjectItem(camera_obj, "TAKE_COUNT");
	if (camera_takecount_obj == NULL) {
		printf("error: No exist TAKE_COUNT in CAMERA");
		return -1;
	}
	config->multi.camera.take_count = camera_takecount_obj->valueint;

	camera_deletecount_obj = cJSON_GetObjectItem(camera_obj, "DELETE_COUNT");
	if (camera_deletecount_obj == NULL) {
		printf("error: No exist DELETE_COUNT in CAMERA");
		return -1;
	}
	config->multi.camera.delete_count = camera_deletecount_obj->valueint;

	return 0;
}

static int parse_basic_app(cJSON *basic_obj)
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
		config->basic_app.apps = malloc(sizeof(struct App) * app_count);
		parse_apps(apps_obj, config->basic_app.apps, config->basic_app.default_update_cycle, config->basic_app.default_loading_cycle);
	}
	
	return 0;
}

static int parse_normal_app(cJSON *normal_obj)
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
		config->normal_app.apps = malloc(sizeof(struct App) * app_count);
		parse_apps(apps_obj, config->normal_app.apps, config->normal_app.default_update_cycle, config->normal_app.default_loading_cycle);
	}

	return 0;
}

static parse_apps(cJSON *apps_obj, struct App *apps, 
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




