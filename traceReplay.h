#include "traceConfig.h"

#define SPRINTF_LOADING_PATH(BUF, PATH, NAME) \
	sprintf(BUF, "%s/TRACE_%s_loading.input", PATH, NAME);
#define SPRINTF_UPDATE_PATH(BUF, PATH, NAME) \
	sprintf(BUF, "%s/TRACE_%s_update.input", PATH, NAME);
#define SPRINTF_INSTALL_PATH(BUF, PATH, NAME) \
	sprintf(BUF, "%s/TRACE_%s_install.input", PATH, NAME);
#define SPRINTF_UNINSTALL_PATH(BUF, PATH, NAME) \
	sprintf(BUF, "%s/TRACE_%s_uninstall.input", PATH, NAME);

enum REPLAY_TYPE
{
	REPLAY_LOADING = 0,
	REPLAY_UPDATE,
	REPLAY_INSTALL,
	REPLAY_UNINSTALL,
	REPLAY_CAMERA,
	REPLAY_CAMERA_DELETE
};

struct ReplayJob
{
	enum REPLAY_TYPE type;
	char name[MAX_NAME];
	char path[PATH_MAX + 1];
	double curTime;
	double cycle;
};

