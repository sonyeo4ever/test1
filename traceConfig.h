#include <limits.h>
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

struct Config
{
	char INIT_FILEMAP[PATH_MAX + 1];
	struct Multimedia multi;
	struct BasicApp basic_app;
	struct NormalApp normal_app;
};

