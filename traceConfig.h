#include <limits.h>
#include <vector>
#include <iostream>
#include <string>

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
	vector<String> ps_install;
	vector<String> ps_update;
	vector<String> ps_loading;
	vector<String> ps_uninstall;
}

struct Config
{
	char INIT_FILEMAP[PATH_MAX + 1];
	struct Multimedia multi;
	struct BasicApp basic_app;
	struct NormalApp normal_app;
	struct PSName ps_name;
};

int parse_config(char *config_name, struct Config *config);

