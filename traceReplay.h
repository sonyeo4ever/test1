#include "traceConfig.h"

#define SPRINTF_LOADING_PATH(BUF, PATH, NAME) \
	sprintf(BUF, "%s/TRACE_%s_loading.input", PATH, NAME);
#define SPRINTF_UPDATE_PATH(BUF, PATH, NAME) \
	sprintf(BUF, "%s/TRACE_%s_update.input", PATH, NAME);
#define INSTALL_TRACE_PATH(BUF, PATH, NAME) \
	sprintf(BUF, "%s/TRACE_%s_install.input", PATH, NAME);
#define UNINSTALL_TRACE_PATH(BUF, PATH, NAME) \
	sprintf(BUF, "%s/TRACE_%s_install.input", PATH, NAME);

enum REPLAY_TYPE
{
	REPLAY_LOAD = 0,
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

struct Node
{
	void *data;
	struct Node* next;
};

struct Node* ll_insert(void **head, void *data, int n);
struct Node* ll_insert_priority(void **head, void *data, int (fn)(void* data1, void* data2));
void* ll_remove(void **head, int n);
int ll_size(void *head);

