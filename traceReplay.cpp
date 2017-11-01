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

using namespace std;

#define TRACE_MERGE		0x01
#define INITFILE	0x02


struct Config *config;
int mode_flag;
static int trace_replay(char *config_name, int start_day, int end_day, int mode_flag);
int print_help(void);
int trace_init(void);
int do_trace_replay(double start_day, double end_day);
struct ReplayJob* create_replayjob(enum REPLAY_TYPE type, const char *name, const char *path, double cycle);
int init_replayjob(list<struct ReplayJob*> *ReplayJob_queue, list<struct App*> *Normal_list);
int insert_replayqueue(list<struct ReplayJob*> *ReplayJob_queue, struct ReplayJob* job);
struct ReplayJob* create_bgjob(struct App* app);
struct ReplayJob* create_replayjob(enum REPLAY_TYPE type, const char *name, const char *path, double cycle);
int uninstall_replayqueue(list<struct ReplayJob*> *ReplayJob_queue, const char *name);

int replay_loading(struct ReplayJob* replay);
int replay_update(struct ReplayJob* replay);
int replay_install(list<struct ReplayJob*> *jobqueue, list<struct App*> *ins_list, list<struct App*> *unins_list, double curTime);
int replay_uninstall(list<struct ReplayJob*> *jobqueue, list<struct App*> *ins_list, list<struct App*> *unins_list, double curTime);
int replay_camera(struct ReplayJob* replay);
int replay_camera_delete(struct ReplayJob* replay);
int replay_bg(struct ReplayJob* replay);

int print_help(void)
{
	printf("traceReplay 0.1 Version\n");
	return 0;
}

