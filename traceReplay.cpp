#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <iostream>
#include <list>
#include <string>

#include "cJSON.h"
#include "traceReplay.h"

using namespace std;

#define MERGE_TRACE		0x01


struct Config *config;
int mode_flag;

int print_help(void);
int parse_config(char *config_name);
static int trace_replay(char *config_name, int day);
static int parse_multimedia(cJSON *multi_obj);
static int parse_basic_app(cJSON *basic_obj);
static int parse_normal_app(cJSON *normal_obj);
static int parse_apps(cJSON *apps_obj, struct App *apps, 
			double default_update, double default_loading);
int trace_init(void);
int do_trace_replay(double day);
struct ReplayJob* create_replayjob(enum REPLAY_TYPE type, const char *name, const char *path, double cycle);
int init_replayjob(list<struct ReplayJob*> *ReplayJob_queue, list<struct App*> *Normal_list);
int insert_replayqueue(list<struct ReplayJob*> *ReplayJob_queue, struct ReplayJob* job);
struct ReplayJob* create_replayjob(enum REPLAY_TYPE type, const char *name, const char *path, double cycle);
int uninstall_replayqueue(list<struct ReplayJob*> *ReplayJob_queue, const char *name);

int replay_loading(struct ReplayJob* replay);
int replay_update(struct ReplayJob* replay);
int replay_install(list<struct ReplayJob*> *jobqueue, list<struct App*> *ins_list, list<struct App*> *unins_list, double curTime);
int replay_uninstall(list<struct ReplayJob*> *jobqueue, list<struct App*> *ins_list, list<struct App*> *unins_list, double curTime);
int replay_camera(struct ReplayJob* replay);
int replay_camera_delete(struct ReplayJob* replay);

int print_help(void)
{
	printf("traceReplay 0.1 Version\n");
	return 0;
}

