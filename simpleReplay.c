#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <dirent.h>

#define MAX_NAME	200

int mkdir_all_path(const char *path);
int create_init_files(FILE* init_fp);
double trace_replay(char *input_name);
int file_create(char *path);
int file_mkdir(char *path);
int file_unlink(char *path);
int file_rmdir(char *path);
int file_fsync(char *path, int option);
int file_rename(char *path1, char *path2);
int file_write(char *path, long long int write_off, 
			long long int write_size, long long int file_size);

struct trace_stat
{
	int create;
	int mkdir;
	int unlink;
	int rmdir;
	int fsync;
	int rename;
	int write_overwrite;
	int write_append;
};

void print_help()
{
	printf("traceReplay 0.1 Version\n");
}

int main (int argc, char *argv[])
{
	FILE* init_fp;
	char init_name[MAX_NAME];
	char camera_path[PATH_MAX];
	int opt, i;

	while ((opt = getopt(argc, argv, "hi:C:R:")) != EOF) {
		switch (opt) {
		case 'h':
			print_help();
			goto out;
		case 'i': 
			strncpy(init_name, optarg, MAX_NAME);
			init_fp= fopen(init_name, "r");
			if (init_fp == NULL) {
				printf("Error: Can not open init file: %s\n", init_name);
				goto out;
			}
			create_init_files(init_fp);
			break;
		case 'C':
			strncpy(camera_path, optarg, PATH_MAX);
			create_camera_file(camera_path);
			return 0;
		case 'R':
			strncpy(camera_path, optarg, PATH_MAX);
			delete_camera_file(camera_path);
			return 0;
		default:
			print_help();
			goto out;
		}
	}

	for (i = optind; i < argc; i++)
	{
		printf("Replay %s TRACE\n", argv[i]);
		trace_replay(argv[i]);	
	}

out:

	return 0;
}

int create_camera_file(char* camera_path)
{
	char path[PATH_MAX];
	time_t cur_time;
	time(&cur_time);

	memset(path, 0, PATH_MAX);
	sprintf(path, "%s/camera_%ld", camera_path, cur_time);

	file_create(path);
	file_write(path, 0, 2097152, 0);
}

int delete_camera_file(char* camera_path)
{
	char path[PATH_MAX];
	DIR *dir_info;
	struct dirent *dir_entry;
	int count = 0;
	int value;

	dir_info = opendir(camera_path);
	if (dir_info != NULL)
	{
		while (dir_entry = readdir(dir_info))
		{
			if (dir_entry->d_type != DT_REG)
				continue;
			if (strstr(dir_entry->d_name, "camera_") == NULL)
				continue;
			count++;
		}
	}
	closedir(dir_info);

	if (count == 0)
		return 0;

	value = rand() % count;
	value++;

	count = 0;

	dir_info = opendir(camera_path);
	if (dir_info != NULL)
	{
		while (dir_entry = readdir(dir_info))
		{
			if (dir_entry->d_type != DT_REG)
				continue;
			if (strstr(dir_entry->d_name, "camera_") == NULL)
				continue;
			count++;
			if (count == value) {
				sprintf(path, "%s/%s", camera_path, dir_entry->d_name);
				file_unlink(path);
				break;
			}
		}
	}
	closedir(dir_info);
}