int main (int argc, char *argv[])
{
	FILE* init_fp;
	int opt, i;
	int end_day = DEFAULT_DAY;
	int start_day = 0;
	mode_flag = 0;

	while ((opt = getopt(argc, argv, "hd:s:Mi")) != EOF) {
		switch (opt) {
		case 'h':
			print_help();
			goto out;
		case 'd': 
			end_day = atoi(optarg);
			break;
		case 's': 
			start_day = atoi(optarg);
			break;
		case 'i':
			mode_flag = INITFILE;
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

	trace_replay(argv[optind], start_day, end_day, mode_flag);

out:
	return 0;
}

static int trace_replay(char *config_name, int start_day, int end_day, int mode_flag)
{
	int app_count;
	config = (struct Config*) malloc(sizeof(struct Config));

	if (parse_config(config_name, config) < 0)
		goto out;

	trace_merge(config);	
	set_background_map(config);

	if (mode_flag & INITFILE)
		trace_init();

	do_trace_replay((double)start_day, (double)end_day);
	
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
	char cmd[PATH_MAX];
	if (strcmp(config->INIT_FILEMAP, "NULL") == 0)
		return -1;

//	execlp("./simpleReplay", "./simpleReplay", "-i", config->INIT_FILEMAP, NULL);
	memset(cmd, 0, PATH_MAX);
	sprintf(cmd, "./simpleReplay -i %s", config->INIT_FILEMAP);
	system(cmd);

	printf("[%3.2lf] %s\n", (double)0.0, cmd);
	return 0;
}

int do_trace_replay(double start_day, double end_day)
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

		if (replay->curTime < start_day) {
			insert_replayqueue(&ReplayJob_queue, replay);
			curTime = replay->curTime;
			replay->curTime = replay->curTime + replay->cycle;
			continue;
		}
		if (curTime > end_day) {
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
			case REPLAY_BG:
				replay_bg(replay);
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
	char cmd[PATH_MAX];
	memset(buf, 0, PATH_MAX);
	SPRINTF_LOADING_PATH(buf, replay->path, replay->name);
	// execlp("./simpleReplay", "./simpleReplay", buf, NULL);
//	printf("[%3.2lf] execlp(\"./simpleReplay\", \"./simpleReplay\", %s, NULL);\n", replay->curTime, buf);
	memset(cmd, 0, PATH_MAX);
	sprintf(cmd, "./simpleReplay %s", buf);
	printf("[%3.2lf] %s\n", replay->curTime, cmd);
	system(cmd);
}

int replay_update(struct ReplayJob* replay)
{
	char buf[PATH_MAX];
	char cmd[PATH_MAX];
	memset(buf, 0, PATH_MAX);
	SPRINTF_UPDATE_PATH(buf, replay->path, replay->name);
//	execlp("./simpleReplay", "./simpleReplay", buf, NULL)
//	printf("[%3.2lf] execlp(\"./simpleReplay\", \"./simpleReplay\", %s, NULL);\n", replay->curTime, buf);
	memset(cmd, 0, PATH_MAX);
	sprintf(cmd, "./simpleReplay %s", buf);
	printf("[%3.2lf] %s\n", replay->curTime, cmd);
	system(cmd);
}

int replay_bg(struct ReplayJob* replay)
{
	char buf[PATH_MAX];
	char cmd[PATH_MAX];
	int size = replay->bgjob.size();
	vector<string>::iterator it;
	string str_path;

	if (size == 0)
		return 0;

	int value;
	value = rand() % size;
	it = replay->bgjob.begin();
	advance(it, value);
	str_path = (*it);

//	printf("[%3.2lf] execlp(\"./simpleReplay\", \"./simpleReplay\", %s, NULL);\n", replay->curTime, str_path.c_str());
	memset(cmd, 0, PATH_MAX);
	sprintf(cmd, "./simpleReplay %s", buf);
	printf("[%3.2lf] %s\n", replay->curTime, cmd);
	system(cmd);
}

int replay_install(list<struct ReplayJob*> *jobqueue, list<struct App*> *ins_list, list<struct App*> *unins_list, double curTime)
{
	char buf[PATH_MAX];
	char cmd[PATH_MAX];

	int size = unins_list->size();
	list<struct App*>::iterator it;
	int value;
	struct ReplayJob* replay_loading;
	struct ReplayJob* replay_update;
	struct ReplayJob* replay_bg;
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
	replay_update = create_replayjob(REPLAY_UPDATE, app->name, app->path, app->update_cycle);
	if (replay_update == NULL)
		return -1;
	replay_update->curTime = curTime + app->update_cycle;
	replay_bg = create_bgjob(app);
	if (replay_bg != NULL)
		replay_bg->curTime = curTime + 0.5;

	insert_replayqueue(jobqueue, replay_loading);
	insert_replayqueue(jobqueue, replay_update);
	if (replay_bg != NULL)
		insert_replayqueue(jobqueue, replay_update);

	memset(buf, 0, PATH_MAX);
	SPRINTF_INSTALL_PATH(buf, app->path, app->name);
//	execlp("./simpleReplay", "./simpleReplay", buf, NULL);
//	printf("[%3.2lf] execlp(\"./simpleReplay\", \"./simpleReplay\", %s, NULL);\n", curTime, buf);
	memset(cmd, 0, PATH_MAX);
	sprintf(cmd, "./simpleReplay %s", buf);
	printf("[%3.2lf] %s\n", curTime, cmd);
	system(cmd);
}

int replay_uninstall(list<struct ReplayJob*> *jobqueue, list<struct App*> *ins_list, list<struct App*> *unins_list, double curTime)
{
	char buf[PATH_MAX];
	char cmd[PATH_MAX];
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
	
	memset(buf, 0, PATH_MAX);
	SPRINTF_UNINSTALL_PATH(buf, app->path, app->name);
//	execlp("./simpleReplay", "./simpleReplay", buf, NULL);
//	printf("[%3.2lf] execlp(\"./simpleReplay\", \"./simpleReplay\", %s, NULL);\n", curTime, buf);
	memset(cmd, 0, PATH_MAX);
	sprintf(cmd, "./simpleReplay %s", buf);
	printf("[%3.2lf] %s\n", curTime, cmd);
	system(cmd);
}

int replay_camera(struct ReplayJob* replay)
{
	char cmd[PATH_MAX];
//	execlp("./simpleReplay", "./simpleReplay", "-c", replay->path, NULL);
//	printf("[%3.2lf] execlp(\"./simpleReplay\", \"./simpleReplay\", \"-c\", %s, NULL);\n", replay->curTime, replay->name);
	memset(cmd, 0, PATH_MAX);
	sprintf(cmd, "./simpleReplay -C %s", replay->name);
	printf("[%3.2lf] %s\n", replay->curTime, cmd);
	system(cmd);
}

int replay_camera_delete(struct ReplayJob* replay)
{
	char cmd[PATH_MAX];
//	execlp("./simpleReplay", "./simpleReplay", "-c", replay->path, NULL);
//	printf("[%3.2lf] execlp(\"./simpleReplay\", \"./simpleReplay\", \"-r\", %s, NULL);\n", replay->curTime, replay->name);
	memset(cmd, 0, PATH_MAX);
	sprintf(cmd, "./simpleReplay -R %s", replay->name);
	printf("[%3.2lf] %s\n", replay->curTime, cmd);
	system(cmd);
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

		job = create_bgjob(&(config->basic_app.apps[i]));
		if (job != NULL)
			insert_replayqueue(ReplayJob_queue, job);
	}

	for (i = 0; i < config->normal_app.app_count; i++)
	{
		Normal_list->push_back(&(config->normal_app.apps[i]));
	}
}

struct ReplayJob* create_bgjob(struct App* app)
{
	vector<struct BGProcess>::iterator it;
	string str_psname = string(app->ps_name);
	struct ReplayJob* job;
    for (it = config->BGMap.begin(); it != config->BGMap.end(); ++it)
    {
		if (str_psname.compare((*it).ps_name) == 0)
			break;
	}
	
	if (it != config->BGMap.end()) 
		return NULL;

	job = create_replayjob(REPLAY_BG, app->name, app->path, 0.5);
	job->bgjob.assign((*it).path.begin(), (*it).path.end());

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