int main (int argc, char *argv[])
{
	FILE* init_fp;
	int opt, i;
	int day = DEFAULT_DAY;
	mode_flag = 0;

	while ((opt = getopt(argc, argv, "hd:M")) != EOF) {
		switch (opt) {
		case 'h':
			print_help();
			goto out;
		case 'd': 
			day = atoi(optarg);
			break;
		case 'M': 
			mode_flag |= TRACE_MERGE;
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

	if (mode_flag & TRACE_MERGE)
		trace_merge(argv[optind]);	

	trace_replay(argv[optind], day);

out:
	return 0;
}

static int trace_replay(char *config_name, int day)
{
	int app_count;
	config = (struct Config*) malloc(sizeof(struct Config));

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

//	execlp("./simpleReplay", "./simpleReplay", "-i", config->INIT_FILEMAP, NULL);
	printf("[%3.2lf] execlp(\"./simpleReplay\", \"./simpleReplay\", \"-i\", config->INIT_FILEMAP, NULL);\n", (double)0.0);


	return 0;
}

int do_trace_replay(double day)
{
	list<struct ReplayJob*> ReplayJob_queue;
	list<struct App*> Install_list;
	list<struct App*> Uninstall_list;
	list<struct ReplayJob*>::iterator iter;

	double curTime = 0;

	if (init_replayjob(&ReplayJob_queue, &Uninstall_list) < 0)
		return -1;

	if (ReplayJob_queue.empty())
		return -1;

	while (1)
	{
		struct ReplayJob* replay = ReplayJob_queue.front();

		ReplayJob_queue.pop_front();

		if (curTime > day) {
			insert_replayqueue(&ReplayJob_queue, replay);
			break;
		}

		switch (replay->type)
		{
			case REPLAY_LOADING:
				replay_loading(replay);
			break;
			case REPLAY_UPDATE:
				replay_update(replay);
			break;
			case REPLAY_INSTALL:
				replay_install(&ReplayJob_queue, &Install_list, &Uninstall_list, curTime);
			break;
			case REPLAY_UNINSTALL:
				replay_uninstall(&ReplayJob_queue, &Install_list, &Uninstall_list, curTime);
			break;
			case REPLAY_CAMERA:
				replay_camera(replay);
			break;
			case REPLAY_CAMERA_DELETE:
				replay_camera_delete(replay);
			break;
		}

		curTime = replay->curTime;
		replay->curTime = replay->curTime + replay->cycle;
		insert_replayqueue(&ReplayJob_queue, replay);
	}

	while (!ReplayJob_queue.empty())
	{
		struct ReplayJob* job;
		iter = ReplayJob_queue.begin();
		job = (*iter);
		ReplayJob_queue.erase(iter);
		free(job);
	}
}

int replay_loading(struct ReplayJob* replay)
{
	char buf[PATH_MAX];
	SPRINTF_LOADING_PATH(buf, replay->path, replay->name);
	// execlp("./simpleReplay", "./simpleReplay", buf, NULL);
	printf("[%3.2lf] execlp(\"./simpleReplay\", \"./simpleReplay\", %s, NULL);\n", replay->curTime, buf);

}

int replay_update(struct ReplayJob* replay)
{
	char buf[PATH_MAX];
	SPRINTF_UPDATE_PATH(buf, replay->path, replay->name);
//	execlp("./simpleReplay", "./simpleReplay", buf, NULL)

	printf("[%3.2lf] execlp(\"./simpleReplay\", \"./simpleReplay\", %s, NULL);\n", replay->curTime, buf);
}

int replay_install(list<struct ReplayJob*> *jobqueue, list<struct App*> *ins_list, list<struct App*> *unins_list, double curTime)
{
	char buf[PATH_MAX];
	int size = unins_list->size();
	list<struct App*>::iterator it;
	int value;
	struct ReplayJob* replay_loading;
	struct ReplayJob* replay_update;
	struct App* app;

	if (size == 0)
		return 0;

	value = rand() % size;
	it = unins_list->begin();
	advance(it, value);
	app = (*it);
	unins_list->erase(it);
	ins_list->push_back(app);

	replay_loading = create_replayjob(REPLAY_LOADING, app->name, app->path, app->loading_cycle);
	if (replay_loading == NULL)
		return -1;
	replay_loading->curTime = curTime + app->loading_cycle;
	replay_update = create_replayjob(REPLAY_UPDATE, app->name, app->path, app->loading_cycle);
	if (replay_update == NULL)
		return -1;
	replay_update->curTime = curTime + app->update_cycle;

	insert_replayqueue(jobqueue, replay_loading);
	insert_replayqueue(jobqueue, replay_update);

	SPRINTF_INSTALL_PATH(buf, app->path, app->name);
//	execlp("./simpleReplay", "./simpleReplay", buf, NULL);
	printf("[%3.2lf] execlp(\"./simpleReplay\", \"./simpleReplay\", %s, NULL);\n", curTime, buf);

}

int replay_uninstall(list<struct ReplayJob*> *jobqueue, list<struct App*> *ins_list, list<struct App*> *unins_list, double curTime)
{
	char buf[PATH_MAX];
	int size = ins_list->size();
	list<struct App*>::iterator it;
	int value;
	struct ReplayJob* replay_loading;
	struct ReplayJob* replay_update;
	struct App* app;

	if (size == 0)
		return 0;
	value = rand() % size;
	it = ins_list->begin();
	advance(it, value);
	app = (*it);
	ins_list->erase(it);
	unins_list->push_back(app);
	
	uninstall_replayqueue(jobqueue, app->name);

	SPRINTF_UNINSTALL_PATH(buf, app->path, app->name);
//	execlp("./simpleReplay", "./simpleReplay", buf, NULL);
	printf("[%3.2lf] execlp(\"./simpleReplay\", \"./simpleReplay\", %s, NULL);\n", curTime, buf);
}

int replay_camera(struct ReplayJob* replay)
{
//	execlp("./simpleReplay", "./simpleReplay", "-c", replay->path, NULL);
	printf("[%3.2lf] execlp(\"./simpleReplay\", \"./simpleReplay\", \"-c\", %s, NULL);\n", replay->curTime, replay->name);
}

int replay_camera_delete(struct ReplayJob* replay)
{
//	execlp("./simpleReplay", "./simpleReplay", "-c", replay->path, NULL);
	printf("[%3.2lf] execlp(\"./simpleReplay\", \"./simpleReplay\", \"-r\", %s, NULL);\n", replay->curTime, replay->name);
}

int init_replayjob(list<struct ReplayJob*> *ReplayJob_queue, list<struct App*> *Normal_list)
{
	int i;
	struct ReplayJob *job;

	job = create_replayjob(REPLAY_CAMERA, config->multi.mul_camera.multimedia_path, 
					config->multi.mul_camera.path, 1/(double)config->multi.mul_camera.take_count);
	if (job == NULL)
		return -1;
	insert_replayqueue(ReplayJob_queue, job);

	job = create_replayjob(REPLAY_CAMERA_DELETE, config->multi.mul_camera.multimedia_path, 
					config->multi.mul_camera.path, 1/(double)config->multi.mul_camera.delete_count);
	if (job == NULL)
		return -1;
	insert_replayqueue(ReplayJob_queue, job);

	job = create_replayjob(REPLAY_INSTALL, "install", "NULL", config->normal_app.install_cycle);
	if (job == NULL)
		return -1;
	insert_replayqueue(ReplayJob_queue, job);

	job = create_replayjob(REPLAY_UNINSTALL, "uninstall", "NULL", config->normal_app.uninstall_cycle);
	if (job == NULL)
		return -1;
	insert_replayqueue(ReplayJob_queue, job);

	for (i = 0; i < config->basic_app.app_count; i++)
	{
		job = create_replayjob(REPLAY_LOADING, config->basic_app.apps[i].name, 
					config->basic_app.apps[i].path, config->basic_app.apps[i].loading_cycle);
		if (job == NULL)
			return -1;
		insert_replayqueue(ReplayJob_queue, job);

		job = create_replayjob(REPLAY_UPDATE, config->basic_app.apps[i].name, 
					config->basic_app.apps[i].path, config->basic_app.apps[i].update_cycle);
		if (job == NULL)
			return -1;
		insert_replayqueue(ReplayJob_queue, job);
	}

	for (i = 0; i < config->normal_app.app_count; i++)
	{
		Normal_list->push_back(&(config->normal_app.apps[i]));
	}
}

int insert_replayqueue(list<struct ReplayJob*> *ReplayJob_queue, struct ReplayJob* job)
{
	list<struct ReplayJob*>::iterator iter = ReplayJob_queue->begin();

	for (iter = ReplayJob_queue->begin(); iter != ReplayJob_queue->end(); ++iter) 
	{
		if ((*iter)->curTime > job->curTime) {
			ReplayJob_queue->insert(iter, job);
			break;
		}
	}

	if (iter == ReplayJob_queue->end()) {
		ReplayJob_queue->insert(iter, job);
	}

	return 0;
}

int uninstall_replayqueue(list<struct ReplayJob*> *ReplayJob_queue, const char *name)
{
	list<struct ReplayJob*>::iterator iter = ReplayJob_queue->begin();

	while (iter != ReplayJob_queue->end())
	{
		if (strcmp((*iter)->name, name) == 0) {
			free(*iter);
			iter = ReplayJob_queue->erase(iter);
		}
		else
			iter++;
	}

	return 0;
}

struct ReplayJob* create_replayjob(enum REPLAY_TYPE type, const char *name, const char *path, double cycle)
{
	struct ReplayJob* job;
	job = (struct ReplayJob*) malloc(sizeof(struct ReplayJob));
	if (job == NULL)
		return NULL;
	job->type = type;
	memset(job->name, 0, MAX_NAME);
	sprintf(job->name, "%s", name);
	memset(job->path, 0, PATH_MAX + 1);
	sprintf(job->path, "%s", path);
	job->cycle = cycle;
	job->curTime = cycle;
	return job;
}

struct ReplayJob* create_replayjob(enum REPLAY_TYPE type, struct ReplayJob *replay)
{
	struct ReplayJob* job;
	job = (struct ReplayJob*) malloc(sizeof(struct ReplayJob));
	if (job == NULL)
		return NULL;
	job->type = type;
	memset(job->name, 0, MAX_NAME);
	sprintf(job->name, "%s", replay->name);
	memset(job->path, 0, PATH_MAX + 1);
	sprintf(job->path, "%s", replay->path);
	job->cycle = replay->cycle;
	job->curTime = replay->cycle;
	return job;
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
		config->basic_app.apps = (struct App*) malloc(sizeof(struct App) * app_count);
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