int create_init_files(FILE* init_fp)
{
	char line[2048];
	int line_count = 0;
	int new_fd;
	int ret;

	while (fgets(line, 2048, init_fp) != NULL)
	{
		const char *tmp;
		char path[2048];
		char *ptr;
		unsigned long long inode_num;
		unsigned long long size;

		line_count = line_count + 1;

		if (line[0] != '/' && line[1] != 'd' &&
				line[2] != 'a' && line[3] != 't' && line[4] != 'a')
			continue;

		ptr = strtok(line, "\t");
		if (ptr == NULL)
			continue;
		strncpy(path, ptr, strlen(ptr));
		path[strlen(ptr)] = 0x00;

		ptr = strtok(NULL, "\t");
		if (ptr == NULL)
			continue;
		inode_num = atoll(ptr);

		ptr = strtok(NULL, "\t");
		if (ptr == NULL)
			continue;
		size = atoll(ptr);

		tmp = path;
		tmp += 1;
//		tmp += 6;

		new_fd = open(tmp, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
		if (new_fd < 0)
		{
			mkdir_all_path(tmp);
			new_fd = open(tmp, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
			if (new_fd < 0) {
				printf("Failed: Create %s %d\n", tmp, errno);
				continue;
			}
		}

		ret = fallocate64(new_fd, 0, 0, size);
		if (ret < 0) {
			// printf("Failed: Allocate %s %llu %d\n", path, size, errno);
			continue;
		}

		close(new_fd);
//		printf("Success: Allocate %s %llu\n", path, size);

	}
	return 0;
}

int mkdir_all_path(const char *path)
{
	const char *tmp = path;
	char tmp_path[2048];
	int len = 0;
	int ret;

	if (path == NULL)
		return -1;
	
	while ((tmp = strchr(tmp, '/')) != NULL) {
		len = tmp - path;
		tmp++;
		if (len == 0)
			continue;

		strncpy(tmp_path, path, len);
		tmp_path[len] = 0x00;
		
		if ((ret = mkdir(tmp_path, 0776)) == -1) {
			if (errno != EEXIST) {
				return -1;
			}
		}
//		printf("mkdir: %s\n", tmp_path);
	}

}

double trace_replay(char *input_name)
{
	double last_time;
	char line[2048];
	FILE *input_fp;
	struct trace_stat Stat;
	if (strstr(input_name, ".input") == NULL)
		return -1;

	memset(&Stat, 0, sizeof(struct trace_stat));

	input_fp = fopen(input_name, "r");
	memset(line, 0, 2048);
	while (fgets(line, 2048, input_fp) != NULL)
	{
		char *ptr;
		char *ptr_2;
		char type[10];
		char tmp[PATH_MAX + 1];
		char path[PATH_MAX + 1];

		if (strstr(line, "TESTEND") != NULL)
			goto out;

		ptr = strtok(line, "\t");
		if (ptr == NULL)
			continue;
		last_time = strtod(ptr, &ptr_2);

		ptr = strtok(NULL, "\t");
		if (ptr == NULL)
			continue;
		strncpy(type, ptr, strlen(ptr));
		type[strlen(ptr)] = 0x00;

		ptr = strtok(NULL, "\t");
		if (ptr == NULL)
			continue;
		strncpy(tmp, ptr, strlen(ptr));
		tmp[strlen(ptr)] = 0x00;
		sprintf(path, "data%s", tmp);

		if (strncmp(type, "[CR]", 4) == 0) {
			if (file_create(path) == 0)
				Stat.create++;
		}
		else if (strncmp(type, "[MD]", 4) == 0) {
			if (file_mkdir(path) == 0)
				Stat.mkdir++;
		}
		else if (strncmp(type, "[UN]", 4) == 0) {
			if (file_unlink(path) == 0)
				Stat.unlink++;
		}
		else if (strncmp(type, "[RD]", 4) == 0) {
			if (file_rmdir(path) == 0)
				Stat.rmdir++;
		}
		else if (strncmp(type, "[FS]", 4) == 0)
		{
			int sync_option;
			ptr = strtok(NULL, "\t");
			if (ptr == NULL)
				continue;
			sync_option = atoi(ptr);
			if (file_fsync(path, sync_option) == 0)
				Stat.fsync++;
		}
		else if (strncmp(type, "[RN]", 4) == 0)
		{
			char path2[PATH_MAX + 1];
			ptr = strtok(NULL, "\t");
			if (ptr == NULL)
				continue;
			strncpy(tmp, ptr, strlen(ptr));
			tmp[strlen(ptr)] = 0x00;
			sprintf(path2, "data/%s", tmp);
			if (file_rename(path, path2) == 0)
				Stat.rename++;
		}
		else if (strncmp(type, "[WO]", 4) == 0)
		{
			long long int write_off;
			long long int write_size;
			long long int file_size;

			ptr = strtok(NULL, "\t");
			if (ptr == NULL)
				continue;
			write_off = atoll(ptr);
			ptr = strtok(NULL, "\t");
			if (ptr == NULL)
				continue;
			write_size = atoll(ptr);
			ptr = strtok(NULL, "\t");
			if (ptr == NULL)
				continue;
			file_size = atoll(ptr);
			if (file_write(path, write_off, write_size, file_size) == 0)
				Stat.write_overwrite++;
		}
		else if (strncmp(type, "[WA]", 4) == 0)
		{
			long long int write_off;
			long long int write_size;
			long long int file_size;

			ptr = strtok(NULL, "\t");
			if (ptr == NULL)
				continue;
			write_off = atoll(ptr);
			ptr = strtok(NULL, "\t");
			if (ptr == NULL)
				continue;
			write_size = atoll(ptr);
			ptr = strtok(NULL, "\t");
			if (ptr == NULL)
				continue;
			file_size = atoll(ptr);
			if (file_append(path, write_off, write_size, file_size) == 0)
				Stat.write_append++;
		}
		else
			continue;
	}

out:
	printf("CR\tMD\tUN\tRD\tFS\tRN\tWO\tWA\n");
	printf("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", Stat.create, Stat.mkdir, 
								Stat.unlink, Stat.rmdir, 
								Stat.fsync, Stat.rename,
								Stat.write_overwrite, Stat.write_append);
	fclose(input_fp);

	return last_time;

}

int file_create(char *path)
{
	int new_fd = open (path, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
	if (new_fd < 0)
	{
		// printf("CR: Retry mkdir_all_path: %s\n", path);
		mkdir_all_path(path);
		new_fd = open (path, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
		if (new_fd < 0) {
		// 	printf("CR: Failed Create file: %s\n", path);
			return -1;
		}
	}
	write(new_fd, "a", 1);
	close(new_fd);
	truncate(path, 0);
	return 0;
}

int file_unlink(char *path)
{
	int ret;
	ret = unlink(path);
//	if (ret < 0)
//		printf("UN: Failed unlink file: %s\n", path);

	return 0;
}

int file_mkdir(char *path)
{
	int ret;
	mkdir_all_path(path);
	ret = mkdir(path, 0776);
/*	if (ret < 0) {
		perror("MD failed");
		printf("MD: Failed mkdir file: %s\n", path);
	}
	*/
	return 0;
}

int file_rmdir(char *path)
{
	int ret = 0;
	ret = rmdir(path);
//	if (ret < 0)
//		printf("RD: Failed rmdir file: %s\n", path);
	return 0;
}

int file_fsync(char *path, int option)
{
	int fd = open(path, O_RDWR, 0644);
	if (fd < 0)
	{
		if (errno == EISDIR)
			return -1;
//		perror("FS OPEN");
//		printf("FS: Fail Open: %s\n", path);
		return -1;
	}
	if (option == 1)
		fsync(fd);
	else
		fdatasync(fd);
	close(fd);
	return 0;
}

int file_rename(char *path1, char *path2)
{
	int ret = rename(path1, path2);
	if (ret < 0)
	{
		// printf("RN: Failed Rename, Retry File Create path 1:%s\n", path1);
		file_create(path1);
		ret = rename(path1, path2);
		if (ret < 0) {
			file_create(path2);
			// printf("RN: Failed Rename, Retry File Create path 2:%s\n", path2);
		}
	}
	return 0;
}

int file_write(char *path, long long int write_off, 
				long long int write_size, long long int file_size)
{
	int fd = open(path, O_RDWR);
	char *data;
	char letter = 'a';
	int letter_count = 'z'- 'a';
	int random_number;
	if (fd < 0) {
		// printf("W: Failed Open, Try file create:%s\n", path);
		file_create(path);
		fd = open(path, O_RDWR);
		if (fd < 0) {
		// 	printf("W: Failed Open:%s\n", path);
			return -1;
		}
	}
	data = malloc(write_size);
	random_number = rand() % letter_count;
	letter = letter + random_number;
	memset(data, letter, write_size);
	lseek(fd, write_off, SEEK_SET);
	write(fd, data, write_size);
	close(fd);
}

int file_append(char *path, long long int write_off, 
				long long int write_size, long long int file_size)
{
	int fd = open(path, O_RDWR);
	char *data;
	char letter = 'a';
	int letter_count = 'z'- 'a';
	int random_number;
	if (fd < 0) {
		// printf("W: Failed Open, Try file create:%s\n", path);
		file_create(path);
		fd = open(path, O_RDWR);
		if (fd < 0) {
			// printf("W: Failed Open:%s\n", path);
			return -1;
		}
	}
	data = malloc(write_size);
	random_number = rand() % letter_count;
	letter = letter + random_number;
	memset(data, letter, write_size);
	lseek(fd, 0, SEEK_END);
	write(fd, data, write_size);
	close(fd);
}

