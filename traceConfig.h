#include <limits.h>
#include <vector>
#include <iostream>
#include <string>
#include <map>

using namespace std;

#define MAX_NAME	200
#define DEFAULT_DAY	50

struct Camera
{
	char path[PATH_MAX + 1];
	char multimedia_path[PATH_MAX+1];
	int take_count;
	int delete_count;
};

struct App
{
	char name[MAX_NAME];
	char path[PATH_MAX+1];
	char ps_name[MAX_NAME];
	double update_cycle;
	double loading_cycle;
};

struct Multimedia 
{
	struct Camera mul_camera;
};

struct BasicApp
{
	double default_update_cycle;
	double default_loading_cycle;
	struct App* apps;
	int app_count;
};

struct NormalApp
{
	double default_update_cycle;
	double default_loading_cycle;
	double install_cycle;
	double uninstall_cycle;
	struct App* apps;
	int app_count;
};

struct PSName
{
	vector<string> ps_install;
	vector<string> ps_update;
	vector<string> ps_loading;
	vector<string> ps_uninstall;
};

struct Config
{
	char INIT_FILEMAP[PATH_MAX + 1];
	char backup_path[PATH_MAX + 1];
	struct Multimedia multi;
	struct BasicApp basic_app;
	struct NormalApp normal_app;
	struct PSName ps_name;
	vector<struct BGProcess> BGMap;
};

struct BGProcess
{
	string ps_name;
	vector<string> path;
};

int parse_config(char *config_name, struct Config *config);
int trace_merge(struct Config *config);
int set_background_map(struct Config *config);
int free_background_map(struct Config *config);

